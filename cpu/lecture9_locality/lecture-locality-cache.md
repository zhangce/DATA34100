---
title: Lecture 9. Memory Hierarchy, Locality, and Caches
subtitle: Introductions to Data Systems and Data Design
author: Ce Zhang
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing}
---

# The Memory Wall

## The Processor-Memory Bottleneck

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.9, transform shape]
    % CPU
    \draw[fill=blue!20, rounded corners] (0, 0) rectangle (2, 1.5);
    \node at (1, 1.1) {\textbf{CPU}};
    \node at (1, 0.5) {\small Registers};

    % Arrow
    \draw[->, very thick] (2.2, 0.75) -- (5.8, 0.75);
    \node[above] at (4, 0.85) {\small Bus bandwidth};

    % Memory
    \draw[fill=green!20, rounded corners] (6, 0) rectangle (9, 1.5);
    \node at (7.5, 0.75) {\textbf{Main Memory}};

    % Performance numbers
    \node[align=left, font=\small] at (1, -1) {Peak: 192 B/cycle\\(with AVX)};
    \node[align=left, font=\small] at (7.5, -1) {Bandwidth:\\16 B/cycle};

    % Gap indicator
    \draw[<->, red, very thick] (2, -2) -- (6, -2);
    \node[below, red] at (4, -2) {\textbf{12x gap!}};
\end{tikzpicture}
\end{document}
```

. . .

**Problem:** CPU can consume data 12x faster than memory can deliver it.

**Solution:** Memory hierarchy with caches.

## Typical Memory Hierarchy

| Level | Name | Size | Latency (cycles) | Bandwidth |
|-------|------|------|-----------------|-----------|
| L0 | Registers | ~1 KB | 0 | - |
| L1 | L1 Cache | 32 KB | 4 | 12 doubles/cycle |
| L2 | L2 Cache | 256 KB | 12 | 8 doubles/cycle |
| L3 | L3 Cache | 8 MB | 42 | 4 doubles/cycle |
| - | Main Memory | 64 GB | ~200 | 2 doubles/cycle |
| - | SSD | 1 TB | ~10,000 | - |

. . .

**Key insight:** Smaller = Faster = More expensive per byte

Each level caches data from the level below.

# Locality

## Why Caches Work: Locality

**Locality:** Programs tend to use data and instructions with addresses near or equal to those they have used recently.

. . .

**Temporal locality:**

- Recently referenced items are likely to be referenced again
- Example: Loop counter variable `i`

. . .

**Spatial locality:**

- Items with nearby addresses tend to be referenced close together in time
- Example: Iterating through an array sequentially

## Locality Example

```cpp
double sum = 0;
for (int i = 0; i < n; i++)
    sum += a[i];
return sum;
```

. . .

**Data locality:**

- `sum`: High temporal locality (accessed every iteration)
- `a[i]`: High spatial locality (consecutive access)

**Instruction locality:**

- Temporal: Loop body executed repeatedly
- Spatial: Instructions stored contiguously

## Row-Major vs Column-Major Access

```cpp
// Row-major traversal (good locality)
for (int i = 0; i < M; i++)
    for (int j = 0; j < N; j++)
        sum += a[i][j];

// Column-major traversal (poor locality)
for (int j = 0; j < N; j++)
    for (int i = 0; i < M; i++)
        sum += a[i][j];
```

. . .

In C/C++, 2D arrays are stored in **row-major order**:

`a[0][0], a[0][1], ..., a[0][N-1], a[1][0], a[1][1], ...`

Row-major traversal has stride 1; column-major has stride N.

## Measuring the Impact

```bash
$ cd examples && make locality
```

```cpp
// locality_test.cpp
#include <iostream>
#include <chrono>
using namespace std;

const int M = 4096, N = 4096;
double a[M][N];

double sum_rows() {
    double sum = 0;
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            sum += a[i][j];
    return sum;
}

