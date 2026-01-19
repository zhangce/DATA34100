// gemv.cpp - Matrix-Vector multiplication: y = A * x
// Compile: g++ -O3 -std=c++14 -mavx2 -mfma gemv.cpp -o gemv
// Run: ./gemv
//
// BLAS Level 2 operation. Memory bound.
// Operational Intensity: I = 2n^2 / 8n^2 = 1/4 = 0.25 flops/byte
//
// Work: 2n^2 flops
// Data: ~8n^2 bytes (dominated by reading matrix A)

#include <iostream>
#include <chrono>
#include <cstdlib>
using namespace std;

void gemv(double* y, double* A, double* x, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = 0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i * n + j] * x[j];
        }
    }
}

int main() {
    const int n = 4096;  // 4K x 4K matrix
    double* A = (double*)aligned_alloc(32, n * n * sizeof(double));
    double* x = (double*)aligned_alloc(32, n * sizeof(double));
    double* y = (double*)aligned_alloc(32, n * sizeof(double));

    // Initialize
    for (int i = 0; i < n * n; i++) A[i] = 1.0;
    for (int i = 0; i < n; i++) x[i] = 1.0;

    auto start = chrono::high_resolution_clock::now();
    gemv(y, A, x, n);
    auto end = chrono::high_resolution_clock::now();

    double seconds = chrono::duration<double>(end - start).count();
    double flops = 2.0 * n * n;
    double bytes = 8.0 * n * n + 8.0 * n + 8.0 * n;  // A + x + y

    cout << "GEMV (y = A * x)" << endl;
    cout << "  n = " << n << endl;
    cout << "  Time: " << seconds * 1000 << " ms" << endl;
    cout << "  Performance: " << flops / seconds / 1e9 << " GFLOPS" << endl;
    cout << "  Bandwidth: " << bytes / seconds / 1e9 << " GB/s" << endl;
    cout << "  Operational Intensity: " << flops / bytes << " flops/byte" << endl;
    cout << "  y[0] = " << y[0] << " (expected: " << n << ")" << endl;

    free(A);
    free(x);
    free(y);
    return 0;
}
