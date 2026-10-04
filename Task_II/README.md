# Task II: Lock-Free SPSC Ring Buffer

## Objective

This task implements a fixed-size single-producer/single-consumer queue from scratch using C11 atomics and C11 threads. The queue stores `int` values in an array of 16 slots. One slot is intentionally unused, so the usable capacity is 15 items.

## Build and run

Run this on Linux or Termux:

```sh
make
./spsc_ringbuf
```

The program transfers values `0` through `99`. The consumer checks that values arrive exactly once and in FIFO order.

## Queue invariants

`head` identifies the next slot the consumer reads. `tail` identifies the next slot the producer writes.

```text
empty: head == tail
full:  (tail + 1) & (QUEUE_CAPACITY - 1) == head
```

Leaving one slot unused makes empty and full states distinguishable without a shared count. Since the capacity is 16, wrapping uses a bit mask instead of remainder division.

## Ownership and memory ordering

The producer is the only thread that writes `tail`. The consumer is the only thread that writes `head`. Their own index loads therefore use `memory_order_relaxed`.

The producer acquires `head` before deciding whether a slot is free. The consumer releases `head` after reading a slot, so the producer cannot overwrite a slot before the consumer has finished with it.

The producer writes the array slot before publishing the new `tail` with `memory_order_release`. The consumer reads `tail` with `memory_order_acquire` before reading the array slot. This release/acquire pair makes the item write visible before the consumer uses the item.

The queue array itself does not need atomic elements. Ownership and index publication ensure that the producer and consumer do not access the same slot concurrently in conflicting ways.

## Progress properties

The queue operations contain no mutex and no internal retry loop. Each `enqueue` or `dequeue` performs a bounded number of operations, so the data-structure methods are wait-free for this SPSC usage. The producer and consumer functions retry outside the queue methods when the queue is full or empty, calling `thrd_yield()` so they do not continuously consume a CPU while waiting.

## Cache-line padding

`head` and `tail` are written by different threads. They are aligned and separated by padding so normal updates do not place both frequently modified atomics on the same cache line. This reduces false sharing.

## Required evidence

Paste the actual Termux output below after running the program:

```text
[producer] enqueued 0
[producer] enqueued 1
[producer] enqueued 2
...
[consumer] dequeued 34
[consumer] dequeued 35
[producer] enqueued 36
...
[producer] enqueued 99
[consumer] dequeued 96
[consumer] dequeued 97
[consumer] dequeued 98
[consumer] dequeued 99
transferred 100 items in FIFO order
```

The complete Termux run showed every producer value from `0` through `99` and every consumer value from `0` through `99`. The consumer verified the order internally, and the final success line was printed. Producer and consumer lines appeared in scheduling-dependent bursts, which is expected for two concurrent threads.
