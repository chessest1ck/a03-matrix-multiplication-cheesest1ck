#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "mm.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define FLUSH_SIZE (20 * 1024 * 1024)

// Task 1: Flush the cache so that we can do our measurement :)
void flush_all_caches() {
	size_t count = 4194304;
	long *flush_array = malloc(sizeof(long) * count);
	if (flush_array == NULL) {
		return;
	}
	for (size_t i = 0; i < count; i++) {
		flush_array[i] = (long)i;
	}
	free(flush_array);
}

void load_matrix_base() {
	long i;
	huge_matrixA = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixB = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixC = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	// Load the input
	// Note: This is suboptimal because each of these loads can be done in parallel.
	for(i=0;i<((long)SIZEX*(long)SIZEY);i++) {
		fscanf(fin1,"%ld", (huge_matrixA+i)); 		
		fscanf(fin2,"%ld", (huge_matrixB+i)); 		
		huge_matrixC[i] = 0;		
	}
}

void free_all() {
	free(huge_matrixA);
	free(huge_matrixB);
	free(huge_matrixC);
}

void multiply_base() {
	// Your code here
	// Implement your baseline matrix multiply here. // Assuming SIZEX == SIZEY
	int i, j, k;
	for (i = 0; i < SIZEX; i++) {
		for (k = 0; k < SIZEY; k++) {
			long row = huge_matrixA[i*SIZEY + k];
			for (j = 0; j < SIZEY; j++) {
				huge_matrixC[i*SIZEY + j] += row * huge_matrixB[k*SIZEY + j];
			}
		}
	}
}

void compare_results() {
	fout = fopen("./out.in","r");
	long i;
	long temp1, temp2;
	for(i=0;i<((long)SIZEX*(long)SIZEY);i++) {
		fscanf(fout, "%ld", &temp1);
		fscanf(ftest, "%ld", &temp2);
		if(temp1!=temp2) {
			printf("Wrong solution!");
			exit(1);
		}
	}
	fclose(fout);
	fclose(ftest);
}

void write_results() {
	// Your code here
	//
	// Basically, make sure the result is written on fout
	// Each line represent value in the X-dimension of your matrix
	for(long i = 0; i < SIZEX; i++) {
		for(long j = 0; j < SIZEY; j++) {
			fprintf(fout, "%ld ", huge_matrixC[i * SIZEY + j]);
		}
		fprintf(fout, "\n");
	}
}

void load_matrix() {
	long i;

	huge_matrixA = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixB = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixC = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);

	if (huge_matrixA == NULL || huge_matrixB == NULL || huge_matrixC == NULL) {
		fprintf(stderr, "Failed to allocate matrices.\n");
		exit(1);
	}

	for(i = 0; i < ((long)SIZEX*(long)SIZEY); i++) {
		fscanf(fin1,"%ld", (huge_matrixA+i));
		fscanf(fin2,"%ld", (huge_matrixB+i));
		huge_matrixC[i] = 0;
	}
}

void multiply() {
	int n = SIZEX;
	int s = 64;
	int i, j, k, ii, kk, jj;
	for (ii = 0; ii < n; ii += s) {
		for (kk = 0; kk < n; kk += s) {
			for (jj = 0; jj < n; jj += s) {				
				for (i = ii; i < min(ii + s, n); i++) {

					long *c_row = &huge_matrixC[i * SIZEY];
					long *a_row = &huge_matrixA[i * SIZEY];

					for (k = kk; k < min(kk + s, n); k++) {
						long temp = a_row[k];
						long *b_row = &huge_matrixB[k * SIZEY];
						
						j = jj;
						int j_end = min(jj + s, n);
						
						for (; j <= j_end - 4; j += 4) {
							c_row[j]   += temp * b_row[j];
							c_row[j+1] += temp * b_row[j+1];
							c_row[j+2] += temp * b_row[j+2];
							c_row[j+3] += temp * b_row[j+3];
						}						
						for (; j < j_end; j++) {
							c_row[j] += temp * b_row[j];
						}
					}
				}
			}
		}
	}
	// for (i = 0; i < n; i++) {
	// 	for (j = 0; j < n; j++) {
	// 		huge_matrixC[i * SIZEY + j] = 0;
	// 	}
	// }
	// for (kk = 0; kk < n; kk += s) {
	// 	for (jj = 0; jj < n; jj += s) {
	// 		for (i = 0; i < n; i++) {
	// 			for (k = kk; k < min(kk + s, n); k++) {
	// 				long temp = huge_matrixA[i*SIZEY + k];
	// 				for (j = jj; j <= min(jj + s, n) - 4; j += 4) {
	// 					huge_matrixC[i*SIZEY + j] += temp * huge_matrixB[k*SIZEY + j];
	// 					huge_matrixC[i*SIZEY + j+1] += temp * huge_matrixB[k*SIZEY + j+1];
	// 					huge_matrixC[i*SIZEY + j+2] += temp * huge_matrixB[k*SIZEY + j+2];
	// 					huge_matrixC[i*SIZEY + j+3] += temp * huge_matrixB[k*SIZEY + j+3];
	// 				}
	// 				for (; j < min(jj + s, n); j++) {
	// 					huge_matrixC[i*SIZEY + j] += temp * huge_matrixB[k*SIZEY + j];
	// 				}
	// 			}
	// 		}
	// 	}
	// }
}

int main() {
	
	clock_t s,t;
	double total_in_base = 0.0;
	double total_in_your = 0.0;
	double total_mul_base = 0.0;
	double total_mul_your = 0.0;
	fin1 = fopen("./input1.in","r");
	fin2 = fopen("./input2.in","r");
	fout = fopen("./out.in","w");
	ftest = fopen("./reference.in","r");
	

	flush_all_caches();

	s = clock();
	load_matrix_base();
	t = clock();
	total_in_base += ((double)t-(double)s) / CLOCKS_PER_SEC;
	printf("[Baseline] Total time taken during the load = %f seconds\n", total_in_base);

	s = clock();
	multiply_base();
	t = clock();
	total_mul_base += ((double)t-(double)s) / CLOCKS_PER_SEC;
	printf("[Baseline] Total time taken during the multiply = %f seconds\n", total_mul_base);
	fclose(fin1);
	fclose(fin2);
	fclose(fout);
	free_all();

	flush_all_caches();

	fin1 = fopen("./input1.in","r");
	fin2 = fopen("./input2.in","r");
	fout = fopen("./out.in","w");
	ftest = fopen("./reference.in","r");
	
	s = clock();
	load_matrix();
	t = clock();
	total_in_your += ((double)t-(double)s) / CLOCKS_PER_SEC;
	printf("Total time taken during the load = %f seconds\n", total_in_your);

	s = clock();
	multiply();
	t = clock();
	total_mul_your += ((double)t-(double)s) / CLOCKS_PER_SEC;
	printf("Total time taken during the multiply = %f seconds\n", total_mul_your);
	write_results();
	fclose(fin1);
	fclose(fin2);
	fclose(fout);
	free_all();
	compare_results();

	return 0;
}
