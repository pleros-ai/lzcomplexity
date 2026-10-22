---
title: CLI cheatsheet
template: cheatsheet.html
sheet_kind: cli
sheet_description: Standalone tools · files, batches and distance matrices
description: A compact command-line reference for lzcomplexity and lzdistance, supported inputs, batch analysis and JSON output.
---

<div class="lz-sheet-grid" markdown="1">

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Install and run

Download the [platform binaries](https://github.com/pleros-ai/lzcomplexity/releases).
Name them `lzcomplexity` and `lzdistance` (`.exe` on Windows), then add them
to your `PATH`. On macOS/Linux:

```sh
chmod +x lzcomplexity lzdistance
```

Or build from a repository clone:

```sh
cargo build --release \
  -p lzcomplexity-cli --bins --locked
```

Executables: `target/release/`. Both run without Python.
[Installation](../guide/install.md#build-the-current-binaries).

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Prepare sequences

One symbolic sequence per text line:

```sh
printf '%s\n' \
  '00010111000101110001011100010111' \
  '00110011001100110011001100110011' \
  > sequences.txt
```

| Format | Sequences |
|---|---|
| `TXT` | Nonblank lines. |
| `CSV` | Columns; remove headers first. |
| `FASTA` | Records; `.gz` supported. |

Continuous signals need symbols first: threshold, bin, or encode them.
[Python example](../guide/quick-reference.md#continuous-signals-symbols).

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Analyze h and E

Factor count, h in bits per byte, and E for every line:

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -o results.json
```

Count and h only, skipping E:

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -n -o entropy.json
```

E up to 16 bytes, including its fitted increments:

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -e 16:f -o ranges.json
```

E is effective measure complexity: correlations across ranges. Use longer
sequences to explore longer ranges.

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Common analysis flags

| Flag | Effect |
|---|---|
| `-F TXT` | Select input format. |
| `-m` | Analyze all sequences. |
| `-l 2` | h in bits per input byte. |
| `-n` | Skip E. |
| `-e 16` | Set E's maximum range. |
| `-e 16:f` | Also include E increments. |
| `-o results.json` | Set output path. |
| `-j 4` | Use four worker threads. |

Without `-m`, only the first text line, CSV column or FASTA record is analyzed.

</section>

<section class="lz-sheet-section lz-sheet-teal lz-sheet-output" markdown="1">

## Read the JSON

Each entry in `sequences` contains:

| Field | Value |
|---|---|
| `lz76Complexity` | LZ76 factor count. |
| `lz76EntropyDensity` | h. |
| <code>lz76RandomShuffleComplexity<wbr>.value</code> | E. |
| <code>lz76RandomShuffleComplexity<wbr>.summands</code> | E increments, with `-e 16:f`. |

For distance between consecutive input sequences:

```sh
lzcomplexity sequences.txt -F TXT \
  -d -l 2 -o consecutive.json
```

`-d` also enables multiline input. Distances appear in
`lz76Distance.InformationDistance`.

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Pairwise distances

A square matrix: every sequence against every sequence.

```sh
lzdistance sequences.txt -I TXT \
  -o distances.json
```

A rectangular matrix: rows from the first source, columns from the second.

```sh
lzdistance first.txt second.txt \
  -I TXT -S TXT -o between.json
```

One sequence per file, in sorted path order:

```sh
lzdistance recordings/ -I TXT \
  -o recordings-distances.json
```

Every FASTA record against every FASTA record:

```sh
lzdistance genomes.fa -I FASTA \
  -o genome-distances.json
```

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Distance inputs and outputs

`lzdistance` reads all sequences from a file. A directory contributes the
first sequence in each file; subdirectories are not searched.

| Flag | Effect |
|---|---|
| `-I TXT` | First source's format. |
| `-S TXT` | Second source's format. |
| `-o distances.json` | Set output path. |
| `-j 4` | Use four worker threads. |

JSON contains `information_distance` and `shuffle_information_distance`
matrices. The first is the same measure as Python's `nid`, derived from
Normalized Information Distance. Lower values indicate greater similarity.

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Help and references

```sh
lzcomplexity --help
lzdistance --help
lzcomplexity --version
lzdistance --version
```

Full references: [`lzcomplexity`](lzcomplexity.md),
[`lzdistance`](lzdistance.md), [input formats](formats.md), and the
[Python cheatsheet](../guide/quick-reference.md).

</section>

</div>