double sum_cols() {
    double sum = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < M; i++)
            sum += a[i][j];
    return sum;
}
```

## Run It Yourself

```cpp
int main() {
    // Initialize
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = 1.0;

    auto t1 = chrono::high_resolution_clock::now();
    volatile double r1 = sum_rows();
    auto t2 = chrono::high_resolution_clock::now();
    volatile double r2 = sum_cols();
    auto t3 = chrono::high_resolution_clock::now();

    cout << "Row-major: "
         << chrono::duration<double,milli>(t2-t1).count() << " ms\n";
    cout << "Col-major: "
         << chrono::duration<double,milli>(t3-t2).count() << " ms\n";
    return 0;
}
```

# Cache Mechanics

## What is a Cache?

**Cache:** Small, fast memory that stores copies of frequently accessed data.

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    % CPU
    \draw[fill=blue!20, rounded corners] (0, 0) rectangle (1.5, 1);
    \node at (0.75, 0.5) {\textbf{CPU}};

    % Cache
    \draw[fill=yellow!30, rounded corners] (2.5, 0) rectangle (4.5, 1);
    \node at (3.5, 0.5) {\textbf{Cache}};

    % Memory
    \draw[fill=green!20, rounded corners] (5.5, 0) rectangle (8, 1);
    \node at (6.75, 0.5) {\textbf{Memory}};

    % Arrows
    \draw[<->, thick] (1.5, 0.5) -- (2.5, 0.5);
    \draw[<->, thick] (4.5, 0.5) -- (5.5, 0.5);

    % Labels
    \node[above, font=\small] at (2, 0.5) {fast};
    \node[above, font=\small] at (5, 0.5) {slow};
\end{tikzpicture}
\end{document}
```

. . .

- **Temporal locality:** Keep recently accessed data in cache
- **Spatial locality:** Transfer data in **blocks** (64 bytes = 8 doubles)

## Cache Hit and Miss

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.75, transform shape]
    % Hit scenario
    \node at (-1.5, 2) {\textbf{Cache Hit:}};

    \draw[fill=blue!20] (0, 1.5) rectangle (1, 2.5);
    \node at (0.5, 2) {CPU};

    \draw[->, thick] (1.2, 2) -- (2.3, 2);
    \node[above, font=\tiny] at (1.75, 2) {request};

    \draw[fill=yellow!30] (2.5, 1.5) rectangle (4, 2.5);
    \node at (3.25, 2) {Cache};
    \draw[fill=green!40] (2.7, 1.7) rectangle (3.1, 2.3);

    \draw[->, thick, green!60!black] (2.3, 1.8) -- (1.2, 1.8);
    \node[below, font=\tiny] at (1.75, 1.8) {data};

    % Miss scenario
    \node at (-1.5, 0) {\textbf{Cache Miss:}};

    \draw[fill=blue!20] (0, -0.5) rectangle (1, 0.5);
    \node at (0.5, 0) {CPU};

    \draw[->, thick] (1.2, 0) -- (2.3, 0);

    \draw[fill=yellow!30] (2.5, -0.5) rectangle (4, 0.5);
    \node at (3.25, 0) {Cache};

    \draw[->, thick, red] (4.2, 0) -- (5.3, 0);
    \node[above, font=\tiny, red] at (4.75, 0) {fetch};

    \draw[fill=green!20] (5.5, -0.5) rectangle (7, 0.5);
    \node at (6.25, 0) {Memory};

    \draw[->, thick] (5.3, -0.2) -- (4.2, -0.2);
    \draw[->, thick] (2.3, -0.2) -- (1.2, -0.2);
\end{tikzpicture}
\end{document}
```

- **Hit:** Data found in cache (fast, ~4 cycles for L1)
- **Miss:** Data not in cache, must fetch from memory (~200 cycles)

## Cache Block and Spatial Locality

When a cache miss occurs, an entire **block** is loaded:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]
    % Memory array
    \node at (-1, 0) {Memory:};
    \foreach \i in {0,...,15} {
        \draw[fill=blue!10] (\i*0.6, -0.3) rectangle (\i*0.6+0.5, 0.3);
        \node[font=\tiny] at (\i*0.6+0.25, 0) {\i};
    }

    % Highlight block
    \draw[very thick, red] (2.4, -0.35) rectangle (5.3, 0.35);
    \node[above, red, font=\small] at (3.85, 0.4) {64-byte block};

    % Arrow to cache
    \draw[->, thick] (3.85, -0.5) -- (3.85, -1.2);

    % Cache line
    \node at (-1, -1.7) {Cache:};
    \foreach \i in {0,...,7} {
        \pgfmathtruncatemacro{\val}{\i+4}
        \draw[fill=yellow!30] (\i*0.6+1.8, -2) rectangle (\i*0.6+2.3, -1.4);
        \node[font=\tiny] at (\i*0.6+2.05, -1.7) {\val};
    }
\end{tikzpicture}
\end{document}
```

