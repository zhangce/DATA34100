// branch_random.cpp - Branching on random (unpredictable) data
// Compile: g++ -O1 branch_random.cpp -o branch_random
// Profile: perf stat -e branches,branch-misses,instructions,cycles ./branch_random

#include <iostream>
#include <chrono>
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
    auto start = std::chrono::high_resolution_clock::now();
    long count = count_above_threshold(threshold);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Branching (random data): count=" << count << ", time=" << time_ms << " ms" << std::endl;

    return 0;
}
