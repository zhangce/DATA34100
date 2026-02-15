// stencil.cpp - 2D 5-point stencil (heat equation)
// Compile: g++ -O3 -std=c++14 stencil.cpp -o stencil
// Run: ./stencil
//
// Common pattern in scientific computing. Memory bound.
// Operational Intensity: I = 4n^2 / 16n^2 = 1/4 = 0.25 flops/byte
//
// Work: 4n^2 flops (3 adds + 1 mul per point)
// Data: ~16n^2 bytes (read A, write B)

#include <iostream>
#include <chrono>
#include <cstdlib>
using namespace std;

void stencil_2d(double* B, double* A, int n) {
    for (int i = 1; i < n - 1; i++) {
        for (int j = 1; j < n - 1; j++) {
            B[i * n + j] = 0.25 * (A[(i-1) * n + j] + A[(i+1) * n + j] +
                                   A[i * n + (j-1)] + A[i * n + (j+1)]);
        }
    }
}

int main() {
    const int n = 4096;  // 4K x 4K grid
    double* A = (double*)aligned_alloc(32, n * n * sizeof(double));
    double* B = (double*)aligned_alloc(32, n * n * sizeof(double));

    // Initialize
    for (int i = 0; i < n * n; i++) {
        A[i] = 1.0;
        B[i] = 0.0;
    }

    auto start = chrono::high_resolution_clock::now();
    stencil_2d(B, A, n);
    auto end = chrono::high_resolution_clock::now();

    double seconds = chrono::duration<double>(end - start).count();
    long long interior = (long long)(n - 2) * (n - 2);
    double flops = 4.0 * interior;  // 3 adds + 1 mul
    double bytes = 16.0 * n * n;    // Read A, write B (approximate)

    cout << "2D 5-point Stencil" << endl;
    cout << "  Grid: " << n << " x " << n << endl;
    cout << "  Interior points: " << interior << endl;
    cout << "  Time: " << seconds * 1000 << " ms" << endl;
    cout << "  Performance: " << flops / seconds / 1e9 << " GFLOPS" << endl;
    cout << "  Bandwidth: " << bytes / seconds / 1e9 << " GB/s" << endl;
    cout << "  Operational Intensity: " << flops / bytes << " flops/byte" << endl;
    cout << "  B[n/2][n/2] = " << B[(n/2) * n + n/2] << " (expected: 1.0)" << endl;

    free(A);
    free(B);
    return 0;
}
