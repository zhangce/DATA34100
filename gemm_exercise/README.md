# GEMM Optimization Exercise

This exercise demonstrates progressive optimization of matrix multiplication (GEMM) on Apple Silicon, achieving performance that matches Apple's optimized Accelerate framework.

## Requirements

- Apple Silicon Mac (M1/M2/M3)
- Xcode Command Line Tools (`xcode-select --install`)

## Quick Start

```bash
# Build
make

# Run (single-threaded for fair comparison)
make run
```

Or manually:
```bash
clang -O3 -mcpu=apple-m1 -o gemm_progressive gemm_progressive.c -framework Accelerate
VECLIB_MAXIMUM_THREADS=1 ./gemm_progressive
```

## Files

| File | Description |
|------|-------------|
| `gemm_progressive.c` | Six GEMM implementations showing optimization progression |
| `amx.h` | Apple AMX (matrix coprocessor) instruction definitions |
| `chapter_gemm_optimization.md` | Detailed textbook chapter explaining each optimization |

## Expected Output

```
╔══════════════════════════════════════════════════════════════════╗
║ Stage  Implementation         GFLOPS   % Peak   vs Accelerate   ║
╠══════════════════════════════════════════════════════════════════╣
║   -    Accelerate (ref)         1377     84%     (baseline)      ║
║   1    Naive                       2      0%     629x slower     ║
║   2    Blocked                    42      3%      33x slower     ║
║   3    + Pack A                  723     44%     1.9x slower     ║
║   4    + Pack B                 1065     65%     1.3x slower     ║
║   5    + Tuned                  1515     93%     1.1x FASTER     ║
║   6    + Lazy                   1491     91%     1.1x FASTER     ║
╚══════════════════════════════════════════════════════════════════╝
```

## Learning Objectives

After completing this exercise, you will understand:

1. **Memory hierarchy impact**: Why naive GEMM achieves only 0.1% of peak
2. **Cache blocking**: The Goto algorithm's 6-level loop structure
3. **Data packing**: How memory layout affects performance (17x improvement)
4. **Hardware acceleration**: Using Apple's AMX matrix coprocessor
5. **Parameter tuning**: Finding optimal block sizes for your cache hierarchy

## Exercises

See Section 14 of `chapter_gemm_optimization.md` for hands-on exercises.
