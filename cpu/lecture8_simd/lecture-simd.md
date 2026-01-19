---
title: Lecture 8. SIMD and Vectorization
subtitle: Introductions to Data Systems and Data Design
author: Ce Zhang
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing}
---

# What is SIMD?

## Flynn's Taxonomy

|                    | Single Instruction | Multiple Instruction |
|--------------------|-------------------|---------------------|
| **Single Data**    | SISD (Uniprocessor) | MISD |
| **Multiple Data**  | **SIMD** (Vector) | MIMD (Multicore) |

. . .

**SIMD = Single Instruction, Multiple Data**

One instruction operates on multiple data elements simultaneously.

## SIMD: The Big Picture

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]

    % Scalar
    \node at (-2, 2) {\textbf{Scalar:}};
    \draw[fill=blue!20] (0, 1.7) rectangle (1.2, 2.3);
    \node at (0.6, 2) {a[0]};
    \node at (1.6, 2) {+};
    \draw[fill=green!20] (2, 1.7) rectangle (3.2, 2.3);
    \node at (2.6, 2) {b[0]};
    \node at (3.6, 2) {=};
    \draw[fill=orange!20] (4, 1.7) rectangle (5.2, 2.3);
    \node at (4.6, 2) {c[0]};
    \node at (6.5, 2) {1 operation};

    % SIMD
    \node at (-2, 0) {\textbf{SIMD (4-way):}};
    \draw[fill=blue!20] (0, 0.9) rectangle (1.2, 1.3);
    \node at (0.6, 1.1) {a[0]};
    \draw[fill=blue!20] (0, 0.4) rectangle (1.2, 0.8);
    \node at (0.6, 0.6) {a[1]};
    \draw[fill=blue!20] (0, -0.1) rectangle (1.2, 0.3);
    \node at (0.6, 0.1) {a[2]};
    \draw[fill=blue!20] (0, -0.6) rectangle (1.2, -0.2);
    \node at (0.6, -0.4) {a[3]};

    \node at (1.6, 0.3) {+};

    \draw[fill=green!20] (2, 0.9) rectangle (3.2, 1.3);
    \node at (2.6, 1.1) {b[0]};
    \draw[fill=green!20] (2, 0.4) rectangle (3.2, 0.8);
    \node at (2.6, 0.6) {b[1]};
    \draw[fill=green!20] (2, -0.1) rectangle (3.2, 0.3);
    \node at (2.6, 0.1) {b[2]};
    \draw[fill=green!20] (2, -0.6) rectangle (3.2, -0.2);
    \node at (2.6, -0.4) {b[3]};

    \node at (3.6, 0.3) {=};

    \draw[fill=orange!20] (4, 0.9) rectangle (5.2, 1.3);
    \node at (4.6, 1.1) {c[0]};
    \draw[fill=orange!20] (4, 0.4) rectangle (5.2, 0.8);
    \node at (4.6, 0.6) {c[1]};
    \draw[fill=orange!20] (4, -0.1) rectangle (5.2, 0.3);
    \node at (4.6, 0.1) {c[2]};
    \draw[fill=orange!20] (4, -0.6) rectangle (5.2, -0.2);
    \node at (4.6, -0.4) {c[3]};

    \node at (6.5, 0.3) {4 operations};
    \node at (6.5, -0.3) {\textbf{1 instruction!}};

\end{tikzpicture}
\end{document}
```

## Why SIMD Matters

**Theoretical speedup:** 4-8x for vectorizable code

| Extension | Register Width | Doubles | Floats |
|-----------|---------------|---------|--------|
| SSE/SSE2 | 128 bits | 2-way | 4-way |
| AVX/AVX2 | 256 bits | 4-way | 8-way |
| AVX-512 | 512 bits | 8-way | 16-way |

. . .

**SIMD + ILP together:**

- ILP: Multiple *different* instructions in parallel
- SIMD: One instruction on *multiple data*
- Combine both for maximum throughput!

## Evolution of x86 SIMD

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]

    % Timeline
    \draw[->, thick] (0, 0) -- (12, 0) node[right] {Time};

    % SSE
    \draw[fill=blue!20] (1, 0.2) rectangle (3, 1.2);
    \node at (2, 0.7) {SSE/SSE2};
    \node[below] at (2, 0) {1999};
    \node[above] at (2, 1.2) {128-bit};

    % AVX
    \draw[fill=green!20] (4, 0.2) rectangle (6, 1.5);
    \node at (5, 0.85) {AVX/AVX2};
    \node[below] at (5, 0) {2011};
    \node[above] at (5, 1.5) {256-bit};

    % AVX-512
    \draw[fill=orange!20] (7, 0.2) rectangle (9.5, 1.8);
    \node at (8.25, 1) {AVX-512};
    \node[below] at (8.25, 0) {2017};
    \node[above] at (8.25, 1.8) {512-bit};

    % Our focus
    \draw[thick, red, dashed] (3.8, -0.3) rectangle (6.2, 2);
    \node[red] at (5, 2.3) {Our focus};

\end{tikzpicture}
\end{document}
```

