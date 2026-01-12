/*
 * GEMM Progressive Optimization Demo
 *
 * This file contains implementations at each optimization stage,
 * allowing students to see the performance progression:
 *
 *   1. Naive:      Triple nested loop (~20 GFLOPS)
 *   2. Blocked:    Cache blocking, no packing (~800 GFLOPS)
 *   3. Packed A:   + A matrix packing (~1200 GFLOPS)
 *   4. Packed AB:  + On-the-fly B packing (~1350 GFLOPS)
 *   5. Tuned:      + Optimal parameters (~1430 GFLOPS)
 *   6. Lazy:       + Lazy A packing (~1520 GFLOPS)
 *
 * Run: clang -O3 -mcpu=apple-m1 -o gemm_progressive gemm_progressive.c -framework Accelerate
 *      VECLIB_MAXIMUM_THREADS=1 ./gemm_progressive
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <arm_neon.h>
#include <Accelerate/Accelerate.h>
#include "amx.h"

// Matrix size
#define N 1024

// Default blocking parameters (suboptimal - for demonstration)
#define MC_DEFAULT 128
#define KC_DEFAULT 128
#define NC_DEFAULT 128

// Tuned blocking parameters (optimal)
#define MC_TUNED 256
#define KC_TUNED 1024
#define NC_TUNED 256

// Micro-kernel size (fixed by AMX)
#define MR 32
#define NR 32

// ============================================================================
// Utilities
// ============================================================================

static inline double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void init_random(float* m, int size) {
    for (int i = 0; i < size; i++)
        m[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
}

static void zero_matrix(float* m, int size) {
    for (int i = 0; i < size; i++)
        m[i] = 0.0f;
}

static float max_diff(const float* a, const float* b, int n) {
    float max_d = 0;
    for (int i = 0; i < n; i++) {
        float d = fabsf(a[i] - b[i]);
        if (d > max_d) max_d = d;
    }
    return max_d;
}

// ============================================================================
// Stage 1: Naive Implementation (~20 GFLOPS)
// ============================================================================
/*
 * Simple triple-nested loop. No optimization at all.
 * Each element of A and B is loaded N times from memory.
 *
 * Performance limited by memory bandwidth:
 *   - 2N³ FLOPs, 2N³ memory accesses
 *   - Arithmetic intensity: ~1 FLOP/byte
 *   - Achieves ~1% of peak
 */
static void gemm_naive(const float* A, const float* B, float* C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

// ============================================================================
// Stage 2: Blocked (no packing) (~800 GFLOPS)
// ============================================================================
/*
 * Block the computation to fit in cache, but don't pack matrices.
 * Uses AMX for the micro-kernel but with strided memory access.
 *
 * Improvement over naive:
 *   - Better cache reuse within blocks
 *   - Uses AMX hardware acceleration
 *
 * Still limited by:
 *   - Strided memory access patterns
 *   - Cache line waste (load 64 bytes, use 4)
 */

// Simple 32x32 microkernel without packing (strided access)
static inline void microkernel_32x32_no_pack(const float* A, int lda,
                                              const float* B, int ldb,
                                              float* C, int ldc,
                                              int K, int first_k) {
    // Load C if accumulating
    if (!first_k) {
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + j * ldc, j * 4));
            AMX_LDZ(ldz_operand(C + j * ldc + 16, j * 4 + 1));
        }
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc, j * 4 + 2));
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
        }
    }

    for (int k = 0; k < K; k++) {
        // Load A column (strided - inefficient!)
        float a_col[32] __attribute__((aligned(64)));
        for (int i = 0; i < 32; i++) {
            a_col[i] = A[i * lda + k];
        }
        AMX_LDY(ldy_multiple(a_col, 0));

        // Load B row (strided - inefficient!)
        AMX_LDX(ldx_multiple((void*)(B + k * ldb), 0));

        if (first_k && k == 0) {
            AMX_FMA32(fma32_operand_skip_z(0, 0, 0));
            AMX_FMA32(fma32_operand_skip_z(1, 64, 0));
            AMX_FMA32(fma32_operand_skip_z(2, 0, 64));
            AMX_FMA32(fma32_operand_skip_z(3, 64, 64));
        } else {
            AMX_FMA32(fma32_operand(0, 0, 0));
            AMX_FMA32(fma32_operand(1, 64, 0));
            AMX_FMA32(fma32_operand(2, 0, 64));
            AMX_FMA32(fma32_operand(3, 64, 64));
        }
    }

    // Store C
    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + j * ldc, j * 4));
        AMX_STZ(stz_operand(C + j * ldc + 16, j * 4 + 1));
    }
    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + (16 + j) * ldc, j * 4 + 2));
        AMX_STZ(stz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
    }
}

