#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "coloringCC_sequential.h"

int main(int argc, char* argv[]){

    struct timeval start;
    struct timeval end;

    if (argc < 2) {
    printf("Usage: %s <edges_file>\n", argv[0]);
    return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) { 
        printf("Cannot open file %s\n", argv[1]); 
        return 1;
    }

    int nnz = 0;
    int r,c;
    int max_row = -1;

    //first pass: count edges and find maximum row index
    while (fscanf(f, "%d %d", &r, &c) == 2) {
        nnz++;
        if (r > max_row) {
            max_row = r;
        }
    }
    rewind(f);

    int nrows = max_row ; // 0-based indices

    //Allocate arrays to store COO
    int *row = malloc(nnz*sizeof(int));
    int *col = malloc(nnz*sizeof(int));
    if (!row || !col) {
        printf("Memory allocation failed\n");
        fclose(f);
        free(row); free(col);
        return 1;
    }

    //Read edges into arrays
    int idx = 0;
    while (fscanf(f, "%d %d", &r, &c) == 2) {
        row[idx] = r-1; //keep 0-based
        col[idx] = c-1;
        idx++;
    }
    fclose(f);

    //Build CSR structure
    int *rowptr = calloc(nrows+1, sizeof(int));  
     if (!rowptr) {
        printf("Memory allocation failed\n");
        free(row); free(col);
        return 1;
    }

    for (int i=0; i<nnz; i++) rowptr[row[i] + 1]++; 
    for (int i=1; i<=nrows; i++) rowptr[i] += rowptr[i-1];


    int *index = malloc(nnz * sizeof(int));
    int *temp = malloc((nrows)*sizeof(int));
    for (int i=0; i<nrows; i++) temp[i] = rowptr[i];

    for (int i=0; i<nnz; i++) {
        int r = row[i]; 
        int dest = temp[r]++;
        index[dest] = col[i];
    }


   //printf("CSR built. Number of rows: %d, nonzeros: %d\n", nrows, nnz);

   /*//example: print first 5 rows
   printf("Rowptr\n");
   for (int i=0; i<=nrows; i++){
        printf("%d ", rowptr[i]);
   }
   printf("\n\n");
   printf("index\n");
   for (int i=0; i<nnz; i++) {
    printf("%d ", index[i]);
   }
   printf("\n\n");*/


    free(row); free(col); free(temp);


    //Above is the code for reading .txt files and tranforming the data into CSR sparse format.
    //Below is the actuall code the the colloring label algorithm.

    
    int *labels = malloc(nrows*sizeof(int));
    if (!labels) {
        printf("Memory allocation failed\n");
        free(rowptr); free(index);
        return 1;
    }

    gettimeofday(&start, NULL);

    coloringCC_sequential(nrows, rowptr, index, labels);

    /*printf("Labels vector\n");
        for (int i=0; i<nrows; i++){
        if (labels[i] != 0){
        printf("wrong");
       }
    }*/

    //printf("\n\n");

    free(index);
    free(rowptr);
    free(labels);
    
    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1e6;
    printf("Executino time: %f seconds\n", elapsed);

}