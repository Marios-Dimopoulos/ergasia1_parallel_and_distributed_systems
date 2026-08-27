# Connected Components Labeling (Sequential vs. Parallel)

University assignment for the **Parallel and Distributed Systems** course,
comparing sequential execution of a connected-components labeling
algorithm against three shared-memory parallelization strategies (OpenMP,
pthreads, OpenCilk).

## Variants

| Variant     | Technology    | Source files                                       |
|-------------|---------------|------------------------------------------------------|
| Sequential  | plain C       | `main_sequential.c`, `coloringCC_sequential.c/.h`     |
| OpenMP      | OpenMP        | `main_openmp.c`, `coloringCC_openmp.c/.h`             |
| pthreads    | POSIX threads | `main_threads.c`, `coloringCC_threads.c/.h`           |
| OpenCilk    | Cilk          | `main_opencilk.c`, `coloringCC_opencilk.c/.h`         |

## Algorithm

Given a graph in CSR (Compressed Sparse Row) format:

1. Every node starts as its own component: `labels[v] = v`.
2. Repeat: every node adopts the **smallest label among itself and its
   direct neighbors**.
3. Stop when a full pass makes no change (fixed point).

At convergence, two nodes share the same label if and only if they belong
to the same connected component (the label is the smallest node id in
that component).

The parallel versions use double-buffering (`old_labels` / `new_labels`)
so that all reads for an iteration see a consistent snapshot, avoiding a
data race between threads reading and writing the same array. An earlier,
in-place version of the OpenCilk implementation swapped the caller's
`labels` pointer with an internally allocated buffer and then freed
whatever ended up in the other pointer — on some inputs this freed memory
the program never allocated, corrupting the allocator ("double free or
corruption"). Keeping internal buffers private and copying the final
result back with `memcpy` fixed it; this is why every parallel variant
follows that pattern.

## Input format

Each program expects a path to a plain-text **edge list** file: one edge
per line, as two whitespace-separated, **1-based** node ids.
`read_mat_files_for_c.m` generates this file from a MATLAB/SuiteSparse
sparse matrix (`Problem.A`) — load a `.mat` matrix collection file so that
`Problem` exists in the workspace, then run the script; it writes
`edges.txt` in the current directory. Example matrices are pulled from
the [SuiteSparse Matrix Collection](https://sparse.tamu.edu/): open a
matrix's page, use its "MATLAB" download button, then run the `.m` script
on it. The `.txt` edge files themselves aren't checked into this repo —
some of the benchmark matrices were too large even zipped.

## Building

Requirements:
- `gcc` with OpenMP support, for the sequential/OpenMP/pthreads targets
- `clang` built with the [OpenCilk](https://www.opencilk.org/) plugin, for the OpenCilk target

```bash
make          # builds all 4 binaries: seq, omp, cilk, threads
make clean    # removes the built binaries
```

All targets compile with `-O2` (optimize for speed, without a large
compile-time cost).

## Running

```bash
./seq     edges.txt
./omp     edges.txt
./threads edges.txt
./cilk    edges.txt
```

Each run prints the wall-clock time (in seconds) spent purely on the
labeling algorithm — CSR construction and file I/O are excluded from the
timing.

## Benchmark findings

Benchmarks swept thread counts **1, 2, 4, 8, 16, 32** on a 4-core /
8-logical-core machine (`NUM_OF_THREADS` is fixed at compile time at the
top of each `coloringCC_openmp.c` / `coloringCC_threads.c` — edit and
rebuild to change it).

- **Thread scaling:** speed improves up to 4-8 threads, then flattens or
  regresses at 16-32, since only 8 threads can run truly simultaneously
  on this hardware — beyond that, context-switching overhead and cache
  contention between threads outweigh any benefit.
- **OpenMP vs. sequential:** on graphs with light work per node (few
  nnz/row), the sequential version is slightly faster — per-iteration
  parallel-region overhead dominates. On graphs with medium-to-heavy work
  (roughly 50-300 nnz/row), OpenMP pulls ahead by up to ~50%. Both
  `schedule(static)` and `schedule(dynamic, N)` were tried;
  `schedule(dynamic, 1024-4096)` was found to be a reasonable,
  cache-friendly chunk size for imbalanced degree distributions.
- **OpenCilk:** its work-stealing scheduler removes the need to manually
  pick a scheduling strategy. Interestingly, an alternative
  implementation that parallelizes only the initialization loop and runs
  the propagation loop sequentially (kept, commented out, at the top of
  `coloringCC_opencilk.c` — labeled "1 cilk_for") outperformed the fully
  parallel version below it ("2 cilk_for") in these benchmarks — cilk_for's
  per-iteration task-creation and work-stealing overhead outweighed the
  actual (cheap) per-node work in the propagation loop.
- **pthreads:** logic mirrors OpenMP/OpenCilk, but threads are spawned
  and joined fresh on every while-loop iteration rather than using a
  persistent pool, and work is split into static, equal-sized chunks
  with no load balancing between threads. A work-stealing pthreads
  implementation is a natural next step to reduce idle time.
- **The `changed` flag race:** every parallel variant lets multiple
  threads write `true` to a shared `changed` flag without synchronization
  (no `reduction`, no atomics, no mutex). This is a benign race — every
  writer only ever writes the same value, so a "lost" concurrent write
  can, at worst, cause one extra harmless loop iteration, never an
  incorrect result. `reduction(||:changed)` (OpenMP) and `<stdatomic.h>`
  atomics (OpenCilk) were both tried and measurably slowed the programs
  down, so they were deliberately removed.
- **A rejected optimization:** skipping the inner neighbor scan whenever
  a node's label is already `0` (the global minimum, since most test
  matrices have a single connected component) gave large speedups
  (~100x on `belgium_osm` for the sequential version, ~2x for the
  parallel ones) but was intentionally **not** adopted — it would be
  overfitting to the fact that most SuiteSparse test matrices happen to
  have exactly one connected component, rather than reflecting the
  algorithm's behavior on general inputs.

## Notes / known limitations

- Node count is inferred from the largest row index seen in the edge
  list, so the input is assumed to represent a graph whose adjacency
  matrix is (at least logically) square — i.e. every node that appears
  only as a column index should also appear as a row index somewhere.
- The four `main_*.c` files share almost identical CSR-building logic;
  this is kept duplicated for now to keep each binary self-contained and
  easy to build/inspect independently.