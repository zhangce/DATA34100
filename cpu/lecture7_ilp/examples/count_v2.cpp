// count_v2.cpp - branchless count
// Compile: g++ -O1 count_v2.cpp -o count_v2
// Run: ./count_v2
//
// Comparison returns 0 or 1, no branch needed.

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

    int count = 0;
    for (int i = 0; i < n; i++) {
        count += (a[i] > threshold);  // 0 or 1, no branch
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Count: " << count << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
