---
title: Lecture 10. The Roofline Model
subtitle: Introductions to Data Systems and Data Design
author: Ce Zhang
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing}
---

# Introduction

## The Fundamental Question

When optimizing code, we need to know:

**Is my code limited by compute or by memory bandwidth?**

. . .

- If **compute bound**: Optimize arithmetic (SIMD, ILP, better algorithms)
- If **memory bound**: Optimize data movement (blocking, prefetching, data layout)

. . .

The **Roofline Model** answers this question visually and quantitatively.

*Reference: Williams, Waterman, and Patterson, "Roofline: An Insightful Visual Performance Model for Multicore Architectures," Communications of the ACM, 2009*

## The Two Ceilings

Every program faces two fundamental limits:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]
    % Compute ceiling
    \draw[fill=red!20, rounded corners] (0, 3) rectangle (5, 4);
    \node at (2.5, 3.5) {\textbf{Compute Ceiling}};
    \node[align=left, font=\small] at (2.5, 2.5) {Peak FLOPS of the processor\\$\pi$ flops/cycle};

    % Memory ceiling
    \draw[fill=blue!20, rounded corners] (7, 3) rectangle (12, 4);
    \node at (9.5, 3.5) {\textbf{Memory Ceiling}};
    \node[align=left, font=\small] at (9.5, 2.5) {Peak memory bandwidth\\$\beta$ bytes/cycle};

    % Program
    \draw[fill=green!20, rounded corners] (3.5, 0) rectangle (8.5, 1);
    \node at (6, 0.5) {\textbf{Your Program}};

    % Arrows
    \draw[->, thick] (6, 1.2) -- (2.5, 2.2);
    \draw[->, thick] (6, 1.2) -- (9.5, 2.2);

    \node[font=\small] at (6, 1.7) {Which ceiling limits you?};
\end{tikzpicture}
\end{document}
```

## Platform Parameters

**Peak Performance $\pi$** [flops/cycle or GFLOPS]:

$$\pi = \text{cores} \times \text{SIMD width} \times \text{FMA units} \times \text{ops/FMA}$$

. . .

**Example:** Intel Core i7 (Skylake), single core:

- 2 FMA units
- 4-way SIMD (AVX, doubles)
- 2 ops per FMA (multiply + add)

$$\pi = 1 \times 4 \times 2 \times 2 = 16 \text{ flops/cycle}$$

. . .

**Peak Bandwidth $\beta$** [bytes/cycle or GB/s]:

- Measured empirically (STREAM benchmark)
- Typically 60-70% of theoretical maximum

## Algorithm Parameters

**Work $W(n)$** [flops]: Total floating-point operations

**Data Movement $Q(n)$** [bytes]: Bytes transferred between cache and memory

. . .

**Operational Intensity:**

$$I(n) = \frac{W(n)}{Q(n)} \quad \text{[flops/byte]}$$

. . .

**Performance:**

$$P(n) = \frac{W(n)}{T(n)} \quad \text{[flops/cycle]}$$

where $T(n)$ is runtime in cycles.

# Deriving the Roofline

## The Two Bounds

**Bound 1: Compute Bound**

$$P \leq \pi$$

Performance cannot exceed peak compute throughput.

. . .

**Bound 2: Memory Bound**

$$\beta \geq \frac{Q}{T} = \frac{W/T}{W/Q} = \frac{P}{I}$$

Rearranging:

$$P \leq \beta \cdot I$$

Performance cannot exceed bandwidth times operational intensity.

## Combining the Bounds

$$P \leq \min(\pi, \beta \cdot I)$$

. . .

Taking logarithms:

$$\log_2(P) \leq \min(\log_2(\pi), \log_2(\beta) + \log_2(I))$$

. . .

This is a **piecewise linear function** in log-log space:

- For low $I$: $P = \beta \cdot I$ (diagonal line, slope 1)
- For high $I$: $P = \pi$ (horizontal line)
- **Ridge point:** $I^* = \pi / \beta$

## The Roofline Plot

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.9, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$ [flops/byte]};
    \draw[->] (0, 0) -- (0, 5) node[above] {$P$ [flops/cycle]};

    % Log scale labels on x-axis
    \foreach \x/\label in {1/1/8, 2.5/1/2, 4/2, 5.5/8, 7/32} {
        \draw (\x, -0.1) -- (\x, 0.1);
        \node[below, font=\small] at (\x, -0.1) {\label};
    }

    % Log scale labels on y-axis
    \foreach \y/\label in {1/1, 2/2, 3/4, 4/8, 4.5/16} {
        \draw (-0.1, \y) -- (0.1, \y);
        \node[left, font=\small] at (-0.1, \y) {\label};
    }

    % Memory bound region (diagonal)
    \draw[very thick, blue] (0.5, 0.5) -- (4, 4);

    % Compute bound region (horizontal)
    \draw[very thick, red] (4, 4) -- (9, 4);

    % Ridge point
    \fill[black] (4, 4) circle (3pt);
    \node[above right] at (4, 4) {Ridge point};

    % Labels
    \node[blue, rotate=45] at (1.8, 1.5) {$P = \beta I$};
    \node[red] at (7, 4.4) {$P = \pi$};

    % Regions
    \node[blue, font=\small] at (1.5, 0.5) {Memory};
    \node[blue, font=\small] at (1.5, 0.1) {bound};
    \node[red, font=\small] at (7, 0.5) {Compute};
    \node[red, font=\small] at (7, 0.1) {bound};

    % Ridge point annotation
    \draw[dashed] (4, 0) -- (4, 4);
    \node[below, font=\small] at (4, -0.3) {$I^* = \pi/\beta$};
\end{tikzpicture}
\end{document}
```

