// Host-only CuTe configuration - layout only
#pragma once

// Force host-only mode
#ifdef __clang__
#undef __clang__
#define __CUTE_CLANG_UNDEF__
#endif

// Only include layout functionality (not tensor/gemm)
#include <cute/layout.hpp>
#include <cute/tile.hpp>
#include <cute/swizzle.hpp>
#include <cute/swizzle_layout.hpp>

// Restore clang if needed
#ifdef __CUTE_CLANG_UNDEF__
#define __clang__ 1
#undef __CUTE_CLANG_UNDEF__
#endif
