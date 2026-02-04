// dot_v2.cpp - SIMD dot product with FMA
// Compile: g++ -O1 -mavx2 -mfma dot_v2.cpp -o dot_v2
// Run: ./dot_v2
//
// Uses FMA (fused multiply-add): vsum = a*b + vsum in one instruction.

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <immintrin.h>
using namespace std;

double dot_simd(double* a, double* b, int n) {
    __m256d vsum = _mm256_setzero_pd();

    for (int i = 0; i < n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        // FMA: vsum = va * vb + vsum (one instruction!)
        vsum = _mm256_fmadd_pd(va, vb, vsum);
    }

    // Horizontal reduction
    __m128d low = _mm256_castpd256_pd128(vsum);
    __m128d high = _mm256_extractf128_pd(vsum, 1);
    __m128d sum128 = _mm_add_pd(low, high);
    sum128 = _mm_hadd_pd(sum128, sum128);
    return _mm_cvtsd_f64(sum128);
}

int main() {
    const int n = 100'000'000;
    double* a = (double*)aligned_alloc(32, n * sizeof(double));
    double* b = (double*)aligned_alloc(32, n * sizeof(double));
    for (int i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 1.0;
    }

    auto start = chrono::high_resolution_clock::now();
    double dot = dot_simd(a, b, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "SIMD dot: " << dot << ", Time: " << ms << " ms" << endl;

    free(a); free(b);
    return 0;
}