**AVX2** is widely available (Sandy Bridge 2011+). We'll focus on it.

# AVX Registers and Data Types

## AVX Vector Registers

**16 registers:** `ymm0` - `ymm15`, each 256 bits wide

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]

    % Register
    \node[left] at (0, 2) {\texttt{ymm0}};
    \draw[fill=blue!10] (0.5, 1.7) rectangle (8.5, 2.3);

    % Subdivisions for doubles
    \draw (2.5, 1.7) -- (2.5, 2.3);
    \draw (4.5, 1.7) -- (4.5, 2.3);
    \draw (6.5, 1.7) -- (6.5, 2.3);

    \node at (1.5, 2) {d0};
    \node at (3.5, 2) {d1};
    \node at (5.5, 2) {d2};
    \node at (7.5, 2) {d3};

    \node[right] at (9, 2) {4 doubles (64-bit each)};

    % Register for floats
    \node[left] at (0, 0.5) {\texttt{ymm1}};
    \draw[fill=green!10] (0.5, 0.2) rectangle (8.5, 0.8);

    \draw (1.5, 0.2) -- (1.5, 0.8);
    \draw (2.5, 0.2) -- (2.5, 0.8);
    \draw (3.5, 0.2) -- (3.5, 0.8);
    \draw (4.5, 0.2) -- (4.5, 0.8);
    \draw (5.5, 0.2) -- (5.5, 0.8);
    \draw (6.5, 0.2) -- (6.5, 0.8);
    \draw (7.5, 0.2) -- (7.5, 0.8);

    \node at (1, 0.5) {f0};
    \node at (2, 0.5) {f1};
    \node at (3, 0.5) {f2};
    \node at (4, 0.5) {f3};
    \node at (5, 0.5) {f4};
    \node at (6, 0.5) {f5};
    \node at (7, 0.5) {f6};
    \node at (8, 0.5) {f7};

    \node[right] at (9, 0.5) {8 floats (32-bit each)};

\end{tikzpicture}
\end{document}
```

**Same register, different interpretations!**

## Data Types in C/C++

```cpp
#include <immintrin.h>  // AVX intrinsics header

__m256d  d;  // 4 doubles (256 bits)
__m256   f;  // 8 floats (256 bits)
__m256i  i;  // 32 bytes / 16 shorts / 8 ints / 4 longs
```

. . .

**Naming convention:**

- `__m256` = 256-bit register
- `d` suffix = double precision
- `i` suffix = integer
- No suffix = single precision (float)

## Register Hierarchy

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]

    % ZMM (512-bit)
    \draw[fill=orange!20] (0, 2) rectangle (10, 2.8);
    \node at (5, 2.4) {\texttt{zmm0} (512 bits) - AVX-512};

    % YMM (256-bit)
    \draw[fill=green!30] (0, 2) rectangle (5, 2.8);
    \node at (2.5, 2.4) {\texttt{ymm0} (256 bits) - AVX};

    % XMM (128-bit)
    \draw[fill=blue!40] (0, 2) rectangle (2.5, 2.8);
    \node at (1.25, 2.4) {\texttt{xmm0}};

    % Labels
    \node[right] at (10.5, 2.4) {Lower half is ymm};
    \draw[->] (5, 1.5) -- (2.5, 2);
    \node at (5, 1.2) {Lower half is xmm};

\end{tikzpicture}
\end{document}
```

`xmm0` is the lower 128 bits of `ymm0`, which is the lower 256 bits of `zmm0`.

# AVX Intrinsics

## What Are Intrinsics?