## Reading the Roofline

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5) node[above] {$P$};

    % Roofline
    \draw[very thick, black] (0.5, 0.5) -- (4, 4) -- (9, 4);

    % Example points
    \fill[red] (2, 1.5) circle (4pt);
    \node[right, red, font=\small] at (2.2, 1.5) {A: memory bound};
    \node[right, red, font=\small] at (2.2, 1.1) {(far from roof)};

    \fill[blue] (2, 2.3) circle (4pt);
    \node[left, blue, font=\small] at (1.8, 2.5) {B: memory bound};
    \node[left, blue, font=\small] at (1.8, 2.1) {(close to roof)};

    \fill[green!60!black] (6, 2.5) circle (4pt);
    \node[right, green!60!black, font=\small] at (6.2, 2.5) {C: compute bound};
    \node[right, green!60!black, font=\small] at (6.2, 2.1) {(far from roof)};

    \fill[orange] (6, 3.8) circle (4pt);
    \node[left, orange, font=\small] at (5.8, 3.8) {D: compute bound};
    \node[left, orange, font=\small] at (5.8, 3.4) {(close to roof)};

    % Vertical arrows showing gap
    \draw[->, dashed, red] (2, 1.5) -- (2, 2.25);
    \draw[->, dashed, green!60!black] (6, 2.5) -- (6, 3.95);
\end{tikzpicture}
\end{document}
```

**Goal:** Move points up toward the roof, or right to increase $I$.

# Computing Operational Intensity

## Example 1: Vector Addition (DAXPY)

$$y = \alpha x + y$$

```cpp
// daxpy.cpp
for (int i = 0; i < n; i++)
    y[i] = alpha * x[i] + y[i];
```

```bash
$ cd examples && make run_daxpy
```

. . .

**Work:** $W = 2n$ flops (1 multiply + 1 add per element)

**Data movement (cold cache):**

- Read $x$: $8n$ bytes
- Read $y$: $8n$ bytes
- Write $y$: $8n$ bytes

$Q = 24n$ bytes

. . .

**Operational Intensity:**

$$I = \frac{2n}{24n} = \frac{1}{12} \approx 0.083 \text{ flops/byte}$$

## Example 2: Dot Product

$$s = x^T y = \sum_{i=0}^{n-1} x_i \cdot y_i$$

```cpp
// dot.cpp
double s = 0;
for (int i = 0; i < n; i++)
    s += x[i] * y[i];
```

```bash
$ cd examples && make run_dot
```

. . .

**Work:** $W = 2n$ flops

**Data movement:**

- Read $x$: $8n$ bytes
- Read $y$: $8n$ bytes

$Q = 16n$ bytes

. . .

**Operational Intensity:**

$$I = \frac{2n}{16n} = \frac{1}{8} = 0.125 \text{ flops/byte}$$

## Example 3: Matrix-Vector Multiplication

$$y = Ax$$

```cpp
// gemv.cpp
for (int i = 0; i < n; i++) {
    y[i] = 0;
    for (int j = 0; j < n; j++)
        y[i] += A[i][j] * x[j];
}
```

```bash
$ cd examples && make run_gemv
```

. . .

**Work:** $W = 2n^2$ flops

**Data movement:**

- Read $A$: $8n^2$ bytes
- Read $x$: $8n$ bytes (reused $n$ times if fits in cache)
- Write $y$: $8n$ bytes

$Q \approx 8n^2$ bytes (dominated by $A$)

. . .

**Operational Intensity:**

$$I = \frac{2n^2}{8n^2} = \frac{1}{4} = 0.25 \text{ flops/byte}$$

## Example 4: Matrix-Matrix Multiplication (Naive)

$$C = AB$$

```cpp
// gemm.cpp - compares naive vs blocked
for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
            C[i][j] += A[i][k] * B[k][j];
