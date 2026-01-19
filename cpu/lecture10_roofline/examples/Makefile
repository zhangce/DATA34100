# Makefile for Roofline model examples
# Assumes Linux

CXX = g++
CXXFLAGS = -O3 -std=c++14 -mavx2 -mfma
NOVECT = -fno-tree-vectorize

TARGETS = daxpy dot gemv gemm stencil

all: $(TARGETS)

# DAXPY - BLAS Level 1, memory bound (I = 1/12)
daxpy: daxpy.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Dot product - BLAS Level 1, memory bound (I = 1/8)
dot: dot.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# GEMV - BLAS Level 2, memory bound (I = 1/4)
gemv: gemv.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# GEMM - BLAS Level 3, can be compute bound with blocking
# Use -fno-tree-vectorize to see algorithmic effect of blocking
gemm: gemm.cpp
	$(CXX) $(CXXFLAGS) $(NOVECT) $< -o $@

# 2D Stencil - memory bound (I = 1/4)
stencil: stencil.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Run individual examples
run_daxpy: daxpy
	@echo "=== DAXPY (memory bound, I = 1/12) ==="
	./daxpy

run_dot: dot
	@echo "=== Dot Product (memory bound, I = 1/8) ==="
	./dot

run_gemv: gemv
	@echo "=== GEMV (memory bound, I = 1/4) ==="
	./gemv

run_gemm: gemm
	@echo "=== GEMM (naive vs blocked) ==="
	./gemm

run_stencil: stencil
	@echo "=== 2D Stencil (memory bound, I = 1/4) ==="
	./stencil

# Run all examples
run: run_daxpy run_dot run_gemv run_gemm run_stencil

# Show operational intensities
intensity: all
	@echo "=== Operational Intensity Comparison ==="
	@echo ""
	@./daxpy | grep -E "(Operational|Performance)"
	@echo ""
	@./dot | grep -E "(Operational|Performance)"
	@echo ""
	@./gemv | grep -E "(Operational|Performance)"
	@echo ""
	@./stencil | grep -E "(Operational|Performance)"
	@echo ""
	@echo "=== GEMM shows effect of blocking on I ==="
	@./gemm | grep -E "(Theoretical|Performance)"

# Performance counters (requires perf)
perf: all
	@echo "=== perf: DAXPY ==="
	perf stat -e cycles,instructions,L1-dcache-load-misses ./daxpy
	@echo ""
	@echo "=== perf: GEMM ==="
	perf stat -e cycles,instructions,L1-dcache-load-misses ./gemm

clean:
	rm -f $(TARGETS)

.PHONY: all run run_daxpy run_dot run_gemv run_gemm run_stencil intensity perf clean