**Intrinsics** = C functions that map directly to assembly instructions

```cpp
// C code with intrinsics
__m256d a = _mm256_load_pd(ptr);    // Load 4 doubles
__m256d b = _mm256_load_pd(ptr2);
__m256d c = _mm256_add_pd(a, b);    // Add 4 doubles
_mm256_store_pd(result, c);          // Store 4 doubles
```

```asm
; Generated assembly
vmovapd ymm0, [rdi]      ; Load aligned
vmovapd ymm1, [rsi]
vaddpd  ymm0, ymm0, ymm1 ; Packed double add
vmovapd [rdx], ymm0      ; Store aligned
```

**Like writing assembly, but in C!**

## Intrinsic Naming Convention

`_mm256_<operation>_<suffix>`

| Part | Meaning |
|------|---------|
| `_mm256` | 256-bit AVX operation |
| `_mm` | 128-bit SSE operation |
| `_mm512` | 512-bit AVX-512 operation |

| Suffix | Data Type |
|--------|-----------|
| `pd` | Packed double (4 doubles) |
| `ps` | Packed single (8 floats) |
| `epi32` | Packed 32-bit integers |
| `si256` | 256-bit integer |

## Load and Store

```cpp
// Aligned load/store (pointer must be 32-byte aligned!)
__m256d a = _mm256_load_pd(ptr);     // Load 4 doubles
_mm256_store_pd(ptr, a);              // Store 4 doubles

// Unaligned load/store (any pointer)
__m256d b = _mm256_loadu_pd(ptr);    // Slower on some CPUs
_mm256_storeu_pd(ptr, b);
```

. . .

**Alignment matters!**

- Aligned: pointer address divisible by 32
- Unaligned load on aligned boundary: seg fault!
- Modern CPUs: unaligned is nearly as fast (but prefer aligned)

## Setting Constants

```cpp
// Set all elements to the same value
__m256d ones = _mm256_set1_pd(1.0);
// Result: [1.0, 1.0, 1.0, 1.0]

// Set each element individually (note: reverse order!)
__m256d v = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
// Result: [1.0, 2.0, 3.0, 4.0]  (LSB first!)

// Set to zero
__m256d zero = _mm256_setzero_pd();
// Result: [0.0, 0.0, 0.0, 0.0]
```

. . .

**Warning:** `_mm256_set_pd` takes arguments in **reverse order**!

## Arithmetic Operations

```cpp
__m256d c;

c = _mm256_add_pd(a, b);   // c[i] = a[i] + b[i]
c = _mm256_sub_pd(a, b);   // c[i] = a[i] - b[i]
c = _mm256_mul_pd(a, b);   // c[i] = a[i] * b[i]
c = _mm256_div_pd(a, b);   // c[i] = a[i] / b[i]

c = _mm256_sqrt_pd(a);     // c[i] = sqrt(a[i])
c = _mm256_max_pd(a, b);   // c[i] = max(a[i], b[i])
c = _mm256_min_pd(a, b);   // c[i] = min(a[i], b[i])

// Fused multiply-add (AVX2): c = a*b + c
c = _mm256_fmadd_pd(a, b, c);  // One instruction!
```

## Performance: Latency and Throughput

| Operation | Latency | Throughput |
|-----------|---------|------------|
| `add_pd` | 4 cycles | 2/cycle |
| `mul_pd` | 4 cycles | 2/cycle |
| `div_pd` | 13-14 cycles | 1/8 cycle |
| `sqrt_pd` | 18 cycles | 1/12 cycle |
| `fmadd_pd` | 4 cycles | 2/cycle |
| `load_pd` | 7 cycles | 2/cycle |

. . .

**FMA is crucial:** `a*b + c` in one instruction with same latency as add alone!

# Example: Vector Addition

## Scalar Version

```cpp
// add_v1.cpp
void add_scalar(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}
```

```bash
$ cd examples && make add
```

## SIMD Version

```cpp
// add_v2.cpp
void add_simd(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        __m256d vc = _mm256_add_pd(va, vb);
        _mm256_storeu_pd(c + i, vc);
    }
}
```

**2x speedup!** (Limited by memory bandwidth, not compute)

