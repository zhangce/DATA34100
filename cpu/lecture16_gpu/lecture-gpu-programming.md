---
title: Lecture 16. GPU Architecture and Tiled GEMM
subtitle: Introductions to Data Systems and Data Design
author: Ce Zhang
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing, patterns}
---

# Introduction

## Journey So Far

We optimized a **single CPU core:**

- **ILP:** Multiple accumulators, loop unrolling -- hide latency
- **SIMD:** Process 4--8 elements per instruction -- multiply throughput
- **Blocking:** Tile data to fit in cache -- reduce data movement

. . .

And we built a framework to reason about performance:

- **Roofline model:** $P \leq \min(\pi, \beta \cdot I)$

. . .

**What if we need even more compute?**

## The Limits of a CPU Core

**CPU Single Core (FP32):**

- Peak: ~96 GFLOPS (16 flops/cycle $\times$ 2 for FP32 $\times$ 3 GHz)
- Memory bandwidth: ~15 GB/s

. . .

**NVIDIA Blackwell B200 GPU:**

- Peak BF16 (Tensor Core, dense): **2,250 TFLOPS**
- Peak FP4 (Tensor Core, dense): **9,000 TFLOPS**
- Memory bandwidth: **8,000 GB/s** (HBM3e, 192 GB)

. . .

**That is ~23,000x more compute (BF16) and ~530x more bandwidth.**


## CPU v GPU?

|                        | Peak compute | Bandwidth  | Ridge point ($\pi / \beta$) |
| ---------------------- | ------------ | ---------- | --------------------------- |
| **Single Core (FP32)** | 96 GFLOPS    | 15 GB/s    | **6.4 flops/byte**          |
| **B200 (BF16)**        | 2,250 TFLOPS | 8,000 GB/s | **281 flops/byte**          |
| **B200 (FP4)**         | 9,000 TFLOPS | 8,000 GB/s | **1,125 flops/byte**        |

. . .

Compute grew **faster** than bandwidth. The GPU ridge point is much higher -- you need far more arithmetic per byte to become compute bound.


## What Does This Scale Mean in Practice?

**1. Every microsecond counts.**

Our $1024 \times 1024 \times 1024$ GEMM on MacBook took **1.3 ms**. Our data loading and compute were on the same thread.

With B200 BF16: **~0.9 $\mu$s** -- about 1,800 cycles at 2 GHz.

A single global memory stall (~400 cycles) wastes **~22%** of the computation.

**Overlapping compute and data movement is not optional.**

. . .

**2. You must feed thousands of cores simultaneously.**

A CPU core has 2 FMA units. A B200 has 576 Tensor Cores across 192 SMs.

Keeping them busy requires thousands of independent operations in flight -- a fundamentally different level of parallelism than ILP or SIMD.

## Three Principles of GPU Performance

It sounds complex, but the good news is that it also governs how we think about GPUs:

1. **Pipeline** data movement behind compute -- hide latency
2. **Decompose** work into thousands of tiles, each with high arithmetic intensity
3. **Use Tensor Cores** to maximize FLOPs per cycle

. . .

Miss **any one** of the three, and you leave most of the GPU idle.

. . .

This, in some sense, actually makes modern GPU programming "easier" not "harder".


<!-- ============================================================
     GPU ARCHITECTURE SLIDES — SOURCE CITATIONS
     All A100 numbers verified against official NVIDIA sources:

     [WP] NVIDIA Ampere Architecture Whitepaper
          https://images.nvidia.com/aem-dam/en-zz/Solutions/data-center/nvidia-ampere-architecture-whitepaper.pdf
     [DS] A100 80GB Datasheet
          https://www.nvidia.com/content/dam/en-zz/Solutions/Data-Center/a100/pdf/a100-80gb-datasheet-update-a4-nvidia-1485612-r12-web.pdf
     [CC] CUDA Programming Guide — Compute Capabilities
          https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/compute-capabilities.html
     [TG] Ampere Tuning Guide
          https://docs.nvidia.com/cuda/ampere-tuning-guide/index.html
     [MB] Jia et al., "Demystifying the Nvidia Ampere Architecture" (arXiv:2208.11174)
          https://arxiv.org/pdf/2208.11174

     108 SMs (128 on full GA100 die, 20 disabled for yield) — [WP]
     64 FP32 cores/SM = 6,912 total — [WP]
     32 FP64 cores/SM = 3,456 total — [WP]
     4 Tensor Cores/SM (3rd gen) = 432 total — [WP]
     256 KB register file/SM = 65,536 × 32-bit — [CC]
     192 KB combined L1+SMEM/SM, up to 164 KB configurable as SMEM — [CC], [TG]
     40 MB L2 cache — [WP], [DS]
     80 GB HBM2e, 2,039 GB/s bandwidth (80GB SXM variant) — [DS]
     Max 1,024 threads/block, 2,048 threads/SM, 64 warps/SM — [CC]
     Warp size = 32 — [CC]
     Boost clock = 1,410 MHz — [DS]
     Shared memory latency ~19–23 cycles — [MB]
     L2 latency ~200 cycles — [MB]
     HBM latency ~290–466 cycles — [MB]
     ============================================================ -->

