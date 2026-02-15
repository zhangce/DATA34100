---
title: LLM Inference System Characteristics
subtitle:
author:
theme: metropolis
aspectratio: 169
monofont: Menlo
header-includes:
  - \usepackage{tikz}
  - \usetikzlibrary{shapes.geometric, arrows.meta, positioning, fit, calc, decorations.pathreplacing}
---

# LLM Inference: Overview

## What is LLM Inference?

Given a prompt, an LLM predicts the **next token**:

```
"The capital of France is"
    → Tokenizer → [token IDs]
    → Embedding Lookup → [vectors]
    → Transformer Layer 1 → ... → Layer 32
    → LM Head → [logits over vocabulary]
    → Sample → "Paris"
```

. . .

**Key insight:** LLMs generate text **one token at a time** (autoregressive).

## Autoregressive Generation

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.85, transform shape,
    box/.style={draw, rectangle, rounded corners, minimum width=3cm, minimum height=0.6cm, fill=blue!15}]

    \node[box] (s1) at (0, 3) {"The capital of France is"};
    \node[right] at (3.5, 3) {$\rightarrow$ "Paris"};

    \node[box] (s2) at (0, 2) {"The capital of France is Paris"};
    \node[right] at (3.5, 2) {$\rightarrow$ "is"};

    \node[box] (s3) at (0, 1) {"The capital of France is Paris is"};
    \node[right] at (3.5, 1) {$\rightarrow$ "a"};

    \node[box] (s4) at (0, 0) {"... is Paris is a"};
    \node[right] at (3.5, 0) {$\rightarrow$ "beautiful"};

    \draw[->, thick, red] (5, 3) -- (5, 0.3);
    \node[right, font=\small, red] at (5.2, 1.5) {Sequential!};
\end{tikzpicture}
\end{document}
```

Each generated token becomes input for the next. We cannot generate token $n+1$ until we've generated token $n$.

## Running Example: Llama-3.1-8B

| Parameter              | Value   | Description                                     |
| ---------------------- | ------- | ----------------------------------------------- |
| Layers                 | 32      | Number of transformer layers                    |
| Hidden dimension ($d$) | 4,096   | Size of token representations                   |
| Query heads            | 32      | Number of attention heads for queries           |
| KV heads               | 8       | Number of KV heads (GQA)                        |
| Head dimension         | 128     | Dimension per attention head                    |
| FFN dimension          | 14,336  | Intermediate size in feed-forward network       |
| Vocabulary size        | 128,256 | Number of possible tokens                       |

Total: ~8 billion parameters

# Model Architecture

## High-Level Dataflow

```tikz
\begin{document}
\begin{tikzpicture}[
    block/.style={draw, rectangle, rounded corners, minimum width=2.5cm, minimum height=0.7cm, fill=blue!20},
    arrow/.style={->, thick, >=stealth}]

    \node[block] (embed) at (0, 0) {Embedding};
    \node[block, fill=orange!20] (t1) at (3, 0) {Layer 1};
    \node at (5.5, 0) {...};
    \node[block, fill=orange!20] (t32) at (8, 0) {Layer 32};
    \node[block, fill=green!20] (lm) at (11.5, 0) {LM Head};

    \draw[arrow] (-2, 0) node[left] {token ID} -- (embed);
    \draw[arrow] (embed) -- (t1);
    \draw[arrow] (t1) -- (4.5, 0);
    \draw[arrow] (6.5, 0) -- (t32);
    \draw[arrow] (t32) -- (lm);
    \draw[arrow] (lm) -- (13.5, 0) node[right] {logits};

    \node[below, font=\scriptsize] at (0, -0.5) {$1 \rightarrow 4096$};
    \node[below, font=\scriptsize] at (5.5, -0.5) {$4096 \rightarrow 4096$};
    \node[below, font=\scriptsize] at (11.5, -0.5) {$4096 \rightarrow 128K$};
