#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>

#include "coloringCC_openmp.h"

void coloringCC_openmp(int nrows, const int *rowptr, const int *index, int *labels) {

    int *old_labels = malloc(nrows * sizeof(int));
    int *new_labels = malloc(nrows * sizeof(int));

    if (!old_labels || !new_labels) {
        free(old_labels); free(new_labels);
        return;
    }   

    omp_set_num_threads(4);

    #pragma omp parallel for schedule(static)
    for (int i=0; i<nrows; i++) {
        old_labels[i] = i;
        new_labels[i] = i;
    }

    bool changed = true;
   
    while (changed) {
        
        changed = false;

        #pragma omp parallel for schedule(static)
        for (int v=0; v<nrows; v++){
            int start = rowptr[v];
            int end = rowptr[v+1];
            int lv = old_labels[v];
            for (int j=start; j<end; j++) {
                int u = index[j];
                int lu = old_labels[u];
               
                if (lu<lv) {
                    lv = lu;
                }
            }

            new_labels[v] = lv;

            if (lv != old_labels[v]) {
                changed = true;
            }
        }
        int *tmp = old_labels;
        old_labels = new_labels;
        new_labels = tmp;
    }
    memcpy(labels, old_labels, nrows * sizeof(int));
    free(old_labels); free(new_labels);
}