## Two Design Philosophies

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]
    % CPU side
    \node[font=\large\bfseries] at (3, 5) {CPU};
    \node[font=\small] at (3, 4.3) {Few powerful cores};

    % CPU cores (big)
    \foreach \i in {0,...,3} {
        \draw[fill=blue!40, rounded corners] (\i*1.5, 1) rectangle (\i*1.5+1.2, 3.5);
        \node[font=\tiny, rotate=90] at (\i*1.5+0.6, 2.25) {Core \i};
        % Internal structure
        \draw[fill=yellow!30] (\i*1.5+0.1, 2.8) rectangle (\i*1.5+1.1, 3.3);
        \node[font=\tiny] at (\i*1.5+0.6, 3.05) {OoO};
        \draw[fill=green!30] (\i*1.5+0.1, 2.2) rectangle (\i*1.5+1.1, 2.7);
        \node[font=\tiny] at (\i*1.5+0.6, 2.45) {Branch};
        \draw[fill=orange!30] (\i*1.5+0.1, 1.6) rectangle (\i*1.5+1.1, 2.1);
        \node[font=\tiny] at (\i*1.5+0.6, 1.85) {Cache};
        \draw[fill=red!20] (\i*1.5+0.1, 1.1) rectangle (\i*1.5+1.1, 1.5);
        \node[font=\tiny] at (\i*1.5+0.6, 1.3) {ALU};
    }

    % GPU side
    \node[font=\large\bfseries] at (11.5, 5) {GPU};
    \node[font=\small] at (11.5, 4.3) {Many simple cores};

    % GPU cores (small, many)
    \foreach \i in {0,...,7} {
        \foreach \j in {0,...,7} {
            \draw[fill=green!40] (\i*0.55+8, \j*0.35+1) rectangle (\i*0.55+8.45, \j*0.35+1.3);
        }
    }
    \node[font=\small] at (10.2, 0.5) {Thousands of simple ALUs};
\end{tikzpicture}
\end{document}
```

## Latency vs Throughput

**CPU design goal:** Minimize latency of each task

- Out-of-order execution, branch prediction, large caches
- Great for serial code with complex control flow

. . .

**GPU design goal:** Maximize throughput of all tasks

- Simple cores, massive parallelism
- Hide latency by switching between thousands of threads

. . .

**Key insight:** Trade single-thread speed for aggregate throughput.

# GPU Architecture

## NVIDIA GPU: High-Level View
<!-- 108 SMs [WP], 40 MB L2 [WP][DS], 80 GB HBM2e @ 2,039 GB/s [DS] -->

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    % GPU chip outline
    \draw[thick, rounded corners] (-0.5, -1) rectangle (13, 6.5);
    \node[font=\large\bfseries] at (6.25, 6) {GPU (e.g., NVIDIA A100)};

    % SMs
    \foreach \i in {0,...,5} {
        \foreach \j in {0,...,2} {
            \draw[fill=green!30, rounded corners] (\i*2, \j*1.5) rectangle (\i*2+1.7, \j*1.5+1.2);
            \pgfmathtruncatemacro{\smnum}{\i*3+\j}
            \node[font=\tiny] at (\i*2+0.85, \j*1.5+0.6) {SM \smnum};
        }
    }

    % Dots for more SMs
    \node at (12.5, 2.2) {...};
    \node[font=\small] at (12.5, 1.7) {108 SMs};

    % L2 cache
    \draw[fill=yellow!30] (-0.3, -0.8) rectangle (12.8, -0.3);
    \node[font=\small] at (6.25, -0.55) {L2 Cache (40 MB)};

    % Memory controllers
    \draw[fill=blue!20] (-0.3, -1.8) rectangle (12.8, -1.1);
    \node[font=\small] at (6.25, -1.45) {Memory Controllers};

    % HBM
    \foreach \i in {0,...,4} {
        \draw[fill=red!20, rounded corners] (\i*2.5+0.5, -3) rectangle (\i*2.5+2.5, -2.2);
        \node[font=\tiny] at (\i*2.5+1.5, -2.6) {HBM2e};
    }
    \node[font=\small] at (6.25, -3.5) {80 GB, 2 TB/s bandwidth};

    \draw[<->, thick] (6.25, -1.8) -- (6.25, -2.2);
\end{tikzpicture}
\end{document}
```

## The Thread Hierarchy
<!-- 1024 threads/block [CC], warp = 32 threads [CC] -->

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    % Grid
    \draw[fill=blue!10, rounded corners, thick] (0, 6) rectangle (12, 10);
    \node[font=\bfseries] at (6, 9.5) {Grid};
    \node[font=\small] at (6, 9) {All thread blocks for one kernel launch};

    % Thread blocks
    \foreach \i in {0,...,2} {
        \draw[fill=green!20, rounded corners] (\i*3.5+0.5, 6.5) rectangle (\i*3.5+3.5, 8.5);
        \node[font=\small\bfseries] at (\i*3.5+2, 8) {Block (\i, 0)};
    }
    \node at (11.5, 7.5) {...};

    % Zoom into one block
    \draw[->, very thick] (2, 6.3) -- (2, 5.5);
    \node[font=\small] at (4, 5.8) {Zoom in};

    % Thread block detail
    \draw[fill=green!20, rounded corners, thick] (0, 0.5) rectangle (10, 5);
    \node[font=\bfseries] at (5, 4.5) {Thread Block (0, 0)};
    \node[font=\small] at (5, 3.8) {Up to 1024 threads, share shared memory};

    % Warps
    \draw[fill=orange!30, rounded corners] (0.3, 1) rectangle (4.8, 3.3);
    \node[font=\small\bfseries] at (2.55, 3) {Warp 0 (32 threads)};
    % Threads in warp
    \foreach \i in {0,...,15} {
        \draw[fill=red!20] (\i*0.27+0.5, 1.3) rectangle (\i*0.27+0.7, 1.8);
    }
    \foreach \i in {0,...,15} {
        \draw[fill=red!20] (\i*0.27+0.5, 2) rectangle (\i*0.27+0.7, 2.5);
    }
    \node[font=\tiny] at (2.55, 1.05) {32 threads execute in lockstep};

    \draw[fill=orange!20, rounded corners] (5.2, 1) rectangle (9.7, 3.3);
    \node[font=\small\bfseries] at (7.45, 3) {Warp 1 (32 threads)};
    \foreach \i in {0,...,15} {
        \draw[fill=red!20] (\i*0.27+5.4, 1.3) rectangle (\i*0.27+5.6, 1.8);
    }
    \foreach \i in {0,...,15} {
        \draw[fill=red!20] (\i*0.27+5.4, 2) rectangle (\i*0.27+5.6, 2.5);
    }
    \node[font=\tiny] at (7.45, 1.05) {32 threads execute in lockstep};