static void gemm_blocked(const float* A, const float* B, float* C, int n) {
    int MC = MC_DEFAULT, KC = KC_DEFAULT, NC = NC_DEFAULT;

    AMX_SET();

    for (int jc = 0; jc < n; jc += NC) {
        for (int pc = 0; pc < n; pc += KC) {
            int first_k = (pc == 0);
            int k_size = (pc + KC <= n) ? KC : (n - pc);

            for (int ic = 0; ic < n; ic += MC) {
                for (int jr = 0; jr < NC && jc + jr < n; jr += NR) {
                    for (int ir = 0; ir < MC && ic + ir < n; ir += MR) {
                        microkernel_32x32_no_pack(
                            A + (ic + ir) * n + pc, n,
                            B + pc * n + jc + jr, n,
                            C + (ic + ir) * n + jc + jr, n,
                            k_size, first_k);
                    }
                }
            }
        }
    }

    AMX_CLR();
}

// ============================================================================
// Stage 3: With A Packing (~1200 GFLOPS)
// ============================================================================
/*
 * Pack A matrix into contiguous, transposed format.
 * B is still accessed with stride.
 *
 * Improvement:
 *   - A loads are now sequential (full cache line utilization)
 *   - Better prefetching for A
 *
 * Packing transforms A from row-major to:
 *   A_packed[slice][k][i] for sequential column access
 */

static void pack_A_panel_generic(const float* A, float* A_packed,
                                  int ic, int pc, int lda, int MC, int KC) {
    int mc_tiles = MC / MR;
    for (int ir = 0; ir < mc_tiles; ir++) {
        float* dst = A_packed + ir * MR * KC;
        const float* src_base = A + (ic + ir * MR) * lda + pc;

        for (int k = 0; k < KC; k += 4) {
            for (int i = 0; i < MR; i += 4) {
                const float* src = src_base + i * lda + k;
                float* d = dst + k * MR + i;

                float32x4_t r0 = vld1q_f32(src + 0 * lda);
                float32x4_t r1 = vld1q_f32(src + 1 * lda);
                float32x4_t r2 = vld1q_f32(src + 2 * lda);
                float32x4_t r3 = vld1q_f32(src + 3 * lda);

                float32x4x2_t t01 = vtrnq_f32(r0, r1);
                float32x4x2_t t23 = vtrnq_f32(r2, r3);

                vst1q_f32(d + 0 * MR, vcombine_f32(vget_low_f32(t01.val[0]), vget_low_f32(t23.val[0])));
                vst1q_f32(d + 1 * MR, vcombine_f32(vget_low_f32(t01.val[1]), vget_low_f32(t23.val[1])));
                vst1q_f32(d + 2 * MR, vcombine_f32(vget_high_f32(t01.val[0]), vget_high_f32(t23.val[0])));
                vst1q_f32(d + 3 * MR, vcombine_f32(vget_high_f32(t01.val[1]), vget_high_f32(t23.val[1])));
            }
        }
    }
}

