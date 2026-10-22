---
title: CLI usage cheatsheet
template: cheatsheet.html
sheet_kind: cli
sheet_description: Common commands · one sequence per text line
description: Short usage examples for the standalone lzcomplexity and lzdistance binaries.
---

<div class="lz-sheet-grid" markdown="1">

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Prepare a file

[Install the binaries](../guide/install.md#command-line-tools), then put one
symbolic sequence on each line:

```sh
printf '%s\n' \
  '00010111000101110001011100010111' \
  '00110011001100110011001100110011' \
  > sequences.txt
```

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Measure h and E

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -o results.json
```

Count and h only:

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -n -o entropy.json
```

E up to range 16, with profile increments:

```sh
lzcomplexity sequences.txt -F TXT \
  -m -l 2 -e 16:f -o ranges.json
```

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Save factors and distances

```sh
lzcomplexity sequences.txt -F TXT \
  -m -f factors.json -o results.json
```

Distances between consecutive sequences:

```sh
lzcomplexity sequences.txt -F TXT \
  -d -l 2 -o consecutive.json
```

Read them from `lz76Distance.InformationDistance`.

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Compare collections

Every sequence against every sequence:

```sh
lzdistance sequences.txt -I TXT \
  -o distances.json
```

Rows from one file, columns from another:

```sh
lzdistance first.txt second.txt \
  -I TXT -S TXT -o between.json
```

One sequence per file, sorted by path:

```sh
lzdistance recordings/ -I TXT \
  -o recordings-distances.json
```

Read the `information_distance` matrix.

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Common options

| Option | Use |
|---|---|
| `-F TXT` | Analysis input format. |
| `-I TXT`, `-S TXT` | Distance input formats. |
| `-m` | Analyse all sequences. |
| `-l 2` | Base-two units. |
| `-e 16:f` | E range and increments. |
| `-j 4` | Four workers. |
| `-o output.json` | Output path. |

Formats: `TXT` lines, `CSV` columns, `FASTA` records.
Without `-m`, `lzcomplexity` reads the first sequence.
`lzdistance` reads all sequences from a file.

</section>

<section class="lz-sheet-section lz-sheet-violet lz-sheet-output" markdown="1">

## Read results and get help

| Analysis JSON field | Read as |
|---|---|
| `lz76Complexity` | Factor count. |
| `lz76EntropyDensity` | h. |
| <code>lz76RandomShuffleComplexity<wbr>.value</code> | E. |

```sh
lzcomplexity --help
lzdistance --help
lzcomplexity --version
lzdistance --version
```

Guides: [`lzcomplexity`](lzcomplexity.md),
[`lzdistance`](lzdistance.md), [input formats](formats.md).

</section>

</div>
