---
title: Lecture 6. CPU Microarchitecture
subtitle: Introductions to Data Systems and Data Design
author: Ce Zhang
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing}
---

# CPU Microarchitecture Overview

## What is a CPU?

At its core, a CPU does two things:

1. **Process instructions** (compute)
2. **Move data** (memory access)

. . .

**Key insight:** Modern CPUs are very good at (1), but fundamentally limited by (2).

Understanding *why* will help you design better data systems.

## Skylake Server Microarchitecture

:::::: {.columns}
::: {.column width="45%"}
**Front End** 

- Fetch instructions at 16 B/cycle
- Decode them into uOps

**Execution Engine**

- 8 ports, 4 ALUs

**Memory Subsystem**

- L1D: 32 KB, 8-way, **4 cycles**
- L2: 1 MB, 16-way, **14 cycles**
- Load buffer: 72 entries
- Store buffer: 56 entries
- 64 B cache line, 64 B/cycle bandwidth
:::
::: {.column width="55%"}
![](Pasted%20image%2020260117215902.png){height=85%}
:::
::::::


## The Front End (Brief Overview)

:::::: {.columns}
::: {.column width="65%"}
**What it does:** Fetches and decodes x86 instructions into µOPs

- **L1I cache** (32 KB, 8-way): Stores instruction bytes. 8-way = each address can map to 8 cache slots (reduces conflicts).

- **Fetch** (16 B/cycle): Reads from L1I using program counter (PC) → outputs raw instruction bytes.

- **Decode** (4 instr/cycle): Translates x86 bytes → µOPs. (1 complex + 3 simple decoders)

- **µOP cache** (6 µOPs/cycle): Caches decoded µOPs for hot loops — skips decode entirely.
:::
::: {.column width="35%"}
\includegraphics[clip, trim=0 580 200 0, height=4.5cm]{Pasted image 20260117215902.png}
:::
::::::

## Front End Bottlenecks

**When does the front end stall?**

- **I-cache miss:** Code doesn't fit in 32 KB L1I → wait ~14 cycles (L2) or ~100+ cycles (RAM)
  - *Example:* Large binaries, heavy inlining, many cold function calls

- **Branch mispredict:** CPU guesses branch direction *before* condition is evaluated (to keep pipeline full). Wrong guess → flush all speculative work, restart.
  - *Predictable:* Loop branches (`for i < n`), always-taken/never-taken → >99% accuracy
  - *Unpredictable:* Data-dependent branches like `if (array[i] > threshold)` on random data → ~50% accuracy
  - ~15–20 cycle penalty per mispredict

**I-cache is real!** We will not be focusing on the front-end in this course, but they will show up (often unexpectedly).

## Our Focus: The Execution Engine & Memory

\includegraphics[clip, trim=0 0 0 875, height=6cm]{Pasted image 20260117215902.png}

Execution Engine (yellow) + Memory Subsystem (green) + L2 Cache (right)

## The Scheduler

**What it does:** Holds decoded µOPs waiting for operands, dispatches to execution ports when ready.

- **97 entries** — can hold ~97 in-flight µOPs
- **Out-of-order execution:** Instructions don't execute in program order; they execute when *operands are ready*
- **Dependency tracking:** Tracks which µOPs depend on which results

**Key insight:** The scheduler hides latency by doing *other work* while waiting for slow operations (e.g., memory loads).

## Registers and Register Renaming

**Architectural registers:** What the programmer sees

- **16 general-purpose** (64-bit each): `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rbp`, `rsp`, `r8`–`r15`
  - Can access as 32-bit (`eax`), 16-bit (`ax`), or 8-bit (`al`, `ah`)
  - Total: 16 × 8 bytes = 128 bytes

- **16/32 vector registers** (AVX-256: 256-bit each, AVX-512: 512-bit): `ymm0`–`ymm15` or `zmm0`–`zmm31`
  - Used for SIMD: process 4 doubles or 8 floats in one instruction
  - Total: 16 × 32 bytes = 512 bytes (AVX-256)

**Physical registers:** What the hardware actually has (Skylake: 180 integer, 168 vector). We will not go into details, do ask ChatGPT if you are curious!

**Register access:** ~0 cycles latency (same cycle), "unlimited bandwidth"

## Execution Ports (Skylake)

**How it works:**

1. Scheduler checks which µOPs have all operands ready
2. Each cycle, scheduler dispatches ready µOPs to available ports
3. Each port has specific execution units — µOP goes to a port that can handle it
4. Execution takes 1+ cycles depending on operation (add: 1 cycle, divide: 10+ cycles)
5. Result is written back, waking up dependent µOPs

**Key constraint:** Each port can only execute **one µOP per cycle**

\tiny

| Port    | What it does               | Latency |
| ------- | -------------------------- | ------- |
| P0, P1  | ALU, FMA (integer/FP math) | 1 / 4 cycles |
| P0 only | Division                   | 10-20 cycles |
| P5      | ALU, vector shuffle        | 1 cycle |
| P6      | ALU, branches              | 1 cycle |
| P2, P3  | Load (2 loads/cycle)       | 4-5 cycles (L1) |
| P4, P7  | Store data + address       | ~4 cycles |

\normalsize



## L1 Data Cache (32 KB, 8-Way Set Associative)

:::::: {.columns}
::: {.column width="65%"}

**32 KB** = 64 sets × 8 ways × 64 B

- **Cache line:** 64 bytes — minimum transfer (even for 1 byte read)
- **Latency:** 4-5 cycles

**Mental model for this course:**

