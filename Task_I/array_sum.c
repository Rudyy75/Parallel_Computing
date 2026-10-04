#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_LENGTH (1ULL << 24)
#define THREAD_COUNT 4

typedef struct {
    const uint64_t *array;
    size_t length;
    size_t thread_index;
    uint64_t partial_sum;
} sum_work_t;

typedef void *(*sum_function_t)(void *);

static double monotonic_seconds(void) {
    struct timespec timestamp;

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    return (double)timestamp.tv_sec +
           (double)timestamp.tv_nsec / 1000000000.0;
}

static uint64_t sum_single_threaded(const uint64_t *array, size_t length) {
    uint64_t sum = 0;

    for (size_t index = 0; index < length; index++) {
        sum += array[index];
    }

    return sum;
}

static void *sum_strided(void *argument) {
    sum_work_t *work = argument;
    uint64_t sum = 0;

    for (size_t index = work->thread_index;
         index < work->length;
         index += THREAD_COUNT) {
        sum += work->array[index];
    }

    work->partial_sum = sum;
    return NULL;
}

static void *sum_contiguous(void *argument) {
    sum_work_t *work = argument;
    const size_t chunk_size = work->length / THREAD_COUNT;
    const size_t start = work->thread_index * chunk_size;
    const size_t end = (work->thread_index == THREAD_COUNT - 1)
                           ? work->length
                           : start + chunk_size;
    uint64_t sum = 0;

    for (size_t index = start; index < end; index++) {
        sum += work->array[index];
    }

    work->partial_sum = sum;
    return NULL;
}

static int sum_with_threads(sum_function_t sum_function,
                            const uint64_t *array,
                            size_t length,
                            uint64_t *total,
                            double *elapsed_seconds) {
    pthread_t threads[THREAD_COUNT];
    sum_work_t work[THREAD_COUNT];
    size_t created_threads = 0;
    int join_failed = 0;
    const double start_time = monotonic_seconds();

    for (size_t thread_index = 0;
         thread_index < THREAD_COUNT;
         thread_index++) {
        work[thread_index] = (sum_work_t){
            .array = array,
            .length = length,
            .thread_index = thread_index,
            .partial_sum = 0,
        };

        const int create_result = pthread_create(
            &threads[thread_index], NULL, sum_function, &work[thread_index]);
        if (create_result != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", create_result);
            for (size_t index = 0; index < created_threads; index++) {
                pthread_join(threads[index], NULL);
            }
            return -1;
        }
        created_threads++;
    }

    *total = 0;
    for (size_t thread_index = 0;
         thread_index < THREAD_COUNT;
         thread_index++) {
        const int join_result = pthread_join(threads[thread_index], NULL);
        if (join_result != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", join_result);
            join_failed = 1;
            continue;
        }
        *total += work[thread_index].partial_sum;
    }

    *elapsed_seconds = monotonic_seconds() - start_time;
    return join_failed ? -1 : 0;
}

int main(void) {
    uint64_t *array = malloc(ARRAY_LENGTH * sizeof(*array));
    if (array == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    srand(42);
    for (size_t index = 0; index < ARRAY_LENGTH; index++) {
        array[index] = (uint64_t)(unsigned int)rand();
    }

    const double single_start = monotonic_seconds();
    const uint64_t single_sum = sum_single_threaded(array, ARRAY_LENGTH);
    const double single_time = monotonic_seconds() - single_start;
    printf("Single-threaded sum = %" PRIu64 "  time = %.6f s\n",
           single_sum, single_time);

    uint64_t strided_sum;
    double strided_time;
    if (sum_with_threads(sum_strided, array, ARRAY_LENGTH,
                         &strided_sum, &strided_time) != 0) {
        free(array);
        return EXIT_FAILURE;
    }
    printf("Strided (4 threads) sum = %" PRIu64 "  time = %.6f s\n",
           strided_sum, strided_time);

    uint64_t contiguous_sum;
    double contiguous_time;
    if (sum_with_threads(sum_contiguous, array, ARRAY_LENGTH,
                         &contiguous_sum, &contiguous_time) != 0) {
        free(array);
        return EXIT_FAILURE;
    }
    printf("Contiguous (4 threads) sum = %" PRIu64 "  time = %.6f s\n",
           contiguous_sum, contiguous_time);

    if (single_sum != strided_sum || single_sum != contiguous_sum) {
        fprintf(stderr, "error: sums do not match\n");
        free(array);
        return EXIT_FAILURE;
    }

    free(array);
    return 0;
}