\end{tikzpicture}
\end{document}
```

- **Embedding:** Token ID $\rightarrow$ 4,096-dim vector (pure memory lookup)
- **32 Transformer Layers:** Attention + FFN
- **LM Head:** Project to vocabulary size, sample next token

## Transformer Layer Structure

```tikz
\begin{document}
\begin{tikzpicture}[
    block/.style={draw, rectangle, minimum width=2cm, minimum height=0.6cm},
    arrow/.style={->, thick}]

    % Input
    \node (in) at (0, 0) {$\mathbf{x}$};

    % Attention branch
    \node[block, fill=yellow!20] (norm1) at (2.5, 0) {RMSNorm};
    \node[block, fill=blue!20] (attn) at (5.5, 0) {Attention};
    \node[circle, draw] (add1) at (8, 0) {+};

    % FFN branch
    \node[block, fill=yellow!20] (norm2) at (10.5, 0) {RMSNorm};
    \node[block, fill=green!20] (ffn) at (13, 0) {FFN};
    \node[circle, draw] (add2) at (15.5, 0) {+};

    % Output
    \node (out) at (17.5, 0) {$\mathbf{x}'$};

    % Arrows
    \draw[arrow] (in) -- (norm1);
    \draw[arrow] (norm1) -- (attn);
    \draw[arrow] (attn) -- (add1);
    \draw[arrow] (add1) -- (norm2);
    \draw[arrow] (norm2) -- (ffn);
    \draw[arrow] (ffn) -- (add2);
    \draw[arrow] (add2) -- (out);

    % Residual connections
    \draw[arrow, dashed, gray] (in) -- ++(0, -0.8) -| (add1);
    \draw[arrow, dashed, gray] (add1) -- ++(0, -0.8) -| (add2);

\end{tikzpicture}
\end{document}
```

**Two blocks per layer:**

1. **Attention block:** RMSNorm $\rightarrow$ Self-Attention $\rightarrow$ Residual add
2. **FFN block:** RMSNorm $\rightarrow$ SwiGLU FFN $\rightarrow$ Residual add

## Attention Mechanism

**Self-attention** allows each token to gather information from all previous tokens:

1. **Projections:** $Q = xW_Q$, $K = xW_K$, $V = xW_V$
2. **Scores:** $\text{softmax}(QK^\top / \sqrt{d_h})$
3. **Output:** Weighted sum of values, then $W_O$ projection

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.7, transform shape]
    \node[draw, fill=blue!10, minimum width=1cm, minimum height=0.5cm] (q) at (0, 2) {Q};
    \node[draw, fill=green!10, minimum width=1cm, minimum height=0.5cm] (k) at (2, 2) {K};
    \node[draw, fill=orange!10, minimum width=1cm, minimum height=0.5cm] (v) at (4, 2) {V};

    \node[draw, circle, fill=yellow!20] (dot) at (1, 0.5) {$\cdot$};
    \node[draw, rectangle, fill=red!10] (soft) at (1, -0.8) {softmax};
    \node[draw, circle, fill=yellow!20] (mul) at (3, -0.8) {$\times$};

    \draw[->, thick] (q) -- (dot);
    \draw[->, thick] (k) -- (dot);
    \draw[->, thick] (dot) -- (soft);
    \draw[->, thick] (soft) -- (mul);
    \draw[->, thick] (v) -- ++(0, -2.3) -- (mul);
    \draw[->, thick] (mul) -- ++(1.5, 0) node[right] {output};
\end{tikzpicture}
\end{document}
```

## Multi-Head Attention Variants

**The KV cache problem:** During generation, we cache K and V to avoid recomputation.

| Variant | Q Heads | KV Heads | KV Cache Size | Quality |
|---------|---------|----------|---------------|---------|
| MHA     | 32      | 32       | 1× (baseline) | Best    |
| MQA     | 32      | 1        | 1/32×         | Lower   |
| **GQA** | 32      | 8        | 1/4×          | Good    |

**Llama-3.1-8B uses GQA:** Each of 8 KV heads is shared by 4 query heads.

$\Rightarrow$ 4× less KV cache memory, ~same quality

## Feed-Forward Network (SwiGLU)

:::::: {.columns}
::: {.column width="50%"}
$$\text{FFN}(x) = W_{\text{down}} \cdot (\text{SiLU}(W_{\text{gate}} \cdot x) \odot W_{\text{up}} \cdot x)$$

- $W_{\text{gate}}, W_{\text{up}}$: $4096 \rightarrow 14336$
- $W_{\text{down}}$: $14336 \rightarrow 4096$
- SiLU: $z \cdot \sigma(z)$
- $\odot$: element-wise multiply (gating)

:::
::: {.column width="50%"}
```tikz
\begin{document}
\begin{tikzpicture}[scale=0.65, transform shape,
    block/.style={draw, rectangle, minimum width=1.5cm, minimum height=0.5cm, font=\small}]

    \node (x) at (0, 3) {$x$};

    \node[block, fill=blue!15] (gate) at (-1.5, 1.5) {$W_{\text{gate}}$};
    \node[block, fill=blue!15] (up) at (1.5, 1.5) {$W_{\text{up}}$};

    \node[block, fill=orange!15] (silu) at (-1.5, 0) {SiLU};

    \node[draw, circle] (mul) at (0, -1) {$\odot$};

    \node[block, fill=green!15] (down) at (0, -2.5) {$W_{\text{down}}$};

    \draw[->, thick] (x) -- ++(-1.5, 0) -- (gate);
    \draw[->, thick] (x) -- ++(1.5, 0) -- (up);
    \draw[->, thick] (gate) -- (silu);
    \draw[->, thick] (silu) -- (mul);
    \draw[->, thick] (up) -- ++(0, -2) -- (mul);
    \draw[->, thick] (mul) -- (down);
    \draw[->, thick] (down) -- ++(0, -0.8) node[below] {output};
\end{tikzpicture}
\end{document}
```
:::
::::::

## Local vs. Global Operations

**Key insight:** Attention is the *only* operation where tokens interact!

| Operation | Scope | Description |
|-----------|-------|-------------|
| Embedding | Per-token | Each token ID maps to its own vector |
| RMSNorm | Per-token | Normalizes each token independently |
| Q, K, V projections | Per-token | Matrix-vector multiply per token |
| FFN (SwiGLU) | Per-token | Same transformation for each token |
| **Attention** | **Global** | **Each token attends to all previous** |

. . .

Token-independent ops are embarrassingly parallel. Attention is what makes LLMs understand context.

# KV Cache, Prefill, and Decode

## The Redundancy Problem

When generating token $t+1$, naive approach processes entire sequence:

$$[x_1, x_2, \ldots, x_t, x_{t+1}]$$

. . .

**But we already computed** $K_1, K_2, \ldots, K_t$ and $V_1, V_2, \ldots, V_t$ in previous steps!

. . .

**Solution:** Cache keys and values, reuse them.

## KV Cache

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape,
    cache/.style={draw, rectangle, minimum width=0.6cm, minimum height=0.5cm, fill=green!20},
    new/.style={draw, rectangle, minimum width=0.6cm, minimum height=0.5cm, fill=orange!40}]

    \node[font=\small\bfseries] at (-2, 2) {Step $t$:};
    \node[cache] at (0, 2) {};
    \node[cache] at (0.7, 2) {};
    \node[cache] at (1.4, 2) {};
    \node at (2.1, 2) {...};
    \node[cache] at (2.8, 2) {$K_t$};
    \node[font=\scriptsize] at (4.5, 2) {Cache K,V for positions 1..t};

    \node[font=\small\bfseries] at (-2, 0.8) {Step $t+1$:};
    \node[cache] at (0, 0.8) {};
    \node[cache] at (0.7, 0.8) {};
    \node[cache] at (1.4, 0.8) {};
    \node at (2.1, 0.8) {...};
    \node[cache] at (2.8, 0.8) {$K_t$};
    \node[new] at (3.5, 0.8) {$K_{t+1}$};
    \node[font=\scriptsize] at (5.5, 0.8) {Only compute new K,V};

    \draw[->, thick, red] (3.5, 0.3) -- (3.5, -0.3) node[below, font=\scriptsize] {Attend to all};

