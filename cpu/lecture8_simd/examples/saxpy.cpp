// saxpy.cpp - SAXPY: y = a*x + y (single-precision)
// Compile: g++ -O1 -mavx2 -mfma saxpy.cpp -o saxpy
// Run: ./saxpy
//
// Classic BLAS operation, uses 8-way float SIMD with FMA.

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <immintrin.h>
using namespace std;

// Scalar version
void saxpy_scalar(float* y, float a, float* x, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = a * x[i] + y[i];
    }
}

// SIMD version
void saxpy_simd(float* y, float a, float* x, int n) {
    __m256 va = _mm256_set1_ps(a);  // Broadcast a to all 8 lanes

    int i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 vx = _mm256_loadu_ps(x + i);
        __m256 vy = _mm256_loadu_ps(y + i);
        vy = _mm256_fmadd_ps(va, vx, vy);  // y = a*x + y
        _mm256_storeu_ps(y + i, vy);
    }

    // Handle remainder
    for (; i < n; i++) {
        y[i] = a * x[i] + y[i];
    }
}

int main() {
    const int n = 100'000'000;
    float* x = (float*)aligned_alloc(32, n * sizeof(float));
    float* y1 = (float*)aligned_alloc(32, n * sizeof(float));
    float* y2 = (float*)aligned_alloc(32, n * sizeof(float));
    float a = 2.0f;

    for (int i = 0; i < n; i++) {
        x[i] = 1.0f;
        y1[i] = 1.0f;
        y2[i] = 1.0f;
    }

    // Scalar version
    auto start1 = chrono::high_resolution_clock::now();
    saxpy_scalar(y1, a, x, n);
    auto end1 = chrono::high_resolution_clock::now();
    double ms1 = chrono::duration<double, milli>(end1 - start1).count();

    // SIMD version
    auto start2 = chrono::high_resolution_clock::now();
    saxpy_simd(y2, a, x, n);
    auto end2 = chrono::high_resolution_clock::now();
    double ms2 = chrono::duration<double, milli>(end2 - start2).count();

    cout << "Scalar SAXPY: " << ms1 << " ms, y[0]=" << y1[0] << endl;
    cout << "SIMD SAXPY:   " << ms2 << " ms, y[0]=" << y2[0] << endl;

    free(x); free(y1); free(y2);
    return 0;
}