```

```bash
$ cd examples && make run_gemm
```

. . .

**Work:** $W = 2n^3$ flops

**Data movement (naive, cache << matrices):**

- Each $C_{ij}$ requires reading row of $A$ and column of $B$
- Total: $Q \approx 8n^3$ bytes (terrible!)

. . .

**Operational Intensity (naive):**

$$I = \frac{2n^3}{8n^3} = \frac{1}{4} \text{ flops/byte}$$

Same as matrix-vector!

## Example 4b: Matrix-Matrix Multiplication (Blocked)

With blocking (block size $b$, $3b^2 \leq$ cache):

**Data movement:**

$$Q = \frac{n^3}{4b} \times 8 = \frac{2n^3}{b} \text{ bytes}$$

. . .

**Operational Intensity (blocked):**

$$I = \frac{2n^3}{2n^3/b} = b \text{ flops/byte}$$

. . .

**With optimal blocking:** $b = O(\sqrt{\gamma})$ where $\gamma$ is cache size

$$I = O(\sqrt{\gamma})$$

**Example:** L1 cache = 32KB, $b \approx 32$, so $I \approx 32$ flops/byte!

## Example 5: FFT

**Discrete Fourier Transform:** $y = F_n x$

**Work:** $W = 5n \log_2 n$ flops (using FFT algorithm)

. . .

**Data movement:**

- Naive: $Q = O(n \log n)$ bytes
- Cache-optimal: $Q = O(n)$ bytes

. . .

**Operational Intensity:**

$$I = O(\log n) \text{ to } O(\log \gamma)$$

where $\gamma$ is cache size.

. . .

FFT has **intermediate** operational intensity -- neither fully memory bound nor compute bound.

## Summary: Operational Intensity

| Operation | $W$ | $Q$ | $I$ |
|-----------|-----|-----|-----|
| DAXPY: $y = \alpha x + y$ | $2n$ | $24n$ | $1/12$ |
| Dot product: $x^T y$ | $2n$ | $16n$ | $1/8$ |
| GEMV: $y = Ax$ | $2n^2$ | $8n^2$ | $1/4$ |
| GEMM naive: $C = AB$ | $2n^3$ | $8n^3$ | $1/4$ |
| GEMM blocked: $C = AB$ | $2n^3$ | $2n^3/b$ | $b \sim O(\sqrt{\gamma})$ |
| FFT | $5n\log n$ | $8n$ | $O(\log n)$ |

. . .

**Key insight:** Only GEMM (with blocking) can achieve high operational intensity!

# Roofline Examples

## Example Platform: Intel Skylake

**Single core specifications:**

- Peak: $\pi = 16$ flops/cycle (2 FMA units $\times$ 4-wide AVX $\times$ 2)
- At 3 GHz: $\pi = 48$ GFLOPS

. . .

**Memory bandwidth:**

- Theoretical: ~20 GB/s per channel
- Measured (STREAM): $\beta \approx 15$ GB/s $= 5$ bytes/cycle at 3 GHz

. . .

**Ridge point:**

$$I^* = \frac{\pi}{\beta} = \frac{16}{5} = 3.2 \text{ flops/byte}$$

## Skylake Roofline

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes (log scale conceptually)
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$ [flops/byte]};
    \draw[->] (0, 0) -- (0, 5.5) node[above] {$P$ [flops/cycle]};

    % X-axis labels
    \foreach \x/\label in {0.5/0.1, 2/0.5, 3.2/1, 4.5/2, 5.7/4, 7/8, 8.2/16} {
        \draw (\x, -0.1) -- (\x, 0.1);
        \node[below, font=\tiny] at (\x, -0.1) {\label};
    }

    % Y-axis labels
    \foreach \y/\label in {0.5/0.25, 1.5/1, 2.5/2, 3.5/4, 4.5/8, 5/16} {
        \draw (-0.1, \y) -- (0.1, \y);
        \node[left, font=\tiny] at (-0.1, \y) {\label};
    }

    % Roofline (beta = 5, pi = 16)
    % Memory bound: P = 5*I, so log2(P) = log2(5) + log2(I)
    \draw[very thick, blue] (0.5, 0.8) -- (5.7, 5);
    \draw[very thick, red] (5.7, 5) -- (9.5, 5);

    % Ridge point
    \fill[black] (5.7, 5) circle (3pt);
    \node[above, font=\small] at (5.7, 5.1) {$I^* = 3.2$};

    % Plot operations
    \fill[orange] (1.5, 1.3) circle (4pt);
    \node[right, orange, font=\small] at (1.7, 1.3) {DAXPY};

    \fill[purple] (2.2, 1.8) circle (4pt);
    \node[right, purple, font=\small] at (2.4, 1.8) {GEMV};

    \fill[green!60!black] (3.2, 2.5) circle (4pt);
    \node[above, green!60!black, font=\small] at (3.2, 2.6) {FFT};

    \fill[cyan] (7.5, 4.8) circle (4pt);
    \node[right, cyan, font=\small] at (7.7, 4.8) {GEMM};

    % Label
    \node[red, font=\small] at (8, 5.4) {$\pi = 16$};
    \node[blue, font=\small, rotate=42] at (2.5, 2.8) {$\beta = 5$};
\end{tikzpicture}
\end{document}
```