\end{tikzpicture}
\end{document}
```

**KV cache** trades compute for memory:

- Store $K$ and $V$ for all previous tokens
- Each new token: compute only its $K$, $V$, append to cache
- Attend using the full cache

## Two Phases of Inference

:::::: {.columns}
::: {.column width="50%"}
**Prefill (prompt processing)**

- Process $p$ prompt tokens in parallel
- Compute and cache all K, V
- Apply **causal mask** (token $i$ only sees $1..i$)
- **Compute-bound** (matrix-matrix ops)

:::
::: {.column width="50%"}
**Decode (token generation)**

- Process 1 token at a time
- Compute new K, V; append to cache
- Attend to entire cache
- **Memory-bound** (matrix-vector ops)

:::
::::::

| Phase   | Tokens/step | Bottleneck       | Arithmetic Intensity |
| ------- | ----------- | ---------------- | -------------------- |
| Prefill | $p$         | Compute          | High                 |
| Decode  | 1           | Memory bandwidth | Low (~1)             |

## KV Cache Memory Requirements

For Llama-3.1-8B with GQA:

$$\text{KV cache per token} = 2 \times n_{\text{layers}} \times n_{\text{kv\_heads}} \times d_h \times \text{bytes}$$
$$= 2 \times 32 \times 8 \times 128 \times 2 = 128 \text{ KB (FP16)}$$

. . .

| Context Length | KV Cache Size |
|----------------|---------------|
| 4K tokens      | 0.5 GB        |
| 8K tokens      | 1 GB          |
| 32K tokens     | 4 GB          |
| 128K tokens    | **16 GB**     |

At 128K context, KV cache equals model weight size!

## Causal Masking in Prefill

During prefill, we process all tokens in parallel but enforce causality:

$$\text{MaskedAttn}(Q, K, V) = \text{softmax}\left(\frac{QK^\top}{\sqrt{d_h}} + M\right) V$$

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.6, transform shape]
    \node[above] at (2, 4.5) {Causal Mask $M$};

    % Matrix
    \foreach \i in {0,...,4} {
        \foreach \j in {0,...,4} {
            \pgfmathparse{\j > \i ? 1 : 0}
            \ifnum\pgfmathresult=1
                \node[draw, fill=red!30, minimum size=0.7cm] at (\j, 4-\i) {$-\infty$};
            \else
                \node[draw, fill=green!20, minimum size=0.7cm] at (\j, 4-\i) {0};
            \fi
        }
    }

    % Labels
    \node[left] at (-0.5, 4) {$t_1$};
    \node[left] at (-0.5, 3) {$t_2$};
    \node[left] at (-0.5, 2) {$t_3$};
    \node[left] at (-0.5, 1) {$t_4$};
    \node[left] at (-0.5, 0) {$t_5$};

    \node[below] at (0, -0.5) {$t_1$};
    \node[below] at (1, -0.5) {$t_2$};
    \node[below] at (2, -0.5) {$t_3$};
    \node[below] at (3, -0.5) {$t_4$};
    \node[below] at (4, -0.5) {$t_5$};

    \node[right, font=\small] at (5.5, 2) {After softmax: $-\infty \rightarrow 0$};
    \node[right, font=\small] at (5.5, 1) {Token $i$ only attends to $1..i$};
\end{tikzpicture}
\end{document}
```

