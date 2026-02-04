// dot_v1.cpp - scalar dot product
// Compile: g++ -O1 dot_v1.cpp -o dot_v1
// Run: ./dot_v1

#include <chrono>
#include <iostream>
#include <cstdlib>
using namespace std;

double dot_scalar(double* a, double* b, int n) {
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

int main() {
    const int n = 100'000'000;
    double* a = (double*)aligned_alloc(32, n * sizeof(double));
    double* b = (double*)aligned_alloc(32, n * sizeof(double));
    for (int i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 1.0;
    }

    auto start = chrono::high_resolution_clock::now();
    double dot = dot_scalar(a, b, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "Scalar dot: " << dot << ", Time: " << ms << " ms" << endl;

    free(a); free(b);
    return 0;
}