## Where Do Common Operations Fall?

| **Memory Bound** | **Compute Bound** |
|------------------|-------------------|
| BLAS Level 1: DAXPY, Dot product, Norms | BLAS Level 3: GEMM, Matrix factorizations |
| BLAS Level 2: GEMV, Matrix-vector solve | Dense linear algebra |
| Stencils, SpMV, Graph algorithms | Convolutions (tiled), Deep learning |

## Numerical Example: DAXPY

**Problem:** $y = 2x + y$, vectors of length $n = 10^8$

**Platform:** 3 GHz Skylake, $\pi = 48$ GFLOPS, $\beta = 15$ GB/s

. . .

**Analysis:**

- $W = 2 \times 10^8$ flops
- $Q = 24 \times 10^8$ bytes = 2.4 GB
- $I = 1/12 \approx 0.083$ flops/byte

. . .

**Maximum achievable performance:**

$$P = \min(\pi, \beta \cdot I) = \min(48, 15 \times 0.083) = \min(48, 1.25) = 1.25 \text{ GFLOPS}$$

. . .

**Minimum runtime:**

$$T = \frac{W}{P} = \frac{2 \times 10^8}{1.25 \times 10^9} = 0.16 \text{ seconds}$$

## Numerical Example: GEMM

**Problem:** $C = AB$, matrices of size $n = 1000$

**Platform:** Same as before

. . .

**Analysis:**

- $W = 2 \times 10^9$ flops
- With blocking ($b = 32$): $I \approx 32$ flops/byte

. . .

**Maximum achievable performance:**

$$P = \min(\pi, \beta \cdot I) = \min(48, 15 \times 32) = \min(48, 480) = 48 \text{ GFLOPS}$$

. . .

**GEMM is compute bound!** We can achieve peak performance.

**Minimum runtime:**

$$T = \frac{W}{P} = \frac{2 \times 10^9}{48 \times 10^9} = 0.042 \text{ seconds}$$

## Numerical Example: Effect of Problem Size on GEMM

| Matrix size $n$ | Block fits in L1? | Effective $I$ | Bound |
|-----------------|-------------------|---------------|-------|
| 32 | Yes | $\sim 32$ | Compute |
| 100 | Borderline | $\sim 10$ | Compute |
| 1000 | No (need L3) | $\sim 5$ | Borderline |
| 10000 | No (need RAM) | $\sim 1$ | Memory |

. . .

**Key insight:** Same algorithm can be memory bound or compute bound depending on problem size and cache behavior!

# Adding More Roofs

## Multiple Compute Ceilings

