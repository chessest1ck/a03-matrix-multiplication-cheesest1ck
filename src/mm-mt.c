#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "mm.h"
#include <pthread.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define FLUSH_SIZE (20 * 1024 * 1024)

typedef struct { 
	FILE *f; 
	long *dst; 
	long n; } ReadArgs;

typedef struct {
	int block_begin;
	int block_end;
	int n;
	int s;
} MulArgs;

// static double wall_seconds(struct timespec start, struct timespec end) {
// 	return (double)(end.tv_sec - start.tv_sec) + 
// 	(double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
// }

static void fast_read_longs(FILE *f, long *dst, long n) {
    const size_t BUFSZ = 1 << 20; // 1MB
    char *buf = malloc(BUFSZ);
    if (!buf) { 
		perror("malloc"); 
		exit(1);
	}
    long idx = 0;
    long value = 0;
    int sign = 1;
    int in_num = 0;
    size_t len;

    while ((len = fread(buf, 1, BUFSZ, f)) > 0 && idx < n) {
        for (size_t i = 0; i < len; i++) {
            unsigned char c = (unsigned char)buf[i];
            if (c == '-') { 
				sign = -1; 
				in_num = 1; 
				value = 0; 
			}
            else if (c >= '0' && c <= '9') {
                if (!in_num) { 
					in_num = 1; 
					sign = 1; 
					value = 0;
				}
                value = value * 10 + (c - '0');
            } else if (in_num) {
                dst[idx++] = sign * value;
                in_num = 0;
                value = 0;
                sign = 1;
                if (idx == n) {
					break;
				}
            }
        }
    }
    if (in_num && idx < n) {
		dst[idx++] = sign * value;
    }
	if (idx != n) {
		fprintf(stderr, "Input parse error: expected %ld numbers, got %ld\n", n, idx);
		free(buf);
		exit(1);
	}
    free(buf);
}

void *read_worker(void *p){ 
	ReadArgs *a = p;
	fast_read_longs(a->f, a->dst, a->n);
	return NULL;
}

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
	long n = (long)SIZEX * (long)SIZEY;
	pthread_t t1, t2;
	huge_matrixA = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixB = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);
	huge_matrixC = malloc(sizeof(long)*(long)SIZEX*(long)SIZEY);

	if (huge_matrixA == NULL || huge_matrixB == NULL || huge_matrixC == NULL) {
		fprintf(stderr, "Failed to allocate matrices.\n");
		exit(1);
	}
	ReadArgs a = { fin1, huge_matrixA, n };
	ReadArgs b = { fin2, huge_matrixB, n };

	if (pthread_create(&t1, NULL, read_worker, &a) != 0) {
		fprintf(stderr, "Failed to create reader thread for input1.in\n");
		exit(1);
	}
	if (pthread_create(&t2, NULL, read_worker, &b) != 0) {
		fprintf(stderr, "Failed to create reader thread for input2.in\n");
		exit(1);
	}
	if (pthread_join(t1, NULL) != 0 || pthread_join(t2, NULL) != 0) {
		fprintf(stderr, "Failed to join reader threads\n");
		exit(1);
	}
	for (long i = 0; i < n; i++) {
		huge_matrixC[i] = 0;
	}
}

