---
title: Command-line tools
description: Use lzcomplexity and lzdistance to analyse sequence files and compare collections from the command line.
---

# Command-line tools

The standalone binaries analyse sequence files and write JSON results. Use
`lzcomplexity` for measurements of each sequence, and `lzdistance` for a matrix
comparing a collection of sequences. Both run without a Python installation.

| Tool | What to use it for |
|---|---|
| [`lzcomplexity`](lzcomplexity.md) | Factor counts, entropy density h, effective measure complexity E and distances between consecutive sequences. |
| [`lzdistance`](lzdistance.md) | All-pairs information distances within a collection or between two collections. |

## Install

Download the executables for your platform or build them from source using
the [installation guide](../guide/install.md#command-line-tools). Add the
executables to your `PATH`, then check:

```sh
lzcomplexity --version
lzdistance --version
```

## Analyse a sequence

Create a text file containing a symbolic sequence, then select the text reader:

```sh
printf '%s\n' '1000110011101' > sample.txt
lzcomplexity sample.txt -F TXT -l 2 -o result.json
```

Open `result.json` in a JSON viewer. Its `sequences` array contains one result:
the sequence has 13 symbols, LZ76 count **5** and h approximately **1.423246**
in base-two units. The report also contains E. A short sequence is convenient
for inspecting the output; use longer recordings for measurements of a source.

## Analyse a collection

For plain text, put one sequence on each line. `-m` processes every line:

```sh
printf '%s\n' 'abcd' 'abce' > trials.txt
lzcomplexity trials.txt -F TXT -m -l 2 -o trials.json
lzdistance trials.txt -I TXT -o distances.json
```

The first command gives a result for each sequence. The second gives a
2 × 2 matrix; the distance between `abcd` and `abce` is **0.25**.

Select a known format explicitly: `-F` for `lzcomplexity`, or `-I` and `-S`
for the first and second `lzdistance` sources. Text lines, table columns and
FASTA records each have different sequence boundaries. See
[input formats](formats.md) to prepare your data.

## Continue

- [`lzcomplexity` guide](lzcomplexity.md): measure sequences, save factors and read the report.
- [`lzdistance` guide](lzdistance.md): compare collections, select ranges and use transformations.
- [Input formats](formats.md): supported readers and how they encode sequences.
- [CLI usage cheatsheet](quick-reference.md): compact commands to keep nearby.
