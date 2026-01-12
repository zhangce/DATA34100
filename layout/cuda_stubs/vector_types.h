// CUDA vector types stub
#pragma once
#include <cstdint>

// Basic vector types
struct char1 { char x; };
struct char2 { char x, y; };
struct char3 { char x, y, z; };
struct char4 { char x, y, z, w; };

struct uchar1 { unsigned char x; };
struct uchar2 { unsigned char x, y; };
struct uchar3 { unsigned char x, y, z; };
struct uchar4 { unsigned char x, y, z, w; };

struct short1 { short x; };
struct short2 { short x, y; };
struct short3 { short x, y, z; };
struct short4 { short x, y, z, w; };

struct ushort1 { unsigned short x; };
struct ushort2 { unsigned short x, y; };
struct ushort3 { unsigned short x, y, z; };
struct ushort4 { unsigned short x, y, z, w; };

struct int1 { int x; };
struct int2 { int x, y; };
struct int3 { int x, y, z; };
struct int4 { int x, y, z, w; };

struct uint1 { unsigned int x; };
struct uint2 { unsigned int x, y; };
struct uint3 { unsigned int x, y, z; };
struct uint4 { unsigned int x, y, z, w; };

struct long1 { long x; };
struct long2 { long x, y; };
struct long3 { long x, y, z; };
struct long4 { long x, y, z, w; };

struct ulong1 { unsigned long x; };
struct ulong2 { unsigned long x, y; };
struct ulong3 { unsigned long x, y, z; };
struct ulong4 { unsigned long x, y, z, w; };

struct longlong1 { long long x; };
struct longlong2 { long long x, y; };
struct longlong3 { long long x, y, z; };
struct longlong4 { long long x, y, z, w; };

struct ulonglong1 { unsigned long long x; };
struct ulonglong2 { unsigned long long x, y; };
struct ulonglong3 { unsigned long long x, y, z; };
struct ulonglong4 { unsigned long long x, y, z, w; };

struct float1 { float x; };
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };

struct double1 { double x; };
struct double2 { double x, y; };
struct double3 { double x, y, z; };
struct double4 { double x, y, z, w; };

struct dim3 { 
    unsigned int x, y, z;
    constexpr dim3(unsigned int vx = 1, unsigned int vy = 1, unsigned int vz = 1) : x(vx), y(vy), z(vz) {}
};