## Visualizing the SIMD Loop

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]

    % Arrays
    \node at (-1, 3) {a:};
    \draw[fill=blue!20] (0, 2.7) rectangle (0.8, 3.3);
    \draw[fill=blue!20] (0.8, 2.7) rectangle (1.6, 3.3);
    \draw[fill=blue!20] (1.6, 2.7) rectangle (2.4, 3.3);
    \draw[fill=blue!20] (2.4, 2.7) rectangle (3.2, 3.3);
    \draw[fill=blue!10] (3.2, 2.7) rectangle (4, 3.3);
    \draw[fill=blue!10] (4, 2.7) rectangle (4.8, 3.3);
    \node at (5.5, 3) {...};

    \node at (-1, 2) {b:};
    \draw[fill=green!20] (0, 1.7) rectangle (0.8, 2.3);
    \draw[fill=green!20] (0.8, 1.7) rectangle (1.6, 2.3);
    \draw[fill=green!20] (1.6, 1.7) rectangle (2.4, 2.3);
    \draw[fill=green!20] (2.4, 1.7) rectangle (3.2, 2.3);
    \draw[fill=green!10] (3.2, 1.7) rectangle (4, 2.3);
    \draw[fill=green!10] (4, 1.7) rectangle (4.8, 2.3);
    \node at (5.5, 2) {...};

    % Bracket for iteration 1
    \draw[thick, red] (0, 2.5) -- (0, 3.5) -- (3.2, 3.5) -- (3.2, 2.5);
    \node[red, above] at (1.6, 3.5) {Iteration 1 (i=0..3)};

    % Bracket for iteration 2
    \draw[thick, orange, dashed] (3.2, 1.5) -- (3.2, 0.8) -- (6.4, 0.8) -- (6.4, 1.5);
    \node[orange, below] at (4.8, 0.8) {Iteration 2 (i=4..7)};

    % Result
    \node at (-1, 0.5) {c:};
    \draw[fill=orange!30] (0, 0.2) rectangle (0.8, 0.8);
    \draw[fill=orange!30] (0.8, 0.2) rectangle (1.6, 0.8);
    \draw[fill=orange!30] (1.6, 0.2) rectangle (2.4, 0.8);
    \draw[fill=orange!30] (2.4, 0.2) rectangle (3.2, 0.8);
    \node at (5.5, 0.5) {...};

\end{tikzpicture}
\end{document}
```

Each loop iteration processes **4 elements** with one SIMD instruction.

# Example: Sum Reduction (Revisited)

## The Challenge

From ILP lecture: reductions have dependency chains.

```cpp
double sum = 0;
for (int i = 0; i < n; i++) {
    sum += a[i];  // Each add depends on previous!
}
```

**SIMD alone doesn't help** — we still have a single accumulator.

. . .

**Solution:** SIMD + multiple accumulators!

## SIMD Sum: Step 1 - Vector Accumulator

```cpp
__m256d vsum = _mm256_setzero_pd();  // [0, 0, 0, 0]

for (int i = 0; i < n; i += 4) {
    __m256d v = _mm256_loadu_pd(a + i);
    vsum = _mm256_add_pd(vsum, v);   // 4 parallel sums!
}