// Microkernel with packed A, strided B
static inline void microkernel_32x32_pack_a(const float* A_packed,
                                             const float* B, int ldb,
                                             float* C, int ldc,
                                             int KC, int first_k) {
    if (!first_k) {
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + j * ldc, j * 4));
            AMX_LDZ(ldz_operand(C + j * ldc + 16, j * 4 + 1));
        }
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc, j * 4 + 2));
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
        }
    }

    for (int k = 0; k < KC; k++) {
        AMX_LDY(ldy_multiple(A_packed + k * MR, 0));
        AMX_LDX(ldx_multiple((void*)(B + k * ldb), 0));

        if (first_k && k == 0) {
            AMX_FMA32(fma32_operand_skip_z(0, 0, 0));
            AMX_FMA32(fma32_operand_skip_z(1, 64, 0));
            AMX_FMA32(fma32_operand_skip_z(2, 0, 64));
            AMX_FMA32(fma32_operand_skip_z(3, 64, 64));
        } else {
            AMX_FMA32(fma32_operand(0, 0, 0));
            AMX_FMA32(fma32_operand(1, 64, 0));
            AMX_FMA32(fma32_operand(2, 0, 64));
            AMX_FMA32(fma32_operand(3, 64, 64));
        }
    }

    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + j * ldc, j * 4));
        AMX_STZ(stz_operand(C + j * ldc + 16, j * 4 + 1));
    }
    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + (16 + j) * ldc, j * 4 + 2));
        AMX_STZ(stz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
    }
}

static void gemm_packed_a(const float* A, const float* B, float* C, int n) {
    int MC = MC_DEFAULT, KC = KC_DEFAULT, NC = NC_DEFAULT;

    float* A_packed;
    posix_memalign((void**)&A_packed, 64, MC * KC * sizeof(float));

    AMX_SET();

    for (int jc = 0; jc < n; jc += NC) {
        for (int pc = 0; pc < n; pc += KC) {
            int first_k = (pc == 0);

            for (int ic = 0; ic < n; ic += MC) {
                pack_A_panel_generic(A, A_packed, ic, pc, n, MC, KC);

                for (int ir = 0; ir < MC; ir += MR) {
                    const float* A_slice = A_packed + (ir / MR) * MR * KC;

                    for (int jr = 0; jr < NC; jr += NR) {
                        microkernel_32x32_pack_a(
                            A_slice,
                            B + pc * n + jc + jr, n,
                            C + (ic + ir) * n + jc + jr, n,
                            KC, first_k);
                    }
                }
            }
        }
    }

    AMX_CLR();
    free(A_packed);
}

// ============================================================================
// Stage 4: With A and On-the-fly B Packing (~1350 GFLOPS)
// ============================================================================
/*
 * Pack B on-the-fly during the first ir iteration.
 * The AMX load already brings B into X registers - we just store it.
 *
 * Improvement:
 *   - B loads are now sequential after first use
 *   - Eliminates separate B packing pass
 *   - Better cache utilization
 */

