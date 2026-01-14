# GEMM Progressive Optimization Demo - Intel AVX2/FMA Version
#
# Requirements:
#   - GCC or Clang with AVX2/FMA support
#   - OpenBLAS: sudo apt install libopenblas-dev

CC = gcc
CFLAGS = -O3 -march=native -mavx2 -mfma -Wall -Wextra
LDFLAGS = -lopenblas -lm

TARGET = gemm_progressive

all: $(TARGET)

$(TARGET): gemm_progressive.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: $(TARGET)
	OPENBLAS_NUM_THREADS=1 ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
