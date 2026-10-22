# `lzdistance`

`lzdistance` compares collections of symbolic sequences. It writes an
information-distance matrix derived from Normalized Information Distance
(NID), together with additional diagnostic output. Lower information-distance
values indicate more shared patterns.

```text
lzdistance [OPTIONS] first-source [second-source]
```

## Compare a collection with itself

Put one sequence on each text line:

```sh
printf '%s\n' 'abcd' 'abce' > trials.txt
lzdistance trials.txt -I TXT -o distances.json
```

The `information_distance` field in `distances.json` is:

```json
[[0.0, 0.25], [0.25, 0.0]]
```

Rows and columns follow the input sequence order. Entry `[0][1]` compares the
first sequence with the second. Diagonal values depend on the finite-sequence
parse; they are not forced to zero for every input. See [information distance](../concepts/nid.md).

## Compare two collections

```sh
printf '%s\n' 'abcd' 'abce' > first.txt
printf '%s\n' 'abcf' 'zzzz' 'dcba' > second.txt
lzdistance first.txt second.txt -I TXT -S TXT -o between.json
```

This produces a 2 × 3 matrix: rows come from `first.txt`, columns from
`second.txt`. One source gives a square self-comparison. An empty second
collection also falls back to self-comparison.

A file source reads all sequences: text lines, table columns, FASTA records
or image rows, depending on the [format](formats.md). A directory instead
contributes the first sequence from each regular file, sorted by path:

```sh
lzdistance recordings/ -I TXT -o recordings-distances.json
lzdistance genomes.fa -I FASTA -o genome-distances.json
```

Directories are not searched recursively; files ending exactly in `.json`
or `.log` are excluded. Keep the file order with your results: the report
does not include a filename label for every matrix row.

## Select rows and columns

```sh
lzdistance first.txt second.txt -I TXT -S TXT \
  -i 2:2 -s 1:2 -o selected.json
```

Ranges are one-based and inclusive. This computes row 2 against columns 1–2.
`2:` starts at the second sequence; `:3` ends at the third. A single `2` means
from the second sequence onward; use `2:2` for exactly one row or column.

The output keeps its full matrix shape. Cells outside the selected ranges
remain zero and mean **not computed**. They must not be interpreted as
measured identity. Range selections do not restrict the optional directed graph.

## Compare transformed sequences

Choose a strategy when reversing or recoding a sequence should count as
similarity for your application. Transformations apply to the second operand.
Each cell takes the smallest distance among that strategy's candidates.

| Flag | Candidates |
|---|---|
| No strategy flag | Original sequence. |
| `-r` | Reversed sequence only. |
| `-b` | Original, reversed, ASCII-bit-flipped and reversed bit-flipped. |
| `-a` | Original/reversed; A↔T-swapped/reversed; C↔G-swapped/reversed. |
| `-y` | Eight rotations of direction codes `1`–`8`, each with its reversal. |

`-b` expects ASCII `0`/`1`; it maps any non-`0` byte to `0`. `-a` tests
A↔T and C↔G substitutions separately and lowercases affected bases. It is
not a full reverse-complement search; see the [genomics recipe](../recipes/genomics.md#reverse-complements)
for that workflow. `-y` requires an existing eight-direction digit encoding.

Strategies do not compose. If several flags are present, precedence is
`-b`, then `-a`, then `-y`, then `-r`. Transformed matrices may be asymmetric.

## Read the report { #the-two-matrices }

<span id="output-schema"></span>

| Field | Meaning |
|---|---|
| `first_data_source`, `first_dim`, `first_data_source_format` | First path, sequence count and configured reader. |
| `second_data_source`, `second_dim`, `second_data_source_format` | Second-source metadata, when a second path is supplied. |
| `information_distance` | The row-by-column LZ-based information distances; the same measure as Python's `nid()`. |
| `shuffle_information_distance` | A separate diagnostic matrix, written on every run. |
| `directed_matrix` | Concatenation-order graph, described below. |

Use `information_distance` for the comparison illustrated above. The additional
diagnostic is not interchangeable with it: it can be asymmetric, exceed one
and have a nonzero diagonal. Both matrices are always computed; there is no
flag to request only the first.

### Optional directed graph { #concatenation-order-graph-g }

`-g 1` also saves a graph based on differences between the factor counts of
`XY` and `YX`:

```sh
lzdistance trials.txt -I TXT -g 1 -o directed.json
```

The integer is a threshold on that count difference. For a pair with
`delta = C(YX) - C(XY)`, an absolute difference above the threshold gives
entries `-delta` and `delta`; otherwise both entries are one. The diagonal
is zero. With two sources, nodes are ordered as the first collection followed
by the second, and only cross-collection edges are filled.

The graph uses original sequences, ignoring transformations and row/column
ranges. With no `-g`, the report contains `directed_matrix: []`. A bare `-g`
or `-g 0` computes the graph but omits it from JSON; use a positive threshold
to request a saved graph. Put a bare `-g` after the paths or write `-g=0`.

## Options

| Option | Default | Use |
|---|---|---|
| `-I`, `--first-format FORMAT` | `AUTO` | First-source reader. |
| `-S`, `--second-format FORMAT` | `AUTO` | Second-source reader. |
| `-i`, `--first START:END` | all | Select rows, without shrinking the matrix. |
| `-s`, `--second START:END` | all | Select columns, without shrinking the matrix. |
| `-j`, `--jobs N` | native default | Use N workers; zero retains the default. |
| `-o`, `--output PATH` | `<first-source>.lzdist.json` | Set the output path. |
| `-L`, `--logs` | off | Print matrix dimensions and output path. |
| `-r`, `--reverse` | off | Reverse the second operand. |
| `-b`, `--binary` | off | Compare binary variants. |
| `-a`, `--adn` | off | Compare the DNA variants above. |
| `-y`, `--trajectory` | off | Compare direction-code rotations. |
| `-g`, `--get-direction [N]` | absent | Compute the optional directed graph. |
| `-l`, `--log-base N` | automatic | Accepted; these count-ratio matrices are independent of logarithm units. |
| `-p`, `--partitions N` | `2` | Accepted; no effect. |
| `-d`, `--default` | on | Accepted; no effect. |
| `-t`, `--text` | off | Accepted; no effect. |
| `-f`, `--factors PATH` | none | Accepted; no factor file is written. |
| `-v`, `--version` | — | Print version and exit. |
| `-h`, `--help` | — | Print help and exit. |

`-a` selects DNA comparisons, not alphabet size. `-v` prints the version;
verbose logs use `-L`.

The output suffix is appended: `trials.txt` produces
`trials.txt.lzdist.json`. Use `-o` to choose a clear path, especially for directory
sources. Existing files are overwritten. Missing sources and I/O failures
return a failing exit status; no arguments prints help successfully.

**Quick commands:** [CLI cheatsheet](quick-reference.md).
