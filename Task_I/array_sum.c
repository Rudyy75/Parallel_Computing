#include <inttypes.h>
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

        const double start_time = monotonic_seconds();
        const uint64_t sum = sum_single_threaded(array, ARRAY_LENGTH);
        const double elapsed_seconds = monotonic_seconds() - start_time;
        printf("Single-threaded sum = %" PRIu64 "  time = %.6f s\n",
            sum, elapsed_seconds);

    free(array);
    return 0;
}
