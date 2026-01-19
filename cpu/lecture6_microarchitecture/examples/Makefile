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

TARGETS = add matrix_row matrix_col branch_test

.PHONY: all run perf asm clean

all: $(TARGETS)

# add.cpp: compiled without optimization to see unoptimized assembly
add: add.cpp
	$(CXX) $(CXXFLAGS_O0) $< -o $@

# matrix examples: compiled with -O1 for fair comparison
matrix_row: matrix_row.cpp
	$(CXX) $(CXXFLAGS_O1) $< -o $@

matrix_col: matrix_col.cpp
	$(CXX) $(CXXFLAGS_O1) $< -o $@

# branch test: compiled with -O1 to preserve branch structure
branch_test: branch_test.cpp
	$(CXX) $(CXXFLAGS_O1) $< -o $@

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
	@echo "=== Running branch_test ==="
	./branch_test

# Run with perf stat (Linux only)
perf: all
	@echo "=== Matrix Row (perf) ==="
	perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses ./matrix_row
	@echo ""
	@echo "=== Matrix Col (perf) ==="
	perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses ./matrix_col
	@echo ""
	@echo "=== Branch Test (perf) ==="
	perf stat -e branches,branch-misses,instructions,cycles ./branch_test

# Generate assembly files
asm: add.s matrix_row.s matrix_col.s branch_test.s
	@echo "Assembly files generated: add.s, matrix_row.s, matrix_col.s, branch_test.s"

add.s: add.cpp
	$(CXX) $(CXXFLAGS_O0) -S $< -o $@

matrix_row.s: matrix_row.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

matrix_col.s: matrix_col.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

branch_test.s: branch_test.cpp
	$(CXX) $(CXXFLAGS_O1) -S $< -o $@

clean:
	rm -f $(TARGETS) *.s *.o