Not all code can achieve peak $\pi$:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5.5) node[above] {$P$};

    % Memory bound
    \draw[very thick, blue] (0.5, 0.5) -- (5.5, 5);

    % Multiple compute ceilings
    \draw[very thick, red] (5.5, 5) -- (9.5, 5);
    \node[right, red, font=\small] at (9.5, 5) {Peak (FMA+SIMD)};

    \draw[thick, red!70] (4.5, 4) -- (9.5, 4);
    \node[right, red!70, font=\small] at (9.5, 4) {No FMA};

    \draw[thick, red!50] (3.5, 3) -- (9.5, 3);
    \node[right, red!50, font=\small] at (9.5, 3) {Scalar only};

    \draw[thick, red!30] (2.5, 2) -- (9.5, 2);
    \node[right, red!30, font=\small] at (9.5, 2) {Add-only};

    % Example point
    \fill[green!60!black] (6, 2.8) circle (4pt);
    \draw[->, thick, green!60!black] (6, 2.8) -- (6, 3.8);
    \node[left, green!60!black, font=\small] at (5.8, 3.3) {Vectorize!};
\end{tikzpicture}
\end{document}
```

## Example: Instruction Mix Ceilings

**Skylake single core:**

| Configuration | Peak [flops/cycle] |
|---------------|-------------------|
| Scalar, add only | 1 |
| Scalar, mul only | 1 |
| Scalar, FMA | 2 |
| AVX (4-wide), add only | 4 |
| AVX, mul only | 4 |
| AVX, FMA | 16 |

. . .

**Your code's ceiling depends on:**

- Whether it vectorizes (SIMD)
- Whether compiler generates FMA
- Mix of adds vs multiplies

## Multiple Memory Ceilings

Different memory levels have different bandwidths:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5.5) node[above] {$P$};

    % Compute ceiling
    \draw[very thick, red] (6, 5) -- (9.5, 5);

    % L1 bandwidth (highest)
    \draw[very thick, blue] (0.5, 2) -- (6, 5);
    \node[blue, font=\small, rotate=25] at (2, 2.7) {L1: 96 B/cycle};

    % L2 bandwidth
    \draw[thick, blue!70] (0.5, 1.3) -- (7, 5);
    \node[blue!70, font=\small, rotate=32] at (1.5, 1.5) {L2: 32 B/cycle};

    % L3 bandwidth
    \draw[thick, blue!50] (0.5, 0.8) -- (8, 5);
    \node[blue!50, font=\small, rotate=33] at (1, 0.7) {L3: 16 B/cycle};

    % DRAM bandwidth
    \draw[thick, blue!30] (0.5, 0.4) -- (9, 5);
    \node[blue!30, font=\small, rotate=28] at (2.5, 0.5) {DRAM: 5 B/cycle};
\end{tikzpicture}
\end{document}
```

## Which Bandwidth Applies?

**Rule:** Use the bandwidth of the memory level where your data resides.

. . .

| Working set size | Memory level | Typical $\beta$ |
|------------------|--------------|-----------------|
| < 32 KB | L1 | 96 B/cycle |
| < 256 KB | L2 | 32 B/cycle |
| < 8 MB | L3 | 16 B/cycle |
| > 8 MB | DRAM | 5 B/cycle |

. . .

**Implication:** Same code, different problem sizes = different rooflines!

## Effect of SIMD on Roofline

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5.5) node[above] {$P$};

    % Original roofline (scalar)
    \draw[thick, gray, dashed] (0.5, 0.5) -- (3, 3) -- (9.5, 3);
    \node[gray, font=\small] at (8, 3.3) {Scalar: $\pi = 2$};

    % New roofline (SIMD 4x)
    \draw[very thick, blue] (0.5, 0.5) -- (5, 5) -- (9.5, 5);
    \node[blue, font=\small] at (8, 5.3) {AVX: $\pi = 8$};

    % Ridge points
    \draw[dashed, gray] (3, 0) -- (3, 3);
    \node[below, gray, font=\tiny] at (3, 0) {$I^* = 2$};

    \draw[dashed, blue] (5, 0) -- (5, 5);
    \node[below, blue, font=\tiny] at (5, 0) {$I^* = 8$};

    % Example point
    \fill[red] (4, 3.5) circle (4pt);
    \node[right, red, font=\small] at (4.2, 3.5) {Was compute bound};
    \node[right, red, font=\small] at (4.2, 3.1) {Now memory bound!};