# Arithmetic Intensity Analysis

## Recap: Arithmetic Intensity

$$\text{AI} = \frac{\text{FLOPs}}{\text{Bytes transferred}}$$

**A100 GPU:** Peak = 312 TFLOPS / 2 TB/s = **156 FLOP/byte**

. . .

- AI > 156: **Compute-bound** (GPU busy computing)
- AI < 156: **Memory-bound** (GPU waiting for data)

## Linear Layers: Prefill vs Decode

For weight matrix $W \in \mathbb{R}^{d_{in} \times d_{out}}$:

| Phase | Input Shape | FLOPs | Bytes (weights) | AI |
|-------|-------------|-------|-----------------|-----|
| Prefill | $(p, d_{in})$ | $2 \cdot p \cdot d_{in} \cdot d_{out}$ | $2 \cdot d_{in} \cdot d_{out}$ | $\approx p$ |
| Decode | $(1, d_{in})$ | $2 \cdot d_{in} \cdot d_{out}$ | $2 \cdot d_{in} \cdot d_{out}$ | $\approx 1$ |

. . .

**Prefill:** Amortize weight loading across $p$ tokens $\Rightarrow$ AI $\approx p$

**Decode:** Each weight loaded once, used once $\Rightarrow$ AI $\approx 1$

## Decode is Memory-Bound

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.9, transform shape]
    % Axes
    \draw[->, thick] (0, 0) -- (8, 0) node[right] {Arithmetic Intensity};
    \draw[->, thick] (0, 0) -- (0, 5) node[above] {Performance};

    % Roofline
    \draw[thick, blue] (0, 0) -- (3, 4);
    \draw[thick, blue] (3, 4) -- (8, 4);

    % Ridge point
    \fill[red] (3, 4) circle (3pt);
    \node[above right, font=\small] at (3, 4) {Ridge (AI=156)};

    % Points
    \fill[orange] (0.3, 0.4) circle (4pt);
    \node[right, font=\small] at (0.5, 0.4) {Decode (AI$\approx$1)};

    \fill[green!60!black] (5, 4) circle (4pt);
    \node[right, font=\small] at (5.2, 4) {Prefill (AI$\approx p$)};

    % Labels
    \node[font=\scriptsize, blue] at (1, 2.5) {Memory-bound};
    \node[font=\scriptsize, blue] at (6, 4.5) {Compute-bound};

    % Dashed line for decode
    \draw[dashed, gray] (0.3, 0) -- (0.3, 0.4);

