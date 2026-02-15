# Makefile for locality/cache examples
# Assumes Linux

CXX = g++
# Use -O1 to avoid aggressive optimizations that hide locality effects
# Use -fno-tree-vectorize to disable auto-vectorization for MMM
CXXFLAGS = -O1 -std=c++14
NOVECT = -fno-tree-vectorize

TARGETS = locality_test mmm_naive mmm_blocked swizzle_demo

all: $(TARGETS)

# Locality test (row vs column access)
locality_test: locality_test.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

locality: locality_test
	@echo "=== Row-major vs Column-major access ==="
	./locality_test

# Matrix multiplication - naive
mmm_naive: mmm_naive.cpp
	$(CXX) $(CXXFLAGS) $(NOVECT) $< -o $@

naive: mmm_naive
	@echo "=== Naive MMM ==="
	./mmm_naive

# Matrix multiplication - blocked
mmm_blocked: mmm_blocked.cpp
	$(CXX) $(CXXFLAGS) $(NOVECT) $< -o $@

blocked: mmm_blocked
	@echo "=== Blocked MMM ==="
	./mmm_blocked

# Combined MMM comparison
mmm: mmm_blocked
	@echo "=== MMM: Naive vs Blocked ==="
	./mmm_blocked

# Swizzle demonstration
swizzle_demo: swizzle_demo.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

swizzle: swizzle_demo
	@echo "=== Swizzle Demo ==="
	./swizzle_demo

# Run all examples
run: locality mmm swizzle

# Build with -O3 for comparison
fast: CXXFLAGS = -O3 -std=c++14
fast: clean all

# Performance counters (requires perf)
perf_locality: locality_test
	@echo "=== perf: row-major access ==="
	perf stat -e L1-dcache-load-misses,L1-dcache-loads ./locality_test

perf_mmm: mmm_blocked
	@echo "=== perf: MMM cache misses ==="
	perf stat -e L1-dcache-load-misses,LLC-load-misses ./mmm_blocked

perf: perf_locality perf_mmm

clean:
	rm -f $(TARGETS)

.PHONY: all locality naive blocked mmm swizzle run fast perf perf_locality perf_mmm clean
