// dot.cpp - Dot product: s = x^T * y
// Compile: g++ -O3 -std=c++14 -mavx2 -mfma dot.cpp -o dot
// Run: ./dot
//
// Classic BLAS Level 1 operation. Memory bound.
// Operational Intensity: I = 2n / 16n = 1/8 = 0.125 flops/byte
//
// Work: 2n flops (1 mul + 1 add per element)
// Data: 16n bytes (read x: 8n, read y: 8n)

#include <iostream>
#include <chrono>
#include <cstdlib>
using namespace std;

double dot(double* x, double* y, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) {
        s += x[i] * y[i];
    }
    return s;
}

int main() {
    const int n = 100'000'000;  // 100M elements
    double* x = (double*)aligned_alloc(32, n * sizeof(double));
    double* y = (double*)aligned_alloc(32, n * sizeof(double));

    // Initialize
    for (int i = 0; i < n; i++) {
        x[i] = 1.0;
        y[i] = 1.0;
    }

    auto start = chrono::high_resolution_clock::now();
    double result = dot(x, y, n);
    auto end = chrono::high_resolution_clock::now();

    double seconds = chrono::duration<double>(end - start).count();
    double flops = 2.0 * n;
    double bytes = 16.0 * n;  // Read x, read y

    cout << "Dot Product (s = x^T * y)" << endl;
    cout << "  n = " << n << endl;
    cout << "  Time: " << seconds * 1000 << " ms" << endl;
    cout << "  Performance: " << flops / seconds / 1e9 << " GFLOPS" << endl;
    cout << "  Bandwidth: " << bytes / seconds / 1e9 << " GB/s" << endl;
    cout << "  Operational Intensity: " << flops / bytes << " flops/byte" << endl;
    cout << "  Result = " << result << " (expected: " << (double)n << ")" << endl;

    free(x);
    free(y);
    return 0;
}