\end{tikzpicture}
\end{document}
```

**Decode runs at <1% GPU utilization** for linear layers!

## Attention Arithmetic Intensity

**Prefill attention** (processing $p$ tokens):

$$\text{AI}_{\text{prefill}} = \frac{p \cdot d}{d + d_{kv}} \approx 0.8p \quad \text{(for Llama-3.1-8B)}$$

. . .

**Decode attention** (1 token, $s$ cached tokens):

$$\text{AI}_{\text{decode}} = \frac{n_q}{n_{kv}} = \frac{32}{8} = 4 \quad \text{(GQA helps!)}$$

. . .

Compare to MHA: decode AI would be 1, not 4. **GQA improves decode efficiency.**

## Summary: Decode is the Bottleneck

| Operation     | Prefill AI    | Decode AI            |
| ------------- | ------------- | -------------------- |
| Linear layers | $\approx p$   | $\approx 1$          |
| Attention     | $\approx 0.8p$| $\approx n_q/n_{kv}$ |
| Normalization | $\approx 4$   | $\approx 4$          |

With prompt length $p = 1000$: prefill is **compute-bound**.

At decode: **everything is memory-bound**.

. . .

**The fundamental challenge:** Loading 16 GB of weights for *one* token's computation.

# Batching: The Key Optimization

## Why Batching Works

At batch size 1, each weight is loaded once, used once (AI $\approx$ 1).

**Batching:** Process $B$ sequences simultaneously, reuse weights $B$ times.

| Batch Size | FLOPs | Bytes | AI |
|------------|-------|-------|-----|
| 1 | $2 \cdot d_{in} \cdot d_{out}$ | $2 \cdot d_{in} \cdot d_{out}$ | 1 |
| $B$ | $B \cdot 2 \cdot d_{in} \cdot d_{out}$ | $2 \cdot d_{in} \cdot d_{out}$ | $B$ |

. . .

**Arithmetic intensity scales linearly with batch size!**

To become compute-bound on A100: need $B \approx 156$.

## Attention Doesn't Batch Well

For attention, each sequence has its own KV cache:

| Batch Size | FLOPs | Bytes (KV cache) | AI |
|------------|-------|------------------|-----|
| 1 | $4 \cdot s \cdot d$ | $4 \cdot s \cdot n_{kv} \cdot d_h$ | $n_q/n_{kv}$ |
| $B$ | $B \cdot 4 \cdot s \cdot d$ | $B \cdot 4 \cdot s \cdot n_{kv} \cdot d_h$ | $n_q/n_{kv}$ |

. . .

**Batching does NOT improve attention's AI** — both compute and memory scale with $B$.

Exception: **prefix sharing** (shared system prompts) does help!

## The Batching Trade-off

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.8, transform shape]
    % Axes
    \draw[->, thick] (0, 0) -- (8, 0) node[right] {Batch Size};
    \draw[->, thick] (0, 0) -- (0, 5) node[above, align=center] {Performance};

    % Throughput curve
    \draw[thick, blue] (0.5, 0.5) .. controls (2, 3) and (4, 4) .. (7.5, 4.5);
    \node[blue, right] at (7.5, 4.5) {Throughput};

    % Latency curve
    \draw[thick, red] (0.5, 1) .. controls (2, 1.1) and (4, 1.5) .. (7.5, 3.5);
    \node[red, right] at (7.5, 3.5) {Latency};

    % Sweet spot region
    \draw[dashed, green!60!black] (2, 0) -- (2, 5);
    \draw[dashed, green!60!black] (4, 0) -- (4, 5);
    \node[green!60!black, font=\small] at (3, 4.8) {Sweet spot};

    % Labels
    \node[below, font=\scriptsize] at (1, 0) {1};
    \node[below, font=\scriptsize] at (3, 0) {32};
    \node[below, font=\scriptsize] at (6, 0) {128};
\end{tikzpicture}
\end{document}
```