\end{tikzpicture}
\end{document}
```

## The Streaming Multiprocessor (SM)
<!-- 4 sub-cores/SM, 16 FP32 + 8 FP64 + 1 Tensor Core each [WP]
     256 KB registers = 65,536 × 32-bit [CC]
     192 KB combined L1+SMEM, up to 164 KB as SMEM [CC][TG]
     108 SMs × 64 = 6,912 FP32 cores [WP] -->

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.65, transform shape]
    % SM outline
    \draw[thick, rounded corners, fill=gray!5] (0, 0) rectangle (12, 10);
    \node[font=\large\bfseries] at (6, 9.5) {Streaming Multiprocessor (SM)};

    % 4 processing blocks
    \foreach \i in {0,...,3} {
        \draw[fill=blue!10, rounded corners] (\i*3, 4.5) rectangle (\i*3+2.7, 8.8);
        \node[font=\small\bfseries] at (\i*3+1.35, 8.4) {Sub-core \i};

        % Warp scheduler
        \draw[fill=yellow!30] (\i*3+0.15, 7.5) rectangle (\i*3+2.55, 8);
        \node[font=\tiny] at (\i*3+1.35, 7.75) {Warp Scheduler};

        % FP32 cores
        \draw[fill=green!30] (\i*3+0.15, 6.5) rectangle (\i*3+2.55, 7.3);
        \node[font=\tiny] at (\i*3+1.35, 6.9) {16 FP32 Cores};

        % FP64 cores
        \draw[fill=green!50] (\i*3+0.15, 5.7) rectangle (\i*3+2.55, 6.3);
        \node[font=\tiny] at (\i*3+1.35, 6) {8 FP64 Cores};

        % Tensor core
        \draw[fill=red!30] (\i*3+0.15, 4.8) rectangle (\i*3+2.55, 5.5);
        \node[font=\tiny] at (\i*3+1.35, 5.15) {Tensor Core};
    }

    % Register file
    \draw[fill=orange!20] (0.3, 3.2) rectangle (11.7, 4.2);
    \node[font=\small] at (6, 3.7) {Register File (256 KB -- 65,536 $\times$ 32-bit registers)};

    % Shared memory / L1
    \draw[fill=purple!20] (0.3, 1.8) rectangle (11.7, 2.9);
    \node[font=\small] at (6, 2.35) {Shared Memory / L1 Cache (up to 164 KB, configurable)};

    % Summary
    \node[font=\small, align=left] at (6, 1) {Per SM: 64 FP32 cores, 32 FP64 cores, 4 Tensor Cores};
    \node[font=\small, align=left] at (6, 0.4) {A100 total: 108 SMs $\times$ 64 = 6,912 FP32 cores};
\end{tikzpicture}
\end{document}
```

## SIMT Execution: Warps
<!-- Warp = 32 threads [CC], independent program counters since Volta [WP] -->

**SIMT** = Single Instruction, Multiple Threads

- A **warp** is 32 threads that execute the **same instruction** at the same time
- Like SIMD, but each thread has its own registers and can follow different paths

. . .

**Key differences from CPU SIMD:**

| | CPU SIMD (AVX) | GPU SIMT (Warp) |
|---|---|---|
| Width | 4--8 elements | 32 threads |
| Control | One instruction stream | 32 program counters |
| Divergence | Not possible | Possible (but costly) |
| Scheduling | Programmer/compiler | Hardware |

## Warp Divergence

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    % No divergence
    \node[font=\bfseries] at (4, 5) {No Divergence (all threads take same path)};

    \foreach \i in {0,...,7} {
        \draw[fill=green!40] (\i*1, 3.5) rectangle (\i*1+0.8, 4.3);
        \node[font=\tiny] at (\i*1+0.4, 3.9) {T\i};
    }
    \draw[->, thick] (4, 3.3) -- (4, 2.8);
    \node[font=\small] at (4, 2.5) {All execute path A -- full throughput};

    % With divergence
    \node[font=\bfseries] at (4, 1.5) {Divergence (threads take different paths)};

    % Step 1: threads 0-3 execute path A
    \foreach \i in {0,...,3} {
        \draw[fill=green!40] (\i*1, -0.5) rectangle (\i*1+0.8, 0.3);
        \node[font=\tiny] at (\i*1+0.4, -0.1) {T\i};
    }
    \foreach \i in {4,...,7} {
        \draw[fill=gray!20] (\i*1, -0.5) rectangle (\i*1+0.8, 0.3);
        \node[font=\tiny] at (\i*1+0.4, -0.1) {T\i};
    }
    \node[right, font=\small] at (8.5, -0.1) {Step 1: Path A (T4--T7 idle)};

    % Step 2: threads 4-7 execute path B
    \foreach \i in {0,...,3} {
        \draw[fill=gray!20] (\i*1, -1.5) rectangle (\i*1+0.8, -0.7);
        \node[font=\tiny] at (\i*1+0.4, -1.1) {T\i};
    }
    \foreach \i in {4,...,7} {
        \draw[fill=orange!40] (\i*1, -1.5) rectangle (\i*1+0.8, -0.7);
        \node[font=\tiny] at (\i*1+0.4, -1.1) {T\i};
    }
    \node[right, font=\small] at (8.5, -1.1) {Step 2: Path B (T0--T3 idle)};

    \node[red, font=\small] at (4, -2.3) {50\% of compute wasted!};
