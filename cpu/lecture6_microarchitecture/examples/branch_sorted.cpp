// branch_sorted.cpp - Sort first, then branch (predictable pattern)
// Compile: g++ -O1 branch_sorted.cpp -o branch_sorted
// Profile: perf stat -e branches,branch-misses,instructions,cycles ./branch_sorted

#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>

const int N = 100000000;  // 100 million elements
static int data[N];

long count_above_threshold(int threshold) {
    long count = 0;
    for (int i = 0; i < N; i++) {
        if (data[i] > threshold)
            count++;
    }
    return count;
}

void fill_random() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (int i = 0; i < N; i++) {
        data[i] = dist(rng);
    }
}

int main() {
    const int threshold = 128;  // ~50% of values will be above

    fill_random();

    // Sort the data first (includes sort time in measurement)
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(data, data + N);
    long count = count_above_threshold(threshold);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Branching (sorted data): count=" << count << ", time=" << time_ms << " ms (includes sort)" << std::endl;

    return 0;
}
