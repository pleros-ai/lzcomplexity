# `lzdistance`

Compute all-pairs distance matrices from one or two sequence sources. A source
can be a file containing multiple sequences or a directory of files. The tool
always writes both an LZ-based information-distance matrix and a separate
shuffle-distance matrix.

```text
lzdistance [OPTIONS] [first source] [second source]
```

```sh
printf '01010101\n00110011\n' > trials.txt
lzdistance trials.txt -I TXT -o distances.json
```

One source compares the set with itself. Two sources produce a rectangular
matrix: rows correspond to the first source, columns to the second.

## Source loading and order

File sources use multiline mode: text lines, CSV columns, FASTA records or
image rows become sequences according to the selected [format](formats.md).
Raw binary remains one expanded bitstream.

Directories are traversed nonrecursively. Regular files are sorted by path;
files whose extensions are exactly `json` or `log` are excluded. Each retained
file is read in single-sequence mode and contributes its **first** sequence.
Format selection applies to the source as a whole. For a directory of FASTA
files, specify `-I FASTA` rather than relying on filename detection.

An omitted or empty second dataset falls back to self-comparison. Keep a
record of the sorted file list alongside matrix output when indices must be
mapped back to filenames; the JSON does not include a per-row label array.

## Options

| Short / long | Value | Default | Current behavior |
|---|---|---|---|
| positional first source | path | required for analysis | First file or directory. |
| positional second source | path | omitted | Second file or directory; omitted means self-comparison. |
| `-I`, `--first-format` | format | `AUTO` | First source's reader. |
| `-S`, `--second-format` | format | `AUTO` | Second source's reader. |
| `-i`, `--first` | `start:end` | all | Gate first-source rows, using one-based indices. |
| `-s`, `--second` | `start:end` | all | Gate second-source columns. |
| `-j`, `--jobs` | unsigned integer | Rayon default | Positive values set the native worker count. |
| `-o`, `--output` | path | `<first source>.lzdist.json` | Output file; the suffix is appended. |
| `-L`, `--logs` | flag | off | Print dimensions and output path. |
| `-r`, `--reverse` | flag | off | Compare against the reversed second operand. |
| `-b`, `--binary` | flag | off | Search original/reverse/ASCII-bit-flipped second operands. |
| `-a`, `--adn` | flag | off | Search the legacy six DNA variants described below. |
| `-y`, `--trajectory` | flag | off | Search eight digit-code rotations and their reversals. |
| `-g`, `--get-direction` | optional integer threshold | absent | Compute the concatenation-order graph; see below. |
| `-l`, `--log-base` | unsigned integer | automatic | Accepted; these count-ratio matrices do not depend on logarithm units. |
| `-p`, `--partitions` | signed integer | `2` | Compatibility field; no current computation uses it. |
| `-d`, `--default` | flag | on | Compatibility flag; no effect. |
| `-t`, `--text` | flag | off | Compatibility flag; no effect. |
| `-f`, `--factors` | path | none | Accepted but currently does not write a factor file. |
| `-v`, `--version` | flag | off | Print version and exit. |
| `-h`, `--help` | flag | — | Print help and exit. |

Unlike `lzcomplexity`, this tool has **no alphabet-size flag**: `-a` selects
the DNA strategy. Also, `-v` means version; use `-L` for verbose logs.

## The two matrices

Let `C` denote the library's LZ76 factor count, and let `XY` mean
concatenation. The information-distance cell is

```text
information_distance(X, Y) =
    max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))
```

This formula is symmetric as a function of untransformed operands. A
finite-sequence diagonal need not be zero, and the implementation does not
clip the result or enforce metric axioms. See [information distance](../concepts/nid.md).

The second matrix uses the concatenation and its block-shuffled copies:

```text
shuffle_information_distance(X, Y) =
    M * C(XY) / sum_l(C(shuffle_l(XY)))
```

`M` is the automatic cap for the concatenated length. Internally this is
`1 - mutual_information(...)`, where that historical function name denotes
an operational shuffle contrast. It is not calibrated Shannon mutual
information, is generally asymmetric, and is not constrained to `[0, 1]`.
Its diagonal is not a zero-distance guarantee.

Both matrices are computed on every run. With a transformation strategy,
each cell is the minimum over candidate second operands, separately for each
matrix. Transforms affect only the second operand, so transformed matrices
need not remain symmetric even for a single input source.

