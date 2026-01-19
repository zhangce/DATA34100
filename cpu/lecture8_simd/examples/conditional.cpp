// conditional.cpp - SIMD with conditionals using masking
// Compile: g++ -O1 -mavx2 conditional.cpp -o conditional
// Run: ./conditional
//
// Demonstrates how to handle if/else in SIMD using blendv.
// Instead of branching, compute both paths and select with a mask.

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <random>
#include <immintrin.h>
using namespace std;

// Scalar version with branch
void conditional_scalar(double* a, double* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] > 0.5)
            b[i] = a[i] + 1.0;
        else
            b[i] = a[i] - 1.0;
    }
}

// SIMD version with masking (no branches)
void conditional_simd(double* a, double* b, int n) {
    __m256d threshold = _mm256_set1_pd(0.5);
    __m256d ones = _mm256_set1_pd(1.0);
    __m256d mones = _mm256_set1_pd(-1.0);

    for (int i = 0; i < n; i += 4) {
        __m256d v = _mm256_loadu_pd(a + i);

        // Compare: creates mask (all 1s or all 0s per element)
        __m256d mask = _mm256_cmp_pd(v, threshold, _CMP_GT_OQ);

        // Blend: select from ones or mones based on mask
        __m256d delta = _mm256_blendv_pd(mones, ones, mask);

        __m256d result = _mm256_add_pd(v, delta);
        _mm256_storeu_pd(b + i, result);
    }
}

int main() {
    const int n = 100'000'000;
    double* a = (double*)aligned_alloc(32, n * sizeof(double));
    double* b = (double*)aligned_alloc(32, n * sizeof(double));

    // Random data
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    // Scalar version
    auto start1 = chrono::high_resolution_clock::now();
    conditional_scalar(a, b, n);
    auto end1 = chrono::high_resolution_clock::now();
    double ms1 = chrono::duration<double, milli>(end1 - start1).count();

    // Reset b
    for (int i = 0; i < n; i++) b[i] = 0;

    // SIMD version
    auto start2 = chrono::high_resolution_clock::now();
    conditional_simd(a, b, n);
    auto end2 = chrono::high_resolution_clock::now();
    double ms2 = chrono::duration<double, milli>(end2 - start2).count();

    cout << "Scalar conditional: " << ms1 << " ms" << endl;
    cout << "SIMD conditional:   " << ms2 << " ms" << endl;

    free(a); free(b);
    return 0;
}
