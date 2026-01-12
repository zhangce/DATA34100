// Minimal CUDA runtime stubs for host-only CuTe
#pragma once
#include "vector_types.h"

// Device/host qualifiers (no-op on host)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif
#ifndef __global__
#define __global__
#endif

// Error handling
typedef int cudaError_t;
#define cudaSuccess 0
inline const char* cudaGetErrorString(cudaError_t) { return "stub"; }
inline cudaError_t cudaGetLastError() { return cudaSuccess; }

// Basic types
typedef unsigned long long cudaTextureObject_t;