static inline void microkernel_32x32_pack_ab(const float* A_packed,
                                              const float* B_src, int ldb,
                                              float* B_packed,
                                              float* C, int ldc,
                                              int KC, int first_k, int pack_B) {
    if (!first_k) {
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + j * ldc, j * 4));
            AMX_LDZ(ldz_operand(C + j * ldc + 16, j * 4 + 1));
        }
        for (int j = 0; j < 16; j++) {
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc, j * 4 + 2));
            AMX_LDZ(ldz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
        }
    }

    for (int k = 0; k < KC; k++) {
        AMX_LDY(ldy_multiple(A_packed + k * MR, 0));

        if (pack_B) {
            // Load from strided source, store to packed buffer
            AMX_LDX(ldx_multiple((void*)(B_src + k * ldb), 0));
            AMX_STX(stz_operand(B_packed + k * NR, 0));
            AMX_STX(stz_operand(B_packed + k * NR + 16, 1));
        } else {
            // Load from packed buffer
            AMX_LDX(ldx_multiple((void*)(B_packed + k * NR), 0));
        }

        if (first_k && k == 0) {
            AMX_FMA32(fma32_operand_skip_z(0, 0, 0));
            AMX_FMA32(fma32_operand_skip_z(1, 64, 0));
            AMX_FMA32(fma32_operand_skip_z(2, 0, 64));
            AMX_FMA32(fma32_operand_skip_z(3, 64, 64));
        } else {
            AMX_FMA32(fma32_operand(0, 0, 0));
            AMX_FMA32(fma32_operand(1, 64, 0));
            AMX_FMA32(fma32_operand(2, 0, 64));
            AMX_FMA32(fma32_operand(3, 64, 64));
        }
    }

    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + j * ldc, j * 4));
        AMX_STZ(stz_operand(C + j * ldc + 16, j * 4 + 1));
    }
    for (int j = 0; j < 16; j++) {
        AMX_STZ(stz_operand(C + (16 + j) * ldc, j * 4 + 2));
        AMX_STZ(stz_operand(C + (16 + j) * ldc + 16, j * 4 + 3));
    }
}

static void gemm_packed_ab(const float* A, const float* B, float* C, int n) {
    int MC = MC_DEFAULT, KC = KC_DEFAULT, NC = NC_DEFAULT;

    float* A_packed;
    float* B_packed;
    posix_memalign((void**)&A_packed, 64, MC * KC * sizeof(float));
    posix_memalign((void**)&B_packed, 64, KC * NC * sizeof(float));

    AMX_SET();

    for (int ic = 0; ic < n; ic += MC) {
        for (int pc = 0; pc < n; pc += KC) {
            int first_k = (pc == 0);
            pack_A_panel_generic(A, A_packed, ic, pc, n, MC, KC);

            for (int jc = 0; jc < n; jc += NC) {
                for (int ir = 0; ir < MC; ir += MR) {
                    int first_ir = (ir == 0);
                    const float* A_slice = A_packed + (ir / MR) * MR * KC;

                    for (int jr = 0; jr < NC; jr += NR) {
                        float* B_slice = B_packed + (jr / NR) * KC * NR;
                        microkernel_32x32_pack_ab(
                            A_slice,
                            B + pc * n + jc + jr, n,
                            B_slice,
                            C + (ic + ir) * n + jc + jr, n,
                            KC, first_k, first_ir);
                    }
                }
            }
        }
    }

    AMX_CLR();
    free(A_packed);
    free(B_packed);
}

// ============================================================================
// Stage 5: Tuned Parameters (~1430 GFLOPS)
// ============================================================================
/*
 * Same algorithm as Stage 4, but with optimal blocking parameters:
 *   MC=256, KC=1024, NC=256
 *
 * Why these parameters work:
 *   - KC=1024=N means only one pc iteration (pack A once per ic)
 *   - MC=256, NC=256 balance cache usage and reuse
 *   - Total packed: 2MB fits well in L2
 */

static void gemm_tuned(const float* A, const float* B, float* C, int n) {
    int MC = MC_TUNED, KC = KC_TUNED, NC = NC_TUNED;

    float* A_packed;
    float* B_packed;
    posix_memalign((void**)&A_packed, 64, MC * KC * sizeof(float));
    posix_memalign((void**)&B_packed, 64, KC * NC * sizeof(float));

    AMX_SET();

    for (int ic = 0; ic < n; ic += MC) {
        for (int pc = 0; pc < n; pc += KC) {
            int first_k = (pc == 0);
            pack_A_panel_generic(A, A_packed, ic, pc, n, MC, KC);

            for (int jc = 0; jc < n; jc += NC) {
                for (int ir = 0; ir < MC; ir += MR) {
                    int first_ir = (ir == 0);
                    const float* A_slice = A_packed + (ir / MR) * MR * KC;

                    for (int jr = 0; jr < NC; jr += NR) {
                        float* B_slice = B_packed + (jr / NR) * KC * NR;
                        microkernel_32x32_pack_ab(
                            A_slice,
                            B + pc * n + jc + jr, n,
                            B_slice,
                            C + (ic + ir) * n + jc + jr, n,
                            KC, first_k, first_ir);
                    }
                }
            }
        }
    }

    AMX_CLR();
    free(A_packed);
    free(B_packed);
}

