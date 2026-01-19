# Makefile for SIMD examples
# Assumes Linux with AVX2 and FMA support

CXX = g++
CXXFLAGS = -O1 -std=c++14

# SIMD flags
SIMD_FLAGS = -mavx2 -mfma

TARGETS = add_v1 add_v2 sum_v1 sum_v2 dot_v1 dot_v2 conditional saxpy

all: $(TARGETS)

# Vector addition
add_v1: add_v1.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

add_v2: add_v2.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

add: add_v1 add_v2
	@echo "=== Scalar add ==="
	./add_v1
	@echo "=== SIMD add ==="
	./add_v2

# Sum reduction
sum_v1: sum_v1.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

sum_v2: sum_v2.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

sum: sum_v1 sum_v2
	@echo "=== SIMD sum (1 accumulator) ==="
	./sum_v1
	@echo "=== SIMD sum (2 accumulators) ==="
	./sum_v2

# Dot product
dot_v1: dot_v1.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

dot_v2: dot_v2.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

dot: dot_v1 dot_v2
	@echo "=== Scalar dot ==="
	./dot_v1
	@echo "=== SIMD dot (FMA) ==="
	./dot_v2

# Conditional/masking
conditional: conditional.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

cond: conditional
	@echo "=== Conditional (scalar vs SIMD masking) ==="
	./conditional

# SAXPY
saxpy: saxpy.cpp
	$(CXX) $(CXXFLAGS) $(SIMD_FLAGS) $< -o $@

sax: saxpy
	@echo "=== SAXPY (scalar vs SIMD) ==="
	./saxpy

# Run all examples
run: add sum dot cond sax

# Build with -O3 for comparison
fast: CXXFLAGS = -O3 -std=c++14
fast: clean all

# Performance counters (requires perf)
perf: all
	@echo "=== perf: scalar add ==="
	perf stat -e cycles,instructions,cache-misses ./add_v1
	@echo "=== perf: SIMD add ==="
	perf stat -e cycles,instructions,cache-misses ./add_v2

clean:
	rm -f $(TARGETS)

.PHONY: all add sum dot cond sax run fast perf clean
