# Input file formats

A file reader determines what bytes reach the LZ76 core. Text lines, CSV
columns, FASTA records and binary rasters represent different sequences even
when they originate from the same measurements. Choose the reader explicitly
and check the reported sequence lengths before a full analysis.

## Select a format

Use `lzcomplexity -F`, or `lzdistance -I`/`-S` for first/second sources.
Names are case-insensitive. This is the actual selector table:

| Accepted names | Reader |
|---|---|
| `TXT`, `TEXT`, `RAWTXT` | Raw text lines. |
| `CSV` | Comma-separated columns. |
| `TCSV` | **Space-separated** columns; the legacy TSV reader. |
| `FASTA`, `DNA`, `RNA` | FASTA-style records; optional gzip by `.gz` suffix. |
| `RAW`, `BIN`, `RAWBIN` | Raw file bytes expanded into numeric zero/one symbols. |
| `PBMTXT` | P1 ASCII bitmap. |
| `PBM`, `PBMBIN` | P4 binary bitmap. |
| `PGMTXT` | P2 ASCII grayscale. |
| `PGM`, `PGMBIN` | P5 binary grayscale. |
| `AUTO`, or any unrecognized name | Legacy content detection, subject to extension handling below. |

!!! note "The current TSV selector is `TCSV`"

    Although help and the historical manual use the name TSV, `-F TSV` is
    not a recognized explicit selector. `TCSV` splits on a literal space,
    not a tab. Convert a conventional tab-separated file to a supported
    representation before analysis; do not assume the delimiter is inferred.

`PBM` and `PGM` select their **binary** readers; use the `TXT` suffix in the
format name for ASCII image variants. `PNM` is not a universal explicit image
selector. The P1/P2/P4/P5 magic bytes can be recognized through `AUTO`.

## Automatic selection

When `AUTO` is requested, command-line extension handling happens first:

| Extension | `lzcomplexity` | `lzdistance` |
|---|---|---|
| `.fna`, `.fasta`, `.gz` | FASTA reader | FASTA reader |
| `.csv` | CSV reader | CSV reader |
| `.tsv` | Legacy space-delimited reader | Falls through to content detection |
| Other, including `.fa` and `.txt` | Content detection | Content detection |

Use `-F FASTA` / `-I FASTA` for a `.fa` file. An arbitrary `.gz` file is not
necessarily FASTA merely because the default extension rule selects that
reader. Directory loading applies the source's chosen reader to each file;
select one explicitly for homogeneous directories.

The content detector inspects the first few bytes. It recognizes P1–P7 magic
numbers, then uses a limited alphanumeric/whitespace test to distinguish text
from binary. Short first lines, punctuation or UTF-8 can fail that heuristic.
For known text, prefer `-F TXT` or `-I TXT`. Unknown format names silently
fall back to `AUTO`; they do not provide a validation error.

The JSON `format` field describes the configured selector. If that selector
remained `AUTO`, output can still say `AUTO` after content detection.

## Raw text

Without multiline mode, read the **first nonblank line** and ignore the rest.
With multiline mode, each nonblank line becomes a sequence. ASCII whitespace
at each end is trimmed; interior spaces remain symbols. `#` lines are data,
not comments. Text reading uses lossy UTF-8 conversion, so invalid UTF-8 bytes
can be replaced rather than preserved.

```text
01010101
00110011
```

```sh
lzcomplexity trials.txt -F TXT -m -o trials.json
```

This yields two eight-byte sequences. To analyse an entire text document as
one sequence, construct its bytes in Python with the intended newline policy
and pass them to `lz.h`/`lz.emc`; the single-line CLI mode does not concatenate
a document automatically.

## CSV and legacy space-separated columns

The first nonempty row establishes the number of columns and **is included
as data**. Subsequent cell text is appended with no separator. Without
multiline mode, only the first column is retained; with it, every column
becomes a sequence.

```text
A,B
0,1
1,0
```

