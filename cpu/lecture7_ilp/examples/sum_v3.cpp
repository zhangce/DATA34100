// sum_v3.cpp - 8x unrolled reduction
// Compile: g++ -O1 sum_v3.cpp -o sum_v3
// Run: ./sum_v3
//
// Eight independent accumulators.
// Diminishing returns beyond 4x - memory bandwidth becomes the bottleneck.

#include <chrono>
#include <iostream>
using namespace std;

int main() {
    const int n = 100'000'000;
    double* a = new double[n];
    for (int i = 0; i < n; i++) a[i] = 1.0;

    auto start = chrono::high_resolution_clock::now();

    double s1=0, s2=0, s3=0, s4=0, s5=0, s6=0, s7=0, s8=0;
    for (int i = 0; i < n; i += 8) {
        s1 += a[i];   s2 += a[i+1]; s3 += a[i+2]; s4 += a[i+3];
        s5 += a[i+4]; s6 += a[i+5]; s7 += a[i+6]; s8 += a[i+7];
    }
    double sum = (s1+s2) + (s3+s4) + (s5+s6) + (s7+s8);

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Sum: " << sum << ", Time: " << ms << " ms\n";

    delete[] a;
    return 0;
}