\end{tikzpicture}
\end{document}
```

. . .

**Avoid divergence within a warp.** Branches across warps are fine.

## GPU Memory Hierarchy
<!-- Registers: 256 KB/SM [CC]. SMEM: up to 164 KB/SM, ~20 cycles [CC][TG][MB].
     L2: 40 MB, ~200 cycles [WP][MB]. HBM: 80 GB, ~400 cycles, 2 TB/s [DS][MB]. -->

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]
    % Registers
    \draw[fill=red!20, rounded corners] (3, 7) rectangle (9, 8);
    \node at (6, 7.5) {\textbf{Registers}};
    \node[right, font=\small] at (9.2, 7.5) {256 KB/SM, 0 cycles, per-thread};

    % Shared memory
    \draw[fill=orange!20, rounded corners] (2.5, 5.5) rectangle (9.5, 6.5);
    \node at (6, 6) {\textbf{Shared Memory (SMEM)}};
    \node[right, font=\small] at (9.7, 6) {up to 164 KB/SM, ~20 cycles, per-block};

    % L2 cache
    \draw[fill=yellow!20, rounded corners] (2, 4) rectangle (10, 5);
    \node at (6, 4.5) {\textbf{L2 Cache}};
    \node[right, font=\small] at (10.2, 4.5) {40 MB, ~200 cycles, shared};

    % Global memory
    \draw[fill=blue!20, rounded corners] (1.5, 2.5) rectangle (10.5, 3.5);
    \node at (6, 3) {\textbf{Global Memory (HBM)}};
    \node[right, font=\small] at (10.7, 3) {80 GB, ~400 cycles, 2 TB/s};

    % Arrows
    \draw[<->, thick] (6, 6.8) -- (6, 6.5);
    \draw[<->, thick] (6, 5.3) -- (6, 5);
    \draw[<->, thick] (6, 3.8) -- (6, 3.5);

    % Key insight
    \node[draw, rounded corners, fill=green!10, font=\small, align=center] at (6, 1.5) {Shared memory is \textbf{programmer-managed}\\(unlike CPU caches which are automatic)};
\end{tikzpicture}
\end{document}
```

## CPU vs GPU Memory: Key Difference

**CPU caches are transparent:**

- Hardware decides what to cache
- Programmer writes normal code, cache "just works"
- Blocking helps, but the hardware manages the cache

. . .

**GPU shared memory is explicit:**

- Programmer declares `__shared__` arrays
- Programmer loads data from global memory into shared memory
- Programmer synchronizes threads with `__syncthreads()`

. . .

**This is like having a manually managed L1 cache.**

More work for the programmer, but more control over data movement.

## GPU vs CPU: By the Numbers

\small

|                 | CPU Single Core     | NVIDIA A100               |
| --------------- | ------------------- | ------------------------- |
| **Cores/ALUs**  | 1 core, 2 FMA units | 108 SMs, 6,912 FP32 cores |
| **Clock**       | ~3 GHz              | ~1.4 GHz                  |
| **Peak FP32**   | ~96 GFLOPS          | 19,500 GFLOPS             |
| **Peak FP64**   | ~48 GFLOPS          | 9,700 GFLOPS              |
| **Bandwidth**   | ~15 GB/s (DDR4)     | ~2,000 GB/s (HBM2e)       |
| **Ridge point** | ~3.2 flops/byte     | ~10 flops/byte            |
| **Cache**       | 32KB L1 (automatic) | 164KB SMEM (manual)       |
| **Threads**     | 1 (with OoO, SIMD)  | Up to 2,048/SM            |

. . .

**The GPU has a higher ridge point in roofline** -- you need more arithmetic per byte to become compute bound.

## The Old Way: Think About Threads

Five years ago, a GPU programming course would focus on **per-thread** concerns:

- **Memory coalescing:** Ensure threads in a warp access consecutive addresses
- **Bank conflicts:** Avoid multiple threads hitting the same shared memory bank
- **Warp divergence:** Minimize branches that cause threads to take different paths
- **Occupancy tuning:** Balance registers and shared memory to maximize active warps
- **Thread-to-data mapping:** Carefully assign `threadIdx` to array indices

. . .

These still matter -- but they are **low-level details**, not the main design challenge.

## The Modern Way: Think About Tiles

Today, high-performance GPU code is designed around **tiles**, not individual threads:

. . .

1. **Decompose the problem into tiles** -- What block of output does each thread block produce?
2. **Design the data movement** -- What tiles of input go into shared memory? How often?
3. **Maximize reuse** -- Each byte loaded from global memory should be used as many times as possible
4. **Match the hardware** -- Tile sizes should align with shared memory capacity, warp size, and Tensor Core dimensions

. . .

**The per-thread details (coalescing, bank conflicts) often follow naturally from a good tiling design.**



# Tensor Cores

## From Scalar to Matrix Instructions

**Scalar FMA:** 1 multiply-add per cycle

**SIMD/Warp:** 32 multiply-adds per cycle (one per thread)

. . .

**Tensor Core:** A hardware unit that performs a **small matrix multiply-accumulate** in a single instruction:

$$D = A \times B + C$$

where $A$, $B$, $C$, $D$ are small matrix tiles (not scalars, not vectors).

. . .

**This is the key hardware innovation:** instead of $2$ FLOPs per FMA, a Tensor Core produces **hundreds of FLOPs per instruction**.

## A100 Tensor Core: The MMA Instruction

<!-- A100: MMA tile 8×4×8, 512 FLOPs/cycle per TC, 432 TCs total [WP] -->

On the A100, a single **MMA (Matrix Multiply-Accumulate)** instruction computes:

$$D_{8 \times 8} = A_{8 \times 4} \times B_{4 \times 8} + C_{8 \times 8}$$

In PTX assembly (per-thread registers, warp-synchronous):

```
mma.sync.aligned.m8n8k4.row.col.f32.f16.f16.f32
    {%d0,...,%d7},   // D fragment: 8 x FP32 regs/thread
    {%a0,%a1},       // A fragment: 2 x FP16x2 regs/thread
    {%b0,%b1},       // B fragment: 2 x FP16x2 regs/thread
    {%c0,...,%c7};   // C fragment: 8 x FP32 regs/thread
```

Each thread holds a **fragment**; the 32 threads in a warp collectively hold the full tiles.

## Tensor Core: By the Numbers

**FLOPs per Tensor Core per cycle:**

$$F_{\text{TC}} = 2 \times 8 \times 4 \times 8 = 512 \text{ FLOPs/cycle}$$

. . .

**Peak compute (FP16 Tensor Cores):**

$$P_{\text{peak}} = 1410 \text{ MHz} \times 512 \text{ FLOPs} \times 432 \text{ TCs} = \mathbf{312 \text{ TFLOPS}}$$


