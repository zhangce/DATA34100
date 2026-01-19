// matrix_row.cpp - Row-major matrix traversal (good locality)
// Compile: g++ -O1 matrix_row.cpp -o matrix_row
// Run: ./matrix_row
// Profile: perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_row

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

    // Row-major traversal: access [0][0], [0][1], [0][2], ...
    // Stride = 4 bytes (sizeof(int))
    // Sequential access - excellent spatial locality!
    for (int iter = 0; iter < 1000; iter++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                sum += matrix[i][j];
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Sum: " << sum << ", Time: " << duration.count() << " ms" << std::endl;

    return 0;
}
