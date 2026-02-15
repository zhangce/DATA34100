// sum_v2.cpp - 4x unrolled reduction
// Compile: g++ -O1 sum_v2.cpp -o sum_v2
// Run: ./sum_v2
//
// Four independent accumulators break the dependency chain.
// Each chain has internal dependencies, but chains are independent.
// Expected: ~3.6x speedup over sum_v1.

#include <chrono>
#include <iostream>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    for (int i = 0; i < n; i++) a[i] = 1.0;

    auto start = chrono::high_resolution_clock::now();

    double sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
    for (int i = 0; i < n; i += 4) {
        sum1 += a[i];
        sum2 += a[i+1];
        sum3 += a[i+2];
        sum4 += a[i+3];
    }
    double sum = sum1 + sum2 + sum3 + sum4;

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Sum: " << sum << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
