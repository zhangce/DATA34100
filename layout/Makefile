# CuTe Layout Examples Makefile
# Uses CUTLASS v3.0.0 with host-only stubs

CXX = clang++
CXXFLAGS = -std=c++17 -I cuda_stubs -I cutlass/include -Wno-format-security

all: layout

layout: layout.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f layout layout_test

run: layout
	./layout

.PHONY: all clean run