- 32 KB of fast memory
- 64 B cache line (minimum unit)
- **~LRU eviction:** Least recently used data gets evicted when full
	- It is not real LRU for two reasons: (1) 8-way set associative and (2) not exact LRU implementation.
	- In this course we will ignore these details and only think about L1 as one big chunk of cache.
:::
::: {.column width="35%"}
```tikz
\begin{document}
\begin{tikzpicture}[scale=0.6, transform shape]

    \node at (2.8, 3.8) {\textbf{8 Ways}};
    \node[rotate=90] at (-1, 1.2) {\textbf{Sets}};

    % Grid - draw as rectangles
    \draw[fill=blue!10] (0,3) rectangle (5.6,3.4);
    \draw[fill=blue!10] (0,2.5) rectangle (5.6,2.9);
    \draw[fill=blue!10] (0,2) rectangle (5.6,2.4);
    \draw[fill=blue!10] (0,1) rectangle (5.6,1.4);
    \draw[fill=blue!10] (0,0.5) rectangle (5.6,0.9);
    \draw[fill=blue!10] (0,0) rectangle (5.6,0.4);

    % Vertical lines for ways
    \draw (0.7,0) -- (0.7,3.4);
    \draw (1.4,0) -- (1.4,3.4);
    \draw (2.1,0) -- (2.1,3.4);
    \draw (2.8,0) -- (2.8,3.4);
    \draw (3.5,0) -- (3.5,3.4);
    \draw (4.2,0) -- (4.2,3.4);
    \draw (4.9,0) -- (4.9,3.4);

    % Set labels
    \node at (-0.4, 3.2) {0};
    \node at (-0.4, 2.7) {1};
    \node at (-0.4, 2.2) {2};
    \node at (-0.4, 1.2) {...};
    \node at (-0.4, 0.7) {62};
    \node at (-0.4, 0.2) {63};

    % Highlight one slot
    \draw[fill=green!50] (1.4,2.5) rectangle (2.1,2.9);

    % Arrow
    \draw[->, very thick, red] (-2, 2.7) -- (-0.1, 2.7);
    \node[left] at (-2, 2.7) {addr};

\end{tikzpicture}
\end{document}
```
:::
::::::

## L2 Cache (1 MB, 16-Way Set Associative)

**1 MB** per core, private

- **Cache line:** 64 bytes
- **Latency:** 14 cycles (3.5× slower than L1)

**Mental model for this course:**

- 1 MB of medium-speed memory per core
- **~LRU eviction**
- **Unified:** Holds both instructions and data
- Miss L1 → check L2 → 14 cycle penalty

## L3 Cache (Last Level Cache)

**Size:** 1.375 MB per core (shared across all cores)

**Latency:** 50–70 cycles

**Bandwidth:** Lower than L2, shared among cores

**Key properties:**

- **Shared:** All cores access the same L3
- **Coherent:** Hardware keeps data consistent across cores

**When you miss L3:** DRAM access — 100+ cycles, ~100 ns.

## Putting All Numbers Together

| Operation       | Latency       | Relative |
| --------------- | ------------- | -------- |
| Register access | ~0.3 ns       | 1x       |
| L1 cache hit    | ~1 ns         | 3x       |
| L2 cache hit    | ~4 ns         | 13x      |
| L3 cache hit    | ~12 ns        | 40x      |
| **Main memory** | ~**100 ns**   | **333x** |
| SSD random read | ~16,000 ns    | 53,000x  |
| HDD random read | ~2,000,000 ns | 6.6M x   |

**Memory access is 100–300× slower than computation!** This is *the* fundamental theme that has governed data systems design and optimization for decades. 

# Life Cycle of an Instruction

## What Happens When You Run Code?

Every instruction goes through a journey:

```tikz
\begin{document}
\begin{tikzpicture}[
    stage/.style={draw, rectangle, rounded corners, minimum width=1.8cm, minimum height=0.8cm, fill=blue!20},
    arrow/.style={->, thick, >=stealth}]

    \node[stage] (fetch) at (0,0) {Fetch};
    \node[stage] (decode) at (3,0) {Decode};
    \node[stage] (exec) at (6,0) {Execute};
    \node[stage] (retire) at (9,0) {Retire};

    \draw[arrow] (fetch) -- (decode);
    \draw[arrow] (decode) -- (exec);
    \draw[arrow] (exec) -- (retire);
\end{tikzpicture}
\end{document}
```

. . .

## A Simple C++ Program

:::::: {.columns}
::: {.column width="25%"}
```cpp
// add.cpp
int main() {
    long a = 5;     
    long b = 3;     
    long c = a + b; 
    return c;
}
```
:::
::: {.column width="75%"}
```bash
$ cd examples && make add       # Compile without optimization
$ ./add && echo $?              # Execute, print return value (8)

$ make asm                      # Generate assembly files
$ cat add.s                     # View assembly
$ objdump -d add | less         # Or disassemble binary
```
:::
::::::

**Try it!** Use `godbolt.org` to see assembly instantly in browser.

## The Assembly (Unoptimized)

```asm
main:
    push rbp                      ; Save caller's base pointer
    mov rbp, rsp                  ; Set up our stack frame

    mov QWORD PTR [rbp-8], 5      ; Store 5 on stack (variable a)
    mov QWORD PTR [rbp-16], 3     ; Store 3 on stack (variable b)

    mov rdx, QWORD PTR [rbp-8]    ; Load a (5) into rdx
    mov rax, QWORD PTR [rbp-16]   ; Load b (3) into rax
    add rax, rdx                  ; rax = rax + rdx = 3 + 5 = 8

    mov QWORD PTR [rbp-24], rax   ; Store result on stack (variable c)
    mov rax, QWORD PTR [rbp-24]   ; Load c into rax (return value)

    pop rbp                       ; Restore caller's base pointer
    ret                           ; Return (result in rax)
```