. . .

**Block size:** 64 bytes = 8 doubles on modern CPUs

Accessing `a[4]` loads `a[4]` through `a[11]` into cache.

## Cache Organization: Direct Mapped

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    % Address breakdown
    \node at (-2, 3) {Address:};
    \draw (0, 2.7) rectangle (3, 3.3);
    \node at (1.5, 3) {Tag};
    \draw (3, 2.7) rectangle (5, 3.3);
    \node at (4, 3) {Set Index};
    \draw (5, 2.7) rectangle (6.5, 3.3);
    \node at (5.75, 3) {Offset};

    % Cache sets
    \node at (-1, 1) {Cache:};
    \foreach \i in {0,...,3} {
        \pgfmathtruncatemacro{\y}{1 - \i*0.6}
        \draw (0.5, \y-0.2) rectangle (1, \y+0.2);
        \node[font=\tiny] at (0.75, \y) {V};
        \draw (1, \y-0.2) rectangle (2.5, \y+0.2);
        \node[font=\tiny] at (1.75, \y) {Tag};
        \draw (2.5, \y-0.2) rectangle (6, \y+0.2);
        \node[font=\tiny] at (4.25, \y) {Data (64 bytes)};
        \node[left, font=\tiny] at (0.4, \y) {Set \i};
    }

    % Arrow from index to set
    \draw[->, thick, red] (4, 2.5) -- (0.3, 0.4);
\end{tikzpicture}
\end{document}
```

**Direct mapped:** Each memory address maps to exactly one cache location.

Set index = (address / block_size) mod num_sets

## Cache Organization: Set Associative

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.65, transform shape]
    \node at (-1.5, 2) {\textbf{2-way set associative:}};

    % Cache sets with 2 ways
    \foreach \i in {0,...,3} {
        \pgfmathtruncatemacro{\y}{1 - \i*0.8}
        \node[left, font=\small] at (-0.3, \y) {Set \i};

        % Way 0
        \draw[fill=blue!10] (0, \y-0.25) rectangle (2.5, \y+0.25);
        \node[font=\tiny] at (1.25, \y) {Way 0};

        % Way 1
        \draw[fill=green!10] (2.7, \y-0.25) rectangle (5.2, \y+0.25);
        \node[font=\tiny] at (3.95, \y) {Way 1};
    }

    \node[align=left, font=\small] at (7.5, 0) {Each address can\\go in 2 locations\\within its set};
\end{tikzpicture}
\end{document}
```

. . .

**E-way set associative:** Each address can map to E different locations.

- E = 1: Direct mapped
- E = S: Fully associative (any location)
- Typical: L1 is 8-way, L2 is 4-way, L3 is 16-way

## The Three C's of Cache Misses

**Compulsory (Cold) Miss:**

- First access to a block
- Unavoidable

. . .

**Capacity Miss:**

- Working set larger than cache
- Solution: Reduce working set, improve locality

. . .

**Conflict Miss:**

- Cache has space, but blocks map to same set
- Solution: Increase associativity, change data layout

# Operational Intensity Revisited

## Operational Intensity

$$I(n) = \frac{W(n)}{Q(n)}$$

- $W(n)$ = Number of floating-point operations
- $Q(n)$ = Bytes transferred between cache and memory

. . .

| Operation | $W(n)$ | $Q(n)$ | $I(n)$ |
|-----------|--------|--------|--------|
| Vector sum: $y = x + y$ | $n$ | $24n$ | $O(1)$ |
| Matrix-vector: $y = Ax$ | $2n^2$ | $8n^2$ | $O(1)$ |
| FFT | $5n\log n$ | $8n$ | $O(\log n)$ |
| Matrix-matrix: $C = AB$ | $2n^3$ | $8n^2$ | $O(n)$ |

## Compute Bound vs Memory Bound

**Memory bound:** Low operational intensity

- Performance limited by memory bandwidth
- Example: Vector operations, most BLAS Level 1 & 2

. . .

**Compute bound:** High operational intensity

- Performance limited by compute throughput
- Example: Matrix multiplication, convolutions

. . .

**Key insight:** MMM has $O(n)$ operational intensity -- we can hide memory latency with computation if we're clever about data reuse.

