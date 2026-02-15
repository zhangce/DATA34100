// max_v1.cpp - naive max with branch
// Compile: g++ -O1 max_v1.cpp -o max_v1
// Run: ./max_v1
//
// Two problems:
// 1. Dependency chain on max_val
// 2. Unpredictable branch on random data!

#include <chrono>
#include <iostream>
#include <random>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];

    // Random data for unpredictable branches
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    double max_val = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] > max_val) max_val = a[i];  // Branch!
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Max: " << max_val << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