## How MMA Works Across a Warp

A Tensor Core MMA is a **warp-level** instruction. All 32 threads in a warp cooperate:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.65, transform shape]
    % Warp
    \draw[fill=orange!20, rounded corners, thick] (0, 5) rectangle (12, 7);
    \node[font=\bfseries] at (6, 6.5) {Warp (32 threads)};

    % Each thread holds fragments
    \foreach \i in {0,...,7} {
        \draw[fill=red!20] (\i*1.4+0.3, 5.3) rectangle (\i*1.4+1.3, 5.9);
        \node[font=\tiny] at (\i*1.4+0.8, 5.6) {T\i: frags};
    }

    % Arrow down
    \draw[->, very thick] (6, 4.8) -- (6, 4.2);
    \node[font=\small, right] at (6.2, 4.5) {mma.sync instruction};

    % A fragment
    \draw[fill=red!30] (0.5, 1.5) rectangle (3.5, 3.5);
    \node at (2, 2.5) {$A_{8\times4}$};
    \node[font=\tiny] at (2, 1.8) {distributed};
    \node[font=\tiny] at (2, 1.3) {across threads};

    \node at (4, 2.5) {$\times$};

    % B fragment
    \draw[fill=blue!30] (4.5, 1.5) rectangle (7.5, 3.5);
    \node at (6, 2.5) {$B_{4\times8}$};
    \node[font=\tiny] at (6, 1.8) {distributed};
    \node[font=\tiny] at (6, 1.3) {across threads};

    \node at (8, 2.5) {$\to$};

    % C fragment
    \draw[fill=green!30] (8.5, 1.5) rectangle (11.5, 3.5);
    \node at (10, 2.5) {$C_{8\times8}$};
    \node[font=\tiny] at (10, 1.8) {accumulated};
    \node[font=\tiny] at (10, 1.3) {in registers};

    % Key point
    \node[draw, rounded corners, fill=yellow!20, font=\small, align=center] at (6, 0.3) {Each thread holds a \textbf{fragment} of A, B, C\\The warp collectively executes one matrix multiply};
\end{tikzpicture}
\end{document}
```

## MMA at the PTX Level {.fragile}

\small

The PTX instruction for FP16 Tensor Core MMA on A100:

```
// D = A * B + C, all held in thread registers
// A: 8x4 (FP16), B: 4x8 (FP16), C/D: 8x8 (FP32)
mma.sync.aligned.m8n8k4.row.col.f32.f16.f16.f32
    {%d0, %d1, %d2, %d3, %d4, %d5, %d6, %d7},  // D (8 regs)
    {%a0, %a1},                                    // A (2 regs)
    {%a2, %a3},                                    // B (2 regs)
    {%c0, %c1, %c2, %c3, %c4, %c5, %c6, %c7};   // C (8 regs)
```

. . .

**Key observations:**

- `mma.sync` -- this is a **warp-synchronous** instruction (all 32 threads must execute it)
- `.m8n8k4` -- the tile dimensions: $M=8, N=8, K=4$
- `.f32.f16.f16.f32` -- accumulate in FP32, multiply FP16 inputs
- Each thread holds only a **fragment** (2 registers for A, 2 for B, 8 for C/D)

## Tensor Core and the Roofline

\small

With Tensor Cores, the roofline shifts dramatically:

|                       | Peak        | Ridge point ($\pi / \beta$) |
| --------------------- | ----------- | --------------------------- |
| **FP32 CUDA cores**   | 19.5 TFLOPS | ~10 flops/byte              |
| **FP16 Tensor Cores** | 312 TFLOPS  | ~156 flops/byte             |

. . .

**With TILE=32, our tiled GEMM achieves $I = 8$ flops/byte.**

That was enough for FP32 cores, but Tensor Cores need $I > 156$ to be compute bound!

. . .

**This is why real GEMM libraries use multi-level tiling:**

- Thread block tile: 128 $\times$ 128 (global $\to$ shared memory)
- Warp tile: 32 $\times$ 64 (shared $\to$ registers)
- MMA tile: 8 $\times$ 4 $\times$ 8 (registers $\to$ Tensor Core)

# Software Pipelining

## The Problem: Sequential Load-Compute

Our tiled GEMM has a fundamental bottleneck:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    \draw[->] (0, 0) -- (14, 0) node[right] {Time};

    % Iteration 0
    \draw[fill=blue!40] (0, 1) rectangle (4, 1.8);
    \node[font=\small] at (2, 1.4) {Load tile 0};
    \draw[fill=gray!30] (4, 1) rectangle (4.5, 1.8);
    \node[font=\tiny] at (4.25, 1.4) {sync};
    \draw[fill=red!40] (4.5, 1) rectangle (6.5, 1.8);
    \node[font=\small] at (5.5, 1.4) {Compute 0};
    \draw[fill=gray!30] (6.5, 1) rectangle (7, 1.8);
    \node[font=\tiny] at (6.75, 1.4) {sync};

    % Iteration 1
    \draw[fill=blue!40] (7, 1) rectangle (11, 1.8);
    \node[font=\small] at (9, 1.4) {Load tile 1};
    \draw[fill=gray!30] (11, 1) rectangle (11.5, 1.8);
    \draw[fill=red!40] (11.5, 1) rectangle (13.5, 1.8);
    \node[font=\small] at (12.5, 1.4) {Compute 1};

    \node[font=\small] at (7, 2.5) {Load takes $\sim$400 cycles, Compute takes $\sim$128 cycles};
    \node[red, font=\small] at (7, -0.5) {GPU is \textbf{idle} during loads -- 75\% of time wasted!};
\end{tikzpicture}
\end{document}
```

. . .

Each iteration: **load** (wait for global memory) $\to$ **sync** $\to$ **compute** $\to$ **sync** $\to$ repeat.

The Tensor Cores sit idle during the ~400-cycle load.

## The Idea: Overlap Load and Compute