## Impact on Performance

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]
    % FFT plot (memory bound)
    \node at (0, 3.5) {\textbf{FFT: I(n) = O(log n)}};
    \draw[->] (0, 0) -- (4.5, 0) node[right] {n};
    \draw[->] (0, 0) -- (0, 3) node[above] {Perf};

    % Performance curve with drop
    \draw[thick, blue] (0.2, 2.5) -- (1.5, 2.5);
    \draw[thick, blue] (1.5, 2.5) -- (2, 1.2);
    \draw[thick, blue] (2, 1.2) -- (4, 1.2);

    % L3 boundary
    \draw[dashed, red] (1.5, 0) -- (1.5, 3);
    \node[below, font=\tiny, red] at (1.5, 0) {L3};

    % MMM plot (compute bound)
    \node at (7, 3.5) {\textbf{MMM: I(n) = O(n)}};
    \draw[->] (6, 0) -- (10.5, 0) node[right] {n};
    \draw[->] (6, 0) -- (6, 3) node[above] {Perf};

    % Performance curve - maintains
    \draw[thick, blue] (6.2, 2.5) -- (10, 2.5);

    % L3 boundary
    \draw[dashed, red] (7.5, 0) -- (7.5, 3);
    \node[below, font=\tiny, red] at (7.5, 0) {L3};
\end{tikzpicture}
\end{document}
```

MMM can maintain high performance even when data exceeds cache.

# Blocking for Cache

## Matrix Multiplication: Naive

```cpp
void mmm_naive(double* A, double* B, double* C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            for (int k = 0; k < n; k++)
                C[i*n + j] += A[i*n + k] * B[k*n + j];
}
```

. . .

**Cache analysis (assuming cache << matrix size):**

- For each of $n^2$ output elements:
  - Read $n$ elements from row of A (n/8 misses)
  - Read $n$ elements from column of B (n misses - poor locality!)
- Total: $\approx 9n^3/8$ cache misses

## The Blocking Idea

Instead of computing one element at a time, compute **blocks**:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.6, transform shape]
    % Matrix A
    \draw (0, 0) rectangle (4, 4);
    \draw[fill=red!30] (0, 3) rectangle (1, 4);
    \node at (2, -0.5) {A};
    \node[font=\tiny] at (0.5, 3.5) {b x b};

    \node at (4.5, 2) {$\times$};

    % Matrix B
    \draw (5, 0) rectangle (9, 4);
    \draw[fill=blue!30] (5, 3) rectangle (6, 4);
    \node at (7, -0.5) {B};

    \node at (9.5, 2) {$=$};

    % Matrix C
    \draw (10, 0) rectangle (14, 4);
    \draw[fill=green!30] (10, 3) rectangle (11, 4);
    \node at (12, -0.5) {C};

    % Block multiplication detail
    \draw[->, thick] (7, -1) -- (7, -2);

    \node at (7, -3) {Process n/b $\times$ n/b blocks};
    \node at (7, -4) {Each block fits in cache};
\end{tikzpicture}
\end{document}
```

**Goal:** Keep working set in cache to exploit temporal locality.

## Blocked Matrix Multiplication

```cpp
void mmm_blocked(double* A, double* B, double* C, int n, int b) {
    for (int ii = 0; ii < n; ii += b)
        for (int jj = 0; jj < n; jj += b)
            for (int kk = 0; kk < n; kk += b)
                // Multiply b x b blocks
                for (int i = ii; i < ii + b; i++)
                    for (int j = jj; j < jj + b; j++)
                        for (int k = kk; k < kk + b; k++)
                            C[i*n + j] += A[i*n + k] * B[k*n + j];
}
```

. . .

**Cache analysis with blocking:**

- Each block multiplication: $b^2/4$ misses (assuming $3b^2 \leq$ cache size)
- Total: $(n/b)^3 \times b^2/4 = n^3/(4b)$ misses
- **Speedup:** $\approx 2.6\sqrt{\gamma}$ where $\gamma$ = cache size

## Choosing the Block Size

**Constraint:** Three $b \times b$ blocks must fit in cache

$$3b^2 \leq \gamma \quad \Rightarrow \quad b \leq \sqrt{\gamma/3}$$

. . .

**Example:** L1 cache = 32 KB = 4096 doubles

$$b \leq \sqrt{4096/3} \approx 37$$

Use $b = 32$ (power of 2 for alignment).

. . .

**Operational intensity with blocking:**

$$I(n) = \frac{2n^3}{n^3/(4b) \times 8} = O(b) = O(\sqrt{\gamma})$$

## Complete Blocking Example

```bash
$ cd examples && make mmm
```

