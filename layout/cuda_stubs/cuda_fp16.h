// CUDA FP16 types stub
#pragma once
#include <cstdint>

struct __half;

struct __half_raw {
    uint16_t x;
    __half_raw() = default;
    __half_raw(uint16_t v) : x(v) {}
    inline __half_raw(const __half& h);
};

struct __half {
    uint16_t x;
    __half() = default;
    __half(__half_raw hr) : x(hr.x) {}
    __half(float f) { 
        union { float f; uint32_t i; } u = {f};
        x = (u.i >> 16) & 0x8000;
        int exp = ((u.i >> 23) & 0xff) - 127 + 15;
        if (exp <= 0) { x = 0; return; }
        if (exp >= 31) { exp = 31; }
        x |= (exp & 0x1f) << 10;
        x |= (u.i >> 13) & 0x3ff;
    }
    operator float() const {
        if ((x & 0x7fff) == 0) return 0.0f;
        uint32_t sign = (x & 0x8000) << 16;
        uint32_t exp = ((x >> 10) & 0x1f);
        uint32_t mant = (x & 0x3ff) << 13;
        exp = (exp - 15 + 127) << 23;
        union { uint32_t i; float f; } u = {sign | exp | mant};
        return u.f;
    }
};

inline __half_raw::__half_raw(const __half& h) : x(h.x) {}

typedef __half half;

struct __half2 {
    __half x, y;
};

inline __half __float2half(float f) { return __half(f); }
inline float __half2float(__half h) { return float(h); }
inline __half __hadd(__half a, __half b) { return __half(float(a) + float(b)); }
inline __half __hmul(__half a, __half b) { return __half(float(a) * float(b)); }
inline __half __hsub(__half a, __half b) { return __half(float(a) - float(b)); }
