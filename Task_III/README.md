# Task III: OpenGL ES Forest Fire Simulation

## Platform result

This task was executed on an Android 16 Vivo V2334 device through Termux. The device reported OpenGL ES 3.2 and an ARM GPU:

```text
EGL version: 1.4 Android META-EGL
EGL client APIs: OpenGL_ES
EGL_KHR_surfaceless_context: yes
OpenGL ES version: OpenGL ES 3.2 v1.r38p1
OpenGL ES vendor: ARM
```

The program uses `EGL_KHR_surfaceless_context` when available. The EGL setup also contains a 1x1 pbuffer fallback for devices without that extension.

## State model

The shader stores each cell as a `uint`:

```text
0 = healthy (H)
1 = burning (B)
2 = nothing (N)
```

For each epoch:

- Burning becomes Nothing.
- Healthy becomes Burning with probability `0.15` if at least one of its eight neighbors is Burning.
- All other cells retain their state.

The random sample is deterministic for a given cell index and epoch. This makes runs reproducible without sharing a mutable random-number generator between GPU invocations.

## GPU design

One compute invocation processes one grid cell. The shader reads from one shader-storage buffer and writes to another. The host waits for the dispatch to complete with `glMemoryBarrier`, maps the output buffer to count burning cells and print small grids, then swaps the two buffer IDs.

This ping-pong design is necessary because every cell in an epoch must read the complete previous state. Updating the same buffer in place would allow one invocation to observe another invocation's already-updated value and would change the cellular-automaton rules.

The compute workgroup is `16 x 16`. For a grid of size `M`, the host dispatches:

```text
ceil(M / 16) x ceil(M / 16) x 1
```

Invocations outside the `M x M` grid return immediately.

## Build and run

The Khronos headers are stored under `include/` because the Termux `ndk-sysroot` package on this device did not provide EGL/GLES headers directly. Android's system libraries provide the runtime implementation.

```sh
make -C Task_III clean
make -C Task_III
cd Task_III
./forest_fire 16
```

For larger grids, the program prints only the epoch count and avoids printing the complete matrix:

```sh
./forest_fire 32
./forest_fire 64
./forest_fire 128
```

## Observed results

The program was run on the same Android device for four grid sizes:

| Grid size | Epochs until extinction |
|---:|---:|
| 16 | 7 |
| 32 | 4 |
| 64 | 1 |
| 128 | 3 |

The larger-grid runs produced:

```text
M=32
Fire extinguished after 4 epochs for M=32
M=64
Fire extinguished after 1 epochs for M=64
M=128
Fire extinguished after 3 epochs for M=128
```

The number of epochs is not expected to increase monotonically with `M`. The initial state contains one burning cell, and the 15% ignition rule is probabilistic. A fire can die out immediately or spread to new cells before extinguishing.

The complete small-grid run for `M=16` printed every epoch and ended with:

The complete small-grid run printed every epoch and ended with:

```text
Epoch 1, burning cells: 2
Epoch 2, burning cells: 3
Epoch 3, burning cells: 3
Epoch 4, burning cells: 3
Epoch 5, burning cells: 5
Epoch 6, burning cells: 1
Epoch 7, burning cells: 0
Fire extinguished after 7 epochs for M=16
```

This shows that the number of burning cells does not need to decrease monotonically: new healthy neighbors may ignite before the existing fire disappears. The termination condition is specifically that the count reaches zero.

## Source files

- `forest_fire.comp`: OpenGL ES 3.1 compute shader.
- `forest_fire.c`: EGL setup, shader compilation, SSBO management, dispatch loop, readback, and grid printing.
- `egl_probe.c`: standalone EGL/OpenGL ES capability probe.
- `Makefile`: builds both the probe and simulation.
