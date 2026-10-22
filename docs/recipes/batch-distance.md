# Compare a collection of sequences

Use `lzdistance` when your sequences are already in files and you want its information-distance and shuffle-distance matrices. Use a Python loop when you only need NID or already hold the sequences in memory.

Before either route, standardize encoding and preprocessing. Keep a manifest linking each matrix row and column to a record identifier.

## File input: one sequence per line

Create a small input file:

```sh
printf '0101010101010101\n0011001100110011\n0100111010001101\n' > sequences.txt
lzdistance sequences.txt -I text -o distances.json
```

The CLI reads a file source with multiline mode enabled. A self-comparison emits square matrices; two input sources produce a matrix with the first source on rows and the second source on columns:

```sh
lzdistance reference.txt query.txt -I text -S text -o cross-distances.json
```

Inspect the result:

```python
import json
from pathlib import Path

report = json.loads(Path("distances.json").read_text())
print("information distance:")
for row in report["information_distance"]:
    print(row)
print("shuffle distance:")
for row in report["shuffle_information_distance"]:
    print(row)
```

These are two different quantities. The shuffle-distance matrix is not the same as taking differences between individual sequences' `emc()` scores. See the [CLI reference](../cli/lzdistance.md) for formulas, dimensions and all output fields.

## Directory input

```sh
lzdistance sequences/ -I text -o distances.json
```

The reader sorts regular files by path, skips files ending in `.json` or `.log`, and takes the **first sequence from each file**. It does not recursively enumerate subdirectories. Keep only intended inputs in that directory and store the sorted path list with your results. For a multiline file, use the file route instead.

## Python: compute a distance matrix { #python-compute-exactly-the-matrix-you-need }

```python
import lzcomplexity as lz

names = ["alternating", "period-four", "mixed"]
sequences = [
    b"01" * 64,
    b"0011" * 32,
    b"0100111010001101" * 8,
]

# Compute every cell, including the actual finite-sequence self-distance.
matrix = [[lz.nid(a, b) for b in sequences] for a in sequences]
print(names)
for row in matrix:
    print(row)
```

`nid(a, b)` evaluates both concatenation orders, giving a symmetric measure. Self-distances can be nonzero for finite sequences. If a downstream method requires a zero diagonal, apply that transformation separately and record it with your matrix.

For a large self-comparison you can compute only `i < j`, mirror the values and calculate the diagonal separately. The number of distinct off-diagonal pairs is `n*(n-1)/2`; a cross-comparison requires `n*m` pairs. Each NID call performs four LZ factorizations. Time a representative subset before committing to a large batch.

## Transformations and ranges

`-r`, `-b`, `-a` and `-y` select transformations of the second operand before comparison. For example, `-r` uses its reversal only; `-b` minimizes across original, reversal, bit-flipped and reversed bit-flipped variants. The DNA mode is not a full reverse-complement matcher. Keep one strategy across comparable matrices and read the [exact transformation table](../cli/lzdistance.md).

The `-i` and `-s` ranges gate computations without shrinking the matrix. Uncomputed cells remain `0.0`, which is indistinguishable from a genuine zero in the JSON. Store the selected ranges as metadata; do not reconstruct a missing-cell mask by testing for zero.

`-g` reports differences between the two concatenation orders. Use a positive threshold, such as `-g 1`, to include `directed_matrix` in the output. See the [graph options](../cli/lzdistance.md#concatenation-order-graph-g) for details.

## Parallelism

The core uses Rayon internally. Set the CLI worker count with `-j`. In Python, configure `RAYON_NUM_THREADS` before the first native call; the `jobs` argument has no effect. The extension holds the GIL during each call, so use processes for independent Python analyses.

For process-based batches, set `RAYON_NUM_THREADS` before starting each worker's first native call and budget the total workers across both levels. Benchmark your workload: process startup, copying inputs and nested parallelism can outweigh any benefit for small jobs. See [determinism and execution](../project/determinism.md).

Interpret distances in terms of the supplied symbolic sequences and their recurring patterns. Match encodings, sequence lengths and preprocessing across the collection.
