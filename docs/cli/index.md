# Command-line tools

Use the command-line tools when sequences already live in files or when a
batch job should produce machine-readable JSON without a Python program.
Both tools use the same Rust core as the Python library.

| Tool | Input | Output |
|---|---|---|
| [`lzcomplexity`](lzcomplexity.md) | One sequence file | Per-sequence LZ76 count, entropy density and EMC-like shuffle score; optionally consecutive distances. |
| [`lzdistance`](lzdistance.md) | One or two files/directories | All-pairs information-distance and shuffle-distance matrices. |

## First analysis

```sh
printf '01010101\n' > sample.txt
lzcomplexity sample.txt -F TXT -l 2 -o result.json
python -m json.tool result.json
```

Pass `-F TXT` when you know the input is text. The automatic detector is a
legacy heuristic; a short first line or unexpected punctuation can send a
file through the binary reader. See [input formats](formats.md) before
processing real data.

## Multiple sequences

```sh
printf '01010101\n00110011\n' > trials.txt
lzcomplexity trials.txt -F TXT -m -d -l 2 -o trials.json
lzdistance trials.txt -I TXT -o distances.json
```

`-m` asks `lzcomplexity` to process all text lines. `-d` additionally computes
distances between neighboring sequences and implies `-m`. `lzdistance` reads
file sources as multiple sequences automatically and computes every pair.

Use `-n` with `lzcomplexity` to skip the more expensive shuffle stage when
only factor counts and entropy density are required. `lzdistance` always
computes both of its distance matrices.

## Threads and versions

Both binaries honor positive `-j` values by initializing the native Rayon
worker pool. Their version flags differ:

```sh
lzcomplexity --version
lzdistance --version
```

`lzcomplexity -v` means **verbose**, while `lzdistance -v` means **version**.
Use `lzdistance -L` for verbose logs.

Build/install instructions are on the [installation page](../guide/install.md).
The 0.9 manual's package-manager and CMake instructions describe the older
C++ distribution; they are not prerequisites for the current Rust binaries.
