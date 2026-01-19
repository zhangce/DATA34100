// max_v3.cpp - branchless + 4x unrolled max
// Compile: g++ -O1 max_v3.cpp -o max_v3
// Run: ./max_v3
//
// Combines branchless code with multiple accumulators.
// Expected: ~7x speedup over max_v1 on random data.

#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];

    // Random data
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    double m1=a[0], m2=a[1], m3=a[2], m4=a[3];
    for (int i = 4; i < n; i += 4) {
        m1 = (a[i]   > m1) ? a[i]   : m1;
        m2 = (a[i+1] > m2) ? a[i+1] : m2;
        m3 = (a[i+2] > m3) ? a[i+2] : m3;
        m4 = (a[i+3] > m4) ? a[i+3] : m4;
    }
    double max_val = max(max(m1,m2), max(m3,m4));

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Max: " << max_val << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
