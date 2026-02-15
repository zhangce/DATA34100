// sum_v1.cpp - naive reduction (single accumulator)
// Compile: g++ -O1 sum_v1.cpp -o sum_v1
// Run: ./sum_v1
// Profile: perf stat -e cycles,instructions ./sum_v1
//
// This creates a dependency chain: each add must wait for the previous.
// With 4-cycle add latency, we get ~0.25 adds/cycle.

#include <chrono>
#include <iostream>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    for (int i = 0; i < n; i++) a[i] = 1.0;

    auto start = chrono::high_resolution_clock::now();

    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i];  // Dependency chain!
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Sum: " << sum << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
