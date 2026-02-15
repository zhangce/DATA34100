// mmm_blocked.cpp - Blocked (tiled) matrix multiplication
// Compile: g++ -O1 -std=c++14 -fno-tree-vectorize mmm_blocked.cpp -o mmm_blocked
// Run: ./mmm_blocked
//
// Uses blocking/tiling to improve cache locality.
// Three b x b blocks should fit in L1 cache (32 KB = 4096 doubles).
// With b = 32: 3 * 32 * 32 = 3072 doubles < 4096

#include <iostream>
#include <chrono>
#include <cstring>
using namespace std;

const int N = 1024;
const int BLOCK = 32;  // Block size (tune for your L1 cache)
double A[N*N], B[N*N], C[N*N];

void mmm_naive() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i*N + j] += A[i*N + k] * B[k*N + j];
}

void mmm_blocked() {
    for (int ii = 0; ii < N; ii += BLOCK)
        for (int jj = 0; jj < N; jj += BLOCK)
            for (int kk = 0; kk < N; kk += BLOCK)
                // Multiply b x b blocks
                for (int i = ii; i < ii + BLOCK; i++)
                    for (int j = jj; j < jj + BLOCK; j++)
                        for (int k = kk; k < kk + BLOCK; k++)
                            C[i*N + j] += A[i*N + k] * B[k*N + j];
}

int main() {
    // Initialize
    for (int i = 0; i < N*N; i++) {
        A[i] = 1.0;
        B[i] = 1.0;
        C[i] = 0.0;
    }

    auto t1 = chrono::high_resolution_clock::now();
    mmm_naive();
    auto t2 = chrono::high_resolution_clock::now();

    memset(C, 0, sizeof(C));

    auto t3 = chrono::high_resolution_clock::now();
    mmm_blocked();
    auto t4 = chrono::high_resolution_clock::now();

    double gflops = 2.0 * N * N * N / 1e9;
    double naive_time = chrono::duration<double>(t2 - t1).count();
    double blocked_time = chrono::duration<double>(t4 - t3).count();

    cout << "Naive:   " << gflops / naive_time << " GFLOPS ("
         << naive_time * 1000 << " ms)" << endl;
    cout << "Blocked: " << gflops / blocked_time << " GFLOPS ("
         << blocked_time * 1000 << " ms)" << endl;
    cout << "Speedup: " << naive_time / blocked_time << "x" << endl;

    return 0;
}
