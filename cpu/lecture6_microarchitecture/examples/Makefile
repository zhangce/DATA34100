# Makefile for Lecture 6: CPU Microarchitecture Examples
# Usage:
#   make          - build all examples
#   make run      - build and run all examples
#   make perf     - run with perf stat (requires Linux + perf)
#   make asm      - generate assembly files
#   make clean    - remove binaries and assembly files

CXX = g++
CXXFLAGS = -std=c++11
CXXFLAGS_O0 = $(CXXFLAGS) -O0 -g
CXXFLAGS_O1 = $(CXXFLAGS) -O1 -g
CXXFLAGS_O2 = $(CXXFLAGS) -O2 -g

TARGETS = add matrix_row matrix_col branch_random branch_branchless branch_sorted branch_presorted

.PHONY: all run perf asm clean

all: $(TARGETS)

# add.cpp: compiled without optimization to see unoptimized assembly
add: add.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# matrix examples: compiled with -O0 for fair comparison
matrix_row: matrix_row.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

matrix_col: matrix_col.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# branch tests: compiled with -O0 to preserve branch structure
branch_random: branch_random.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

branch_branchless: branch_branchless.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

branch_sorted: branch_sorted.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

branch_presorted: branch_presorted.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# Run all examples
run: all
	@echo "=== Running add ==="
	./add; echo "Return value: $$?"
	@echo ""
	@echo "=== Running matrix_row (good locality) ==="
	./matrix_row
	@echo ""
	@echo "=== Running matrix_col (bad locality) ==="
	./matrix_col
	@echo ""
	@echo "=== Running branch_random ==="
	./branch_random
	@echo ""
	@echo "=== Running branch_branchless ==="
	./branch_branchless
	@echo ""
	@echo "=== Running branch_sorted ==="
	./branch_sorted
	@echo ""
	@echo "=== Running branch_presorted ==="
	./branch_presorted

# Run with perf stat (Linux only)
perf: all
	@echo "=== Matrix Row (perf) ==="
	perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses ./matrix_row
	@echo ""
	@echo "=== Matrix Col (perf) ==="
	perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses ./matrix_col
	@echo ""
	@echo "=== Branch Random (perf) ==="
	perf stat -e branches,branch-misses,instructions,cycles ./branch_random
	@echo ""
	@echo "=== Branch Branchless (perf) ==="
	perf stat -e branches,branch-misses,instructions,cycles ./branch_branchless
	@echo ""
	@echo "=== Branch Sorted (perf) ==="
	perf stat -e branches,branch-misses,instructions,cycles ./branch_sorted
	@echo ""
	@echo "=== Branch Presorted (perf) ==="
	perf stat -e branches,branch-misses,instructions,cycles ./branch_presorted

# Generate assembly files
asm: add.s matrix_row.s matrix_col.s branch_random.s branch_branchless.s
	@echo "Assembly files generated"

add.s: add.cpp
	$(CXX) $(CXXFLAGS_O0) -S $< -o $@

matrix_row.s: matrix_row.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

matrix_col.s: matrix_col.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

branch_random.s: branch_random.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

branch_branchless.s: branch_branchless.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

clean:
	rm -f $(TARGETS) *.s *.o
