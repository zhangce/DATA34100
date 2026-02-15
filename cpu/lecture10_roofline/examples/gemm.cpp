// gemm.cpp - Matrix-Matrix multiplication: C = A * B
// Compile: g++ -O3 -std=c++14 -fno-tree-vectorize gemm.cpp -o gemm
// Run: ./gemm
//
// BLAS Level 3 operation. Can be compute bound with blocking.
//
// Naive:
//   Operational Intensity: I = 2n^3 / 8n^3 = 1/4 flops/byte (memory bound)
//
// Blocked (b = 32):
//   Operational Intensity: I = 2n^3 / (2n^3/b) = b = 32 flops/byte (compute bound!)
//
// This demonstrates how blocking changes operational intensity.

#include <iostream>
#include <chrono>
#include <cstring>
using namespace std;

const int N = 1024;
const int BLOCK = 32;

double A[N * N], B[N * N], C[N * N];

void gemm_naive() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i * N + j] += A[i * N + k] * B[k * N + j];
}

void gemm_blocked() {
    for (int ii = 0; ii < N; ii += BLOCK)
        for (int jj = 0; jj < N; jj += BLOCK)
            for (int kk = 0; kk < N; kk += BLOCK)
                for (int i = ii; i < ii + BLOCK; i++)
                    for (int j = jj; j < jj + BLOCK; j++)
                        for (int k = kk; k < kk + BLOCK; k++)
                            C[i * N + j] += A[i * N + k] * B[k * N + j];
}

int main() {
    // Initialize
    for (int i = 0; i < N * N; i++) {
        A[i] = 1.0;
        B[i] = 1.0;
        C[i] = 0.0;
    }

    double flops = 2.0 * N * N * N;

    // Naive GEMM
    auto t1 = chrono::high_resolution_clock::now();
    gemm_naive();
    auto t2 = chrono::high_resolution_clock::now();
    double naive_sec = chrono::duration<double>(t2 - t1).count();
    double naive_gflops = flops / naive_sec / 1e9;

    memset(C, 0, sizeof(C));

    // Blocked GEMM
    auto t3 = chrono::high_resolution_clock::now();
    gemm_blocked();
    auto t4 = chrono::high_resolution_clock::now();
    double blocked_sec = chrono::duration<double>(t4 - t3).count();
    double blocked_gflops = flops / blocked_sec / 1e9;

    // Calculate operational intensities
    double bytes_naive = 8.0 * N * N * N;  // Pessimistic: re-read each time
    double bytes_blocked = 2.0 * N * N * N / BLOCK * 8;  // Optimistic with blocking

    cout << "GEMM (C = A * B), n = " << N << endl;
    cout << endl;
    cout << "Naive:" << endl;
    cout << "  Time: " << naive_sec * 1000 << " ms" << endl;
    cout << "  Performance: " << naive_gflops << " GFLOPS" << endl;
    cout << "  Theoretical I: 0.25 flops/byte (memory bound)" << endl;
    cout << endl;
    cout << "Blocked (b = " << BLOCK << "):" << endl;
    cout << "  Time: " << blocked_sec * 1000 << " ms" << endl;
    cout << "  Performance: " << blocked_gflops << " GFLOPS" << endl;
    cout << "  Theoretical I: " << BLOCK << " flops/byte (compute bound)" << endl;
    cout << endl;
    cout << "Speedup: " << naive_sec / blocked_sec << "x" << endl;
    cout << "C[0] = " << C[0] << " (expected: " << N << ")" << endl;

    return 0;
}