```cpp
// mmm_blocked.cpp
#include <iostream>
#include <chrono>
#include <cstring>
using namespace std;

const int N = 1024;
const int BLOCK = 32;
double A[N*N], B[N*N], C[N*N];

void mmm_naive() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i*N + j] += A[i*N + k] * B[k*N + j];
}

void mmm_blocked() {
    for (int ii = 0; ii < N; ii += BLOCK)
      for (int jj = 0; jj < N; jj += BLOCK)
        for (int kk = 0; kk < N; kk += BLOCK)
          for (int i = ii; i < ii + BLOCK; i++)
            for (int j = jj; j < jj + BLOCK; j++)
              for (int k = kk; k < kk + BLOCK; k++)
                C[i*N + j] += A[i*N + k] * B[k*N + j];
}
```

## Run It Yourself

```cpp
int main() {
    // Initialize
    for (int i = 0; i < N*N; i++) {
        A[i] = 1.0; B[i] = 1.0; C[i] = 0.0;
    }

    auto t1 = chrono::high_resolution_clock::now();
    mmm_naive();
    auto t2 = chrono::high_resolution_clock::now();

    memset(C, 0, sizeof(C));

    auto t3 = chrono::high_resolution_clock::now();
    mmm_blocked();
    auto t4 = chrono::high_resolution_clock::now();

    double gflops = 2.0 * N * N * N / 1e9;
    cout << "Naive:   " << gflops / chrono::duration<double>(t2-t1).count()
         << " GFLOPS\n";
    cout << "Blocked: " << gflops / chrono::duration<double>(t4-t3).count()
         << " GFLOPS\n";
}
```

# Measuring Cache Performance

## Cache Misses with perf

```bash
$ cd examples && make perf_locality
$ cd examples && make perf_mmm
```

. . .

**Key metrics:**

- L1 miss rate: L1-dcache-load-misses / L1-dcache-loads
- LLC miss rate: LLC-load-misses / LLC-loads

High LLC miss rate = memory bound.

## Example: Comparing Access Patterns

```bash
$ perf stat -e L1-dcache-load-misses ./sum_rows
  16,000,000 L1-dcache-load-misses    # 1 miss per 8 elements

$ perf stat -e L1-dcache-load-misses ./sum_cols
 128,000,000 L1-dcache-load-misses    # 1 miss per element
```

. . .

Column-major access has **8x more cache misses** than row-major!

(Exact numbers depend on array size and cache configuration)

## Cache Miss Diagnosis Flowchart

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape,
    box/.style={rectangle, draw, rounded corners, minimum width=2.5cm, minimum height=0.8cm, align=center, font=\small},
    decision/.style={diamond, draw, aspect=2, minimum width=2cm, align=center, font=\small}]

    \node[box, fill=blue!20] (start) at (0, 0) {High cache\\miss rate?};

    \node[decision] (size) at (0, -2) {Working set\\$>$ cache?};

    \node[box, fill=red!20] (capacity) at (-3, -4) {Capacity miss\\Block/tile data};

    \node[decision] (stride) at (3, -4) {Stride $>$ 1?};

    \node[box, fill=orange!20] (spatial) at (1.5, -6) {Poor spatial\\locality};

    \node[box, fill=yellow!20] (conflict) at (4.5, -6) {Possible\\conflict miss};

    \draw[->] (start) -- (size);
    \draw[->] (size) -- node[left] {Yes} (capacity);
    \draw[->] (size) -- node[right] {No} (stride);
    \draw[->] (stride) -- node[left] {Yes} (spatial);
    \draw[->] (stride) -- node[right] {Power of 2} (conflict);
\end{tikzpicture}
\end{document}
```

# Conflict Misses and Power-of-2 Strides

## The Power-of-2 Problem

```cpp
// Accessing every 1024th element
for (int i = 0; i < n; i += 1024)
    sum += a[i];
```

. . .

**Problem:** If stride is a power of 2, elements may map to same cache set!

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    \node at (-1.5, 0) {Memory:};

    % Array elements
    \foreach \i in {0,...,7} {
        \draw[fill=blue!20] (\i*1.2, -0.3) rectangle (\i*1.2+1, 0.3);
        \pgfmathtruncatemacro{\val}{\i*1024}
        \node[font=\tiny] at (\i*1.2+0.5, 0) {a[\val]};
    }

    % Cache
    \node at (-1.5, -2) {Cache:};
    \draw[fill=yellow!30] (0, -2.3) rectangle (4, -1.7);
    \node at (2, -2) {Set 0};

    % Arrows showing all map to same set
    \foreach \i in {0,...,7} {
        \draw[->, red] (\i*1.2+0.5, -0.5) -- (2, -1.5);
    }

    \node[red, font=\small] at (5, -2) {All map to same set!};
\end{tikzpicture}
\end{document}
```

