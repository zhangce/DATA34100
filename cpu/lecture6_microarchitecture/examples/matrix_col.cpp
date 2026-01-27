// matrix_col.cpp - Column-major matrix traversal (bad locality)
// Compile: g++ -O1 matrix_col.cpp -o matrix_col
// Run: ./matrix_col
// Profile: perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_col

#include <iostream>
#include <chrono>

int main() {
    const int N = 1000;
    static int matrix[N][N];  // static to avoid stack overflow

    // Initialize matrix
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            matrix[i][j] = i + j;

    long sum = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Column-major traversal: access [0][0], [1][0], [2][0], ...
    // Stride = 4000 bytes (N * sizeof(int))
    // Each access jumps 62 cache lines - terrible locality!
    for (int iter = 0; iter < 1000; iter++) {
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < N; i++) {
                sum += matrix[i][j];
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Sum: " << sum << ", Time: " << duration.count() << " ms" << std::endl;

    return 0;
}
