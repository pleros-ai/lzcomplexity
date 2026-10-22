# Standalone binaries cheatsheet

Use **`lzcomplexity`** for factor count, h and E; use **`lzdistance`** for
pairwise distance matrices. Both write JSON and run without Python.

## Get the tools

Download the [platform binaries](https://github.com/pleros-ai/lzcomplexity/releases).
Name them `lzcomplexity` and `lzdistance` (`.exe` on Windows), and add them
to your `PATH`. On macOS/Linux, make them executable:
`chmod +x lzcomplexity lzdistance`.

??? info "Build from source"

    From a repository clone:

    ```sh
    cargo build --release -p lzcomplexity-cli --bins --locked
    ```

    Find the executables in `target/release/`. See
    [installation](../guide/install.md#build-the-current-binaries) for prerequisites.

## Analyze sequences

Create a text file with one sequence per line:

```sh
printf '%s\n' \
  '00010111000101110001011100010111' \
  '00110011001100110011001100110011' > sequences.txt
```

```sh
# Factor count, h in bits per byte, and E for every line.
lzcomplexity sequences.txt -F TXT -m -l 2 -o results.json

# Factor count and h only: skip E.
lzcomplexity sequences.txt -F TXT -m -l 2 -n -o entropy.json

# E across ranges up to 16 bytes, including its fitted increments.
lzcomplexity sequences.txt -F TXT -m -l 2 -e 16:f -o ranges.json

# Also measure distance between consecutive sequences.
lzcomplexity sequences.txt -F TXT -d -l 2 -o consecutive.json
```

**Text input:** `-m` reads every nonblank line; without it, only the first
nonblank line is analyzed. `-d` enables multiline input automatically.
Use longer sequences when exploring E across ranges.

Read each entry in the JSON's `sequences` array:

| JSON field | Measurement |
|---|---|
| `lz76Complexity` | LZ76 factor count. |
| `lz76EntropyDensity` | h. |
| `lz76RandomShuffleComplexity.value` | E. |
| `lz76RandomShuffleComplexity.summands` | E increments, included with `-e 16:f`. |

Consecutive distances appear in `lz76Distance.InformationDistance`.

## Compare sets of sequences

```sh
# Every line against every line: a square matrix.
lzdistance sequences.txt -I TXT -o distances.json

# Rows from first.txt, columns from second.txt: a rectangular matrix.
lzdistance first.txt second.txt -I TXT -S TXT -o between.json

# One sequence per file in a directory, in sorted path order.
lzdistance recordings/ -I TXT -o recordings-distances.json

# Every FASTA record against every FASTA record.
lzdistance genomes.fa -I FASTA -o genome-distances.json
```

`lzdistance` reads all sequences from a file. A directory contributes the
first sequence from each file and is not searched recursively. Its JSON
contains `information_distance` and `shuffle_information_distance` matrices;
`information_distance` is the same measure as Python's `nid`.

## Useful flags and formats

| Task | `lzcomplexity` | `lzdistance` |
|---|---|---|
| Input format | `-F TXT` | `-I TXT`; `-S TXT` for the second source. |
| Output path | `-o results.json` | `-o distances.json` |
| Use four worker threads | `-j 4` | `-j 4` |
| Help / version | `--help` / `--version` | `--help` / `--version` |

| Format | Sequence layout |
|---|---|
| `TXT` | One sequence per nonblank line. |
| `CSV` | One sequence per column; remove header rows first. |
| `FASTA` | One sequence per record; gzip files are supported. |

With `lzcomplexity`, add `-m` to analyze all CSV columns or FASTA records.

Continuous measurements need conversion to symbols before analysis, for
example by thresholding or binning. See the [Python cheatsheet](../guide/quick-reference.md#continuous-signals-symbols).

Full references: [`lzcomplexity`](lzcomplexity.md), [`lzdistance`](lzdistance.md)
and [input formats](formats.md).