**What if we load the NEXT tile while computing the CURRENT tile?**

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    \draw[->] (0, 0) -- (14, 0) node[right] {Time};

    % Prologue: load tile 0
    \draw[fill=blue!40] (0, 2) rectangle (4, 2.8);
    \node[font=\small] at (2, 2.4) {Load tile 0};
    \node[left, font=\small] at (0, 2.4) {Memory};

    % Steady state
    \draw[fill=blue!40] (4, 2) rectangle (8, 2.8);
    \node[font=\small] at (6, 2.4) {Load tile 1};
    \draw[fill=blue!40] (8, 2) rectangle (12, 2.8);
    \node[font=\small] at (10, 2.4) {Load tile 2};

    \draw[fill=red!40] (4, 1) rectangle (6, 1.8);
    \node[font=\small] at (5, 1.4) {Compute 0};
    \node[left, font=\small] at (0, 1.4) {Compute};
    \draw[fill=red!40] (8, 1) rectangle (10, 1.8);
    \node[font=\small] at (9, 1.4) {Compute 1};
    \draw[fill=red!40] (12, 1) rectangle (14, 1.8);
    \node[font=\small] at (13, 1.4) {Compute 2};

    % Overlap arrows
    \draw[<->, thick, green!60!black] (4.2, 1.9) -- (4.2, 2.1);
    \draw[<->, thick, green!60!black] (8.2, 1.9) -- (8.2, 2.1);

    \node[green!60!black, font=\small] at (7, 0.2) {Load and compute happen \textbf{simultaneously}!};
    \node[font=\small] at (7, 3.5) {Memory latency is \textbf{hidden} behind computation};
\end{tikzpicture}
\end{document}
```

. . .

This is **software pipelining** -- the same idea as CPU instruction pipelining, but at the tile level.

## Double Buffering: The Simplest Pipeline

**Use two shared memory buffers** instead of one:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.6, transform shape]
    % Shared memory
    \draw[fill=gray!10, rounded corners, thick] (0, 3) rectangle (12, 6.5);
    \node[font=\bfseries] at (6, 6) {Shared Memory};

    \draw[fill=red!20, rounded corners] (0.5, 3.5) rectangle (5.5, 5.5);
    \node[font=\small] at (3, 4.5) {Buffer 0};

    \draw[fill=blue!20, rounded corners] (6.5, 3.5) rectangle (11.5, 5.5);
    \node[font=\small] at (9, 4.5) {Buffer 1};

    % Timeline
    \draw[->] (0, 0) -- (12, 0) node[right] {Time};

    % Stage 0: Load buf0
    \draw[fill=red!30] (0, 1.5) rectangle (2.5, 2.2);
    \node[font=\tiny] at (1.25, 1.85) {Load$\to$Buf0};

    % Stage 1: Compute buf0, Load buf1
    \draw[fill=red!50] (2.5, 0.6) rectangle (5, 1.3);
    \node[font=\tiny] at (3.75, 0.95) {Compute Buf0};
    \draw[fill=blue!30] (2.5, 1.5) rectangle (5, 2.2);
    \node[font=\tiny] at (3.75, 1.85) {Load$\to$Buf1};

    % Stage 2: Compute buf1, Load buf0
    \draw[fill=blue!50] (5, 0.6) rectangle (7.5, 1.3);
    \node[font=\tiny] at (6.25, 0.95) {Compute Buf1};
    \draw[fill=red!30] (5, 1.5) rectangle (7.5, 2.2);
    \node[font=\tiny] at (6.25, 1.85) {Load$\to$Buf0};

    % Stage 3
    \draw[fill=red!50] (7.5, 0.6) rectangle (10, 1.3);
    \node[font=\tiny] at (8.75, 0.95) {Compute Buf0};
    \draw[fill=blue!30] (7.5, 1.5) rectangle (10, 2.2);
    \node[font=\tiny] at (8.75, 1.85) {Load$\to$Buf1};

    \node[font=\small, align=center] at (6, -0.7) {While computing from one buffer,\\load the next tile into the other buffer};
\end{tikzpicture}
\end{document}
```

## Why Double Buffering Is Not Enough

Let's assume the following:

| | Cycles |
|---|---|
| HBM $\to$ shared memory latency | **~1000 cycles** |
| Tensor Core compute per tile | **~128 cycles** |
| Ratio (latency / compute) | **~8x** |

. . .

The load is **8x slower** than the compute. This is the fundamental problem.