\end{tikzpicture}
\end{document}
```

**SIMD increases $\pi$, shifting the ridge point right!**

# Practical Examples

## Example: Sum Reduction

```cpp
double sum(double* a, int n) {
    double s = 0;
    for (int i = 0; i < n; i++)
        s += a[i];
    return s;
}
```

. . .

**Work:** $W = n$ flops (additions only)

**Data:** $Q = 8n$ bytes

**Operational intensity:** $I = n / 8n = 1/8 = 0.125$ flops/byte

. . .

**On Skylake ($\beta = 5$ B/cycle):**

$$P_{\max} = \beta \cdot I = 5 \times 0.125 = 0.625 \text{ flops/cycle}$$

**Heavily memory bound!** Even perfect code is limited to 0.625 flops/cycle.

## Example: Polynomial Evaluation

Evaluate $p(x) = a_0 + a_1 x + a_2 x^2 + \cdots + a_{n-1} x^{n-1}$

Using Horner's method:

```cpp
double horner(double* a, int n, double x) {
    double result = a[n-1];
    for (int i = n-2; i >= 0; i--)
        result = result * x + a[i];  // FMA!
    return result;
}
```

. . .

**Work:** $W = 2n - 1 \approx 2n$ flops

**Data:** $Q = 8n$ bytes (read coefficients)

**Operational intensity:** $I = 2n / 8n = 1/4$ flops/byte

. . .

Still memory bound, but better than sum!

## Example: Stencil Computation

2D 5-point stencil (heat equation):

```cpp
// stencil.cpp
for (int i = 1; i < n-1; i++)
    for (int j = 1; j < n-1; j++)
        B[i][j] = 0.25 * (A[i-1][j] + A[i+1][j] +
                         A[i][j-1] + A[i][j+1]);
```

```bash
$ cd examples && make run_stencil
```

. . .

**Work:** $W = 4n^2$ flops (3 adds + 1 mul per point)

**Data (naive):** $Q \approx 8n^2 + 8n^2 = 16n^2$ bytes

**Operational intensity:** $I = 4n^2 / 16n^2 = 1/4$ flops/byte

. . .

**With temporal blocking:** Can increase to $I = O(B)$ where $B$ is block size.

## Example: Sparse Matrix-Vector (SpMV)

$y = Ax$ where $A$ is sparse with $nnz$ nonzeros

```cpp
// CSR format
for (int i = 0; i < n; i++) {
    double sum = 0;
    for (int j = row_ptr[i]; j < row_ptr[i+1]; j++)
        sum += val[j] * x[col_idx[j]];
    y[i] = sum;
}
```

. . .

**Work:** $W = 2 \times nnz$ flops

**Data:**

- Values: $8 \times nnz$ bytes
- Column indices: $4 \times nnz$ bytes
- $x$ vector: irregular access (hard to reuse)

$Q \approx 12 \times nnz + 8n$ bytes

. . .

**Operational intensity:** $I \approx 2/12 = 1/6$ flops/byte

**SpMV is almost always memory bound!**

## Example: Deep Learning - Batch Size Effect

Forward pass of fully-connected layer: $Y = XW$ where $X$ is batch $\times$ input, $W$ is input $\times$ output

. . .

**Work:** $W = 2 \times \text{batch} \times \text{input} \times \text{output}$

**Data:**

- $W$ matrix: $8 \times \text{input} \times \text{output}$ bytes
- $X$ matrix: $8 \times \text{batch} \times \text{input}$ bytes
- $Y$ matrix: $8 \times \text{batch} \times \text{output}$ bytes

. . .

**Operational intensity:**

$$I \approx \frac{2 \times \text{batch} \times \text{in} \times \text{out}}{8(\text{in} \times \text{out} + \text{batch} \times \text{in} + \text{batch} \times \text{out})}$$

. . .

**For large batch:** $I \rightarrow \frac{\text{batch}}{4}$ (weights reused across batch)

**For batch = 1:** $I \approx 1/4$ (memory bound!)

## Batch Size and Roofline

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5) node[above] {$P$};

    % Roofline
    \draw[very thick, black] (0.5, 0.5) -- (5, 5) -- (9.5, 5);

    % Different batch sizes
    \fill[red] (1.5, 1.3) circle (4pt);
    \node[above, red, font=\small] at (1.5, 1.4) {batch=1};

    \fill[orange] (2.5, 2.3) circle (4pt);
    \node[above, orange, font=\small] at (2.5, 2.4) {batch=4};

    \fill[yellow!80!black] (3.5, 3.3) circle (4pt);
    \node[above, yellow!80!black, font=\small] at (3.5, 3.4) {batch=16};

    \fill[green!60!black] (5, 4.5) circle (4pt);
    \node[above, green!60!black, font=\small] at (5, 4.6) {batch=64};

    \fill[blue] (7, 4.8) circle (4pt);
    \node[above, blue, font=\small] at (7, 4.9) {batch=256};

    % Arrow
    \draw[->, thick, dashed] (1.5, 0.8) -- (7, 0.8);
    \node[below, font=\small] at (4.25, 0.6) {Increasing batch size};
\end{tikzpicture}
\end{document}
```

