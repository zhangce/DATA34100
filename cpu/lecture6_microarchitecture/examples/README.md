# Lecture 6: CPU Microarchitecture Examples

## Files

| File | Description |
|------|-------------|
| `add.cpp` | Simple addition to examine unoptimized assembly |
| `matrix_row.cpp` | Row-major traversal (good spatial locality) |
| `matrix_col.cpp` | Column-major traversal (bad spatial locality) |
| `branch_test.cpp` | Branch prediction comparison |

## Quick Start

```bash
make        # Build all
make run    # Run all examples
```

## Individual Examples

### 1. add.cpp - Assembly Inspection

```bash
make add
./add
echo $?                    # Should print 8

# View assembly
g++ -O0 -S add.cpp -o add.s
cat add.s

# Or use objdump
objdump -d add | less
```

### 2. matrix_row.cpp vs matrix_col.cpp - Cache Locality

```bash
make matrix_row matrix_col

./matrix_row    # ~2-3 ms (good locality)
./matrix_col    # ~15-20 ms (bad locality)
```

With perf (Linux):
```bash
# Row-major: expect ~6.25% L1 miss rate
perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_row

# Column-major: expect ~100% L1 miss rate
perf stat -e L1-dcache-loads,L1-dcache-load-misses ./matrix_col
```

### 3. branch_test.cpp - Branch Prediction

```bash
make branch_test
./branch_test
```

Expected output:
```
Version A (branching):  ~50-60 ms   (unpredictable branches)
Version B (branchless): ~8-10 ms    (no branches)
Version C (sorted):     ~15-20 ms   (includes sort time)
Version C (pre-sorted): ~8-10 ms    (predictable branches)
```

With perf:
```bash
perf stat -e branches,branch-misses ./branch_test
```

## Makefile Targets

```bash
make          # Build all examples
make run      # Build and run all
make perf     # Run with perf stat (Linux)
make asm      # Generate .s assembly files
make clean    # Remove binaries
```

## Requirements

- g++ (GCC)
- perf (for profiling, Linux only)
- For macOS: use Instruments or dtrace instead of perf
