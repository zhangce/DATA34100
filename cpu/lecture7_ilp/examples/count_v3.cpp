// count_v3.cpp - branchless + 4x unrolled count
// Compile: g++ -O1 count_v3.cpp -o count_v3
// Run: ./count_v3
//
// Combines branchless with multiple accumulators for ILP.

#include <chrono>
#include <iostream>
#include <random>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    const double threshold = 0.5;

    // Random data (same seed as v1 for fair comparison)
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    int c1 = 0, c2 = 0, c3 = 0, c4 = 0;
    for (int i = 0; i < n; i += 4) {
        c1 += (a[i]   > threshold);
        c2 += (a[i+1] > threshold);
        c3 += (a[i+2] > threshold);
        c4 += (a[i+3] > threshold);
    }
    int count = c1 + c2 + c3 + c4;

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Count: " << count << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