**Notice:** Unoptimized code stores/loads from memory unnecessarily!

## Stage 1: Fetch

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape,
    memcell/.style={draw, minimum width=2.5cm, minimum height=0.5cm, font=\ttfamily\small}]

    % Memory
    \node[above] at (2, 2.5) {Instruction Memory};
    \node[memcell] (m1) at (2, 2) {...};
    \node[memcell, fill=yellow!30] (m2) at (2, 1.5) {add rax, rbx};
    \node[memcell] (m3) at (2, 1) {sub rcx, 1};
    \node[memcell] (m4) at (2, 0.5) {jmp 0x1020};

    % PC
    \node[left, font=\small] at (-0.5, 1.5) {PC = 0x1000};
    \draw[->, thick] (0, 1.5) -- (m2.west);

    % Instruction register
    \node[draw, minimum width=3cm, minimum height=0.6cm, fill=blue!10] (ir) at (2, -1) {\ttfamily add rax, rbx};
    \node[above, font=\small] at (2, -0.6) {Instruction Register};

    \draw[->, thick] (m2.south) -- ++(0, -0.8) -- (ir.north);
\end{tikzpicture}
\end{document}
```

- Read instruction bytes from memory (via I-cache)

## Stage 2: Decode

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape,
    box/.style={draw, rectangle, minimum width=4cm, align=left, font=\small}]

    \node[font=\ttfamily] (instr) at (0, 2) {"add rax, rbx" (bytes: 48 01 D8)};

    \draw[->, thick] (instr.south) -- ++(0, -0.5);

    \node[box, minimum height=2.2cm, fill=orange!10, align=left] (decoder) at (0, -0.5) {\textbf{Decoder}\\Opcode: ADD\\Source 1: RAX\\Source 2: RBX\\Dest: RAX};

    \draw[->, thick] (decoder.south) -- ++(0, -0.5) node[below] {$\mu$ops to execution};
\end{tikzpicture}
\end{document}
```

Modern CPUs decode complex instructions into simpler micro-operations.

## Stage 3: Execute

```tikz
\begin{document}
\begin{tikzpicture}[
    reg/.style={draw, rectangle, minimum width=1.2cm, minimum height=0.8cm, fill=blue!10}]

    \node[reg, label=above:RAX] (rax) at (0, 2) {5};
    \node[reg, label=above:RBX] (rbx) at (3, 2) {3};

    \draw[->, thick] (rax.south) -- ++(0, -0.5) -- ++(0.8, 0);
    \draw[->, thick] (rbx.south) -- ++(0, -0.5) -- ++(-0.8, 0);

    \node[draw, circle, minimum size=1cm, fill=green!20] (alu) at (1.5, 0.3) {ALU};

    \node[below of=alu, node distance=1.2cm, font=\large] {$5 + 3 = 8$};

    \draw[->, thick] (alu.south) -- ++(0, -0.8);
\end{tikzpicture}
\end{document}
```

- Read operands from registers
- Perform the computation
- ALU operations are **fast**: 1 cycle for this case

## Stage 4: Retire (Write Back)

```tikz
\begin{document}
\begin{tikzpicture}[
    reg/.style={draw, rectangle, minimum width=1.2cm, minimum height=0.8cm}]

    \node[font=\large] at (1.5, 2) {Result: 8};
    \draw[->, thick] (1.5, 1.6) -- (1.5, 1);

    \node[reg, fill=green!30, label=below:RAX] (rax) at (0, 0.3) {8};
    \node[reg, fill=blue!10, label=below:RBX] (rbx) at (3, 0.3) {3};

    \node[right, font=\small] at (4, 0.3) {(updated!)};
\end{tikzpicture}
\end{document}
```

- Write result back to destination register
- Update architectural state
- Instruction is now "complete"

## Pipelining: Overlapping Instructions

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.65, transform shape,
    F/.style={draw, fill=blue!30, minimum width=0.9cm, minimum height=0.5cm, font=\tiny},
    D/.style={draw, fill=green!30, minimum width=0.9cm, minimum height=0.5cm, font=\tiny},
    E/.style={draw, fill=orange!30, minimum width=0.9cm, minimum height=0.5cm, font=\tiny},
    R/.style={draw, fill=red!30, minimum width=0.9cm, minimum height=0.5cm, font=\tiny}]

    % Time axis
    \node[above, font=\scriptsize] at (1, 3) {1};
    \node[above, font=\scriptsize] at (2, 3) {2};
    \node[above, font=\scriptsize] at (3, 3) {3};
    \node[above, font=\scriptsize] at (4, 3) {4};
    \node[above, font=\scriptsize] at (5, 3) {5};
    \node[above, font=\scriptsize] at (6, 3) {6};
    \node[above, font=\scriptsize] at (7, 3) {7};
    \node[above, font=\scriptsize] at (8, 3) {8};
    \node[above] at (4.5, 3.5) {Time};

    % I1
    \node[left, font=\scriptsize] at (0, 2) {I1};
    \node[F] at (1, 2) {F}; \node[D] at (2, 2) {D}; \node[E] at (3, 2) {E}; \node[R] at (4, 2) {R};

    % I2
    \node[left, font=\scriptsize] at (0, 1.3) {I2};
    \node[F] at (2, 1.3) {F}; \node[D] at (3, 1.3) {D}; \node[E] at (4, 1.3) {E}; \node[R] at (5, 1.3) {R};

    % I3
    \node[left, font=\scriptsize] at (0, 0.6) {I3};
    \node[F] at (3, 0.6) {F}; \node[D] at (4, 0.6) {D}; \node[E] at (5, 0.6) {E}; \node[R] at (6, 0.6) {R};

    % I4
    \node[left, font=\scriptsize] at (0, -0.1) {I4};
    \node[F] at (4, -0.1) {F}; \node[D] at (5, -0.1) {D}; \node[E] at (6, -0.1) {E}; \node[R] at (7, -0.1) {R};

    % I5
    \node[left, font=\scriptsize] at (0, -0.8) {I5};
    \node[F] at (5, -0.8) {F}; \node[D] at (6, -0.8) {D}; \node[E] at (7, -0.8) {E}; \node[R] at (8, -0.8) {R};