**This is why LLM inference (batch=1) is memory bound!**

# Using the Roofline for Optimization

## Optimization Strategy

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]
    \node[draw, rounded corners, fill=blue!20] (start) at (0, 0) {Measure P and I};

    \node[draw, diamond, aspect=2, fill=yellow!20] (check) at (0, -2) {Close to roof?};

    \node[draw, rounded corners, fill=green!20] (done) at (-4, -4) {Done!};

    \node[draw, diamond, aspect=2, fill=yellow!20] (bound) at (4, -4) {Which bound?};

    \node[draw, rounded corners, fill=red!20] (mem) at (1, -6.5) {Improve locality};

    \node[draw, rounded corners, fill=orange!20] (comp) at (7, -6.5) {SIMD/ILP};

    \draw[->] (start) -- (check);
    \draw[->] (check) -- node[left] {Yes} (done);
    \draw[->] (check) -- node[right] {No} (bound);
    \draw[->] (bound) -- node[left] {Memory} (mem);
    \draw[->] (bound) -- node[right] {Compute} (comp);
\end{tikzpicture}
\end{document}
```

## If Memory Bound...

**Goal:** Increase operational intensity $I$ or get closer to bandwidth roof.

. . .

**Techniques:**

1. **Blocking/Tiling:** Reuse data in cache
2. **Loop fusion:** Combine loops to reuse loaded data
3. **Data layout:** Improve spatial locality
4. **Prefetching:** Hide memory latency
5. **Compression:** Reduce data volume

. . .

**Example:** Naive MMM ($I = 1/4$) $\rightarrow$ Blocked MMM ($I = 32$)

## If Compute Bound...

**Goal:** Increase performance $P$ toward compute roof.

. . .

**Techniques:**

1. **Vectorization (SIMD):** Process multiple elements per instruction
2. **ILP:** Multiple accumulators, loop unrolling
3. **FMA:** Fused multiply-add instructions
4. **Algorithm improvement:** Reduce total work
5. **Parallelization:** Use multiple cores

. . .

**Example:** Scalar sum $\rightarrow$ SIMD sum with 4 accumulators

## Measuring Operational Intensity

**Option 1: Calculate from algorithm**

- Count flops and bytes analytically
- Assumes perfect cache behavior

. . .

**Option 2: Use performance counters**

```bash
# Measure actual cache misses
perf stat -e fp_arith_inst_retired.scalar_double,\
fp_arith_inst_retired.256b_packed_double,\
LLC-load-misses,LLC-store-misses ./program
```

. . .

**Option 3: Use profiling tools**

- Intel Advisor (roofline analysis built-in)
- NVIDIA Nsight (for GPUs)
- likwid-perfctr

## Example: Diagnosing Poor Performance

**Scenario:** Your GEMM achieves 5 GFLOPS on a 48 GFLOPS machine.

. . .

**Step 1:** Calculate theoretical $I$

With proper blocking: $I \approx 32$ flops/byte

. . .

**Step 2:** Check which roof applies

$P_{\max} = \min(48, 5 \times 32) = \min(48, 160) = 48$ GFLOPS

Should be compute bound!

. . .

**Step 3:** Diagnose compute bottleneck

- Check SIMD: Is code vectorized? (expect 4x)
- Check ILP: Multiple accumulators? (expect 2x from FMA latency hiding)
- Check FMA: Are multiply-adds fused? (expect 2x)

# Roofline Variations

## Cold Cache vs Warm Cache

**Cold cache:** Data starts in main memory

- Use DRAM bandwidth
- $Q$ = all data accessed

. . .

**Warm cache:** Data pre-loaded in cache

- Use cache bandwidth
- $Q$ = only capacity misses

. . .

**Which to use?**

- Cold cache: One-time computations, streaming
- Warm cache: Repeated computations, iterative algorithms

## Roofline for Different Data Types

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 5.5) node[above] {$P$};

    % FP64 roofline
    \draw[thick, blue] (0.5, 0.5) -- (4, 4) -- (9.5, 4);
    \node[blue, font=\small] at (8, 4.3) {FP64};

    % FP32 roofline (2x compute, same BW means 2x I for same bytes)
    \draw[thick, red] (0.5, 0.5) -- (5, 5) -- (9.5, 5);
    \node[red, font=\small] at (8, 5.3) {FP32 (2x peak)};

    % FP16/BF16 (4x compute)
    \draw[thick, green!60!black, dashed] (0.5, 0.5) -- (5.5, 5.5);
    \draw[thick, green!60!black, dashed] (5.5, 5.5) -- (9.5, 5.5);
    \node[green!60!black, font=\small] at (8, 5.8) {FP16 (4x peak)};

    \node[font=\small, align=left] at (2, 5) {Lower precision:\\More compute,\\same bandwidth};
\end{tikzpicture}
\end{document}
```

