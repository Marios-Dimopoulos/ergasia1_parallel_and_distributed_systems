#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "coloringCC_sequential.h"

void coloringCC_sequential(int nrows, const int *rowptr, const int *index, int *labels) {

    for (int i=0; i<nrows; i++) {
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
}