## Transformation strategies

When multiple strategy flags are supplied, precedence is `-b`, then `-a`,
then `-y`, then `-r`. They do not compose.

| Strategy | Candidates for the second operand |
|---|---|
| Default | Original sequence only. |
| `-r` | Reversed sequence only. |
| `-b` | Original, reversed, bit-flipped, reversed bit-flipped. |
| `-a` | Original/reversed; A↔T swapped/reversed; C↔G swapped/reversed. |
| `-y` | Eight rotations of digit codes `1`–`8`, each with its reversal. |

The binary flip maps ASCII `0` to ASCII `1` and **every other byte** to
ASCII `0`. Use `-b` only with an appropriate ASCII `0`/`1` representation;
PBM and raw-binary readers produce numeric bytes zero/one instead.

DNA swaps lowercase the affected bases and preserve other bytes. The six
candidates are the actual legacy operation, not a general canonical
reverse-complement preprocessing pipeline. There is no separate RNA strategy
flag. Normalize biological representations explicitly when required by your
analysis. The trajectory strategy expects an eight-direction digit code;
it does not infer geometric trajectories from coordinates.

## Row and column ranges

```sh
lzdistance trials.txt -I TXT -i 2:4 -s 1:3 -o subset.json
```

Indices are one-based and inclusive. `2:` starts at the second sequence and
continues to the end; `:3` starts at the first and ends at the third. A single
value such as `-i 2` is also a lower bound in the current implementation,
not a single-row selection. Use `-i 2:2` for exactly the second row.

Ranges **do not shrink the matrices**. Cells outside the selected rows or
columns remain zero. Those zeros mean *not computed* and must not be
interpreted as measured identity. Range gates affect the two distance
matrices, not the optional directed matrix.

## Output schema

The result file contains these keys:

| Key | Meaning |
|---|---|
| `first_data_source` | First path. |
| `first_dim` | Full number of first-source sequences. |
| `first_data_source_format` | Configured format label. |
| `second_data_source`, `second_dim`, `second_data_source_format` | Included when a second path was supplied. |
| `information_distance` | Full row-by-column array of count-based distances. |
| `shuffle_information_distance` | Corresponding shuffle-distance array. |
| `directed_matrix` | Legacy graph array, subject to the flag behavior below. |

For one source, both distance matrices are `first_dim × first_dim`. With two
nonempty sources they are `first_dim × second_dim`. JSON cell indices are
zero-based even though command-line ranges are one-based. Output is compact
JSON; pretty-print with `python -m json.tool distances.json`.

## Concatenation-order graph: `-g`

This optional graph compares `C(XY)` with `C(YX)`. It describes an asymmetry
of the finite parse; it does **not** establish causal direction or predictive
influence between measured systems.

The current compute/save conventions are unusual:

| Invocation | Behavior |
|---|---|
| No `-g` | Graph is not computed; JSON includes `directed_matrix: []`. |
| Bare `-g` or `-g 0` | Threshold-zero graph is computed but omitted from JSON. |
| `-g 1` | Graph is computed and included. |

Place a bare `-g` after source paths or use `-g=0` so its optional value does
not consume a path. For a pair, define `delta = C(YX) - C(XY)`. If
`abs(delta) > threshold`, entries are `-delta` in the first-to-second position
and `delta` in the reverse position; otherwise both entries are one. The
self-comparison diagonal remains zero.

With two sources, graph nodes are ordered as all first-source sequences then
all second-source sequences. Only cross-source edges are filled; within-set
blocks remain zero. The graph uses original sequences and ignores strategy
transforms and row/column gates.

## Output paths and cost

The default name appends `.lzdist.json`: `trials.txt` becomes
`trials.txt.lzdist.json`. A directory ending in `/` can therefore place the
output inside that directory; pass `-o` for an unambiguous location. Existing
output files are overwritten.

Pairwise work grows with the product of source sizes, with additional work
for shuffle scales and candidate variants. Start with a small representative
subset, choose a suitable `-j`, and retain sequence order/preprocessing with
results. There is no flag to compute only the first matrix.

No arguments prints help and exits successfully. Version exits before
loading sources. Missing sources and I/O failures return a failing exit
status; do not consume an older result after a failed command.

Source: [distance implementation](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-cli/src/bin/lzdistance.rs).
