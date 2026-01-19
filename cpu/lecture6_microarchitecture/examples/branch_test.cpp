// branch_test.cpp - Branch prediction comparison
// Compile: g++ -O1 branch_test.cpp -o branch_test
// Run: ./branch_test
// Profile: perf stat -e branches,branch-misses ./branch_test

#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>

const int N = 100000000;  // 100 million elements
static int data[N];

// Version A: Branching (unpredictable on random data)
long version_a_branching(int threshold) {
    long count = 0;
    for (int i = 0; i < N; i++) {
        if (data[i] > threshold)
            count++;
    }
    return count;
}

// Version B: Branchless (no branch misprediction possible)
long version_b_branchless(int threshold) {
    long count = 0;
    for (int i = 0; i < N; i++) {
        count += (data[i] > threshold);
    }
    return count;
}

// Version C: Sort first, then branch (predictable pattern)
long version_c_sorted(int threshold) {
    std::sort(data, data + N);

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

    // Test Version A: Branching
    fill_random();
    auto start = std::chrono::high_resolution_clock::now();
    long count_a = version_a_branching(threshold);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_a = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Version A (branching):  count=" << count_a << ", time=" << time_a << " ms" << std::endl;

    // Test Version B: Branchless
    fill_random();
    start = std::chrono::high_resolution_clock::now();
    long count_b = version_b_branchless(threshold);
    end = std::chrono::high_resolution_clock::now();
    auto time_b = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Version B (branchless): count=" << count_b << ", time=" << time_b << " ms" << std::endl;

    // Test Version C: Sorted (includes sort time)
    fill_random();
    start = std::chrono::high_resolution_clock::now();
    long count_c = version_c_sorted(threshold);
    end = std::chrono::high_resolution_clock::now();
    auto time_c = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Version C (sorted):     count=" << count_c << ", time=" << time_c << " ms (includes sort)" << std::endl;

    // Test sorted without sort overhead
    // data is already sorted from version_c
    start = std::chrono::high_resolution_clock::now();
    long count_d = version_a_branching(threshold);
    end = std::chrono::high_resolution_clock::now();
    auto time_d = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Version C (pre-sorted): count=" << count_d << ", time=" << time_d << " ms (branch on sorted)" << std::endl;

    return 0;
}
