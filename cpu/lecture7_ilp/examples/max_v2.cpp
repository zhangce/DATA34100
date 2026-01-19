// max_v2.cpp - branchless max
// Compile: g++ -O1 max_v2.cpp -o max_v2
// Run: ./max_v2
//
// Ternary operator compiles to cmov (conditional move) - no branch.
// Still has dependency chain, but no branch mispredictions.
// Expected: ~2x speedup over max_v1 on random data.

#include <chrono>
#include <iostream>
#include <random>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];

    // Random data
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    double max_val = a[0];
    for (int i = 1; i < n; i++) {
        max_val = (a[i] > max_val) ? a[i] : max_val;  // Branchless!
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Max: " << max_val << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
