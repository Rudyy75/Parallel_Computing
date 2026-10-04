# Task I: Parallel Array Sum

## Objective

Sum a randomly generated array of 64-bit unsigned integers using:

1. One sequential loop.
2. Four pthreads using a strided partition.
3. Four pthreads using four contiguous partitions.

The program uses `N = 2^24` elements, which satisfies the required `N >= 1024` and creates a large enough memory workload for access-pattern differences to be observable.

## Build and run

This task must be built and run on Linux or Android through Termux:

```sh
make
./array_sum
```

The equivalent direct command is:

```sh
clang -std=c11 -Wall -Wextra -O2 -pthread -o array_sum array_sum.c
```

The program exits unsuccessfully if the three sums differ.

## Partitioning

For the strided strategy, worker `i` processes:

```text
array[i], array[i + 4], array[i + 8], ...
```

For the contiguous strategy, worker `i` processes:

```text
[i * N / 4, (i + 1) * N / 4)
```

The final contiguous worker ends at `N`, so a length that is not divisible by four does not lose its remainder elements.

Each worker accumulates into its own `partial_sum`. The main thread reads those results only after `pthread_join`, so no shared accumulator or mutex is needed.

## Timing

`CLOCK_MONOTONIC` measures elapsed time without being affected by wall-clock corrections. The single-threaded timing covers only the sequential sum. Each threaded timing includes thread creation, summation, and joining, because those are part of the complete threaded operation.

Run the program several times on the Termux device and record the output here:

```text
Paste the actual Termux output here.
```

A single run is not enough to establish a reliable performance ranking because scheduling, thermal state, background work, and CPU frequency can change the result.

## Expected explanation

The contiguous strategy usually performs best because each worker reads a sequential, non-overlapping range. Sequential access uses spatial locality and allows the hardware prefetcher to bring nearby values into cache efficiently.

The strided strategy makes all four workers touch the same cache lines, while each worker uses only some of the values in each line. This wastes fetched data and gives the memory system a less convenient access pattern. Because the array is read-only, this should not be described as false sharing: false sharing requires different threads to write different values on the same cache line.

The single-threaded version can still win on some devices because the threaded versions pay thread creation and joining overhead. The final conclusion must be based on repeated measurements from the required Termux device.
