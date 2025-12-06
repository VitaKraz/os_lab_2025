#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <limits.h>

#include "utils.h"
#include "sum.h"

static struct timespec start_time, finish_time;

void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    int *result = malloc(sizeof(int));
    if (result == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }
    *result = Sum(sum_args);
    return (void *)result;
}

void start_timer() {
    clock_gettime(CLOCK_MONOTONIC, &start_time);
}

void stop_timer() {
    clock_gettime(CLOCK_MONOTONIC, &finish_time);
}

double get_elapsed_time() {
    double elapsed = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed += (finish_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    return elapsed;
}

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;
    
    static struct option options[] = {
        {"threads_num", required_argument, 0, 't'},
        {"array_size", required_argument, 0, 'a'},
        {"seed", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "t:a:s:h", options, &option_index)) != -1) {
        switch (c) {
            case 't':
                threads_num = atoi(optarg);
                if (threads_num <= 0) {
                    printf("Threads number must be positive\n");
                    return 1;
                }
                break;
            case 'a':
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    printf("Array size must be positive\n");
                    return 1;
                }
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
                printf("Example: %s --threads_num 4 --array_size 1000000 --seed 42\n", argv[0]);
                return 0;
            default:
                printf("Invalid option. Use --help for usage information.\n");
                return 1;
        }
    }
    
    if (threads_num == 0 || array_size == 0) {
        printf("Usage: %s --threads_num <num> --array_size <num> [--seed <num>]\n", argv[0]);
        printf("Example: %s --threads_num 4 --array_size 1000000 --seed 42\n", argv[0]);
        return 1;
    }
    
    printf("Configuration:\n");
    printf("  Threads: %u\n", threads_num);
    printf("  Array size: %u\n", array_size);
    printf("  Seed: %u\n", seed);
    
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        perror("malloc");
        return 1;
    }
    
    GenerateArray(array, array_size, seed);
    
    int sequential_sum = 0;
    for (uint32_t i = 0; i < array_size; i++) {
        sequential_sum += array[i];
    }
    
    struct SumArgs args[threads_num];
    int chunk_size = array_size / threads_num;
    
    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * chunk_size;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
        printf("Thread %u: [%d, %d)\n", i, args[i].begin, args[i].end);
    }
    
    pthread_t threads[threads_num];
    
    start_timer();
    
    for (uint32_t i = 0; i < threads_num; i++) {
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
            perror("pthread_create");
            free(array);
            return 1;
        }
    }
    
    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        int *thread_result = NULL;
        if (pthread_join(threads[i], (void **)&thread_result) != 0) {
            perror("pthread_join");
            free(array);
            return 1;
        }
        
        if (thread_result != NULL) {
            total_sum += *thread_result;
            free(thread_result);
        }
    }
    
    stop_timer();
    
    printf("\nResults:\n");
    printf("  Sequential sum: %d\n", sequential_sum);
    printf("  Parallel sum:   %d\n", total_sum);
    printf("  Sums match:     %s\n", (sequential_sum == total_sum) ? "YES" : "NO");
    printf("  Elapsed time:   %.3f ms\n", get_elapsed_time());
    
    free(array);
    
    return 0;
}