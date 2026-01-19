// locality_test.cpp - Row-major vs Column-major access pattern
// Compile: g++ -O1 -std=c++14 locality_test.cpp -o locality_test
// Run: ./locality_test
//
// Demonstrates the impact of spatial locality on cache performance.
// In C/C++, 2D arrays are stored in row-major order.

#include <iostream>
#include <chrono>
using namespace std;

const int M = 4096, N = 4096;
double a[M][N];

// Row-major traversal (good locality, stride = 1)
double sum_rows() {
    double sum = 0;
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            sum += a[i][j];
    return sum;
}

// Column-major traversal (poor locality, stride = N)
double sum_cols() {
    double sum = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < M; i++)
            sum += a[i][j];
    return sum;
}

int main() {
    // Initialize
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = 1.0;

    auto t1 = chrono::high_resolution_clock::now();
    volatile double r1 = sum_rows();
    auto t2 = chrono::high_resolution_clock::now();
    volatile double r2 = sum_cols();
    auto t3 = chrono::high_resolution_clock::now();

    cout << "Row-major: "
         << chrono::duration<double,milli>(t2-t1).count() << " ms\n";
    cout << "Col-major: "
         << chrono::duration<double,milli>(t3-t2).count() << " ms\n";

    return 0;
}