\end{tikzpicture}
\end{document}
```

- After pipeline fills: 1 instruction completes per cycle
- Modern CPUs: 14-20 stages, 4-6 instructions/cycle

## Superscalar: Instruction-Level Parallelism

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape,
    unit/.style={draw, rectangle, minimum width=1.2cm, minimum height=0.6cm, fill=blue!15},
    stage/.style={draw, rectangle, minimum width=1.5cm, minimum height=0.8cm}]

    \node[stage, align=center] (fetch) at (0, 0) {\small Fetch};
    \node[stage] (decode) at (2.5, 0) {\small Decode};

    \node[unit] (alu1) at (5.5, 1) {ALU};
    \node[unit] (alu2) at (5.5, 0.3) {ALU};
    \node[unit] (load) at (5.5, -0.4) {Load};
    \node[unit] (store) at (5.5, -1.1) {Store};

    \node[stage, align=center] (retire) at (8.5, 0) {\small Retire};

    \draw[->, thick] (fetch) -- (decode);
    \draw[->, thick] (decode.east) -- ++(0.5,0) |- (alu1.west);
    \draw[->, thick] (decode.east) -- ++(0.5,0) |- (alu2.west);
    \draw[->, thick] (decode.east) -- ++(0.5,0) |- (load.west);
    \draw[->, thick] (decode.east) -- ++(0.5,0) |- (store.west);
    \draw[->, thick] (alu1.east) -- ++(0.5,0) |- (retire.west);
    \draw[->, thick] (store.east) -- ++(0.5,0) |- (retire.west);
\end{tikzpicture}
\end{document}
```

Multiple instructions execute **in parallel** each cycle.

**All modern CPUs are superscalar.** Intel, AMD, Apple Silicon, ARM — they all execute multiple instructions per cycle.

**Mental model for this course:** Think of the CPU as being able to execute ~$K$ independent instructions per cycle. If instruction B depends on the result of instruction A, they must run sequentially. If they are independent, they can run in parallel, as long as there are available *ports that can run the operation*.

# Life Cycle of Data

## Where Does Data Live? {.fragile}

Data has its own journey through the memory hierarchy:

```tikz
\begin{document}
\begin{tikzpicture}[
    level/.style={draw, rectangle, rounded corners, minimum height=0.7cm, fill=blue!20},
    arrow/.style={->, thick, >=stealth}]

    \node[level, minimum width=1.2cm] (disk) at (0,0) {Disk};
    \node[level, minimum width=1.4cm] (ram) at (2.2,0) {RAM};
    \node[level, minimum width=1cm] (l3) at (4.2,0) {L3};
    \node[level, minimum width=1cm] (l2) at (5.8,0) {L2};
    \node[level, minimum width=1cm] (l1) at (7.4,0) {L1};
    \node[level, minimum width=1.6cm, fill=green!30] (reg) at (9.4,0) {Registers};

    \draw[arrow] (disk) -- (ram);
    \draw[arrow] (ram) -- (l3);
    \draw[arrow] (l3) -- (l2);
    \draw[arrow] (l2) -- (l1);
    \draw[arrow] (l1) -- (reg);
\end{tikzpicture}
\end{document}
```

Each level is: **Smaller**, **Faster**, **More expensive**

## The Memory Hierarchy 

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape]
    \node[draw, fill=red!30, minimum width=2cm, minimum height=0.5cm] (reg) at (0, 3) {Registers};
    \node[right, font=\scriptsize] at (1.5, 3) {128 B, 0.3 ns};

    \node[draw, fill=orange!30, minimum width=3cm, minimum height=0.5cm] (l1) at (0, 2.2) {L1 Cache};
    \node[right, font=\scriptsize] at (2, 2.2) {32-48 KB, 1 ns};

    \node[draw, fill=yellow!30, minimum width=4cm, minimum height=0.5cm] (l2) at (0, 1.4) {L2 Cache};
    \node[right, font=\scriptsize] at (2.5, 1.4) {256 KB - 1 MB, 4 ns};

    \node[draw, fill=green!30, minimum width=5cm, minimum height=0.5cm] (l3) at (0, 0.6) {L3 Cache};
    \node[right, font=\scriptsize] at (3, 0.6) {8-64 MB, 12 ns};

    \node[draw, fill=blue!30, minimum width=6cm, minimum height=0.5cm] (ram) at (0, -0.2) {Main Memory};
    \node[right, font=\scriptsize] at (3.5, -0.2) {16-256 GB, 100 ns};

    \node[draw, fill=gray!30, minimum width=7cm, minimum height=0.5cm] (disk) at (0, -1) {SSD / HDD};
    \node[right, font=\scriptsize] at (4, -1) {TB scale, us - ms};

    \draw[<->, thick] (-4.5, 3) -- (-4.5, -1);
    \node[rotate=90] at (-5, 1) {Faster / Smaller};