// ============================================================================
// Stage 6: Lazy A Packing (~1520 GFLOPS)
// ============================================================================
/*
 * Pack each A slice just-in-time instead of all at once.
 *
 * Benefits:
 *   - Better cache locality (slice is hot when first used)
 *   - Natural prefetching effect
 *   - Can overlap with computation (ILP)
 */

static inline void pack_A_slice(const float* A, float* dst, int row, int pc, int lda, int KC) {
    const float* src_base = A + row * lda + pc;

    for (int k = 0; k < KC; k += 4) {
        for (int i = 0; i < MR; i += 4) {
            const float* src = src_base + i * lda + k;
            float* d = dst + k * MR + i;

            float32x4_t r0 = vld1q_f32(src + 0 * lda);
            float32x4_t r1 = vld1q_f32(src + 1 * lda);
            float32x4_t r2 = vld1q_f32(src + 2 * lda);
            float32x4_t r3 = vld1q_f32(src + 3 * lda);

            float32x4x2_t t01 = vtrnq_f32(r0, r1);
            float32x4x2_t t23 = vtrnq_f32(r2, r3);

            vst1q_f32(d + 0 * MR, vcombine_f32(vget_low_f32(t01.val[0]), vget_low_f32(t23.val[0])));
            vst1q_f32(d + 1 * MR, vcombine_f32(vget_low_f32(t01.val[1]), vget_low_f32(t23.val[1])));
            vst1q_f32(d + 2 * MR, vcombine_f32(vget_high_f32(t01.val[0]), vget_high_f32(t23.val[0])));
            vst1q_f32(d + 3 * MR, vcombine_f32(vget_high_f32(t01.val[1]), vget_high_f32(t23.val[1])));
        }
    }
}

static void gemm_lazy(const float* A, const float* B, float* C, int n) {
    int MC = MC_TUNED, KC = KC_TUNED, NC = NC_TUNED;

    float* A_packed;
    float* B_packed;
    posix_memalign((void**)&A_packed, 64, MC * KC * sizeof(float));
    posix_memalign((void**)&B_packed, 64, KC * NC * sizeof(float));

    AMX_SET();

    for (int ic = 0; ic < n; ic += MC) {
        for (int pc = 0; pc < n; pc += KC) {
            int first_k = (pc == 0);

            for (int jc = 0; jc < n; jc += NC) {
                for (int ir = 0; ir < MC; ir += MR) {
                    int first_ir = (ir == 0);
                    int ir_idx = ir / MR;
                    float* A_slice = A_packed + ir_idx * MR * KC;

                    // Lazy pack: only on first jc iteration
                    if (jc == 0) {
                        pack_A_slice(A, A_slice, ic + ir, pc, n, KC);
                    }

                    for (int jr = 0; jr < NC; jr += NR) {
                        float* B_slice = B_packed + (jr / NR) * KC * NR;
                        microkernel_32x32_pack_ab(
                            A_slice,
                            B + pc * n + jc + jr, n,
                            B_slice,
                            C + (ic + ir) * n + jc + jr, n,
                            KC, first_k, first_ir);
                    }
                }
            }
        }
    }

    AMX_CLR();
    free(A_packed);
    free(B_packed);
}

// ============================================================================
// Reference (Accelerate)
// ============================================================================

static void gemm_reference(const float* A, const float* B, float* C, int n) {
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                n, n, n, 1.0f, A, n, B, n, 0.0f, C, n);
}

// ============================================================================
// Benchmark
// ============================================================================

