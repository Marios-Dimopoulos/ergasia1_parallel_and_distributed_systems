#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cilk/cilk.h>
#include <string.h>

#include "coloringCC_opencilk.h"

//1 cilk_for
/*void coloringCC_opencilk(int nrows, const int *rowptr, const int *index, int *labels) {

    cilk_for (int i=0; i<nrows; i++) {
        labels[i] = i;
    }

    bool changed = true;

    while (changed) {

        changed = false;

        for (int v=0; v<nrows; v++){
            int start = rowptr[v];
            int end = rowptr[v+1];
            int lv = labels[v];

            for (int j=start; j<end; j++) {
                int u = index[j];
                int lu = labels[u];

                if (lu<lv) lv = lu;
            }

            if (lv != labels[v]) {
                labels[v] = lv;
                changed = true;
            }
        }

    }
}*/

//2 cilk_for
void coloringCC_opencilk(int nrows, const int *rowptr, const int *index, int *labels) {

    int *old_labels = malloc(nrows * sizeof(int));
    int *new_labels = malloc(nrows * sizeof(int));
    if (old_labels == NULL || new_labels == NULL) {
        free(old_labels);
        free(new_labels);
        return;
    }   

    cilk_for (int i=0; i<nrows; i++) {
        new_labels[i] = i;
        old_labels[i] = i;
    }

    bool changed = true;

    while (changed) {

       changed = false;
        
        cilk_for (int v=0; v<nrows; v++){
            int start = rowptr[v];
            int end = rowptr[v+1];
            int lv = old_labels[v];
            for (int j=start; j<end; j++) {
                int u = index[j];
                int lu = old_labels[u];
                if (lu<lv) lv = lu;
            }
            
            new_labels[v] = lv;

            if (lv != old_labels[v]) changed = true;
        }
        int *temp = old_labels;
        old_labels = new_labels;
        new_labels= temp;
    }
    memcpy(labels, old_labels, nrows * sizeof(int));
    free(new_labels); free(old_labels);
}
