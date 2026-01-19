// dot_v2.cpp - 4x unrolled dot product
// Compile: g++ -O1 dot_v2.cpp -o dot_v2
// Run: ./dot_v2
//
// Four independent accumulators break the dependency chain.
// Expected: ~3x speedup over dot_v1.

#include <chrono>
#include <iostream>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    double* b = new double[n];
    for (int i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 1.0;
    }

    auto start = chrono::high_resolution_clock::now();

    double d1=0, d2=0, d3=0, d4=0;
    for (int i = 0; i < n; i += 4) {
        d1 += a[i]   * b[i];
        d2 += a[i+1] * b[i+1];
        d3 += a[i+2] * b[i+2];
        d4 += a[i+3] * b[i+3];
    }
    double dot = d1 + d2 + d3 + d4;

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Dot: " << dot << ", Time: " << ms << " ms\n";

    delete[] a;
    delete[] b;
    return 0;
}
