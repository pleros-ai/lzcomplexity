# `lzcomplexity`

Analyse one file and write per-sequence results as JSON. By default each
reported sequence gets a factor count, entropy density h and effective measure complexity E.

```text
lzcomplexity [OPTIONS] [file]
```

```sh
printf '01010101\n00110011\n' > trials.txt
lzcomplexity trials.txt -F TXT -m -l 2 -e 8:f -o trials.json
```

## Options

| Short / long | Value | Default | Behavior |
|---|---|---|---|
| positional `file` | path | required for analysis | Input sequence file. |
| `-a`, `--alphabet` | integer or `auto` | `auto` | Alphabet parameter; also selects the default log base when explicitly supplied. |
| `-d`, `--dlz` | flag | off | Add distance between consecutive sequences; implies multiline. |
| `-e`, `--rsemc-opt` | optional option string | automatic cap | Set the maximum range for E and request profile increments; see below. |
| `-f`, `--factors` | path | none | Write an additional factor-boundary JSON file. |
| `-F`, `--format` | format name | `AUTO` | Override [input interpretation](formats.md). |
| `-j`, `--jobs` | unsigned integer | Rayon default | Positive values set the native worker count; zero leaves the default. |
| `-l`, `--log-base` | unsigned integer | explicit `-a`, otherwise observed alphabet | Logarithm units for h and E; use `2` for base-two normalization. |
| `-m`, `--multi-line` | flag | off | Treat lines/columns/records/image rows as separate sequences. |
| `-n`, `--entropy-density` | flag | off | Skip E; retain factor count and entropy density. |
| `-o`, `--output` | path | `<stem>.lz76.json` | Result file; replaces the input's final extension. |
| `-p`, `--partitions` | signed integer | `0` | Accepted; no effect. |
| `-v`, `--verbose` | flag | off | Print sequence count and output path. |
| `-V`, `--version` | flag | off | Print package version and exit. |
| `-w`, `--warn-out` | flag | off | Accepted; no effect. |
| `-h`, `--help` | flag | — | Show help and exit. |

Unsigned bases below two are floored at two by normalization. The CLI's
permissive alphabet/base parsers fall back to automatic/default values for
unparseable strings; scripts should pass validated integers instead of
relying on that fallback.

!!! example "Explicit units"

    For `01010101`, `-l 2` reports `lz76EntropyDensity = 0.75` and `-l 4`
    reports `0.375`. An explicit `-a 4` also selects base four unless `-l`
    overrides it. Python's `alphabet=4` alone does **not** select base four;
    pass explicit units when comparing interfaces.

## One sequence or many?

| Format | Without `-m` | With `-m` |
|---|---|---|
| `TXT` | First nonblank line, trimmed | Each nonblank line |
| `CSV` / `TCSV` | First column | Each column |
| FASTA / DNA / RNA | First record | Each record |
| PBM / PGM | One concatenated raster | Each image row |
| Raw binary | One expanded bitstream | Still one bitstream |

A plain text file is **not concatenated** without `-m`; subsequent lines are
ignored. A `#` line is an ordinary text sequence, not a comment. Header rows
in CSV are also data. See [formats](formats.md) for exact parser behavior.

## E options: `-e` { #shuffle-options-e }

The accepted syntax is `cap:f:start:end`. Only the cap and `f` fields are
used:

| Example | Effect |
|---|---|
| No `-e`, bare `-e`, or `-e a` | Automatic maximum block size. |
| `-e 16` | Compute scales one through 16. |
| `-e 16:f` | Same cap and include `summands` in JSON. |
| `-e a:f` or `-e f` | Automatic cap and include `summands`. |
| `-e=0` or a negative cap | Automatic cap. |

For a negative value, use the equals form, such as `-e=-1:f`, to avoid
option parsing ambiguity. Put a bare `-e` after the positional file or use
`-e=a`, so the optional argument does not consume your file path.

The `start:end` slots are ignored. They do not restrict which
lines receive E analysis. An empty first field, such as `-e=:f`,
changes nothing. For a selected subset, prepare the input file yourself.

The automatic cap depends on sequence length and adds ten for inputs longer than 50 bytes. The result records the cap used. See [effective measure complexity E](../concepts/emc.md) for the range profile and its interpretation.

## JSON result

This example requests factor counts and h in text mode:

```sh
printf '01010101\n' > sample.txt
lzcomplexity sample.txt -F TXT -n -l 2 -o result.json
```

Pretty-printed `result.json`:

```json
{
  "filename": "sample.txt",
  "format": "PNM_RAWTXT",
  "size": 1,
  "sequences": [
    {
      "alphabet": [49, 48],
      "alphabet_size": 2,
      "lz76Complexity": 2,
      "lz76EntropyDensity": 0.75,
      "lz76RandomShuffleComplexity": {
        "max_block_size": -1,
        "multi_information": 0.0,
        "value": 0.0
      },
      "size": 8
    }
  ]
}
```

| Field | Meaning |
|---|---|
| Root `filename` | Input path as supplied. |
| Root `format` | Configured format label; `AUTO` can remain `AUTO` even after detection. |
| Root `size` | Number of sequences read. |
| `sequences` | Per-sequence results, in input order. |
| Sequence `size` | Number of bytes after format conversion. |
| `alphabet` | Distinct byte values, descending; bytes 128–255 are serialized as negative signed-byte values. |
| `alphabet_size` | Observed alphabet size, normally floored at two. |
| `lz76Complexity` | Library LZ76 factor count. |
| `lz76EntropyDensity` | Entropy-density estimate in requested logarithm units. |
| `lz76RandomShuffleComplexity` | E in `value`, the maximum range, first-range contrast and optional profile `summands`. |
| `lz76Distance.InformationDistance` | Present with `-d`; neighboring-sequence distance values. |

!!! note "Skipped analysis has a sentinel"

    Under `-n`, the E object remains in JSON with `max_block_size=-1`,
    zero scalars and no `summands`. This means **uncomputed**, not a measured
    score of zero. A computed result records its actual positive cap.

The CLI writes compact JSON. Use
`python -m json.tool result.json` to inspect it; consumers should not depend
on object-key order.

With `-d`, the distance list has `n - 1` entries for `n > 1`. For zero or one
read sequence it contains `[0.0]`.

## Factor-boundary file

```sh
lzcomplexity trials.txt -F TXT -m -f factors.json -o trials.json
```

The additional file has `filename`, numeric `format`, `size` and `factors`.
Each element of `factors` is the boundary list for one read sequence. Unlike
the main report, `format` is a numeric code: `TXT=7`, raw binary
`=8`, `CSV=9`, `TCSV=10`, `DNA=11`, `RNA=12`, `FASTA=13`, `AUTO=14`;
PNM P1–P7 use zero through six.

Boundaries can exceed sequence length, and the returned LZ count can differ
from the number of displayed fragments. See [factorization](../api/python.md#factorization)
before reconstructing components or deriving counts from boundaries.

## Paths and exit behavior

`sample.txt` defaults to `sample.lz76.json`; `folder/data.csv` defaults to
`folder/data.lz76.json`. `-o` and `-f` overwrite existing output files.
Use distinct output paths in batch pipelines.

No arguments prints help and exits successfully. `--version` exits before
reading the file. Flags without a positional input produce an error, as do
missing files and I/O failures. Parsing failures use Clap's error handling;
check process exit status before reading a previous output file.

Source: [standalone implementation](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-cli/src/bin/lzcomplexity.rs).
