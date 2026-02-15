// swizzle_demo.cpp - Demonstrates swizzling to avoid cache conflicts
// Compile: g++ -O1 -std=c++14 swizzle_demo.cpp -o swizzle_demo
// Run: ./swizzle_demo
//
// Swizzling remaps indices to spread column accesses across cache sets.
// Formula: j_physical = j XOR (i AND mask)
// This avoids conflict misses when accessing columns of power-of-2 matrices.

#include <iostream>
#include <chrono>
using namespace std;

const int N = 1024;  // Power of 2 - causes conflict misses
double A_normal[N][N];
double A_swizzled[N][N];

// Swizzle function: spreads column accesses across 4 cache sets
inline int swizzle(int i, int j) {
    return j ^ (i & 0x3);  // mask = 3 (binary: 11) -> 4 sets
}

// Store in swizzled layout
void store_swizzled(int i, int j, double val) {
    A_swizzled[i][swizzle(i, j)] = val;
}

// Read from swizzled layout
double read_swizzled(int i, int j) {
    return A_swizzled[i][swizzle(i, j)];
}

// Sum column without swizzling (conflict misses!)
double sum_column_normal(int col) {
    double sum = 0;
    for (int i = 0; i < N; i++)
        sum += A_normal[i][col];
    return sum;
}

// Sum column with swizzling (no conflicts)
double sum_column_swizzled(int col) {
    double sum = 0;
    for (int i = 0; i < N; i++)
        sum += read_swizzled(i, col);
    return sum;
}

int main() {
    // Initialize both arrays with same data
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double val = i * N + j;
            A_normal[i][j] = val;
            store_swizzled(i, j, val);
        }
    }

    // Show how swizzling changes physical layout
    cout << "Swizzle demonstration (first 8 rows, column 0):\n";
    cout << "Row | Logical col | Physical col | Value\n";
    cout << "----|-------------|--------------|------\n";
    for (int i = 0; i < 8; i++) {
        int logical_col = 0;
        int physical_col = swizzle(i, logical_col);
        cout << "  " << i << " |      " << logical_col
             << "      |       " << physical_col
             << "      | " << read_swizzled(i, logical_col) << "\n";
    }
    cout << "\n";

    // Benchmark: sum all columns
    const int COLS_TO_SUM = 64;

    auto t1 = chrono::high_resolution_clock::now();
    volatile double sum1 = 0;
    for (int c = 0; c < COLS_TO_SUM; c++)
        sum1 += sum_column_normal(c);
    auto t2 = chrono::high_resolution_clock::now();

    volatile double sum2 = 0;
    for (int c = 0; c < COLS_TO_SUM; c++)
        sum2 += sum_column_swizzled(c);
    auto t3 = chrono::high_resolution_clock::now();

    double normal_ms = chrono::duration<double, milli>(t2 - t1).count();
    double swizzled_ms = chrono::duration<double, milli>(t3 - t2).count();

    cout << "Column sum benchmark (summing " << COLS_TO_SUM << " columns):\n";
    cout << "Normal:   " << normal_ms << " ms\n";
    cout << "Swizzled: " << swizzled_ms << " ms\n";
    cout << "Speedup:  " << normal_ms / swizzled_ms << "x\n";

    return 0;
}
