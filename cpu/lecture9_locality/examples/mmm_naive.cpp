// mmm_naive.cpp - Naive matrix multiplication
// Compile: g++ -O1 -std=c++14 -fno-tree-vectorize mmm_naive.cpp -o mmm_naive
// Run: ./mmm_naive
//
// Simple triple-nested loop without any cache optimization.
// Poor cache performance due to column access pattern on B.

#include <iostream>
#include <chrono>
using namespace std;

const int N = 1024;
double A[N*N], B[N*N], C[N*N];

void mmm_naive() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i*N + j] += A[i*N + k] * B[k*N + j];
}

int main() {
    // Initialize
    for (int i = 0; i < N*N; i++) {
        A[i] = 1.0;
        B[i] = 1.0;
        C[i] = 0.0;
    }

    auto start = chrono::high_resolution_clock::now();
    mmm_naive();
    auto end = chrono::high_resolution_clock::now();

    double seconds = chrono::duration<double>(end - start).count();
    double gflops = 2.0 * N * N * N / 1e9 / seconds;

    cout << "Naive MMM: " << gflops << " GFLOPS" << endl;
    cout << "Time: " << seconds * 1000 << " ms" << endl;
    cout << "C[0] = " << C[0] << " (should be " << N << ")" << endl;

    return 0;
}