// vsum = [sum0, sum1, sum2, sum3]
// Final: sum = sum0 + sum1 + sum2 + sum3
```

. . .

Now we have **4 independent accumulator chains** — 4x ILP!

## SIMD Sum: Step 2 - Horizontal Reduction

```cpp
// Reduce [sum0, sum1, sum2, sum3] to a single value
// Method 1: Extract and add scalars
double result[4];
_mm256_storeu_pd(result, vsum);
double sum = result[0] + result[1] + result[2] + result[3];
```

. . .

```cpp
// Method 2: Use horizontal add (faster)
__m128d low  = _mm256_castpd256_pd128(vsum);      // [sum0, sum1]
__m128d high = _mm256_extractf128_pd(vsum, 1);    // [sum2, sum3]
__m128d sum128 = _mm_add_pd(low, high);           // [sum0+sum2, sum1+sum3]
sum128 = _mm_hadd_pd(sum128, sum128);             // [total, total]
double sum = _mm_cvtsd_f64(sum128);               // Extract scalar
```

## Complete SIMD Sum

```cpp
// sum_v1.cpp
double sum_simd(double* a, int n) {
    __m256d vsum = _mm256_setzero_pd();

    for (int i = 0; i < n; i += 4) {
        __m256d v = _mm256_loadu_pd(a + i);
        vsum = _mm256_add_pd(vsum, v);
    }

    // Horizontal reduction
    __m128d low = _mm256_castpd256_pd128(vsum);
    __m128d high = _mm256_extractf128_pd(vsum, 1);
    __m128d sum128 = _mm_add_pd(low, high);
    sum128 = _mm_hadd_pd(sum128, sum128);
    return _mm_cvtsd_f64(sum128);
}
```

## SIMD Sum: Even Better with Unrolling

```cpp
// sum_v2.cpp
double sum_simd_unrolled(double* a, int n) {
    __m256d vsum1 = _mm256_setzero_pd();
    __m256d vsum2 = _mm256_setzero_pd();  // Two vector accumulators!

    for (int i = 0; i < n; i += 8) {
        __m256d v1 = _mm256_loadu_pd(a + i);
        __m256d v2 = _mm256_loadu_pd(a + i + 4);
        vsum1 = _mm256_add_pd(vsum1, v1);
        vsum2 = _mm256_add_pd(vsum2, v2);
    }

    __m256d vsum = _mm256_add_pd(vsum1, vsum2);
    // ... horizontal reduction ...
}
```

**8 independent chains** = maximum ILP for FP add!

## Sum Performance Comparison

```bash
$ cd examples && make sum
```

| Version | Time | Speedup |
|---------|------|---------|
| SIMD (1 accumulator) | 42 ms | 3.6x |
| SIMD + 2x unrolled | 25 ms | 6x |

. . .

**Key insight:** SIMD with one accumulator = scalar with 4 accumulators

For maximum performance: **SIMD + unrolling** together!

# Example: Dot Product

## Scalar Dot Product

```cpp
// dot_v1.cpp
double dot_scalar(double* a, double* b, int n) {
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}
```

## SIMD Dot Product with FMA

```cpp
// dot_v2.cpp
double dot_simd(double* a, double* b, int n) {
    __m256d vsum = _mm256_setzero_pd();

    for (int i = 0; i < n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        // FMA: vsum = va * vb + vsum (one instruction!)
        vsum = _mm256_fmadd_pd(va, vb, vsum);
    }

    // Horizontal reduction (same as sum)
    __m128d low = _mm256_castpd256_pd128(vsum);
    __m128d high = _mm256_extractf128_pd(vsum, 1);
    __m128d sum128 = _mm_add_pd(low, high);
    sum128 = _mm_hadd_pd(sum128, sum128);
    return _mm_cvtsd_f64(sum128);
}
```

## Dot Product Performance

```bash
$ cd examples && make dot
```

**4.8x speedup!**

- FMA does multiply+add in one instruction
- 4 parallel multiply-adds per iteration
- Still need unrolling for maximum throughput

# Handling Conditionals

## The Problem with Branches

```cpp
// Scalar version with branch
for (int i = 0; i < n; i++) {
    if (a[i] > 0.5)
        b[i] = a[i] + 1.0;
    else
        b[i] = a[i] - 1.0;
}
```

**SIMD processes 4 elements at once** — what if some need `+1` and others need `-1`?

. . .

**Solution:** Compute **both** paths, use **mask** to select results.

## SIMD Comparison and Masking

```bash
$ cd examples && make cond
```

```cpp
// conditional.cpp
__m256d threshold = _mm256_set1_pd(0.5);
__m256d ones = _mm256_set1_pd(1.0);
__m256d mones = _mm256_set1_pd(-1.0);
for (int i = 0; i < n; i += 4) {
    __m256d v = _mm256_loadu_pd(a + i);

    // Compare: creates mask (all 1s or all 0s per element)
    __m256d mask = _mm256_cmp_pd(v, threshold, _CMP_GT_OQ);

    // Blend: select from ones or mones based on mask
    __m256d delta = _mm256_blendv_pd(mones, ones, mask);

    __m256d result = _mm256_add_pd(v, delta);
    _mm256_storeu_pd(b + i, result);
}
```

## Visualizing Masking

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]

    % Input
    \node at (-1.5, 4) {v:};
    \draw[fill=blue!20] (0, 3.7) rectangle (1.5, 4.3);
    \node at (0.75, 4) {0.3};
    \draw[fill=blue!20] (1.5, 3.7) rectangle (3, 4.3);
    \node at (2.25, 4) {0.8};
    \draw[fill=blue!20] (3, 3.7) rectangle (4.5, 4.3);
    \node at (3.75, 4) {0.2};
    \draw[fill=blue!20] (4.5, 3.7) rectangle (6, 4.3);
    \node at (5.25, 4) {0.9};

    % Comparison
    \node at (-1.5, 3) {mask:};
    \draw[fill=red!30] (0, 2.7) rectangle (1.5, 3.3);
    \node at (0.75, 3) {0x0};
    \draw[fill=green!30] (1.5, 2.7) rectangle (3, 3.3);
    \node at (2.25, 3) {0xff..};
    \draw[fill=red!30] (3, 2.7) rectangle (4.5, 3.3);
    \node at (3.75, 3) {0x0};
    \draw[fill=green!30] (4.5, 2.7) rectangle (6, 3.3);
    \node at (5.25, 3) {0xff..};

    \node[right] at (6.5, 3) {(v > 0.5?)};

    % Delta
    \node at (-1.5, 2) {delta:};
    \draw[fill=red!20] (0, 1.7) rectangle (1.5, 2.3);
    \node at (0.75, 2) {-1};
    \draw[fill=green!20] (1.5, 1.7) rectangle (3, 2.3);
    \node at (2.25, 2) {+1};
    \draw[fill=red!20] (3, 1.7) rectangle (4.5, 2.3);
    \node at (3.75, 2) {-1};
    \draw[fill=green!20] (4.5, 1.7) rectangle (6, 2.3);
    \node at (5.25, 2) {+1};

    \node[right] at (6.5, 2) {(blendv)};

    % Result
    \node at (-1.5, 1) {result:};
    \draw[fill=orange!30] (0, 0.7) rectangle (1.5, 1.3);
    \node at (0.75, 1) {-0.7};
    \draw[fill=orange!30] (1.5, 0.7) rectangle (3, 1.3);
    \node at (2.25, 1) {1.8};
    \draw[fill=orange!30] (3, 0.7) rectangle (4.5, 1.3);
    \node at (3.75, 1) {-0.8};
    \draw[fill=orange!30] (4.5, 0.7) rectangle (6, 1.3);
    \node at (5.25, 1) {1.9};

\end{tikzpicture}
\end{document}
```

