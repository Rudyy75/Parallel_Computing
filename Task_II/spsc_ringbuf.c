#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define QUEUE_CAPACITY 16
#define QUEUE_MASK (QUEUE_CAPACITY - 1)
#define CACHE_LINE_SIZE 64
#define ITEM_COUNT 100

typedef struct {
    alignas(CACHE_LINE_SIZE) _Atomic size_t head;
    char head_padding[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];

    alignas(CACHE_LINE_SIZE) _Atomic size_t tail;
    char tail_padding[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];

    int values[QUEUE_CAPACITY];
} spsc_queue_t;

static void queue_init(spsc_queue_t *queue) {
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
}

static bool queue_enqueue(spsc_queue_t *queue, int value) {
    const size_t tail =
        atomic_load_explicit(&queue->tail, memory_order_relaxed);
    const size_t next_tail = (tail + 1) & QUEUE_MASK;
    const size_t head =
        atomic_load_explicit(&queue->head, memory_order_acquire);

    if (next_tail == head) {
        return false;
    }

    queue->values[tail] = value;
    atomic_store_explicit(&queue->tail, next_tail, memory_order_release);
    return true;
}

static bool queue_dequeue(spsc_queue_t *queue, int *value) {
    const size_t head =
        atomic_load_explicit(&queue->head, memory_order_relaxed);
    const size_t tail =
        atomic_load_explicit(&queue->tail, memory_order_acquire);

    if (head == tail) {
        return false;
    }

    *value = queue->values[head];
    atomic_store_explicit(
        &queue->head, (head + 1) & QUEUE_MASK, memory_order_release);
    return true;
}

static int producer_thread(void *argument) {
    spsc_queue_t *queue = argument;

    for (int value = 0; value < ITEM_COUNT; value++) {
        while (!queue_enqueue(queue, value)) {
            thrd_yield();
        }
        printf("[producer] enqueued %d\n", value);
    }

    return 0;
}

static int consumer_thread(void *argument) {
    spsc_queue_t *queue = argument;

    for (int expected = 0; expected < ITEM_COUNT; expected++) {
        int value;
        while (!queue_dequeue(queue, &value)) {
            thrd_yield();
        }

        printf("[consumer] dequeued %d\n", value);
        if (value != expected) {
            fprintf(stderr,
                    "error: expected %d but dequeued %d at position %d\n",
                    expected, value, expected);
            return EXIT_FAILURE;
        }
    }

    return 0;
}

int main(void) {
    spsc_queue_t queue;
    thrd_t producer;
    thrd_t consumer;
    int producer_result;
    int consumer_result;

    queue_init(&queue);

    if (thrd_create(&producer, producer_thread, &queue) != thrd_success) {
        fprintf(stderr, "error: could not create producer thread\n");
        return EXIT_FAILURE;
    }
    if (thrd_create(&consumer, consumer_thread, &queue) != thrd_success) {
        fprintf(stderr, "error: could not create consumer thread\n");
        thrd_detach(producer);
        return EXIT_FAILURE;
    }

    if (thrd_join(producer, &producer_result) != thrd_success ||
        thrd_join(consumer, &consumer_result) != thrd_success) {
        fprintf(stderr, "error: could not join worker thread\n");
        return EXIT_FAILURE;
    }

    if (producer_result != 0 || consumer_result != 0) {
        return EXIT_FAILURE;
    }

    puts("transferred 100 items in FIFO order");
    return EXIT_SUCCESS;
}
