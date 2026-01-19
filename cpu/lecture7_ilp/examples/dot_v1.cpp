// dot_v1.cpp - naive dot product
// Compile: g++ -O1 dot_v1.cpp -o dot_v1
// Run: ./dot_v1
//
// Same problem as sum_v1: single accumulator creates dependency chain.

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

    double dot = 0;
    for (int i = 0; i < n; i++) {
        dot += a[i] * b[i];  // multiply then add to accumulator
    }

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Dot: " << dot << ", Time: " << ms << " ms\n";

    delete[] a;
    delete[] b;
    return 0;
}
