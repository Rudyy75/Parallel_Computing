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
Single-threaded sum = 18010371126344528  time = 0.012102 s
Strided (4 threads) sum = 18010371126344528  time = 0.037458 s
Contiguous (4 threads) sum = 18010371126344528  time = 0.011998 s
Single-threaded sum = 18010371126344528  time = 0.011535 s
Strided (4 threads) sum = 18010371126344528  time = 0.041397 s
Contiguous (4 threads) sum = 18010371126344528  time = 0.011457 s
Single-threaded sum = 18010371126344528  time = 0.011466 s
Strided (4 threads) sum = 18010371126344528  time = 0.038905 s
Contiguous (4 threads) sum = 18010371126344528  time = 0.011061 s
Single-threaded sum = 18010371126344528  time = 0.010912 s
Strided (4 threads) sum = 18010371126344528  time = 0.037723 s
Contiguous (4 threads) sum = 18010371126344528  time = 0.012638 s
Single-threaded sum = 18010371126344528  time = 0.012104 s
Strided (4 threads) sum = 18010371126344528  time = 0.041494 s
Contiguous (4 threads) sum = 18010371126344528  time = 0.013450 s
```

A single run is not enough to establish a reliable performance ranking because scheduling, thermal state, background work, and CPU frequency can change the result. The medians of these five runs were:

| Strategy | Median time |
|---|---:|
| Single-threaded | 0.011535 s |
| Strided, 4 threads | 0.038905 s |
| Contiguous, 4 threads | 0.011998 s |

The single-threaded strategy was the fastest in this sample. The contiguous strategy was approximately as fast, but its median was about 4% slower. The strided strategy was about 3.4 times slower than the single-threaded median. This is a valid result: the workload is memory-bound and the fixed cost of creating and joining four threads can be larger than the benefit of parallel execution on this device.

## Expected explanation

The contiguous strategy has the best threaded memory-access pattern because each worker reads a sequential, non-overlapping range. Sequential access uses spatial locality and allows the hardware prefetcher to bring nearby values into cache efficiently. It did not beat the single-threaded version in this sample because the array sum is limited by memory throughput and the threaded path also pays thread creation and joining overhead.

The strided strategy makes all four workers touch the same cache lines, while each worker uses only some of the values in each line. This wastes fetched data and gives the memory system a less convenient access pattern. Because the array is read-only, this should not be described as false sharing: false sharing requires different threads to write different values on the same cache line.

The single-threaded version can still win on some devices because the threaded versions pay thread creation and joining overhead. The final conclusion must be based on repeated measurements from the required Termux device.