## Where Conflict Misses Occur

**Common scenarios:**

1. Column access in power-of-2 sized matrices
2. FFT with power-of-2 sizes
3. Stencil computations on power-of-2 grids

. . .

**Example: 2D array column access**

```cpp
double a[1024][1024];  // Power of 2!

// Column access - all elements map to same cache sets
for (int i = 0; i < 1024; i++)
    sum += a[i][0];  // Stride = 1024 doubles = 8KB
```

## Solutions to Conflict Misses

1. **Padding:** Add extra elements to break alignment

```cpp
double a[1024][1024 + 8];  // +8 padding
```

. . .

2. **Use non-power-of-2 sizes:**

```cpp
double a[1000][1000];  // Avoid power of 2
```

. . .

3. **Blocking with appropriate sizes:**

Choose block size that doesn't create power-of-2 strides.

## Swizzling: A Smarter Solution

**Problem with padding:** Wastes memory, changes array dimensions.

**Swizzling:** Remap indices to spread accesses across cache sets without changing array size.

. . .

**Key idea:** Use XOR to transform column index based on row index.

$$j_{\text{swizzled}} = j \oplus (i \land \text{mask})$$

where $\oplus$ is XOR and $\land$ is bitwise AND.

## Why Does XOR Work?

**XOR properties:**

- $a \oplus 0 = a$ (identity)
- $a \oplus a = 0$ (self-inverse)
- $a \oplus b \oplus b = a$ (reversible!)

. . .

**Key insight:** XOR with different row indices produces different results:

| Row i | i AND 3 | 0 XOR (i AND 3) | Result |
|-------|---------|-----------------|--------|
| 0 | 0 | 0 XOR 0 | **0** |
| 1 | 1 | 0 XOR 1 | **1** |
| 2 | 2 | 0 XOR 2 | **2** |
| 3 | 3 | 0 XOR 3 | **3** |

Column 0 now maps to 4 different physical columns!

## Swizzling Example: Reading Column 0

**Setup:** 8x8 matrix, 4 cache sets, each row maps to one set.

```cpp
double a[8][8];  // Row i maps to cache set (i % 4)

// Want to read column 0: a[0][0], a[1][0], a[2][0], ...
```

. . .

**Without swizzling:** All accesses go to column 0

| Access | Location | Cache Set |
|--------|----------|-----------|
| a[0][0] | Row 0, Col 0 | Set 0 |
| a[1][0] | Row 1, Col 0 | Set 1 |
| a[2][0] | Row 2, Col 0 | Set 2 |
| a[3][0] | Row 3, Col 0 | Set 3 |
| a[4][0] | Row 4, Col 0 | Set 0 (conflict!) |
| a[5][0] | Row 5, Col 0 | Set 1 (conflict!) |

## Swizzling Example: The Transformation

**With swizzling:** $j' = j \oplus (i \land 3)$

To read "logical column 0", we access different physical columns:

| Row $i$ | Want col | $i \land 3$ | $j' = 0 \oplus (i \land 3)$ | Access |
|---------|----------|-------------|----------------------------|--------|
| 0 | 0 | 0 | 0 | a[0][**0**] |
| 1 | 0 | 1 | 1 | a[1][**1**] |
| 2 | 0 | 2 | 2 | a[2][**2**] |
| 3 | 0 | 3 | 3 | a[3][**3**] |
| 4 | 0 | 0 | 0 | a[4][**0**] |
| 5 | 0 | 1 | 1 | a[5][**1**] |

. . .

**Result:** Accesses spread across columns, avoiding conflicts!

