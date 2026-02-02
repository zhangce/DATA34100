# Makefile for Lecture 7: Instruction-Level Parallelism Examples
# Usage:
#   make          - build all examples
#   make run      - build and run all examples
#   make perf     - run with perf stat (requires Linux + perf)
#   make clean    - remove binaries

CXX = g++
CXXFLAGS = -std=c++14
CXXFLAGS_O0 = $(CXXFLAGS) -O0 -g
CXXFLAGS_O3 = $(CXXFLAGS) -O3 -g
CXXFLAGS_FAST = $(CXXFLAGS) -O3 -ffast-math -g

TARGETS = sum_v1 sum_v2 sum_v3 dot_v1 dot_v2 max_v1 max_v2 max_v3 count_v1 count_v2 count_v3

.PHONY: all run perf clean sum dot max count fast

all: $(TARGETS)

# Sum examples
sum_v1: sum_v1.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

sum_v2: sum_v2.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

sum_v3: sum_v3.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# Dot product examples
dot_v1: dot_v1.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

dot_v2: dot_v2.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# Max examples
max_v1: max_v1.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

max_v2: max_v2.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

max_v3: max_v3.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# Count examples
count_v1: count_v1.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

count_v2: count_v2.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

count_v3: count_v3.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# Build sum_v1 with -O3 -ffast-math for comparison
sum_v1_fast: sum_v1.cpp
	$(CXX) $(CXXFLAGS_FAST) $< -o $@

# Run groups
sum: sum_v1 sum_v2 sum_v3
	@echo "=== Sum Reduction Comparison ==="
	@./sum_v1
	@./sum_v2
	@./sum_v3

dot: dot_v1 dot_v2
	@echo "=== Dot Product Comparison ==="
	@./dot_v1
	@./dot_v2

max: max_v1 max_v2 max_v3
	@echo "=== Max Finding Comparison ==="
	@./max_v1
	@./max_v2
	@./max_v3

count: count_v1 count_v2 count_v3
	@echo "=== Count Elements Comparison ==="
	@./count_v1
	@./count_v2
	@./count_v3

# Run all examples
run: all
	@echo "=== Sum Reduction ==="
	@./sum_v1
	@./sum_v2
	@./sum_v3
	@echo ""
	@echo "=== Dot Product ==="
	@./dot_v1
	@./dot_v2
	@echo ""
	@echo "=== Max Finding ==="
	@./max_v1
	@./max_v2
	@./max_v3
	@echo ""
	@echo "=== Count Elements ==="
	@./count_v1
	@./count_v2
	@./count_v3

# Compare -O1 vs -O3 -ffast-math
fast: sum_v1 sum_v1_fast
	@echo "=== Compiler Optimization Comparison ==="
	@echo "sum_v1 (-O1):"
	@./sum_v1
	@echo "sum_v1 (-O3 -ffast-math):"
	@./sum_v1_fast

# Run with perf stat (Linux only)
perf: all
	@echo "=== sum_v1 (naive) ==="
	perf stat -e cycles,instructions,uops_executed.thread ./sum_v1
	@echo ""
	@echo "=== sum_v2 (4x unrolled) ==="
	perf stat -e cycles,instructions,uops_executed.thread ./sum_v2
	@echo ""
	@echo "=== IPC Comparison ==="
	@echo "sum_v1:"
	perf stat -e cycles,instructions ./sum_v1 2>&1 | grep -E "(cycles|instructions|insn per cycle)"
	@echo "sum_v2:"
	perf stat -e cycles,instructions ./sum_v2 2>&1 | grep -E "(cycles|instructions|insn per cycle)"

clean:
	rm -f $(TARGETS) sum_v1_fast
