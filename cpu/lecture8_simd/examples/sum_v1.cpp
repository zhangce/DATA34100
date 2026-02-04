// sum_v1.cpp - SIMD sum reduction (single vector accumulator)
// Compile: g++ -O1 -mavx2 sum_v1.cpp -o sum_v1
// Run: ./sum_v1
//
// Uses 4 parallel accumulators via SIMD (one __m256d register).
// This gives same ILP as scalar 4x unrolled.

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <immintrin.h>
using namespace std;

double sum_simd(double* a, int n) {
    __m256d vsum = _mm256_setzero_pd();  // [0, 0, 0, 0]

    for (int i = 0; i < n; i += 4) {
        __m256d v = _mm256_loadu_pd(a + i);
        vsum = _mm256_add_pd(vsum, v);   // 4 parallel sums
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
    for (int i = 0; i < n; i++) a[i] = 1.0;

    auto start = chrono::high_resolution_clock::now();
    double sum = sum_simd(a, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "SIMD sum (1 vec acc): " << sum << ", Time: " << ms << " ms" << endl;

    free(a);
    return 0;
}