\end{tikzpicture}
\end{document}
```

## What Happens on a Memory Access?

Consider: `mov rdx, QWORD PTR [rbp-8]`

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape,
    cache/.style={draw, rectangle, minimum width=1.5cm, minimum height=0.6cm}]

    \node[cache, fill=red!20] (l1) at (0, 0) {L1};
    \node[cache, fill=orange!20] (l2) at (2.5, 0) {L2};
    \node[cache, fill=yellow!20] (l3) at (5, 0) {L3};
    \node[cache, fill=green!20, minimum width=2cm] (ram) at (8, 0) {RAM};

    \draw[->, thick, dashed, red] (l1.south) -- ++(0,-0.3) node[below, font=\scriptsize] {Miss!};
    \draw[->, thick] (l1) -- (l2);
    \draw[->, thick, dashed, red] (l2.south) -- ++(0,-0.3) node[below, font=\scriptsize] {Miss!};
    \draw[->, thick] (l2) -- (l3);
    \draw[->, thick, dashed, red] (l3.south) -- ++(0,-0.3) node[below, font=\scriptsize] {Miss!};
    \draw[->, thick] (l3) -- (ram);
    \draw[->, thick, green!50!black] (ram.south) -- ++(0,-0.5) node[below, font=\scriptsize] {Found! (100ns)};
\end{tikzpicture}
\end{document}
```

## Cache Lines: The Unit of Transfer

```tikz
\begin{document}
\begin{tikzpicture}[
    cell/.style={draw, minimum width=1cm, minimum height=0.7cm, font=\scriptsize}]

    \node[above, font=\small] at (4, 1.5) {You request array[0] (8 bytes), memory loads 64 bytes:};

    \node[cell, fill=green!40] at (0, 0) {[0]};
    \node[cell, fill=blue!15] at (1.1, 0) {[1]};
    \node[cell, fill=blue!15] at (2.2, 0) {[2]};
    \node[cell, fill=blue!15] at (3.3, 0) {[3]};
    \node[cell, fill=blue!15] at (4.4, 0) {[4]};
    \node[cell, fill=blue!15] at (5.5, 0) {[5]};
    \node[cell, fill=blue!15] at (6.6, 0) {[6]};
    \node[cell, fill=blue!15] at (7.7, 0) {[7]};

    \node[below, font=\scriptsize] at (0, -0.5) {requested};
    \node[below, font=\scriptsize, green!50!black] at (4.4, -0.5) {FREE! (already in cache)};

    % Brace drawn manually (decorations.pathreplacing may not work in TikZJax)
    \draw[thick] (-0.5, -0.9) -- (-0.5, -1.1) -- (8.3, -1.1) -- (8.3, -0.9);
    \draw[thick] (3.9, -1.1) -- (3.9, -1.3);
    \node[below, font=\scriptsize] at (3.9, -1.3) {64-byte cache line};
\end{tikzpicture}
\end{document}
```

If you access `array[1]` next: **already in cache!**

**Spatial locality:** Sequential access is "free".

## Why Locality Matters

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape,
    cell/.style={draw, minimum width=0.45cm, minimum height=0.45cm, font=\tiny}]

    % Memory layout: 24 elements in 3 cache lines
    \node[above, font=\small] at (6.5, 2.8) {Memory: 24 elements across 3 cache lines};

    % Cache line 0 (elements 0-7)
    \node[cell, fill=blue!15] (c0) at (0, 2) {0};
    \node[cell, fill=blue!15] at (0.55, 2) {1};
    \node[cell, fill=blue!15] at (1.1, 2) {2};
    \node[cell, fill=blue!15] at (1.65, 2) {3};
    \node[cell, fill=blue!15] at (2.2, 2) {4};
    \node[cell, fill=blue!15] at (2.75, 2) {5};
    \node[cell, fill=blue!15] at (3.3, 2) {6};
    \node[cell, fill=blue!15] at (3.85, 2) {7};

    % Cache line 1 (elements 8-15)
    \node[cell, fill=orange!15] (c8) at (4.7, 2) {8};
    \node[cell, fill=orange!15] at (5.25, 2) {9};
    \node[cell, fill=orange!15] at (5.8, 2) {10};
    \node[cell, fill=orange!15] at (6.35, 2) {11};
    \node[cell, fill=orange!15] at (6.9, 2) {12};
    \node[cell, fill=orange!15] at (7.45, 2) {13};
    \node[cell, fill=orange!15] at (8.0, 2) {14};
    \node[cell, fill=orange!15] at (8.55, 2) {15};

    % Cache line 2 (elements 16-23)
    \node[cell, fill=green!15] (c16) at (9.4, 2) {16};
    \node[cell, fill=green!15] at (9.95, 2) {17};
    \node[cell, fill=green!15] at (10.5, 2) {18};
    \node[cell, fill=green!15] at (11.05, 2) {19};
    \node[cell, fill=green!15] at (11.6, 2) {20};
    \node[cell, fill=green!15] at (12.15, 2) {21};
    \node[cell, fill=green!15] at (12.7, 2) {22};
    \node[cell, fill=green!15] at (13.25, 2) {23};

    % Labels for cache lines
    \node[below, font=\tiny] at (1.9, 1.6) {cache line 0};
    \node[below, font=\tiny] at (6.6, 1.6) {cache line 1};
    \node[below, font=\tiny] at (11.3, 1.6) {cache line 2};

    % Sequential access arrows
    \node[left, font=\small\bfseries, green!50!black] at (-0.5, 0.6) {Sequential:};
    \draw[->, green!50!black, thick] (0, 0.6) -- (0, 1.75);
    \draw[->, green!50!black, thick] (0.55, 0.6) -- (0.55, 1.75);
    \draw[->, green!50!black, thick] (1.1, 0.6) -- (1.1, 1.75);
    \draw[->, green!50!black, thick] (1.65, 0.6) -- (1.65, 1.75);
    \draw[->, green!50!black, thick] (2.2, 0.6) -- (2.2, 1.75);
    \draw[->, green!50!black, thick] (2.75, 0.6) -- (2.75, 1.75);
    \draw[->, green!50!black, thick] (3.3, 0.6) -- (3.3, 1.75);
    \draw[->, green!50!black, thick] (3.85, 0.6) -- (3.85, 1.75);
    \node[right, font=\scriptsize, green!50!black] at (3.5, 0.4) {8 accesses, 1 cache miss};

    % Stride access arrows
    \node[left, font=\small\bfseries, red] at (-0.5, -0.3) {Stride-8:};
    \draw[->, red, thick] (0, -0.3) -- (0, 1.75);
    \draw[->, red, thick] (4.7, -0.3) -- (4.7, 1.75);
    \draw[->, red, thick] (9.4, -0.3) -- (9.4, 1.75);
    \node[right, font=\scriptsize, red] at (9, -0.4) {3 accesses, 3 cache misses};

