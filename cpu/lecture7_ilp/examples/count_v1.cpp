// count_v1.cpp - naive count with branch
// Compile: g++ -O1 count_v1.cpp -o count_v1
// Run: ./count_v1
//
// Unpredictable branch on random data causes mispredictions.

#include <chrono>
#include <iostream>
#include <random>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    const double threshold = 0.5;

    // Random data
    mt19937 rng(42);
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) a[i] = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    int count = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] > threshold) count++;
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Count: " << count << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