## Visualizing Swizzled Layout

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.6, transform shape]
    % Original layout
    \node[font=\small] at (2, 1.5) {\textbf{Original: Column 0}};
    \foreach \i in {0,...,7} {
        \foreach \j in {0,...,7} {
            \ifnum\j=0
                \draw[fill=red!40] (\j*0.6, -\i*0.6) rectangle (\j*0.6+0.5, -\i*0.6+0.5);
            \else
                \draw[fill=gray!15] (\j*0.6, -\i*0.6) rectangle (\j*0.6+0.5, -\i*0.6+0.5);
            \fi
        }
    }
    \node[font=\tiny] at (-0.5, 0.25) {Row 0};
    \node[font=\tiny] at (-0.5, -4.55) {Row 7};

    % Arrow
    \draw[->, very thick] (5.5, -2) -- (7, -2);
    \node[above, font=\small] at (6.25, -2) {Swizzle};

    % Swizzled layout
    \node[font=\small] at (11, 1.5) {\textbf{Swizzled: "Column 0"}};
    \foreach \i in {0,...,7} {
        \foreach \j in {0,...,7} {
            \draw[fill=gray!15] (\j*0.6+8, -\i*0.6) rectangle (\j*0.6+8.5, -\i*0.6+0.5);
        }
    }
    % Highlight swizzled positions (j = i & 3)
    \draw[fill=red!40] (0*0.6+8, -0*0.6) rectangle (0*0.6+8.5, -0*0.6+0.5);
    \draw[fill=red!40] (1*0.6+8, -1*0.6) rectangle (1*0.6+8.5, -1*0.6+0.5);
    \draw[fill=red!40] (2*0.6+8, -2*0.6) rectangle (2*0.6+8.5, -2*0.6+0.5);
    \draw[fill=red!40] (3*0.6+8, -3*0.6) rectangle (3*0.6+8.5, -3*0.6+0.5);
    \draw[fill=red!40] (0*0.6+8, -4*0.6) rectangle (0*0.6+8.5, -4*0.6+0.5);
    \draw[fill=red!40] (1*0.6+8, -5*0.6) rectangle (1*0.6+8.5, -5*0.6+0.5);
    \draw[fill=red!40] (2*0.6+8, -6*0.6) rectangle (2*0.6+8.5, -6*0.6+0.5);
    \draw[fill=red!40] (3*0.6+8, -7*0.6) rectangle (3*0.6+8.5, -7*0.6+0.5);

    % Labels
    \node[red, font=\small] at (2, -5.5) {All same column};
    \node[red, font=\small] at (2, -6) {= conflict misses};
    \node[green!60!black, font=\small] at (11, -5.5) {Diagonal pattern};
    \node[green!60!black, font=\small] at (11, -6) {= no conflicts};
\end{tikzpicture}
\end{document}
```

## Swizzling Code: Read Column

```cpp
// Reading "logical column c" from swizzled array
double read_column_swizzled(double a[N][N], int c) {
    double sum = 0;
    for (int i = 0; i < N; i++) {
        int j_physical = c ^ (i & 0x3);  // Swizzle formula
        sum += a[i][j_physical];
    }
    return sum;
}
```

. . .

**The data must be stored in swizzled layout:**

```cpp
// Store value at logical position (i, j)
void store_swizzled(double a[N][N], int i, int j, double val) {
    int j_physical = j ^ (i & 0x3);
    a[i][j_physical] = val;
}

// Read value from logical position (i, j)
double read_swizzled(double a[N][N], int i, int j) {
    int j_physical = j ^ (i & 0x3);
    return a[i][j_physical];
}
```

## Complete Example: Swizzled Matrix Storage

```bash
$ cd examples && make swizzle
```

```cpp
// swizzle_demo.cpp
#include <iostream>
using namespace std;

const int N = 8;
double A[N][N];  // Physical storage

// Swizzle function
int swizzle(int i, int j) { return j ^ (i & 0x3); }

// Store in swizzled layout
void store(int i, int j, double val) { A[i][swizzle(i,j)] = val; }

// Read from swizzled layout
double read(int i, int j) { return A[i][swizzle(i,j)]; }

int main() {
    // Initialize: logical[i][j] = i * N + j
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            store(i, j, i * N + j);

    // Read column 0 - physically accesses different columns!
    cout << "Logical column 0:\n";
    for (int i = 0; i < N; i++)
        cout << "  logical[" << i << "][0] = " << read(i, 0)
             << " (physical col " << swizzle(i, 0) << ")\n";
}
```

## Example Output

```
Logical column 0:
  logical[0][0] = 0 (physical col 0)
  logical[1][0] = 8 (physical col 1)
  logical[2][0] = 16 (physical col 2)
  logical[3][0] = 24 (physical col 3)
  logical[4][0] = 32 (physical col 0)
  logical[5][0] = 40 (physical col 1)
  logical[6][0] = 48 (physical col 2)
  logical[7][0] = 56 (physical col 3)
