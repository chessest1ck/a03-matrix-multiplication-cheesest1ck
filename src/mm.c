#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "mm.h"

#define FLUSH_SIZE (20 * 1024 * 1024)

// Task 1: Flush the cache so that we can do our measurement :)
void flush_all_caches() {
	size_t count = (size_t)FLUSH_SIZE;
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
	int s = 64; // Block size, can be tuned for better performance
	for(int jj = 0; jj < SIZEX; jj += s){
        for(int kk = 0; kk < SIZEX; kk += s){
            for(int i = 0; i < SIZEX; i++){
				for(int j = jj; j < ((jj + s) > SIZEX ? SIZEX :(jj + s)); j++){
					long temp = 0;
					for(int k = kk; k < ((kk + s) > SIZEX ? SIZEX : (kk + s)); k++){
						temp += huge_matrixA[i * SIZEY + k] * huge_matrixB[k * SIZEY + j];
					}
					huge_matrixC[i * SIZEY + j] += temp;
				}
			}
        }
	}
	// for (int i = 0; i < SIZEX; i++) {
	// 	printf("Row %d: ", i);
	// 	for (int j = 0; j < SIZEY; j++) {
	// 		printf("%ld ", huge_matrixC[i * SIZEY + j]);
	// 	}
	// 	printf("\n");
	// }
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
	fin1 = fopen("./input1.in","r");
	fin2 = fopen("./input2.in","r");
	fout = fopen("./out.in","w");
	
	if (fin1 == NULL || fin2 == NULL || fout == NULL) {
		fprintf(stderr, "Failed to open one or more input/output files.\n");
		exit(1);
	}

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
	int s = 64;
	for(int jj = 0; jj < SIZEX; jj += s){
        for(int kk = 0; kk < SIZEX; kk += s){
            int j_end = (jj + s < SIZEX) ? (jj + s) : SIZEX;
		int k_end = (kk + s < SIZEX) ? (kk + s) : SIZEX;

		for (int i = 0; i < SIZEX; i++) {
			for (int j = jj; j < j_end; j++) {
				long temp = 0;
				int k = kk;
				for (; k + 3 < k_end; k += 4) {   // unroll by 4
					temp += huge_matrixA[i*SIZEY + k]     * huge_matrixB[k*SIZEY + j];
					temp += huge_matrixA[i*SIZEY + (k+1)] * huge_matrixB[(k+1)*SIZEY + j];
					temp += huge_matrixA[i*SIZEY + (k+2)] * huge_matrixB[(k+2)*SIZEY + j];
					temp += huge_matrixA[i*SIZEY + (k+3)] * huge_matrixB[(k+3)*SIZEY + j];
				}
				for (; k < k_end; k++) temp += huge_matrixA[i*SIZEY + k] * huge_matrixB[k*SIZEY + j];
				huge_matrixC[i*SIZEY + j] += temp;
			}
		}
        }
	}
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
