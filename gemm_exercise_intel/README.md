# GEMM Progressive Optimization Demo - Intel AVX2/FMA Version

This is the Intel port of the GEMM optimization exercise, demonstrating how to achieve high-performance matrix multiplication on Intel (and AMD) CPUs with AVX2 and FMA instructions.

## Requirements

- Linux (tested on Ubuntu)
- GCC or Clang with AVX2/FMA support
- OpenBLAS library
- CPU with AVX2+FMA support (Intel Haswell+, AMD Zen+)

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential libopenblas-dev
```

### Install Dependencies (Fedora/RHEL)

```bash
sudo dnf install gcc openblas-devel
```

## Build and Run

```bash
make
make run
```

Or manually:

```bash
gcc -O3 -march=native -mavx2 -mfma -o gemm_progressive gemm_progressive.c -lopenblas -lm
OPENBLAS_NUM_THREADS=1 ./gemm_progressive
```

## Expected Output

```
╔══════════════════════════════════════════════════════════════════╗
║      GEMM Progressive Optimization - Intel AVX2/FMA (N=1024)     ║
╠══════════════════════════════════════════════════════════════════╣
║ Stage  Implementation          GFLOPS    %Peak    vs OpenBLAS    ║
╠══════════════════════════════════════════════════════════════════╣
║   -   OpenBLAS (ref)            169       94%     (baseline)     ║
║   1   1. Naive                    1        1%        0.6%        ║
║   2   2. Blocked                 10        6%        5.8%        ║
║   3   3. Pack A (8x8)           139       78%       82%          ║
║   4   4. Pack B (8x8)           140       78%       83%          ║
║   5   5. Kernel (6x16+4x16)     150       84%       89%          ║
║   6   6. Tuned                  166       93%       98%          ║
║   7   7. Lazy                   167       93%       99%          ║
╚══════════════════════════════════════════════════════════════════╝
```

*Actual numbers vary depending on CPU model and current clock speed.*

## The Seven Optimization Stages

| Stage | Description | GFLOPS | Key Technique |
|-------|-------------|--------|---------------|
| 1. Naive | Triple nested loop | ~1 | Baseline - no optimization |
| 2. Blocked | Cache blocking | ~10 | Reuse data in L1/L2 cache |
| 3. Pack A | + A packing + 8x8 AVX2 kernel | ~139 | SIMD + sequential access |
| 4. Pack B | + On-the-fly B packing | ~140 | Eliminate strided B access |
| 5. Kernel | 6x16+4x16 hybrid kernel | ~150 | Optimal FLOPs/load ratio |
| 6. Tuned | Optimal blocking parameters | ~166 | MC=1024, KC=64, NC=1024 |
| 7. Lazy | Lazy A packing | ~167 | JIT packing for cache locality |

### Why 6x16 Beats 8x8

The key insight is **FLOPs per memory load**:

```
┌──────────┬─────────┬───────────┬───────────────┐
│ Kernel   │ C regs  │ FLOPs/ld  │ Status        │
├──────────┼─────────┼───────────┼───────────────┤
│ 8×8      │ 8       │ 14.2      │ Baseline      │
│ 4×16     │ 8       │ 21.3      │ Good          │
│ 6×16     │ 12      │ 24.0      │ OPTIMAL       │
│ 8×16     │ 16      │ 25.6      │ SPILLS!       │
└──────────┴─────────┴───────────┴───────────────┘
```

6x16 uses 12 of 16 YMM registers for C, leaving 4 for A/B temps. 8x16 would need all 16 registers for C alone, causing costly memory spills.

## Key Differences from Apple Silicon Version

| Aspect | Apple AMX | Intel AVX2+FMA |
|--------|-----------|----------------|
| SIMD width | 512-bit (AMX tiles) | 256-bit (YMM registers) |
| Optimal micro-kernel | 32x32 (fixed) | 6x16 (derived) |
| Peak (single-core) | ~1600 GFLOPS | ~160-180 GFLOPS |
| Matrix instruction | 16x16 outer product | 8-wide FMA |

While Intel's absolute performance is lower than Apple's AMX coprocessor, the **optimization principles are identical**:

1. Memory hierarchy awareness
2. Cache blocking (Goto algorithm)
3. Data packing for sequential access
4. Micro-kernel optimization (FLOPs/load analysis)
5. Parameter tuning for cache sizes

## AMD Compatibility

This code runs on AMD Ryzen/EPYC processors without modification (AVX2+FMA supported since Zen). For optimal AMD performance, you may want to adjust blocking parameters due to different cache sizes:

```c
// AMD Zen 3/4 has smaller L1 (32KB vs 48KB)
#define KC_TUNED 48    // instead of 64
```

## Learning Objectives

After studying this code, you should understand:

1. Why naive matrix multiplication is slow (memory bandwidth limited)
2. How cache blocking improves performance
3. Why data layout matters (packing)
4. How SIMD instructions (AVX2/FMA) accelerate computation
5. **How to choose optimal micro-kernel size (FLOPs/load analysis)**
6. The importance of tuning parameters for your specific hardware

## Files

- `gemm_progressive.c` - Main implementation with all 7 stages
- `Makefile` - Build configuration
- `README.md` - This file
- `chapter_gemm_optimization_intel.md` - Detailed documentation

## Exercises

1. **Observe the progression**: Run the demo and identify the single biggest performance jump. Why?

2. **Verify FLOPs/load**: Calculate FLOPs/load for 4x8, 8x8, and 6x16 kernels. Do the measured results match?

3. **Tune parameters**: Modify `KC_TUNED` from 64 to 128. What happens? Why?

4. **Edge case analysis**: Change `N` to 1000, 1001, 1002. How does performance vary?

5. **Profile with perf**: Run `perf stat ./gemm_progressive` to see cache miss rates across stages.

6. **Try 8x16**: Implement an 8x16 kernel and verify it's slower due to register spilling.
