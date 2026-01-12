# GEMM Progressive Optimization Exercise
# Requires: Apple Silicon Mac with Accelerate framework

CC = clang
CFLAGS = -O3 -mcpu=apple-m1
LDFLAGS = -framework Accelerate

all: gemm_progressive

gemm_progressive: gemm_progressive.c amx.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: gemm_progressive
	VECLIB_MAXIMUM_THREADS=1 ./gemm_progressive

clean:
	rm -f gemm_progressive

.PHONY: all run clean