# Compiler Vectorization

## Can the Compiler Do This Automatically?

```cpp
void add(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}
```

```bash
$ g++ -O3 -march=native add.cpp -S
$ grep vaddpd add.s
    vaddpd  (%rsi,%rax), %ymm0, %ymm0   # Yes! Vectorized!
```

. . .

**Yes, often!** With `-O3 -march=native`, compilers auto-vectorize simple loops.

## When Auto-Vectorization Fails

**1. Aliasing** — pointers might overlap

```cpp
void add(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];  // What if c overlaps a or b?
}
```

**Fix:** Use `restrict` keyword or `#pragma`

```cpp
void add(double* restrict a, double* restrict b,
         double* restrict c, int n) { ... }
```

## When Auto-Vectorization Fails (cont'd)

**2. Complex control flow**

```cpp
while (a[i] > 0) {  // Not a countable loop!
    ...
}
```

**3. Function calls in loop**

**4. Floating-point associativity** (for reductions)

```cpp
sum += a[i];  // Can't reorder without -ffast-math
```

**Fix:** Use `-ffast-math` or `-fassociative-math`

## Helping the Compiler

```cpp
// Tell compiler there's no aliasing
#pragma GCC ivdep
for (int i = 0; i < n; i++)
    c[i] = a[i] + b[i];

// Force vectorization (GCC)
#pragma GCC optimize("tree-vectorize")

// Check if compiler vectorized
$ g++ -O3 -fopt-info-vec-optimized file.cpp
file.cpp:5: optimized: loop vectorized using 32 byte vectors
```

## When to Use Intrinsics vs Compiler

**Use compiler vectorization when:**

- Simple loops (add, multiply arrays)
- Performance is "good enough"
- Portability matters

**Use intrinsics when:**

- Complex algorithms (reductions, shuffles)
- Need guaranteed vectorization
- Maximum performance required
- Compiler fails to vectorize

# Alignment

## Why Alignment Matters

**32-byte alignment** = address divisible by 32

```cpp
// Unaligned (any address)
double* a = (double*)malloc(n * sizeof(double));

// Aligned to 32 bytes
double* a = (double*)aligned_alloc(32, n * sizeof(double));
```