- **Throughput** (tokens/sec total): improves with batch size
- **Latency** (time per request): barely increases at small batches!

At small $B$: GPU is memory-bound anyway—extra work is "free".

## Memory Limits Batch Size

For Llama-3.1-8B at 4K context:

- Model weights: ~16 GB
- KV cache per sequence: ~0.5 GB
- On 80 GB GPU: room for ~128 concurrent sequences

. . .

**Practical batch sizes:** 32–64 sequences

**Key insight:** Batching is nearly universal in LLM serving because it improves throughput with minimal latency penalty.

## System Challenges from Batching

1. **Variable sequence lengths:** Requests have different prompt/output lengths
   - Solution: *Continuous batching* — dynamically add/remove requests

2. **Memory fragmentation:** KV caches grow unpredictably
   - Solution: *PagedAttention* — manage KV cache in fixed blocks

3. **Prefill-decode interference:** Different compute characteristics
   - Solution: *Chunked prefill* — interleave carefully

. . .

These techniques form the core of vLLM, TensorRT-LLM, and SGLang.

# Architectural Variants

## Mixture of Experts (MoE)

Replace dense FFN with $E$ expert FFNs; router selects top-$k$ per token:

$$\text{MoE}(x) = \sum_{i \in \text{TopK}(r(x))} g_i \cdot \text{FFN}_i(x)$$

