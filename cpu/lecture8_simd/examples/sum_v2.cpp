// sum_v2.cpp - SIMD sum with 2x unrolling (two vector accumulators)
// Compile: g++ -O1 -mavx2 sum_v2.cpp -o sum_v2
// Run: ./sum_v2
//
// Uses 8 parallel accumulators via 2 SIMD registers.
// Maximum ILP for FP add (latency=4, throughput=2/cycle -> need 8 in flight).

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <immintrin.h>
using namespace std;

double sum_simd_unrolled(double* a, int n) {
    __m256d vsum1 = _mm256_setzero_pd();
    __m256d vsum2 = _mm256_setzero_pd();  // Two vector accumulators!

    for (int i = 0; i < n; i += 8) {
        __m256d v1 = _mm256_loadu_pd(a + i);
        __m256d v2 = _mm256_loadu_pd(a + i + 4);
        vsum1 = _mm256_add_pd(vsum1, v1);
        vsum2 = _mm256_add_pd(vsum2, v2);
    }

    __m256d vsum = _mm256_add_pd(vsum1, vsum2);

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
    double sum = sum_simd_unrolled(a, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "SIMD sum (2 vec acc): " << sum << ", Time: " << ms << " ms" << endl;

    free(a);
    return 0;
}
