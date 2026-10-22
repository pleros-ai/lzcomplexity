# `lzcomplexity`

`lzcomplexity` measures the organization of symbolic sequences in a file.
It writes the LZ76 factor count, entropy density h and effective measure
complexity E for each selected sequence.

```text
lzcomplexity [OPTIONS] file
```

## Analyse one sequence

```sh
printf '%s\n' '1000110011101' > sample.txt
lzcomplexity sample.txt -F TXT -l 2 -o result.json
```

`-F TXT` reads a text sequence; `-l 2` selects base-two units. The first entry
in `result.json` has `size = 13`, `lz76Complexity = 5` and
`lz76EntropyDensity = 1.4232460454388816`.

Without `-o`, the output replaces the input's final extension:
`sample.txt` produces `sample.lz76.json`. An existing output file is overwritten.

## Analyse several sequences

```sh
printf '%s\n' \
  '00010111000101110001011100010111' \
  '00110011001100110011001100110011' > trials.txt
lzcomplexity trials.txt -F TXT -m -l 2 -e 16:f -o trials.json
```

`-m` reads all sequences. In this example, the report contains two results in
input order. With text input and no `-m`, only the first nonblank line is used.
Other readers use columns, FASTA records or image rows; see [input formats](formats.md).

Use `-n` when you need only the count and h. Use `-d` to add distances between
consecutive sequences; it also enables multiline input:

```sh
lzcomplexity trials.txt -F TXT -m -n -l 2 -o entropy.json
lzcomplexity trials.txt -F TXT -d -l 2 -o consecutive.json
```

For two sequences, `lz76Distance.InformationDistance` contains one value.
For `n > 1` sequences it has `n - 1` entries; a zero- or one-sequence input
produces `[0.0]`. Use [`lzdistance`](lzdistance.md) for all-pairs comparisons.

## Choose E's range { #shuffle-options-e }

`-e 16` measures E up to range 16; `-e 16:f` also includes the profile increments
in the JSON report. Omitting `-e` selects the range automatically. The report
records the range used in `max_block_size`.

| Invocation | Range and output |
|---|---|
| No `-e`, `-e a`, or bare `-e` | Automatic range. |
| `-e 16` | Maximum range 16. |
| `-e 16:f` | Maximum range 16 and profile increments. |
| `-e a:f` or `-e f` | Automatic range and profile increments. |
| `-e=0` or `-e=-1:f` | Automatic range; the second also includes increments. |

Place a bare `-e` after the file path, or use `-e=a`, so its optional value
does not consume the path. The additional `start:end` slots accepted by the
parser have no effect. To analyse selected records, prepare a file containing
those records. See [E](../concepts/emc.md) for using and comparing range profiles.

## Save the factor boundaries

```sh
lzcomplexity sample.txt -F TXT -f factors.json -o result.json
```

`factors.json` contains one boundary list per sequence. For `1000110011101`,
that list is `[0, 1, 2, 5, 9, 13]`, giving `1·0·001·1001·1101`.
Boundaries index input bytes. A final copied tail can produce a boundary beyond
the sequence length and is excluded from the returned count; see
[factorization](../api/python.md#factorization) before deriving counts from a list.

The factor file also records `filename`, numeric `format` and sequence `size`.
Its format codes are P1–P7 = 0–6, TXT = 7, RAWBIN = 8, CSV = 9,
TCSV = 10, DNA = 11, RNA = 12, FASTA = 13 and AUTO = 14.

## Read the JSON report

The root object records `filename`, configured `format`, sequence count `size`
and the `sequences` array. Each array entry contains:

| Field | Meaning |
|---|---|
| `size` | Sequence length after file conversion, in bytes. |
| `alphabet`, `alphabet_size` | Distinct byte values and observed alphabet size. |
| `lz76Complexity` | LZ76 factor count. |
| `lz76EntropyDensity` | h in the selected logarithm units. |
| `lz76RandomShuffleComplexity.value` | E. |
| `lz76RandomShuffleComplexity.max_block_size` | Maximum range used for E. |
| `lz76RandomShuffleComplexity.summands` | Profile increments, present when requested with `:f`. |
| `lz76RandomShuffleComplexity.multi_information` | First-range contrast. |

With `-n`, the E object remains present with `max_block_size = -1` and zero
values: E was not computed. A computed result has a positive range. The
alphabet array is descending; bytes 128–255 are represented as negative
signed-byte values. Format labels follow the [reader conventions](formats.md).

## Options

| Option | Default | Use |
|---|---|---|
| `-F`, `--format FORMAT` | `AUTO` | Select the input reader. |
| `-m`, `--multi-line` | off | Process every sequence. |
| `-l`, `--log-base N` | alphabet size | Set logarithm units for h and E. |
| `-a`, `--alphabet N` | observed size | Set the alphabet parameter and, unless `-l` is supplied, the log base. |
| `-e`, `--rsemc-opt [OPTS]` | automatic range | Configure E as described above. |
| `-n`, `--entropy-density` | off | Skip E; retain count and h. |
| `-d`, `--dlz` | off | Add consecutive distances; implies `-m`. |
| `-f`, `--factors PATH` | none | Save a separate boundary file. |
| `-o`, `--output PATH` | `<stem>.lz76.json` | Set the result path. |
| `-j`, `--jobs N` | native default | Use N worker threads; zero retains the default. |
| `-v`, `--verbose` | off | Print sequence count and output path. |
| `-V`, `--version` | — | Print version and exit. |
| `-h`, `--help` | — | Print help and exit. |
| `-p`, `--partitions N` | `0` | Accepted; no effect. |
| `-w`, `--warn-out` | off | Accepted; no effect. |

Explicit logarithm bases below two are treated as two. `-a auto` selects the
observed alphabet. Unparseable alphabet/base values fall back to automatic
settings, so use valid integers in scripts. An explicit `-a 4` selects base
four unless `-l` overrides it; Python's `alphabet=4` alone does not change units.

Missing files, missing input paths and I/O errors return a failing exit status.
No arguments prints help successfully. Check the exit status before consuming
an output file, particularly when reusing an existing path.

**Quick commands:** [CLI cheatsheet](quick-reference.md).