```

. . .

**Physical columns accessed:** 0, 1, 2, 3, 0, 1, 2, 3

Instead of all mapping to column 0, accesses are distributed!

## Swizzling for Matrix Transpose

Matrix transpose has conflict misses when writing columns:

```cpp
// Naive: writes to B are column-wise (conflicts!)
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        B[j][i] = A[i][j];
```

. . .

**Solution:** Store B in swizzled layout

```cpp
// B stored swizzled: writes become distributed
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
        int j_swiz = i ^ (j & 0x3);  // Swizzle the write
        B[j][j_swiz] = A[i][j];
    }

// Later, read B with matching swizzle
double val = B[row][col ^ (row & 0x3)];
```

## Swizzling: The Key Insight

**Swizzling trades logical simplicity for physical distribution**

. . .

**Without swizzling:**

- Logical column $j$ = Physical column $j$
- Simple indexing
- Cache conflicts on column access

. . .

**With swizzling:**

- Logical column $j$ = Physical column $j \oplus (i \land \text{mask})$
- Must swizzle on every access
- No cache conflicts!

## Choosing the Swizzle Mask

The mask determines how many cache sets to distribute across:

| Mask | Binary | Sets distributed |
|------|--------|------------------|
| 0x1 | 001 | 2 sets |
| 0x3 | 011 | 4 sets |
| 0x7 | 111 | 8 sets |
| 0xF | 1111 | 16 sets |

. . .

**Rule of thumb:** Mask should cover enough bits to span cache associativity.

```cpp
// For 8-way associative cache, use mask 0x7
int j_swizzled = j ^ (i & 0x7);
```

## When to Use Each Solution

| Solution | Pros | Cons |
|----------|------|------|
| **Padding** | Simple, no code changes | Wastes memory |
| **Non-power-of-2** | Simple | May not be possible |
| **Blocking** | General purpose | Complex code |
| **Swizzling** | No memory waste | Complex indexing |

. . .

**Swizzling is especially important in GPU programming:**

- Shared memory has 32 banks
- Power-of-2 strides cause bank conflicts
- NVIDIA uses swizzling extensively in cuBLAS, cuDNN

# Write Policies

## What Happens on a Write?

**Write-hit policies:**

- **Write-through:** Write to both cache and memory immediately
- **Write-back:** Write only to cache; write to memory when evicted

. . .

**Write-miss policies:**

- **Write-allocate:** Load block into cache, then write
- **No-write-allocate:** Write directly to memory

. . .

**Modern CPUs:** Write-back + Write-allocate (most efficient for repeated writes)

## Write-Back Implications

```cpp
// This pattern benefits from write-back
for (int i = 0; i < n; i++)
    a[i] = a[i] * 2;  // Read and write same location
```

. . .

**With write-back:**

1. First access: Load block into cache (miss)
2. Subsequent accesses: All in cache (hits)
3. Eviction: Write modified block back to memory

**Traffic:** Read once + Write once per block (not per element)

# Summary

## Key Takeaways

1. **Locality is crucial** for cache performance
   - Temporal: Reuse data while in cache
   - Spatial: Access data sequentially

. . .

2. **Cache organization** (S sets, E ways, B bytes/block)
   - Direct mapped vs set-associative
   - Block size typically 64 bytes

. . .

3. **Three C's of misses:** Compulsory, Capacity, Conflict

. . .

4. **Blocking** improves locality for matrix operations
   - Choose block size based on cache size
   - Increases operational intensity

## Try It Yourself

```bash
# Run all examples
$ cd examples && make run

# Build with -O3 for comparison
$ cd examples && make fast && make run

# Use perf to measure cache misses
$ cd examples && make perf
```

## Practical Guidelines

**Do:**

- Access arrays sequentially (stride 1)
- Use blocking for matrix operations
- Keep working set small enough to fit in cache
- Measure with `perf stat` to verify

. . .

**Avoid:**

- Column-major access in row-major languages (C/C++)
- Power-of-2 array dimensions when accessing columns
- Random access patterns when sequential is possible

## Coming Up

Next lectures will apply these concepts:

- **GEMM optimization:** Combining ILP + SIMD + Blocking
- **Database systems:** Buffer management and I/O optimization
- **GPU memory hierarchy:** Different tradeoffs, same principles
