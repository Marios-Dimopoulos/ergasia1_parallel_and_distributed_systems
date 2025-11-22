#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "coloringCC_threads.h"

#define NUM_OF_THREADS 4

typedef struct {
    int start;
    int end;
    const int *rowptr;
    const int *index;
    int *old_labels;
    int *new_labels;
    bool *changed;
}ThreadData;

void* routine(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    for (int v=data->start; v<data->end; v++) {
        int start = data->rowptr[v];
        int end = data->rowptr[v+1];
        int lv = data->old_labels[v];
        for (int j=start; j<end; j++) {
            int u = data->index[j];
            int lu = data->old_labels[u];
            if (lu<lv) lv = lu;
        }

        data->new_labels[v] = lv;

        if (lv != data->old_labels[v]) *data->changed = true;
    }
    return NULL;
}

void* routine2(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    for (int i=data->start; i<data->end; i++) {
        data->new_labels[i] = i;
        data->old_labels[i] = i;
    }
    return NULL;
}

void coloringCC_threads(int nrows, const int *rowptr, const int *index, int *labels) {
    int *old_labels = malloc(nrows * sizeof(int));
    int *new_labels = malloc(nrows * sizeof(int));
    if (!old_labels || !new_labels) {
        free(old_labels); free(new_labels);
        return;
    }

    pthread_t threads[NUM_OF_THREADS];
    ThreadData tdata[NUM_OF_THREADS];

   int chunk_size = (nrows + NUM_OF_THREADS - 1) / NUM_OF_THREADS;

   for (int t=0; t<NUM_OF_THREADS; t++) {
        int start = t * chunk_size;
        int end = (start + chunk_size > nrows) ? nrows : start + chunk_size;
        
        tdata[t].start = start;
        tdata[t].end = end;
        tdata[t].old_labels = old_labels;
        tdata[t].new_labels = new_labels;

        pthread_create(&threads[t], NULL, routine2, &tdata[t]);
   }

   for (int t=0; t<NUM_OF_THREADS; t++) {
        pthread_join(threads[t], NULL);
   }

    bool changed = true;

    while(changed) {

        changed = false;

        for (int t=0; t<NUM_OF_THREADS; t++) {
            int start = t * chunk_size;
            int end = (start + chunk_size > nrows) ? nrows : start + chunk_size;

            tdata[t].start = start;
            tdata[t].end = end;
            tdata[t].rowptr = rowptr;
            tdata[t].index = index;
            tdata[t].old_labels = old_labels;
            tdata[t].new_labels = new_labels;
            tdata[t].changed = &changed;

            pthread_create(&threads[t], NULL, routine, &tdata[t]);
        }

        for (int t=0; t<NUM_OF_THREADS; t++) {
            pthread_join(threads[t], NULL);
        }

        int *tmp = old_labels;
        old_labels = new_labels;
        new_labels = tmp;
    }
    memcpy(labels, old_labels, nrows * sizeof(int));
    free(old_labels); free(new_labels);
}