| Model | Total Params | Active Params | Experts | Top-K |
|-------|--------------|---------------|---------|-------|
| Llama-3.1-8B | 8B | 8B | 1 (dense) | — |
| Mixtral 8x7B | 47B | 13B | 8 | 2 |
| DeepSeek-V2 | 236B | 21B | 160 | 6 |

**MoE decouples total parameters from active parameters.**

## MoE Trade-offs

:::::: {.columns}
::: {.column width="50%"}
**Benefits:**

- More parameters = better quality
- Same compute per token as smaller dense model
- 8× parameters, 2× compute (top-2)

:::
::: {.column width="50%"}
**Challenges:**

- All expert weights must fit in memory
- Load balancing: want even routing
- Communication in distributed settings

:::
::::::

## Sliding Window Attention

Standard attention: $O(n^2)$ — each token attends to all previous.

**Sliding window:** Limit attention to last $w$ tokens:

```tikz
\begin{document}
\begin{tikzpicture}[scale=0.55, transform shape]
    % Tokens
    \foreach \i in {1,...,8} {
        \node[draw, circle, minimum size=0.6cm] (t\i) at (\i, 0) {\i};
    }

    % Window for token 6
    \draw[thick, red, rounded corners] (3.4, -0.5) rectangle (6.6, 0.5);
    \node[red, font=\small] at (5, -1) {Window for token 6};

    % Arrows showing attention
    \draw[->, blue, thick] (6, 0.4) .. controls (5.5, 1) .. (4, 0.4);
    \draw[->, blue, thick] (6, 0.4) .. controls (5.7, 0.8) .. (5, 0.4);
    \draw[->, blue, thick] (6, 0.3) -- (6, 0.3);

    % Label
    \node[font=\small] at (11, 0) {$w = 4$: attend to last 4 tokens};
\end{tikzpicture}
\end{document}
```

- Mistral 7B: $w = 4096$
- Bounds KV cache and compute regardless of sequence length
- Information propagates through layer stacking

## Multi-head Latent Attention (MLA)

**MLA** (DeepSeek-V2) compresses K, V into low-dimensional latent:

$$c = W_{\text{compress}} \cdot x, \quad K = W_K \cdot c, \quad V = W_V \cdot c$$

Cache $c$ instead of $K$ and $V$.

| Method | KV Cache per Token |
|--------|-------------------|
| MHA | $2 \cdot n_h \cdot d_h$ |
| GQA | $2 \cdot n_{kv} \cdot d_h$ (4× less) |
| MLA | $d_c$ (~5× less) |

Trade-off: Extra compute to decompress during attention.

# Summary

## Key Takeaways

1. **LLMs generate tokens autoregressively** — sequential dependency limits parallelism

2. **Attention is the only global operation** — everything else is per-token

3. **Two phases:** Prefill (compute-bound, parallel) vs Decode (memory-bound, sequential)

4. **KV cache** trades compute for memory — critical for long contexts

5. **Decode is memory-bound** (AI ≈ 1) — loading weights dominates

6. **Batching** improves throughput by reusing weights across sequences

## The Fundamental Trade-off

```tikz
\begin{document}
\begin{tikzpicture}[
    box/.style={draw, rectangle, rounded corners, minimum width=3cm, minimum height=1cm, align=center}]

    \node[box, fill=blue!20] (compute) at (0, 0) {Compute\\(FLOPs)};
    \node[box, fill=green!20] (memory) at (6, 0) {Memory\\(GB, GB/s)};
    \node[box, fill=orange!20] (latency) at (3, -2.5) {Latency\\(ms/token)};

    \draw[<->, thick] (compute) -- (memory) node[midway, above] {Arithmetic Intensity};
    \draw[->, thick] (compute) -- (latency);
    \draw[->, thick] (memory) -- (latency);

    \node[font=\small, gray] at (3, 1) {LLM inference is dominated by memory bandwidth};
\end{tikzpicture}
\end{document}
```

**Data movement is the bottleneck** — just like CPUs, but at a different scale.