. . .

**Performance impact:**

- Modern CPUs: unaligned nearly as fast
- But: `_mm256_load_pd` on unaligned = **crash!**
- Aligned loads enable more optimizations

## Allocating Aligned Memory

```cpp
// C11 standard
double* a = aligned_alloc(32, n * sizeof(double));

// POSIX
double* a;
posix_memalign((void**)&a, 32, n * sizeof(double));

// C++ 17
double* a = static_cast<double*>(
    std::aligned_alloc(32, n * sizeof(double)));

// On stack
alignas(32) double a[1024];
```

## Safe Loading Pattern

```cpp
void process(double* a, int n) {
    int i = 0;

    // Handle unaligned prefix
    while (((uintptr_t)(a + i) & 31) != 0 && i < n) {
        // Scalar processing
        process_one(a[i]);
        i++;
    }

    // Main loop: aligned
    for (; i + 4 <= n; i += 4) {
        __m256d v = _mm256_load_pd(a + i);  // Safe!
        // ...
    }

    // Handle remainder
    for (; i < n; i++) {
        process_one(a[i]);
    }
}
```

# Practical Guidelines

## SIMD Best Practices

1. **Use aligned memory** when possible (32-byte for AVX)

2. **Combine SIMD with unrolling** for maximum ILP

3. **Minimize shuffles** — they're expensive

4. **Process data in chunks of vector width** (4 doubles for AVX)

5. **Handle remainders** — what if `n % 4 != 0`?

6. **Benchmark!** — SIMD doesn't always help (memory-bound code)

## When SIMD Helps Most

**Good candidates:**

- Element-wise operations (add, multiply arrays)
- Reductions (sum, max, dot product)
- Stencil computations
- Image processing
- Small matrix operations

**Poor candidates:**

- Already memory-bound code
- Highly irregular access patterns
- Lots of branches/conditionals
- Gather/scatter intensive

## Complete Example: SAXPY

```bash
$ cd examples && make sax
```

```cpp
// saxpy.cpp - y = a*x + y
void saxpy_simd(float* y, float a, float* x, int n) {
    __m256 va = _mm256_set1_ps(a);  // Broadcast a

    int i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 vx = _mm256_loadu_ps(x + i);
        __m256 vy = _mm256_loadu_ps(y + i);
        vy = _mm256_fmadd_ps(va, vx, vy);  // y = a*x + y
        _mm256_storeu_ps(y + i, vy);
    }

    // Handle remainder
    for (; i < n; i++) {
        y[i] = a * x[i] + y[i];
    }
}
```

# Summary

## Key Takeaways

1. **SIMD** = process 4-8 elements with one instruction
   - AVX: 4 doubles or 8 floats per instruction

2. **Intrinsics** give you direct control over SIMD
   - `_mm256_add_pd`, `_mm256_load_pd`, etc.

3. **SIMD + ILP** together for maximum performance
   - Use multiple vector accumulators

4. **Compiler can auto-vectorize** simple cases
   - But intrinsics give you more control

5. **Alignment** matters — use 32-byte aligned memory

## Performance Hierarchy

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]

    \draw[->, thick] (0, 0) -- (10, 0) node[right] {Performance};

    \draw[fill=red!30] (0, 0.5) rectangle (2, 1.5);
    \node at (1, 1) {Scalar};

    \draw[fill=orange!30] (2.5, 0.5) rectangle (5, 1.5);
    \node at (3.75, 1) {Scalar+ILP};

    \draw[fill=yellow!30] (5.5, 0.5) rectangle (7.5, 1.5);
    \node at (6.5, 1) {SIMD};

    \draw[fill=green!30] (8, 0.5) rectangle (10.5, 1.5);
    \node at (9.25, 1) {SIMD+ILP};

    \node[below] at (1, 0.3) {1x};
    \node[below] at (3.75, 0.3) {4x};
    \node[below] at (6.5, 0.3) {4x};
    \node[below] at (9.25, 0.3) {8-16x};

\end{tikzpicture}
\end{document}
```

## Try It Yourself

```bash
# Run all examples
$ cd examples && make run

# Build with -O3 for comparison
$ cd examples && make fast && make run

# Use perf to measure
$ cd examples && make perf
```

**Reference:** [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
