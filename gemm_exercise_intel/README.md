# GEMM Progressive Optimization Demo - Intel AVX2/FMA Version

This is the Intel port of the GEMM optimization exercise, demonstrating how to achieve high-performance matrix multiplication on Intel CPUs with AVX2 and FMA instructions.

## Requirements

- Linux (tested on Ubuntu)
- GCC or Clang with AVX2/FMA support
- OpenBLAS library

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
+====================================================================+
|     GEMM Progressive Optimization Demo - Intel AVX2/FMA           |
|                        (N=1024)                                     |
+====================================================================+
| Stage  Implementation         GFLOPS   % Peak   vs OpenBLAS      |
+--------------------------------------------------------------------+
|   -    OpenBLAS (ref)          166      93%     (baseline)        |
|   1    Naive                     1       1%     ~150x slower      |
|   2    Blocked                  10       6%      ~17x slower      |
|   3    + Pack A                120      67%      1.4x slower      |
|   4    + Pack B                135      76%      1.2x slower      |
|   5    + Tuned                 136      76%      1.2x slower      |
|   6    + Lazy                  137      77%      1.2x slower      |
+--------------------------------------------------------------------+
| Theoretical peak: 179 GFLOPS (single P-core, AVX2+FMA @ 5.6GHz)  |
+====================================================================+
```

*Actual numbers vary depending on CPU model and current clock speed.*

## Optimization Stages

| Stage | Description | Key Technique |
|-------|-------------|---------------|
| 1. Naive | Triple nested loop | Baseline - no optimization |
| 2. Blocked | Cache blocking | Reuse data in L1/L2 cache |
| 3. + Pack A | A matrix packing | Sequential memory access for A |
| 4. + Pack B | On-the-fly B packing | Sequential memory access for B |
| 5. + Tuned | Optimal parameters | MC=512, KC=64, NC=512 |
| 6. + Lazy | Lazy A packing | Just-in-time packing for cache locality |

## Key Differences from Apple Silicon Version

| Aspect | Apple AMX | Intel AVX2+FMA |
|--------|-----------|----------------|
| SIMD width | 512-bit (AMX tiles) | 256-bit (YMM registers) |
| Micro-kernel | 32x32 | 8x8 |
| Peak (single-core) | ~1600 GFLOPS | ~100-160 GFLOPS |
| Matrix instruction | 16x16 outer product | 8-wide FMA |

While Intel's absolute performance is lower than Apple's AMX coprocessor, the **optimization principles are identical**:

1. Memory hierarchy awareness
2. Cache blocking (Goto algorithm)
3. Data packing for sequential access
4. Parameter tuning for cache sizes

## Learning Objectives

After studying this code, you should understand:

1. Why naive matrix multiplication is slow (memory bandwidth limited)
2. How cache blocking improves performance
3. Why data layout matters (packing)
4. How SIMD instructions (AVX2/FMA) accelerate computation
5. The importance of tuning parameters for your specific hardware

## Files

- `gemm_progressive.c` - Main implementation with all 6 stages
- `Makefile` - Build configuration
- `README.md` - This file
- `chapter_gemm_optimization_intel.md` - Detailed documentation

## Exercises

1. **Vary matrix size**: Change `N` from 512 to 2048 and observe how performance changes.

2. **Tune parameters**: Modify `MC_TUNED`, `KC_TUNED`, `NC_TUNED` and measure the impact.

3. **Disable FMA**: Replace `_mm256_fmadd_ps` with separate multiply and add. Measure the difference.

4. **Profile with perf**: Run `perf stat ./gemm_progressive` to see cache miss rates.

5. **Compare with MKL**: If you have Intel MKL installed, link against it instead of OpenBLAS.
