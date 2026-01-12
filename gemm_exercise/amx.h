/*
 * Apple AMX (Apple Matrix eXtensions) Header
 *
 * AMX is Apple's undocumented matrix coprocessor available on M1/M2/M3 chips.
 *
 * Register layout:
 *   X registers: 8 x 64 bytes (8 x 16 floats)
 *   Y registers: 8 x 64 bytes (8 x 16 floats)
 *   Z registers: 64 rows x 64 bytes = 4KB accumulator
 *                For fp32: organized as z[16][4][16] - 16 output rows, 4 z_rows, 16 columns
 *
 * FMA32 operation (matrix mode):
 *   z[j][z_row][i] += x[i] * y[j]  where i,j in [0,16)
 *   This computes a 16x16 outer product
 */

#ifndef AMX_H
#define AMX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// AMX instruction encoding
#define AMX_NOP_OP_IMM5(op, imm5) \
    __asm__ volatile("nop\nnop\nnop\n.word (0x201000 + (%0 << 5) + %1)" \
                     : : "i"(op), "i"(imm5) : "memory")

#define AMX_OP_GPR(op, gpr) \
    __asm__ volatile(".word (0x201000 + (%0 << 5) + 0%1 - ((0%1 >> 4) * 6))" \
                     : : "i"(op), "r"((uint64_t)(gpr)) : "memory")

// Load/Store operations
#define AMX_LDX(gpr)   AMX_OP_GPR(0, gpr)   // Load to X register
#define AMX_LDY(gpr)   AMX_OP_GPR(1, gpr)   // Load to Y register
#define AMX_STX(gpr)   AMX_OP_GPR(2, gpr)   // Store from X register
#define AMX_STY(gpr)   AMX_OP_GPR(3, gpr)   // Store from Y register
#define AMX_LDZ(gpr)   AMX_OP_GPR(4, gpr)   // Load to Z register
#define AMX_STZ(gpr)   AMX_OP_GPR(5, gpr)   // Store from Z register
#define AMX_LDZI(gpr)  AMX_OP_GPR(6, gpr)   // Load to Z interleaved
#define AMX_STZI(gpr)  AMX_OP_GPR(7, gpr)   // Store from Z interleaved

// Extract operations
#define AMX_EXTRX(gpr) AMX_OP_GPR(8, gpr)
#define AMX_EXTRY(gpr) AMX_OP_GPR(9, gpr)

// Compute operations
#define AMX_FMA64(gpr) AMX_OP_GPR(10, gpr)  // FMA for f64
#define AMX_FMS64(gpr) AMX_OP_GPR(11, gpr)  // FMS for f64
#define AMX_FMA32(gpr) AMX_OP_GPR(12, gpr)  // FMA for f32
#define AMX_FMS32(gpr) AMX_OP_GPR(13, gpr)  // FMS for f32
#define AMX_MAC16(gpr) AMX_OP_GPR(14, gpr)  // MAC for i16
#define AMX_FMA16(gpr) AMX_OP_GPR(15, gpr)  // FMA for f16
#define AMX_FMS16(gpr) AMX_OP_GPR(16, gpr)  // FMS for f16

// Control operations
#define AMX_SET()      AMX_NOP_OP_IMM5(17, 0)  // Enable AMX
#define AMX_CLR()      AMX_NOP_OP_IMM5(17, 1)  // Disable AMX

// Additional operations
#define AMX_VECINT(gpr) AMX_OP_GPR(18, gpr)
#define AMX_VECFP(gpr)  AMX_OP_GPR(19, gpr)
#define AMX_MATINT(gpr) AMX_OP_GPR(20, gpr)
#define AMX_MATFP(gpr)  AMX_OP_GPR(21, gpr)
#define AMX_GENLUT(gpr) AMX_OP_GPR(22, gpr)

// ============================================================================
// Operand builders for load/store
// ============================================================================

// Load X/Y operand: encodes address + register index + load mode
static inline uint64_t ldx_operand(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56);
}

static inline uint64_t ldy_operand(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56);
}

// Load X/Y with multiple registers (2 registers = 128 bytes)
static inline uint64_t ldx_multiple(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56) | (1ULL << 62);
}

static inline uint64_t ldy_multiple(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56) | (1ULL << 62);
}

// Load X/Y with 4 registers (256 bytes)
static inline uint64_t ldx_multiple4(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56) | (1ULL << 62) | (1ULL << 60);
}

static inline uint64_t ldy_multiple4(const void* ptr, uint64_t reg_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (reg_idx << 56) | (1ULL << 62) | (1ULL << 60);
}

// Store Z operand: encodes address + row index
static inline uint64_t stz_operand(void* ptr, uint64_t row_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (row_idx << 56);
}

// Store Z with multiple rows (2 rows = 128 bytes)
static inline uint64_t stz_multiple(void* ptr, uint64_t row_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (row_idx << 56) | (1ULL << 62);
}

// Load Z operand
static inline uint64_t ldz_operand(const void* ptr, uint64_t row_idx) {
    return ((uint64_t)ptr & ((1ULL << 57) - 1)) | (row_idx << 56);
}

// ============================================================================
// FMA32 operand builder
// ============================================================================

/*
 * FMA32 operand encoding for matrix mode:
 *   z[j][z_row][i] += x[x_offset/4 + i] * y[y_offset/4 + j]
 *
 * Bits:
 *   [63]    : vector_mode (0 = matrix mode)
 *   [27]    : skip_z (1 = initialize to zero instead of accumulate)
 *   [20-25] : z_row (0-3 for fp32)
 *   [10-18] : x_offset in bytes
 *   [0-8]   : y_offset in bytes
 */
static inline uint64_t fma32_operand(uint64_t z_row, uint64_t x_offset, uint64_t y_offset) {
    return (z_row << 20) | (x_offset << 10) | y_offset;
}

static inline uint64_t fma32_operand_skip_z(uint64_t z_row, uint64_t x_offset, uint64_t y_offset) {
    return (1ULL << 27) | (z_row << 20) | (x_offset << 10) | y_offset;
}

#ifdef __cplusplus
}
#endif

#endif // AMX_H