void *multiply_worker(void *p) {
	MulArgs *args = (MulArgs *)p;
	int n = args->n;
	int s = args->s;

	for (int block = args->block_begin; block < args->block_end; block++) {
		int ii = block * s;
		for (int kk = 0; kk < n; kk += s) {
			for (int jj = 0; jj < n; jj += s) {
				for (int i = ii; i < min(ii + s, n); i++) {
					long *c_row = &huge_matrixC[i * SIZEY];
					long *a_row = &huge_matrixA[i * SIZEY];

					for (int k = kk; k < min(kk + s, n); k++) {
						long temp = a_row[k];
						long *b_row = &huge_matrixB[k * SIZEY];

						int j = jj;
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

	return NULL;
}

void multiply() {
	int n = SIZEX;
	int s = 64;
	int num_blocks = (n + s - 1) / s;
	int num_threads = 4;

	if (num_threads > num_blocks) {
		num_threads = num_blocks;
	}
	pthread_t threads[num_threads];
	MulArgs args[num_threads];
	
	for (int t = 0; t < num_threads; t++) {
		int block_begin = (t * num_blocks) / num_threads;
		int block_end = ((t + 1) * num_blocks) / num_threads;
		args[t].block_begin = block_begin;
		args[t].block_end = block_end;
		args[t].n = n;
		args[t].s = s;

		if (pthread_create(&threads[t], NULL, multiply_worker, &args[t]) != 0) {
			fprintf(stderr, "Failed to create multiply thread %d\n", t);
			exit(1);
		}
	}
	for (int t = 0; t < num_threads; t++) {
		if (pthread_join(threads[t], NULL) != 0) {
			fprintf(stderr, "Failed to join multiply thread %d\n", t);
			exit(1);
		}
	}
}

int main() {
	
	clock_t s,t;
	// struct timespec ws, wt;
	double total_in_base = 0.0;
	double total_in_your = 0.0;
	double total_mul_base = 0.0;
	double total_mul_your = 0.0;
	// double total_in_base_wall = 0.0;
	// double total_in_your_wall = 0.0;
	// double total_mul_base_wall = 0.0;
	// double total_mul_your_wall = 0.0;
	fin1 = fopen("./input1.in","r");
	fin2 = fopen("./input2.in","r");
	fout = fopen("./out.in","w");
	ftest = fopen("./reference.in","r");
	flush_all_caches();

	// clock_gettime(CLOCK_MONOTONIC, &ws);
	s = clock();
	load_matrix_base();
	t = clock();
	// clock_gettime(CLOCK_MONOTONIC, &wt);
	total_in_base += ((double)t-(double)s) / CLOCKS_PER_SEC;
	// total_in_base_wall += wall_seconds(ws, wt);
	printf("[Baseline] Total time taken during the load = %f seconds\n", total_in_base);
	// printf("[Baseline][Wall] Total time taken during the load = %f seconds\n", total_in_base_wall);

	// clock_gettime(CLOCK_MONOTONIC, &ws);
	s = clock();
	multiply_base();
	t = clock();
	// clock_gettime(CLOCK_MONOTONIC, &wt);
	total_mul_base += ((double)t-(double)s) / CLOCKS_PER_SEC;
	// total_mul_base_wall += wall_seconds(ws, wt);
	printf("[Baseline] Total time taken during the multiply = %f seconds\n", total_mul_base);
	// printf("[Baseline][Wall] Total time taken during the multiply = %f seconds\n", total_mul_base_wall);
	fclose(fin1);
	fclose(fin2);
	fclose(fout);
	free_all();

	flush_all_caches();

	fin1 = fopen("./input1.in","r");
	fin2 = fopen("./input2.in","r");
	fout = fopen("./out.in","w");
	ftest = fopen("./reference.in","r");
	
	// clock_gettime(CLOCK_MONOTONIC, &ws);
	s = clock();
	load_matrix();
	t = clock();
	// clock_gettime(CLOCK_MONOTONIC, &wt);
	total_in_your += ((double)t-(double)s) / CLOCKS_PER_SEC;
	// total_in_your_wall += wall_seconds(ws, wt);
	printf("Total time taken during the load = %f seconds\n", total_in_your);
	// printf("[Wall] Total time taken during the load = %f seconds\n", total_in_your_wall);

	// clock_gettime(CLOCK_MONOTONIC, &ws);
	s = clock();
	multiply();
	t = clock();
	// clock_gettime(CLOCK_MONOTONIC, &wt);
	total_mul_your += ((double)t-(double)s) / CLOCKS_PER_SEC;
	// total_mul_your_wall += wall_seconds(ws, wt);
	printf("Total time taken during the multiply = %f seconds\n", total_mul_your);
	// printf("[Wall] Total time taken during the multiply = %f seconds\n", total_mul_your_wall);
	write_results();
	fclose(fin1);
	fclose(fin2);
	fclose(fout);
	free_all();
	compare_results();

	return 0;
}
