// add_v2.cpp - SIMD vector addition using AVX
// Compile: g++ -O1 -mavx2 add_v2.cpp -o add_v2
// Run: ./add_v2

#include <chrono>
#include <iostream>
#include <cstdlib>
#include <immintrin.h>
using namespace std;

void add_simd(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i += 4) {
        __m256d va = _mm256_loadu_pd(a + i);
        __m256d vb = _mm256_loadu_pd(b + i);
        __m256d vc = _mm256_add_pd(va, vb);
        _mm256_storeu_pd(c + i, vc);
    }
}

int main() {
    const int n = 100'000'000;
    double* a = (double*)aligned_alloc(32, n * sizeof(double));
    double* b = (double*)aligned_alloc(32, n * sizeof(double));
    double* c = (double*)aligned_alloc(32, n * sizeof(double));

    for (int i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    auto start = chrono::high_resolution_clock::now();
    add_simd(a, b, c, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "SIMD add: " << ms << " ms, c[0]=" << c[0] << endl;

    free(a); free(b); free(c);
    return 0;
}
