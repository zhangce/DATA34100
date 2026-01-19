// add_v1.cpp - scalar vector addition
// Compile: g++ -O1 add_v1.cpp -o add_v1
// Run: ./add_v1

#include <chrono>
#include <iostream>
#include <cstdlib>
using namespace std;

void add_scalar(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

int main() {
    const int n = 100'000'000;
    double* a = (double*)aligned_alloc(32, n * sizeof(double));
    double* b = (double*)aligned_alloc(32, n * sizeof(double));
    double* c = (double*)aligned_alloc(32, n * sizeof(double));

    for (int i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    auto start = chrono::high_resolution_clock::now();
    add_scalar(a, b, c, n);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "Scalar add: " << ms << " ms, c[0]=" << c[0] << endl;

    free(a); free(b); free(c);
    return 0;
}