`-F CSV -m` produces `A01` and `B10`, not the numeric columns `01` and `10`.
Remove headers before analysis if they are metadata. Cell text is not parsed
as numbers, and there is no quoted-field CSV parser. Missing cells append
nothing; cells beyond the first row's column count are ignored. A trailing
carriage return is removed, but other cell whitespace is retained.

`TCSV` follows the same rules with a literal space delimiter. Repeated spaces
create empty cells. Neither reader bins continuous numeric values: a cell
`12.5` contributes four character symbols.

## FASTA, DNA and RNA

Sequence lines within a record are concatenated, trimming ASCII whitespace
at their ends. Header lines beginning with `>` start records and are excluded
from the sequence. The legacy reader also treats a line starting with `;` as
a record boundary, rather than merely ignoring an arbitrary comment line.
Data before the first header starts an implicit record.

Without multiline mode, retain the first record; with it, retain all records.
All letter bytes are otherwise preserved: no automatic uppercase conversion,
base validation, ambiguity filtering or complement normalization occurs.
Interior spaces in a sequence line also remain bytes.

A path with the exact `.gz` extension is decoded with a multi-member gzip
reader. Explicit FASTA mode still requires valid gzip data for that suffix.

```sh
lzcomplexity genomes.fa -F FASTA -m -l 2 -o genomes.json
lzdistance genomes.fa -I FASTA -o genome-distances.json
```

`DNA`, `RNA` and `FASTA` use the same reader. These selector names do not
activate `lzdistance` transformation strategies. JSON uses an empty format
label for all three for historical compatibility.

## PBM and PGM images

Supported raster readers convert pixels to numeric byte symbols. Without
multiline mode, the raster is one sequence; with it, each image row becomes
one sequence.

| Reader | Pixel representation |
|---|---|
| P1 / `PBMTXT` | ASCII `0`/`1` pixels become numeric byte values zero/one. |
| P4 / `PBM` | MSB-first bitstream, read **contiguously** across rows. |
| P2 / `PGMTXT` | Parsed integers truncated to the low eight bits. |
| P5 / `PGM` | One raw byte per pixel; no 16-bit grayscale decoding. |

The header parser accepts dimensions following the magic and skips header
comments. The raster begins after the newline ending the last header line.
Keep raster data on a following line.

The P4 reader does **not** remove standard per-row padding bits. For a width
not divisible by eight, its result differs from a general PBM decoder's
pixel raster. Binary PBM input shorter than declared dimensions is padded
with one bits; binary PGM is padded with `255`. PGM `maxvalue` does not
rescale samples. Validate image structure or decode the image yourself before
measurement when these conventions do not match the intended data.

P3/P6 color images and P7 PAM can be detected but currently produce an empty
sequence. Detection is not support. Do not use their output as a valid color
image analysis.

## Raw binary

`RAWBIN` expands each file byte MSB first into **numeric** byte symbols zero
and one, rather than ASCII `0` and `1`. Multiline has no effect.

The legacy length rule is material:

| File size | Symbols analysed |
|---|---|
| Zero bytes | Empty sequence. |
| One byte | All eight bits. |
| Two or more bytes | First `(file_size - 1) * 8` bits: the final file byte is omitted. |

For example, the two bytes `0x0f, 0x00` become
`[0, 0, 0, 0, 1, 1, 1, 1]`. The last byte is not measured.

Use Python `bytes` input to analyse file bytes as byte symbols without this
reader's bit expansion or truncation:

```python
from pathlib import Path
import lzcomplexity as lz

raw = Path("recording.bin").read_bytes()
result = lz.lz76(raw, log_base=2, max_block_size=16)
```

That intentionally measures one symbol per original byte. For one symbol
per bit, expand all desired bytes explicitly with your chosen bit order.

## Verify the representation

Before processing a corpus, inspect one small result and verify the number
of sequences, each sequence's `size`, the alphabet and the encoding. Keep
the format selector and preprocessing with the results. A successfully
written JSON file alone does not establish that the reader measured the
sequence you intended.

Source: [format selectors and readers](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-cli/src/formats.rs).
