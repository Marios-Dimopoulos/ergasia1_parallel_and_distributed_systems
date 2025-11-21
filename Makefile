GCC   = gcc
CLANG = clang
CFLAGS = -march=native -O2 

SEQ  = seq
OMP  = omp
CILK = cilk
THREADS = threads

all: $(SEQ) $(OMP) $(CILK) $(THREADS)

$(SEQ): main_sequential.c coloringCC_sequential.c
	$(GCC) $(CFLAGS) $^ -o $@

$(OMP): main_openmp.c coloringCC_openmp.c
	$(GCC) $(CFLAGS) -fopenmp $^ -o $@

$(CILK): main_opencilk.c coloringCC_opencilk.c
	$(CLANG) $(CFLAGS) -fopencilk $^ -o $@

$(THREADS): main_threads.c coloringCC_threads.c
	$(GCC) $(CFLAGS) -pthread $^ -o $@

clean:
	rm -f $(SEQ) $(OMP) $(CILK)