\end{tikzpicture}
\end{document}
```

**Sequential:** All 8 accesses hit the same cache line → 1 miss total

**Stride-8:** Each access hits a different cache line → 1 miss per access (8× worse!)

## Two Types of Locality

**Temporal Locality:**

- Data accessed recently will likely be accessed again
- Example: loop counter, frequently-used variables
- Keep hot data in cache

**Spatial Locality:**

- Data near recently accessed data will likely be accessed
- Example: array traversal, struct fields
- Access data sequentially when possible

**Data systems design is all about locality.** Databases, ML systems, and every high-performance system we study in this course optimizes for these two properties. We will see this pattern again and again.

## What Can Go Wrong?

Let's see locality in action with a simple experiment.

```cpp
int matrix[1000][1000];
long sum = 0;

for (int i = 0; i < 1000; i++)
    for (int j = 0; j < 1000; j++)
        sum += matrix[i][j];
```

```bash
$ cd examples && make matrix_row && ./matrix_row
Sum: ..., Time: 2 ms
```

## Try this one

```cpp
int matrix[1000][1000];
long sum = 0;

for (int j = 0; j < 1000; j++)
    for (int i = 0; i < 1000; i++)
        sum += matrix[i][j];
```

```bash
$ cd examples && make matrix_col && ./matrix_col
Sum: ..., Time: 15 ms
```

## Why 7× Slower?

:::::: {.columns}
::: {.column width="50%"}
**Row-major traversal**
```cpp
for (i = 0; i < 1000; i++)
  for (j = 0; j < 1000; j++)
    sum += matrix[i][j];
```

\vspace{1em}

Access: `[0][0]`, `[0][1]`, `[0][2]`, ...

Linearized: **0, 1, 2, 3, 4, ...**

Stride = 4 bytes
:::
::: {.column width="50%"}
**Column-major traversal**
```cpp
for (j = 0; j < 1000; j++)
  for (i = 0; i < 1000; i++)
    sum += matrix[i][j];
```

\vspace{1em}

Access: `[0][0]`, `[1][0]`, `[2][0]`, ...

Linearized: **0, 1000, 2000, 3000, ...**

Stride = 4000 bytes
:::
::::::

## Why 7× Slower? (Cache View)

:::::: {.columns}
::: {.column width="50%"}
**Row-major: stride = 4 bytes**

Cache line = 64 bytes = 16 ints

Access 0, 1, 2, ..., 15 → **1 miss, 15 hits**

Then 16, 17, ..., 31 → **1 miss, 15 hits**

Miss rate: **6.25%**
:::
::: {.column width="50%"}
**Column-major: stride = 4000 bytes**

Each access jumps 4000 bytes (62 cache lines!)

Access 0 → miss, load line

Access 1000 → miss, load line

Access 2000 → miss, load line

Miss rate: **100%**
:::
::::::

## The Result: 7× Slower

|Traversal|Time|Cache behavior|
|---|---|---|
|Row-major|~2 ms|1 miss per 16 accesses|
|Column-major|~15 ms|1 miss per access|

**Same computation. Same data. 7× performance difference.**

# Measuring Cache Performance

## Performance Counters (PMU)

Real-world applications are far more complex than our matrix example. Reasoning alone isn't enough—we need to **benchmark, profile, and measure** to understand where time is spent.

Modern CPUs have **Performance Monitoring Units** that count hardware events:

- Cache hits/misses at each level (L1, L2, L3)
- Branch predictions/mispredictions
- Instructions retired, cycles, etc.

On Linux, use `perf` to read these counters:

```bash
$ cd examples && make perf      # Run all with perf profiling
```

## Measuring Row-Major Access

```bash
$ cd examples
$ perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_row
```

```
 1,000,000,000      L1-dcache-loads
    62,500,000      L1-dcache-load-misses  # 6.25% miss rate
```

**Matches our prediction!** 1M accesses, 1 miss per 16 accesses = 62.5K misses per iteration.

(1000 iterations × 62,500 = 62.5M misses)

## Measuring Column-Major Access

```bash
$ cd examples
$ perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_col
```

```
 1,000,000,000      L1-dcache-loads
 1,000,000,000      L1-dcache-load-misses  # 100% miss rate
