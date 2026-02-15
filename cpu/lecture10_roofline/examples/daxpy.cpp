// daxpy.cpp - DAXPY: y = alpha * x + y
// Compile: g++ -O3 -std=c++14 -mavx2 -mfma daxpy.cpp -o daxpy
// Run: ./daxpy
//
// Classic BLAS Level 1 operation. Memory bound.
// Operational Intensity: I = 2n / 24n = 1/12 ~ 0.083 flops/byte
//
// Work: 2n flops (1 mul + 1 add per element)
// Data: 24n bytes (read x: 8n, read y: 8n, write y: 8n)

#include <iostream>
#include <chrono>
#include <cstdlib>
using namespace std;

void daxpy(double* y, double alpha, double* x, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = alpha * x[i] + y[i];
    }
}

int main() {
    const int n = 100'000'000;  // 100M elements
    double* x = (double*)aligned_alloc(32, n * sizeof(double));
    double* y = (double*)aligned_alloc(32, n * sizeof(double));
    double alpha = 2.0;

    // Initialize
    for (int i = 0; i < n; i++) {
        x[i] = 1.0;
        y[i] = 1.0;
    }

    auto start = chrono::high_resolution_clock::now();
    daxpy(y, alpha, x, n);
    auto end = chrono::high_resolution_clock::now();

    double seconds = chrono::duration<double>(end - start).count();
    double flops = 2.0 * n;
    double bytes = 24.0 * n;  // Read x, read y, write y

    cout << "DAXPY (y = alpha*x + y)" << endl;
    cout << "  n = " << n << endl;
    cout << "  Time: " << seconds * 1000 << " ms" << endl;
    cout << "  Performance: " << flops / seconds / 1e9 << " GFLOPS" << endl;
    cout << "  Bandwidth: " << bytes / seconds / 1e9 << " GB/s" << endl;
    cout << "  Operational Intensity: " << flops / bytes << " flops/byte" << endl;
    cout << "  y[0] = " << y[0] << " (expected: 3.0)" << endl;

    free(x);
    free(y);
    return 0;
}