## The Double Buffering Timeline (Real Scale)

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.48, transform shape]
    % Timeline
    \draw[->, thick] (0, 0) -- (26, 0) node[right, font=\small] {Cycles};

    % Load row
    \node[left, font=\small\bfseries] at (-0.5, 3.5) {Load};
    % Load 0 (takes ~1000 cycles, represented at scale ~20 units = 512 cycles... let's scale: 1 unit = 40 cycles)
    % Actually let's use 1 unit = 40 cycles. So 1000 cycles = 25 units, 128 cycles = 3.2 units

    % Re-scale: 1 unit = 50 cycles. 1000 cycles = 20 units, 128 cycles = 2.56 units
    \draw[fill=blue!30] (0, 3) rectangle (20, 4);
    \node[font=\small] at (10, 3.5) {Load Tile 0 (\textbf{1000 cycles})};

    \draw[fill=blue!50] (20, 3) rectangle (25, 4);
    \node[font=\tiny] at (22.5, 3.5) {Load 1...};

    % Compute row
    \node[left, font=\small\bfseries] at (-0.5, 1.5) {Compute};
    % Cannot start compute until load 0 completes at cycle 1000 (= 20 units)
    \draw[fill=red!50] (20, 1) rectangle (22.56, 2);
    \node[font=\tiny] at (21.28, 1.5) {C0};

    % Stall region
    \draw[pattern=north east lines, pattern color=red!60] (0, 1) rectangle (20, 2);
    \node[font=\small, red!70!black] at (10, 1.5) {\textbf{STALL --- Tensor Cores idle for 1000 cycles!}};

    % Brace for stall
    \draw[decorate, decoration={brace, amplitude=8pt, mirror}, thick, red!60!black]
        (0, 0.7) -- (20, 0.7);
    \node[below, font=\small, red!60!black] at (10, 0) {Waiting for data from HBM};

    % Only 2.56 units of compute vs 20 units of load
    \node[draw, rounded corners, fill=yellow!20, font=\small, align=center] at (13, -1.8)
        {With double buffering: hide \textbf{at most} $2 \times 128 = 256$ cycles.  Still stall for $1000 - 256 = 744$ cycles!};
\end{tikzpicture}
\end{document}
```

## The Core Problem: Load $\gg$ Compute

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    % Load bar
    \draw[fill=blue!30, thick] (0, 2) rectangle (10, 3);
    \node[font=\small] at (5, 2.5) {Load one tile: \textbf{1000 cycles}};

    % Compute bar (to scale)
    \draw[fill=red!50, thick] (0, 0) rectangle (1.28, 1);
    \node[font=\small, right] at (1.5, 0.5) {Compute one tile: \textbf{128 cycles}};

    % Scale comparison
    \draw[<->, thick, green!60!black] (10.5, 0) -- (10.5, 3);
    \node[right, font=\small\bfseries, green!60!black] at (10.7, 1.5) {8x gap!};

    % Explanation
    \node[draw, rounded corners, fill=orange!10, font=\small, align=left] at (5, -1.5)
        {With \textbf{2 buffers}: while computing Buf0 (128 cycles),\\we only cover 128/1000 = \textbf{12.8\%} of the next load.};
\end{tikzpicture}
\end{document}
```

. . .

To **fully hide** the load latency, we need enough compute work to fill 1000 cycles.

## How Many Buffers Do We Need?

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.38, transform shape]
    % Timeline
    \draw[->, thick] (0, -0.3) -- (22, -0.3) node[right, font=\small] {Cycles};
    \foreach \x/\label in {0/0, 1.8/128, 14/1000} {
        \draw (\x, -0.5) -- (\x, -0.1);
        \node[below, font=\tiny] at (\x, -0.6) {\label};
    }

    % 8 load pipelines (staggered, each 1000 cycles = 14 units wide)
    \node[left, font=\small\bfseries, align=right] at (-0.3, 7) {8 loads\\in flight};
    \foreach \i in {0,...,7} {
        \pgfmathtruncatemacro{\shade}{20+\i*9}
        \pgfmathparse{\i*0.8}
        \edef\ybase{\pgfmathresult}
        \pgfmathparse{\i*1.8}
        \edef\xstart{\pgfmathresult}
        \pgfmathparse{\xstart+14}
        \edef\xend{\pgfmathresult}
        \draw[fill=blue!\shade, rounded corners=1pt]
            (\xstart, \ybase+0.8) rectangle (\xend, \ybase+1.4);
        \node[font=\tiny, white] at ({(\xstart+\xend)/2}, \ybase+1.1) {Load tile \i};
        \node[font=\tiny, right] at (\xend+0.1, \ybase+1.1) {Buf \i};
    }

    % Brace on left
    \draw[decorate, decoration={brace, amplitude=4pt, mirror}, thick]
        (-0.2, 0.8) -- (-0.2, 7.8);

    % Compute pipeline at bottom
    \node[left, font=\small\bfseries, align=right] at (-0.3, 0.2) {Compute};
    \foreach \i in {0,...,4} {
        \pgfmathtruncatemacro{\shade}{40+\i*12}
        \pgfmathparse{14+\i*1.8}
        \edef\xstart{\pgfmathresult}
        \pgfmathparse{\xstart+1.8}
        \edef\xend{\pgfmathresult}
        \draw[fill=red!\shade, rounded corners=1pt]
            (\xstart, -0.1) rectangle (\xend, 0.5);
        \node[font=\tiny] at ({(\xstart+\xend)/2}, 0.2) {C\i};
    }

    % Dashed line: first load completes
    \draw[dashed, gray] (14, -0.1) -- (14, 8.2);
    \node[font=\tiny, align=center, gray] at (14, 8.6) {Tile 0 ready};

    % Annotation: tiles arrive staggered
    \node[draw, rounded corners, fill=yellow!20, font=\tiny, align=center] at (10.5, -1.5)
        {Each load = 1000 cycles, staggered by 128 cycles $\Rightarrow$ tiles arrive one per compute};
\end{tikzpicture}
\end{document}
```


## 8 Buffers: Just Enough

We need enough compute to fill the 1000-cycle wait. Each tile = 128 cycles:

$$\left\lceil\frac{1000}{128}\right\rceil = 8 \text{ buffers}$$

. . .

| Buffers | Compute while waiting | Still stalling? |
|---|---|---|
| 2 (double) | $2 \times 128 = 256$ cycles | Yes --- 744 cycles idle |
| **8** | $8 \times 128 = 1024$ cycles | **No!** |

## Applying This to Our Pipeline

Loads are **non-blocking** (`cp.async` --- more on this later).

You **issue** a load and immediately move on. The data arrives ~1000 cycles later.

. . .

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.52, transform shape]
    \node[font=\bfseries\small] at (10, 3.8) {Steady State: Compute + Issue One Load Per Iteration};

    \node[left, font=\small\bfseries] at (-0.3, 2.5) {Compute};
    \foreach \i in {0,...,7} {
        \pgfmathtruncatemacro{\shade}{30+\i*8}
        \draw[fill=red!\shade] (\i*2.3+1, 2.1) rectangle (\i*2.3+2.8, 2.9);
        \node[font=\tiny] at (\i*2.3+1.9, 2.5) {C\i};
    }

    \node[left, font=\small\bfseries] at (-0.3, 1) {Issue load};
    \foreach \i in {0,...,7} {
        \pgfmathtruncatemacro{\idx}{\i+8}
        \draw[fill=blue!30] (\i*2.3+1, 0.6) rectangle (\i*2.3+1.5, 1.4);
        \node[font=\tiny] at (\i*2.3+1.25, 1) {L\idx};
    }
    \node[font=\small, right] at (19.5, 1) {(for 8 iterations later)};

    % Takeaway
    \node[draw, rounded corners, fill=green!15, font=\small, align=center] at (10, -0.8)
        {Each iteration: compute one tile (128 cycles) + issue one future load.\\Loads complete 1000 cycles later = 8 iterations later = just in time!};
\end{tikzpicture}
\end{document}
```