```

**Every single access misses L1 cache!**

The data we need is never in cache because we jump 4000 bytes each time.

## Cache Miss Summary

|Traversal|L1 Loads|L1 Misses|Miss Rate|
|---|---|---|---|
|Row-major|1B|62.5M|6.25%|
|Column-major|1B|1B|100%|

**16× more L1 cache misses → 7× slower**

(Not 16× slower because L2/L3 partially hide the latency)

**Takeaway:** `perf` lets you verify your mental model. When optimizing, measure first!

```bash
$ cd examples && make perf   # Try it yourself!
```

# Hazards: When Things Go Wrong

## Pipeline Hazards

The pipeline assumes instructions flow smoothly. Three things break this:

1. **Data hazards:** Instruction needs result from previous instruction
2. **Control hazards:** Branch—which instruction comes next?
3. **Structural hazards:** Two instructions need same hardware

. . .

**All of these are fundamentally about waiting for data.**

## Data Hazard Example {.fragile}

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape,
    stage/.style={draw, minimum width=0.7cm, minimum height=0.5cm, font=\tiny},
    stall/.style={draw, minimum width=0.7cm, minimum height=0.5cm, font=\tiny, fill=red!30}]

    % Time labels (unrolled from \foreach)
    \node[above, font=\tiny] at (1, 2) {1};
    \node[above, font=\tiny] at (2, 2) {2};
    \node[above, font=\tiny] at (3, 2) {3};
    \node[above, font=\tiny] at (4, 2) {4};
    \node[above, font=\tiny] at (5, 2) {5};
    \node[above, font=\tiny] at (6, 2) {6};
    \node[above, font=\tiny] at (7, 2) {7};
    \node[above, font=\tiny] at (8, 2) {8};
    \node[font=\tiny] at (9.5, 2) {...};
    \node[above, font=\tiny] at (10, 2) {102};
    \node[above, font=\tiny] at (11, 2) {103};
    \node[above, font=\tiny] at (12, 2) {104};

    % Load instruction
    \node[left, font=\scriptsize\ttfamily] at (0, 1) {load rax,[addr]};
    \node[stage, fill=blue!20] at (1, 1) {F};
    \node[stage, fill=green!20] at (2, 1) {D};
    \node[stage, fill=orange!20] at (3, 1) {E};
    \node[stage, fill=orange!20] at (4, 1) {E};
    \node[font=\tiny] at (6, 1) {...};
    \node[stage, fill=orange!20] at (10, 1) {E};
    \node[stage, fill=purple!20] at (11, 1) {M};
    \node[stage, fill=red!20] at (12, 1) {R};

    % Add instruction
    \node[left, font=\scriptsize\ttfamily] at (0, 0) {add rbx,rax,1};
    \node[stage, fill=blue!20] at (2, 0) {F};
    \node[stage, fill=green!20] at (3, 0) {D};
    \node[stall] at (4, 0) {S};
    \node[stall] at (5, 0) {S};
    \node[font=\tiny] at (6, 0) {...};
    \node[stage, fill=orange!20] at (11, 0) {E};
    \node[stage, fill=red!20] at (12, 0) {R};

    % Brace drawn manually
    \draw[thick] (4, -0.5) -- (4, -0.7) -- (9, -0.7) -- (9, -0.5);
    \draw[thick] (6.5, -0.7) -- (6.5, -0.9);
    \node[below, font=\scriptsize] at (6.5, -0.9) {Waiting ~100 cycles!};
\end{tikzpicture}
\end{document}
```

The `add` is **starved**—cannot execute until data arrives.

## Control Hazard: Branches {.fragile}

```tikz
\begin{document}
\begin{tikzpicture}[
    box/.style={draw, rectangle, minimum width=2cm, minimum height=0.8cm}]

    \node[box] (cond) at (0, 2) {\ttfamily if (x > 0)};
    \node[box, fill=green!20] (then) at (-2.5, 0) {then block};
    \node[box, fill=blue!20] (else) at (2.5, 0) {else block};

    \draw[->, thick] (cond.south) -- ++(-1, 0) -- (then.north);
    \draw[->, thick] (cond.south) -- ++(1, 0) -- (else.north);

    \node[font=\large, red] at (0, -1.5) {Which path to fetch???};
    \node[font=\small] at (0, -2.2) {Pipeline must guess \textbf{before} condition is known!};
\end{tikzpicture}
\end{document}
```

## Branch Misprediction Cost

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape,
    stage/.style={draw, minimum width=0.6cm, minimum height=0.4cm, font=\tiny},
    flush/.style={draw, minimum width=0.6cm, minimum height=0.4cm, font=\tiny, fill=red!40}]

    % Correct prediction
    \node[left, font=\small\bfseries, green!50!black] at (-1, 2) {Correct:};
    \node[stage, fill=blue!20] at (0, 2) {F};
    \node[stage, fill=green!20] at (0.7, 2) {D};
    \node[stage, fill=orange!20] at (1.4, 2) {E};

    \node[stage, fill=blue!20] at (0.7, 1.4) {F};
    \node[stage, fill=green!20] at (1.4, 1.4) {D};
    \node[stage, fill=orange!20] at (2.1, 1.4) {E};

    \node[stage, fill=blue!20] at (1.4, 0.8) {F};
    \node[stage, fill=green!20] at (2.1, 0.8) {D};
    \node[stage, fill=orange!20] at (2.8, 0.8) {E};

    % Wrong prediction
    \node[left, font=\small\bfseries, red] at (5, 2) {Wrong:};
    \node[stage, fill=blue!20] at (6, 2) {F};
    \node[stage, fill=green!20] at (6.7, 2) {D};
    \node[stage, fill=orange!20] at (7.4, 2) {E};

    \node[flush] at (6.7, 1.4) {F};
    \node[flush] at (7.4, 1.4) {X};
    \node[right, font=\tiny, red] at (8, 1.4) {Flush!};

    \node[stage, fill=blue!20] at (8.4, 0.8) {F};
    \node[stage, fill=green!20] at (9.1, 0.8) {D};
    \node[stage, fill=orange!20] at (9.8, 0.8) {E};
    \node[right, font=\tiny] at (10.2, 0.8) {Restart};

    \draw[<->, thick, red] (7.8, 1.4) -- (8.2, 0.8) node[midway, right, font=\scriptsize] {10-20 cyc};