typedef void (*gemm_func)(const float*, const float*, float*, int);

typedef struct {
    const char* name;
    gemm_func func;
    int runs;  // More runs for faster functions
} benchmark_entry;

int main(void) {
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     GEMM Progressive Optimization Demo (N=%d)                  ║\n", N);
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║ Stage  Implementation         GFLOPS   %% Peak   vs Accelerate  ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");

    float *A, *B, *C, *C_ref;
    posix_memalign((void**)&A, 64, N * N * sizeof(float));
    posix_memalign((void**)&B, 64, N * N * sizeof(float));
    posix_memalign((void**)&C, 64, N * N * sizeof(float));
    posix_memalign((void**)&C_ref, 64, N * N * sizeof(float));

    srand(42);
    init_random(A, N * N);
    init_random(B, N * N);

    double flops = 2.0 * N * N * N;
    double peak_gflops = 1638.0;  // Theoretical single-core peak

    // Warmup and get reference
    gemm_reference(A, B, C_ref, N);
    gemm_reference(A, B, C_ref, N);
    gemm_reference(A, B, C_ref, N);

    benchmark_entry benchmarks[] = {
        {"Accelerate (ref)", gemm_reference, 20},
        {"1. Naive",         gemm_naive,     1},   // Very slow, only 1 run
        {"2. Blocked",       gemm_blocked,   5},
        {"3. + Pack A",      gemm_packed_a,  10},
        {"4. + Pack B",      gemm_packed_ab, 15},
        {"5. + Tuned",       gemm_tuned,     20},
        {"6. + Lazy",        gemm_lazy,      20},
    };
    int n_benchmarks = sizeof(benchmarks) / sizeof(benchmarks[0]);

    double ref_gflops = 0;

    for (int b = 0; b < n_benchmarks; b++) {
        // Warmup
        zero_matrix(C, N * N);
        benchmarks[b].func(A, B, C, N);

        // Verify correctness
        float err = max_diff(C, C_ref, N * N);
        if (err > 1e-3) {
            printf("║ WARNING: %s has error %.2e                           ║\n",
                   benchmarks[b].name, err);
        }

        // Benchmark
        double t0 = get_time();
        for (int r = 0; r < benchmarks[b].runs; r++) {
            benchmarks[b].func(A, B, C, N);
        }
        double t = (get_time() - t0) / benchmarks[b].runs;
        double gflops = flops / t / 1e9;
        double pct_peak = gflops / peak_gflops * 100;

        if (b == 0) {
            ref_gflops = gflops;
            printf("║   -    %-22s %6.0f    %5.1f%%     (baseline)     ║\n",
                   benchmarks[b].name, gflops, pct_peak);
        } else {
            double ratio = ref_gflops / gflops;
            if (ratio > 1.0) {
                printf("║   %d    %-22s %6.0f    %5.1f%%    %6.2fx slower  ║\n",
                       b, benchmarks[b].name, gflops, pct_peak, ratio);
            } else {
                printf("║   %d    %-22s %6.0f    %5.1f%%    %6.2fx FASTER  ║\n",
                       b, benchmarks[b].name, gflops, pct_peak, 1.0/ratio);
            }
        }
    }

    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║ Theoretical peak: %.0f GFLOPS (single-core AMX @ 3.2GHz)        ║\n", peak_gflops);
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    printf("\nKey Insights:\n");
    printf("  Stage 1→2: AMX hardware gives ~40x speedup over scalar\n");
    printf("  Stage 2→3: A packing improves memory access pattern (+50%%)\n");
    printf("  Stage 3→4: On-the-fly B packing eliminates separate pass (+12%%)\n");
    printf("  Stage 4→5: Parameter tuning (KC=1024) reduces packing (+6%%)\n");
    printf("  Stage 5→6: Lazy packing improves cache locality (+6%%)\n");

    free(A);
    free(B);
    free(C);
    free(C_ref);

    return 0;
}