## GPU Roofline

GPUs have different characteristics:

| Parameter  | CPU (Skylake) | GPU (A100)    |
| ---------- | ------------- | ------------- |
| Peak FP64  | 48 GFLOPS     | 9,700 GFLOPS  |
| Peak FP32  | 96 GFLOPS     | 19,500 GFLOPS |
| Bandwidth  | 50 GB/s       | 2,000 GB/s    |
| Ridge FP32 | 2 flops/byte  | 10 flops/byte |

. . .

**GPU has higher ridge point** -- more operations become memory bound!

**GPU roofline also considers:**

- HBM vs L2 vs shared memory bandwidth
- Tensor core peak vs CUDA core peak

## Hierarchical Roofline

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]
    % Axes
    \draw[->] (0, 0) -- (10, 0) node[right] {$I$};
    \draw[->] (0, 0) -- (0, 6) node[above] {$P$};

    % Compute ceiling
    \draw[very thick, red] (6, 5.5) -- (9.5, 5.5);

    % Multiple bandwidth ceilings
    \draw[thick, blue] (0.3, 1) -- (6, 5.5);
    \draw[thick, blue!70] (0.3, 0.6) -- (7, 5.5);
    \draw[thick, blue!50] (0.3, 0.3) -- (8, 5.5);

    % Labels
    \node[blue, font=\tiny, rotate=40] at (1.5, 1.8) {L1: 96 B/c};
    \node[blue!70, font=\tiny, rotate=45] at (1.2, 1) {L2: 32 B/c};
    \node[blue!50, font=\tiny, rotate=48] at (0.9, 0.3) {DRAM: 5 B/c};

    % Example trajectory
    \fill[green!60!black] (2, 1.5) circle (3pt);
    \fill[green!60!black] (3.5, 3) circle (3pt);
    \fill[green!60!black] (5, 4.5) circle (3pt);
    \draw[->, thick, green!60!black, dashed] (2, 1.5) -- (3.5, 3) -- (5, 4.5);

    \node[green!60!black, font=\small] at (5.5, 3) {Improving};
    \node[green!60!black, font=\small] at (5.5, 2.6) {cache reuse};
\end{tikzpicture}
\end{document}
```

# Summary

## The Roofline Model

$$P \leq \min(\pi, \beta \cdot I)$$

. . .

**Two bounds:**

1. **Compute bound:** $P \leq \pi$ (limited by FLOPS)
2. **Memory bound:** $P \leq \beta I$ (limited by bandwidth)

. . .

**Ridge point:** $I^* = \pi / \beta$ separates the two regions

. . .

**Key insight:** Operational intensity $I = W/Q$ determines which bound applies!

## Key Takeaways

1. **Calculate $I$ before optimizing** -- know your bottleneck!

2. **Memory bound** ($I < I^*$): Improve locality, increase reuse

3. **Compute bound** ($I > I^*$): SIMD, ILP, better algorithms

4. **Multiple roofs** exist for different:
   - Instruction mixes (FMA vs non-FMA)
   - Cache levels (L1 vs L2 vs DRAM)
   - Data types (FP64 vs FP32 vs FP16)

5. **Measure** with perf counters to validate analysis

## Try It Yourself

```bash
# Run all examples and see operational intensities
$ cd examples && make run

# Compare intensities side by side
$ cd examples && make intensity

# Use perf to measure actual cache behavior
$ cd examples && make perf
```

## Operational Intensity Cheat Sheet

| Low $I$ (memory bound) | High $I$ (compute bound) |
|------------------------|--------------------------|
| BLAS 1, 2 | BLAS 3 |
| SpMV | Dense LA |
| Stencils (naive) | Stencils (blocked) |
| Graph algorithms | Convolutions (tiled) |
| LLM inference (batch=1) | LLM training (large batch) |
| Streaming | Iterative |

. . .

**The roofline model is your first tool for performance analysis!**