\end{tikzpicture}
\end{document}
```

Modern predictors are >95% accurate, **but** data-dependent branches can be unpredictable.

## Which is Fastest? (Random Data)

\small

:::::: {.columns}
::: {.column width="33%"}
**Version A**
```cpp
for (i = 0; i < n; i++)
  if (data[i] > t)
    count++;
```
:::
::: {.column width="33%"}
**Version B**
```cpp
for (i = 0; i < n; i++)
  count += (data[i] > t);
```
:::
::: {.column width="33%"}
**Version C**
```cpp
sort(data, data + n);
for (i = 0; i < n; i++)
  if (data[i] > t)
    count++;
```
:::
::::::

\normalsize

```bash
$ cd examples && make branch_test && ./branch_test
```

|Version|Time|
|---|---|
|A (branching)|56 ms|
|B (branchless)|8 ms|
|C (sorted)|12 ms|

Branchless is **7× faster** than branching on random data!

## Data-Dependent Branches: The Problem

:::::: {.columns}
::: {.column width="50%"}
**C++ code**
```cpp
for (int i = 0; i < n; i++) {
    if (data[i] > threshold)
        count++;
}
```

On random data: branch taken ~50% of the time, **unpredictably**.

Branch predictor accuracy: ~50%

~15-20 cycle penalty per mispredict!
:::
::: {.column width="50%"}
**Assembly (x86-64)**
```asm
loop:
    mov  eax, [rdi + rcx*4]
    cmp  eax, esi
    jle  skip        ; BRANCH!
    inc  edx
skip:
    inc  rcx
    cmp  rcx, r8
    jl   loop
```

The `jle skip` is the problem—CPU must guess which way to go.
:::
::::::

## Solution 1: Branchless Code

:::::: {.columns}
::: {.column width="50%"}
**C++ code**
```cpp
for (int i = 0; i < n; i++) {
    count += (data[i] > threshold);
}
```

The comparison `(data[i] > threshold)` evaluates to 0 or 1.

No branch = no misprediction penalty!
:::
::: {.column width="50%"}
**Assembly (x86-64)**
```asm
loop:
    mov  eax, [rdi + rcx*4]
    cmp  eax, esi
    setg al          ; al = 1 if greater
    movzx eax, al
    add  edx, eax    ; No branch!
    inc  rcx
    cmp  rcx, r8
    jl   loop
```

`setg` + `add` replaces the conditional branch.
:::
::::::

## Solution 2: Sort First

:::::: {.columns}
::: {.column width="50%"}
**C++ code**
```cpp
sort(data, data + n);

for (int i = 0; i < n; i++) {
    if (data[i] > threshold)
        count++;
}
```

After sorting: all values below threshold come first, then all above.

Branch pattern: `FFFFF...TTTTT`

Predictor accuracy: **>99%**
:::
::: {.column width="50%"}
**Why it works**

Sorted data:
```
[1, 3, 5, 8, 12, 15, 20, ...]
         ^ threshold = 10
```

First iterations: always skip (predict: skip)

Later iterations: always take (predict: take)

Only ~1 misprediction at the transition!
:::
::::::

## Branch Performance Comparison

```bash
$ cd examples && perf stat -e branches,branch-misses ./branch_test
```

|Approach|Time (random data)|Why|
|---|---|---|
|Branching|~15 ms|50% misprediction rate|
|Branchless|~3 ms|No branches to mispredict|
|Sort + Branch|~4 ms|Predictable pattern|

**Note:** Sorting has O(n log n) cost, only worth it if you traverse multiple times or need sorted data anyway.

## The Fundamental Problem

> **Every hazard is a data movement problem**
>
> - **Data hazard:** Waiting for data from memory
> - **Control hazard:** Waiting for branch condition (data!)
> - **Structural hazard:** Waiting for hardware to move data

The CPU can execute billions of operations per second...

...but only if the data is **in the right place at the right time**.

## Strategies to Minimize Data Movement

**1. Improve locality**

- Sequential access patterns
- Keep working set small (fit in cache)
- Co-locate related data

**2. Hide latency**

- Prefetching (hardware and software)
- Out-of-order execution (do other work while waiting)
- Parallelism (multiple outstanding requests)

**3. Reduce data volume**

- Compression
- Filtering early (don't move data you'll discard)

# Summary

## Key Takeaways

1. **Instructions** flow through: Fetch → Decode → Execute → Retire
   - Pipelining and superscalar execution enable high throughput
   - But only if data is ready!

2. **Data** flows through: Disk → RAM → L3 → L2 → L1 → Registers
   - Each level faster but smaller
   - Cache lines (64 bytes) are the unit of transfer

3. **Hazards** occur when data isn't where it needs to be
   - Data hazards, control hazards—all about waiting for data

4. **Optimization is fundamentally about data movement**