## Async Copies: Hardware Support {.fragile}

Since Ampere (A100), NVIDIA added **hardware async copy** instructions:

\small

```
// PTX: async copy from global to shared memory
cp.async.ca.shared.global [dst_smem], [src_global], 16;
cp.async.commit_group;
// ... do compute on previous tile ...
cp.async.wait_group 0;  // wait for copy to finish
```

. . .

\normalsize

**Key benefits:**

- Copy happens **in the background** -- no thread involvement during transfer
- Threads are free to compute while the copy engine moves data
- No need for threads to load into registers first, then store to shared memory

. . .

This is the hardware mechanism that makes multi-stage software pipelining practical.

## Software Pipeline: The Complete Picture {.fragile}

\tiny

```cpp
// Pseudocode: 3-stage pipelined GEMM
__shared__ float As[3][TILE][TILE];  // 3 buffers for A
__shared__ float Bs[3][TILE][TILE];  // 3 buffers for B

// Prologue: fill the pipeline
async_load(As[0], Bs[0], tile=0);
async_load(As[1], Bs[1], tile=1);

for (int t = 0; t < num_tiles; t++) {
    int cur = t % 3;        // buffer to compute from
    int nxt = (t + 2) % 3;  // buffer to load into

    wait_for(cur);           // ensure current tile is loaded

    if (t + 2 < num_tiles)
        async_load(As[nxt], Bs[nxt], tile=t+2);  // prefetch

    // Compute from current buffer (Tensor Core MMA)
    for (int k = 0; k < TILE; k++)
        mma(acc, As[cur], Bs[cur], k);
}
```

. . .

**The pipeline has 3 phases running simultaneously:**

1. **Load** tile $t+2$ from global $\to$ shared memory (async)
2. **Compute** tile $t$ from shared memory (Tensor Cores)
3. Tile $t+1$ is **in flight** (being loaded)

# Tile-Level Programming

## The Semantic Gap

Writing high-performance GPU kernels in CUDA requires managing:

- Thread-to-data mapping for thousands of threads
- Shared memory allocation, loading, and synchronization
- Bank conflict avoidance (swizzling)
- Multi-stage pipeline buffers and async copies
- Tensor Core fragment layout across warp threads

. . .

**But our algorithm thinks in tiles, not threads.**

We want to say: "Load this tile, compute this tile, pipeline the loads."

. . .

**Tile-level languages** (TileLang, Triton) close this gap: you write tile operations, the compiler generates the thread-level code.

## TileLang: GEMM in 20 Lines {.fragile}

\tiny

```python
import tilelang as T
import tilelang.language as TL

def matmul(M, N, K, block_M, block_N, block_K):
    @T.prim_func
    def main(
        A: T.Buffer((M, K), "float16"),
        B: T.Buffer((K, N), "float16"),
        C: T.Buffer((M, N), "float16"),
    ):
        with T.Kernel(
            T.ceildiv(N, block_N), T.ceildiv(M, block_M),
            threads=128
        ) as (bx, by):
            acc = T.alloc_fragment((block_M, block_N), "float32")
            T.clear(acc)
            A_s = T.alloc_shared((block_M, block_K), "float16")
            B_s = T.alloc_shared((block_K, block_N), "float16")
            for k in T.Pipelined(T.ceildiv(K, block_K), num_stages=3):
                T.copy(A[by*block_M, k*block_K], A_s)
                T.copy(B[k*block_K, bx*block_N], B_s)
                T.gemm(A_s, B_s, acc)
            T.copy(acc, C[by*block_M, bx*block_N])
    return main
```

## Reading the TileLang Code

**Line by line, this maps directly to our concepts:**

. . .

`T.Kernel(ceildiv(N, block_N), ceildiv(M, block_M), threads=128)`

$\to$ Grid of thread blocks, each is a **team** of 128 threads

. . .

`acc = T.alloc_fragment(...)` -- accumulator in **registers** (the "hands")

`A_s = T.alloc_shared(...)` -- tile buffer in **shared memory** (the "workbench")

. . .

`T.Pipelined(..., num_stages=3)` -- **3-stage software pipeline** (auto-generates async copies and buffer rotation)

. . .

`T.copy(A[...], A_s)` -- explicit data movement: global $\to$ shared

`T.gemm(A_s, B_s, acc)` -- shared $\to$ registers $\to$ **Tensor Core MMA**

## What the Compiler Generates

From the 20-line TileLang code, the compiler produces:

. . .

1. **Thread mapping:** 128 threads per block, work distribution for loads and computes
2. **Shared memory layout:** Swizzled to avoid bank conflicts
3. **Async copies:** `cp.async` instructions for global $\to$ shared transfers
4. **Pipeline scheduling:** 3 shared memory buffers with proper synchronization
5. **Tensor Core instructions:** `mma.sync` PTX for the matrix multiply
6. **Coalesced access:** Threads load consecutive addresses from global memory

. . .

**This is hundreds of lines of CUDA/PTX generated from 20 lines of TileLang.**

## Beyond This Lecture

\small

Of course, the real world is more complex:

- **Irregular shapes:** Real models have non-power-of-2 dimensions, requiring padding and masking
- **Mixed precision:** FP8, FP4, and sparsity on Blackwell push the roofline even higher
- **Operator fusion:** Fusing GEMM with activation, bias, and normalization avoids round-trips to HBM
- **Multi-GPU:** Distributing across GPUs introduces communication (NVLink, PCIe) as a new bottleneck
- **Kernel tuning:** Tile sizes, pipeline depths, and occupancy trade-offs are workload-dependent

. . .

**But the core principles from this lecture carry through:**

Tiling reduces data movement. Tensor Cores accelerate compute. Software pipelining hides latency. The roofline tells you which one matters.