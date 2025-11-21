#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "coloringCC_threads.h"


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
        int lv = data->old_labels[v];
        int start = data->rowptr[v];
        int end = data->rowptr[v+1];
        for (int j=start; j<end; j++) {
            int u = data->index[j];
            int lu = data->old_labels[u];
            if (lu<lv) lv = lu;
        }
        data->new_labels[v] = lv;
        if (lv != data->old_labels[v]) {
            *data->changed = true;
        }
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

    int num_threads = 4;
    pthread_t threads[num_threads];
    ThreadData tdata[num_threads];

   int chunk_size = (nrows + num_threads - 1) / num_threads;

   for (int t=0; t<num_threads; t++) {
        int start = t * chunk_size;
        int end = (start + chunk_size > nrows) ? nrows : start + chunk_size;
        tdata[t].start = start;
        tdata[t].end = end;
        tdata[t].old_labels = old_labels;
        tdata[t].new_labels = new_labels;
        pthread_create(&threads[t], NULL, routine2, &tdata[t]);
   }

   for (int t=0; t<num_threads; t++) {
        pthread_join(threads[t], NULL);
   }
    
    

    bool changed = true;
    chunk_size = (nrows + num_threads - 1) / num_threads;

    while(changed) {
        changed = false;


        for (int t=0; t<num_threads; t++) {
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

        for (int t=0; t<num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        int *tmp = old_labels;
        old_labels = new_labels;
        new_labels = tmp;
    }
    memcpy(labels, old_labels, nrows * sizeof(int));
    free(old_labels); free(new_labels);
}

