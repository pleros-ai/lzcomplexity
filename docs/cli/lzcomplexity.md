# lzcomplexity

*Every flag, every JSON key, and the exact behaviour of the single-sequence analysis tool.*

`lzcomplexity` reads one file, factorises the sequences it finds with LZ76, and writes a JSON
report next to the input. It computes three things per sequence: the factor count, a normalised
entropy density, and the random-shuffle effective measure complexity (EMC). With `-d` it also
emits the information distance between consecutive sequences.

<div class="lz-run" markdown>

```console
$ lzcomplexity -v random.txt
 [ Info ] Sequences to process: 1
 [ Info ] Saved results in: random.lz76.json
```

</div>

Output is written **compact** — one line, no whitespace, no trailing newline. The one indented JSON
block on this page was pretty-printed afterwards with `python3 -m json.tool --indent 2`; every other
JSON block is the file as written. Key order is lexicographic, not insertion order, because the
writer uses `serde_json::Map`, which is a `BTreeMap` here.

The companion tool for all-pairs distance matrices is [`lzdistance`](lzdistance.md). Input parsing
— formats, magic-number detection, the `-F` name table — is documented in
[Input formats](formats.md).

<div class="lz-tickrule"></div>

## Synopsis

```text
lzcomplexity [OPTIONS] [file]
```

The positional `file` is optional. With no arguments at all the tool prints its help to stdout and
exits `0`. With other flags but no file it errors:

<div class="lz-run" markdown>

```console
$ lzcomplexity -v
 [ Error ] Input file is missing
$ echo $?
1
```

</div>

## Flags

<div class="lz-scroll lz-compare" markdown>

| Short | Long | Argument | Default | Effect |
|---|---|---|---|---|
| — | — | `[file]` | none | Input sequence file. |
| `-a` | `--alphabet` | `<value>` | `auto` | Alphabet cardinality used in the **reported entropy density**. `auto`, empty, or unparseable falls back to the detected size. Also sets the log base unless `-l` is given. |
| `-d` | `--dlz` | flag | off | Add `lz76Distance.InformationDistance` for consecutive sequences. **Implies `-m`.** |
| `-e` | `--rsemc-opt` | `[<opts>]` | absent → auto block size; bare `-e` → `a` | Random-shuffle options `v1:f:v2:v3`. See [below](#the-e-option-string). |
| `-f` | `--factors` | `<file_name>` | none | Also write a separate factor-boundary file. See [below](#the-factors-file-f). |
| `-F` | `--format` | `<value>` | `AUTO` | Input format override. Names in [Input formats](formats.md). |
| `-j` | `--jobs` | `<value>` | hardware concurrency | Size of the rayon thread pool. Wall-clock only; results are identical. |
| `-l` | `--log-base` | `<value>` | the `-a` value | Log base for the **shuffle/EMC stage only**. Does not touch `lz76EntropyDensity`. |
| `-m` | `--multi-line` | flag | off | Treat each line, column, record or image row as its own sequence. |
| `-n` | `--entropy-density` | flag | off | Skip the random-shuffle stage. Complexity and entropy density are still computed. |
| `-o` | `--output` | `<file_name>` | `<stem>.lz76.json` | Output path. An empty string falls back to the default. |
| `-p` | `--partitions` | `<value>` | `0` | **No effect on anything.** Retained for C++ command-line compatibility. |
| `-v` | `--verbose` | flag | off | Print `Sequences to process: N` and `Saved results in: <path>` to stdout. |
| `-V` | `--version` | flag | off | Print `[ Info ] v1.0.0` and exit `0` before reading anything. |
| `-w` | `--warn-out` | flag | off | Accepted and inert — this port never emits warnings. |
| `-h` | `--help` | flag | — | Print help, exit `0`. |

</div>

!!! note "`-p/--partitions` changes nothing, including performance."
    The value lands in `LzArgs::chunks`, which no `lzcomplexity-core` computation reads — only the
    `PartialEq` impl looks at it, and that touches no result.
    `lzcomplexity -p 4 random.txt -o p4.json` and `lzcomplexity -p 0 random.txt -o p0.json`
    produce byte-identical files. The flag exists so old C++ command lines keep working.
    To control parallelism, use `-j`.

### Default output path — the extension is replaced, not appended

The default is the input's file stem plus `.lz76.json`, re-joined to the input's directory. The
report lands next to the input, never in the current directory.

| Input | Output |
|---|---|
| `random.txt` | `random.lz76.json` |
| `sub/nested.dat` | `sub/nested.lz76.json` |
| `noext` | `noext.lz76.json` |
| `archive.tar.gz2` | `archive.tar.lz76.json` |

[`lzdistance`](lzdistance.md) *appends* instead (`a.txt` → `a.txt.lzdist.json`). The two tools do
not agree, and that is not a typo on this page.

<div class="lz-tickrule"></div>

## What `-m` does

`-m` redefines what counts as "one sequence", per format:

| Format | `-m` off | `-m` on |
|---|---|---|
| RAWTXT | first non-blank line, trimmed | one sequence per non-blank line |
| CSV / TSV | first column only | one sequence per column |
| FASTA / DNA / RNA | first record only | one sequence per record |
| PBM, PGM | whole raster concatenated | one sequence per image row |
| RAWBIN | whole file as one bitstream | still one sequence — `-m` is ignored |

It also controls how many entries reach `sequences[]`: with `-m`, all of them; without, at most
one. Since every non-multiline reader returns exactly one sequence anyway, the practical effect is
"report them all".

<div class="lz-run" markdown>

```console
$ lzcomplexity -n trials.txt -o F.json          # 3-line file, no -m
$ python3 -c "import json;print(json.load(open('F.json'))['size'])"
1
$ lzcomplexity -m trials.txt -o E.json          # same file, with -m
$ python3 -c "import json;d=json.load(open('E.json'));print(d['size'],[s['lz76Complexity'] for s in d['sequences']])"
3 [2, 10, 1]
```

</div>

## What `-n` suppresses

`-n` skips **only** the random-shuffle stage. `lz76Complexity` and `lz76EntropyDensity` are still
computed and reported in full, and `-d` still works.

The `lz76RandomShuffleComplexity` object is **still emitted**, filled with its default-constructed
values, so downstream schemas stay stable:

```json
"lz76RandomShuffleComplexity":{"max_block_size":-1,"multi_information":0.0,"value":0.0}
```

!!! warning "A `max_block_size` of `-1` means the EMC was never computed, not that the EMC is zero."
    `-1` is the uninitialised sentinel. A real EMC run always reports a positive `max_block_size`
    (15 for a 200-symbol sequence). If you aggregate reports across a pipeline, filter on
    `max_block_size > 0` before averaging `value`, or the `-n` runs will drag your mean toward
    zero. `summands` is never present under `-n`.

`-n` is the dominant cost saving: the shuffle stage runs `max_block_size + 1` extra full
factorizations. On a 500 000-symbol binary sequence (`max_block_size` 25, 16 cores) the default run
took 0.43 s against 0.035 s for `-n`. See [Performance](../project/performance.md) for the full
measurements.

<div class="lz-tickrule"></div>

## The `-e` option string

`-e` takes an optional value of the form `v1:f:v2:v3`. The string is split on `:` and then:

| Slot | Value | Meaning |
|---|---|---|
| `v1` | `a` | block size `0` — auto |
| `v1` | `f` | leave the block size on auto **and** enable summands |
| `v1` | an integer | pin `max_block_size` to it; values of `0` or less mean auto |
| `v1` | anything else | ignored; block size stays auto |
| slot 1 (the `f` position) | `f` | emit the per-scale `summands` array |
| `v2`, `v3` | — | **parsed by nobody.** The help text calls them a "line range"; no code reads them. |

If `v1` is the literal `f`, the summand check reads slot 0 instead of slot 1 — which is why `-e=f`
alone enables summands. If the whole string is empty, or `v1` is empty, the parser returns
immediately and changes nothing, which is why `-e=:f` does **not** give you summands.

Measured on a 200-symbol binary sequence (auto block size 15):

<div class="lz-scroll lz-compare" markdown>

| Invocation | `max_block_size` | `summands` |
|---|---|---|
| *(no `-e`)* | 15 | absent |
| bare `-e`, `-e=a`, `-e=0`, `-e=-3` | 15 | absent |
| `-e=x` | 15 | absent |
| `-e=:f` | 15 | absent — early return |
| `-e=f` | 15 | 15 entries |
| `-e=5` | 5 | absent |
| `-e=5:f` | 5 | 5 entries |
| `-e=5:x:f` | 5 | **absent** — only slot 1 is checked |
| `-e=3:f:1:2` | 3 | 3 entries |

</div>

!!! warning "`lzcomplexity -e random.txt` fails with `Input file is missing`."
    `-e` accepts an optional value, so clap swallows the next token as that value and the filename
    disappears. Write `-e=f` and `-e=5` with an equals sign, and put a bare `-e` last on the line
    (`lzcomplexity random.txt -e`).

### Only the largest block size affects the total

The EMC sum telescopes:

<div class="lz-formula">
  <p class="lz-math"><i>Ê</i> = Σ<sub><i>l</i> = 1..<i>mm</i></sub> [ (<i>H</i><sub><i>l</i></sub> − <i>H</i><sub><i>l</i>−1</sub>) − <i>ĥ</i> ] = <i>mm</i> · <i>g</i> · [ <i>C</i><sub>LZ</sub>(shuffled at <i>mm</i>) − <i>C</i><sub>LZ</sub>(original) ]</p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd><code>max_block_size</code>, the largest block size used</dd>
    <dt><i>H</i><sub><i>l</i></sub></dt><dd>block entropy at scale <i>l</i>, with <i>H</i><sub>0</sub> = 0</dd>
    <dt><i>ĥ</i></dt><dd>the entropy-rate estimate subtracted at every scale</dd>
    <dt><i>g</i></dt><dd>log<sub><i>b</i></sub>(<i>N</i>) ⁄ <i>N</i>, the per-symbol scaling</dd>
    <dt><i>C</i><sub>LZ</sub></dt><dd>LZ76 factor count</dd>
  </dl>
  <p class="lz-formula__cite">Both sides checked against the shipped binary — see below.</p>
</div>

The intermediate scales cancel algebraically, so **`value` depends only on the shuffle at
`max_block_size`**. The running total after `l` scales is bit-identical to a fresh run with `-e=l`:

<div class="lz-run" markdown>

```console
$ lzcomplexity -e=1 random.txt -o T.json    # value  -0.07643856189774723
$ lzcomplexity -e=2 random.txt -o T.json    # value   0.0
$ lzcomplexity -e=3 random.txt -o T.json    # value   0.0
$ lzcomplexity -e=4 random.txt -o T.json    # value  -0.1528771237954949
$ lzcomplexity -e=5 random.txt -o T.json    # value  -0.3821928094887359
```

</div>

Those five numbers are exactly the running sums of the first five entries of the 15-element
`summands` array from `-e=f`, compared with `==` on the raw doubles. For the full run,
`value / (mm * g) = -3.0` to within one ULP: the shuffled sequence factorises into three fewer
components than the original, and that single integer difference is the whole EMC.

The per-scale `summands` stay informative — they show *where* structure lives — but the total does
not depend on the intermediate scales. [Effective measure complexity](../concepts/emc.md) works
through what that means for interpretation.

<div class="lz-tickrule"></div>

## `-a` versus `-l`

!!! danger "`-l` does not change the reported entropy density. `-a` does."
    Passing `-l 2` to get "bits per symbol" out of `lz76EntropyDensity` silently does nothing — the
    expression the CLI evaluates takes its base from `-a` and never reads `-l` at all. The knob that
    moves the reported entropy density is `-a`. This is the mirror image of the Python API, where
    [`lz.h`](../api/python.md) is controlled by `log_base` and ignores `alphabet`.

The CLI computes the reported entropy density as:

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>c</i> · log<sub><i>α</i></sub>(<i>n</i>) ⁄ <i>n</i> = <i>c</i> · ln(<i>n</i>) ⁄ ( <i>n</i> · ln <i>α</i> )</p>
  <dl class="lz-formula__key">
    <dt><i>c</i></dt><dd><code>lz76Complexity</code>, the complete-component count</dd>
    <dt><i>n</i></dt><dd>sequence length in symbols</dd>
    <dt><i>α</i></dt><dd>the <code>-a</code> value, or the detected alphabet size; floored at 2</dd>
  </dl>
  <p class="lz-formula__cite">The <code>-l</code> log base appears nowhere in this expression.</p>
</div>

Measured on a 200-symbol binary sequence (`c = 30`, detected alphabet size 2):

<div class="lz-scroll lz-compare" markdown>

| Flags | `lz76EntropyDensity` | `lz76RandomShuffleComplexity.value` |
|---|---|---|
| *(none)*, `-a 2`, `-a auto`, `-a xyz`, `-a 0`, `-l 2` | `1.1465784284662086` | `-1.7198676426993127` |
| `-a 4` | `0.5732892142331043` | `-0.8599338213496563` |
| `-a 10` | `0.34515449934959713` | `-0.517731749024395` |
| `-l 4` | `1.1465784284662086` | `-0.8599338213496563` |
| `-l 10` | `1.1465784284662086` | `-0.517731749024395` |
| `-a 4 -l 2` | `0.5732892142331043` | `-1.7198676426993127` |

</div>

Read that table twice. `-a` moves both columns, because when `-l` is absent the log base is copied
from `-a`. `-l` moves only the EMC column. Neither flag changes `lz76Complexity` — the factor count
is alphabet-free. And `alphabet_size` in the report is always the **detected** value, never the
`-a` override.

Values below 2 are clamped to 2, so `-a 0` and `-a 1` behave like `-a 2`.
[Entropy density](../concepts/entropy-density.md) covers units and convergence;
[Alphabets](../concepts/alphabets.md) covers how the alphabet is detected in the first place.

<div class="lz-tickrule"></div>

## Output JSON

### A complete report

Input: a three-line file — one perfectly periodic line, one irregular line, one constant line.

<div class="lz-run" markdown>

```console
$ cat trials.txt
010101010101010101010101010101010101010101010101
011010001101110100110010001011010011101100101101
000000000000000000000000000000000000000000000000
$ lzcomplexity -v -d -e 4:f trials.txt -o trials.lz76.json
 [ Info ] Sequences to process: 3
 [ Info ] Saved results in: trials.lz76.json
```

</div>

```json title="trials.lz76.json — pretty-printed; the file itself is one compact line"
{
  "filename": "trials.txt",
  "format": "AUTO",
  "lz76Distance": {
    "InformationDistance": [
      0.8,
      0.9
    ]
  },
  "sequences": [
    {
      "alphabet": [
        49,
        48
      ],
      "alphabet_size": 2,
      "lz76Complexity": 2,
      "lz76EntropyDensity": 0.23270677086338154,
      "lz76RandomShuffleComplexity": {
        "max_block_size": 4,
        "multi_information": 0.8144736980218353,
        "summands": [
          0.8144736980218353,
          -0.8144736980218353,
          1.7453007814753616,
          -1.7453007814753616
        ],
        "value": 0.0
      },
      "size": 48
    },
    {
      "alphabet": [
        49,
        48
      ],
      "alphabet_size": 2,
      "lz76Complexity": 10,
      "lz76EntropyDensity": 1.1635338543169076,
      "lz76RandomShuffleComplexity": {
        "max_block_size": 4,
        "multi_information": 0.0,
        "summands": [
          0.0,
          0.0,
          0.3490601562950726,
          -0.8144736980218354
        ],
        "value": -0.46541354172676286
      },
      "size": 48
    },
    {
      "alphabet": [
        48
      ],
      "alphabet_size": 2,
      "lz76Complexity": 1,
      "lz76EntropyDensity": 0.11635338543169077,
      "lz76RandomShuffleComplexity": {
        "max_block_size": 4,
        "multi_information": 0.0,
        "summands": [
          0.0,
          0.0,
          0.0,
          0.0
        ],
        "value": 0.0
      },
      "size": 48
    }
  ],
  "size": 3
}
```

Three details in that report are worth reading closely.

The periodic line gets `"value": 0.0` — exactly zero, not a rounded zero. Its summands are two
equal-and-opposite pairs that cancel to the last bit.

The constant line reports `"alphabet": [48]`, a one-element array, next to `"alphabet_size": 2`.
That is not a serialiser bug. `alphabet` lists the distinct bytes actually present;
`alphabet_size` is floored at 2 so the entropy formula never has to evaluate a base-1 logarithm.

`-d` produced two distances for three sequences: cell `i` is the distance between sequence `i` and
sequence `i+1`.

### Top level

| Key | Type | Meaning |
|---|---|---|
| `filename` | string | the input path exactly as you typed it |
| `format` | string | the format resolved **by `-F` or by file extension** — not by magic-number sniffing |
| `size` | integer | number of sequences the reader produced |
| `sequences` | array | the per-sequence objects |
| `lz76Distance` | object | present **only with `-d`** |

!!! note "A `format` of `AUTO` does not mean the format was unknown."
    The field records name-and-extension resolution only. A plain `.txt` file is read as RAWTXT but
    still reports `AUTO`, because that decision is made by the magic-number peek, which happens
    later and is never written down. A `.csv` reports `CSV`; a `.fasta` reports the empty string
    `""` (the C++ lookup table had no FASTA entry, and this port matches it). Both CSV and TSV
    report `CSV`. [Input formats](formats.md) has the full table.

### Per sequence

| Key | Type | Meaning |
|---|---|---|
| `size` | integer | sequence length in symbols |
| `alphabet` | array of int | distinct symbol bytes, sorted descending, **serialised as signed `i8`** |
| `alphabet_size` | integer | count of distinct bytes, floored at 2 |
| `lz76Complexity` | integer | number of **complete** LZ76 components |
| `lz76EntropyDensity` | float | `c * log_α(n) / n` — see [`-a` versus `-l`](#-a-versus-l) |
| `lz76RandomShuffleComplexity` | object | see below |

!!! warning "Alphabet values of 128 and above come out negative."
    The C++ original stored the alphabet in a `std::vector<char>`, signed on x86-64, and this port
    reproduces that serialisation exactly. Byte 255 is written `-1`; byte 128 is written `-128`.
    Recover the unsigned value with `v & 0xff` (Python: `v % 256`). A greyscale row with pixel
    values 0, 64, 128, 255 reports:

    ```json
    "alphabet":[-1,-128,64,0]
    ```

    Text data is unaffected — ASCII is all below 128.

### `lz76RandomShuffleComplexity`

| Key | Type | Meaning |
|---|---|---|
| `value` | float | the EMC total |
| `max_block_size` | integer | the largest block size used; `-1` when `-n` skipped the stage |
| `multi_information` | float | the scale-1 summand |
| `summands` | array of float | **optional** — present only when `-e` requested `f`. One entry per scale, with `sum(summands) == value` and `summands[0] == multi_information` |

The auto block size is a function of length alone: 1 at n = 8, 4 at n = 50, 14 at n = 51 (there is
a `+10` step past 50), 15 at n = 200, 17 at n = 1000, 23 at n = 100 000. Cross-length EMC
comparisons therefore need an explicit `-e=<k>`.

### `lz76Distance` — with `-d` only

```json
"lz76Distance":{"InformationDistance":[0.8,0.9]}
```

`n - 1` floats for `n` sequences; cell `i` is the normalised information distance between sequence
`i` and sequence `i+1`. `-d` implies `-m`, so a single-line file yields one sequence — and in that
case the array is `[0.0]`, a single element rather than an empty list:

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -d random.txt -o O.json
$ python3 -c "import json;print(json.load(open('O.json'))['lz76Distance'])"
{'InformationDistance': [0.0]}
```

</div>

See [Normalized information distance](../concepts/nid.md) for the estimator, and
[`lzdistance`](lzdistance.md) for full matrices.

<div class="lz-tickrule"></div>

## The factors file (`-f`)

`-f` writes a **second, separate file**. It does not add a key to the main report.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n random.txt -f random.factors.json -o C.json
$ cat random.factors.json
{"factors":[[0,1,3,6,10,13,17,21,26,32,37,42,52,59,66,73,79,85,93,101,109,118,127,138,146,153,161,170,179,187,194,201]],"filename":"random.txt","format":14,"size":1}
```

</div>

| Key | Type | Meaning |
|---|---|---|
| `factors` | array of arrays | one raw boundary vector per sequence the reader produced — the same set that reaches `sequences[]`, so `-m` widens both together |
| `filename` | string | the input path |
| `format` | **integer** | the format enum **ordinal**, not the string used everywhere else |
| `size` | integer | number of sequences |

!!! danger "`format` in the factors file is an integer, and it carries a different value from the `format` string in the main report."
    A parser that reads `format` from both files with the same code will break. The factors writer
    emits the raw enum ordinal; the report writer emits a display name. They are not
    interchangeable — and the ordinal is strictly more informative, since it is the only place the
    CSV-versus-TSV distinction survives into the output.

| `-F` | factors `format` | report `format` |
|---|---|---|
| `pbmtxt` | `0` | `"PNM_P1"` |
| `pgmtxt` | `1` | `"PNM_P2"` |
| `pbm` | `3` | `"PNM_P4"` |
| `rawtxt` | `7` | `"PNM_RAWTXT"` |
| `rawbin` | `8` | `"PNM_RAWBIN"` |
| `csv` | `9` | `"CSV"` |
| `tcsv` | `10` | `"CSV"` |
| `dna` | `11` | `""` |
| `fasta` | `13` | `""` |
| *(none, on a `.txt` file)* | `14` | `"AUTO"` |

The full ordinal sequence is P1, P2, P3, P4, P5, P6, P7, RawTxt, RawBin, Csv, Tsv, Dna, Rna,
Fasta, Auto — indices 0 through 14.

### Boundaries, and why the last one overshoots

!!! example "Converting a boundary vector into the textbook component count"
    The 200-symbol sequence above has 32 boundaries ending at `201`, and `lz76Complexity` is 30.

    ```text
    n               = 200
    factors         = [0, 1, 3, 6, …, 194, 201]      (32 entries)
    factors[-1]     = 201  >  200
    lz76Complexity  = 32 − 2 = 30
    ```

    `lz76Complexity` counts only **complete** components. The final component ran past the end of
    the sequence, so it is not counted, and the library's number is one less than the textbook
    exhaustive-history count. The conversion:

    ```python
    c_textbook = c + (1 if factors[-1] > len(seq) else 0)
    ```

    Here `c_textbook = 30 + 1 = 31`. When `factors[-1] == len(seq)` the sequence ended on a
    component boundary and the two counts already agree.

!!! warning "The conversion does not hold for a constant sequence."
    A sequence with fewer than two distinct bytes skips the factorizer: it reports
    `lz76Complexity` 1 with the synthetic vector `[0, 1, n]`. The last boundary equals `n`, so the
    formula returns 1 — but the exhaustive history of `0000…0` is `0 · 000…0`, textbook count 2.
    The constant line in the report above is exactly this case. Guard with
    `len(set(seq)) < 2` before converting.

    Checked against a reference exhaustive-history implementation over 400 random binary and DNA
    strings plus a handful of periodic and degenerate ones: two mismatches, both constant strings.

[LZ76 factorization](../concepts/lz76.md) works through the algorithm and this convention in full,
and repeats the check exhaustively over every binary string of length 2–18.

!!! tip "`-f` costs one extra factorization pass, not one extra full run."
    The factors file is produced by re-factorizing every sequence, which doubles the *plain*
    factorization work — measured 0.73 s → 1.46 s on a 5 000 000-symbol file under `-n`.

    In a default run the shuffle stage dominates, so the extra pass is a smaller share of the wall
    clock, but not as small as the factorization count suggests: the shuffle spreads across the
    thread pool while the `-f` pass is serial. On a 500 000-symbol file (`max_block_size` 25,
    16 cores) the default run went 0.43 s → 0.48 s, about 10 % — not `1/25`. Pair `-f` with `-n`
    when you only want the boundaries.

<div class="lz-tickrule"></div>

## Threads

`-j N` with `N > 0` builds the global rayon thread pool with `N` threads; otherwise rayon uses
hardware concurrency. Only the shuffle stage is parallel, so `-j` barely matters under `-n`.

Results do not depend on the thread count:

<div class="lz-run" markdown>

```console
$ lzcomplexity -j 1 random.txt -o j1.json
$ lzcomplexity -j 8 random.txt -o j8.json
$ cmp j1.json j8.json && echo "byte-identical"
byte-identical
```

</div>

That is a property of the implementation, not an accident of this input — see
[Determinism](../project/determinism.md).

## Exit codes and messages

Diagnostics carry hard-coded ANSI colour codes with no isatty check, so redirecting to a file keeps
the escape sequences. `[ Info ]` goes to stdout, `[ Error ]` to stderr.

| Situation | Message | Exit |
|---|---|---|
| `-V` | `[ Info ] v1.0.0` | 0 |
| no arguments at all | clap help on stdout | 0 |
| flags but no file | `[ Error ] Input file is missing` | 1 |
| file does not exist | `[ Error ] File doesn't exist: <path>` | 1 |
| output directory missing | `[ Error ] No such file or directory (os error 2)` | 1 |
| unknown flag | clap usage error on stderr | 2 |

<div class="lz-tickrule"></div>

## Worked command lines

Every transcript below was run against `random.txt` (200 binary symbols), `trials.txt` (the
three-line file above), `table.csv` and a 4×3 P2 greyscale `img.pgm`.

**1 — Baseline.** Complexity, entropy density and EMC for one time series.

<div class="lz-run" markdown>

```console
$ lzcomplexity -v random.txt
 [ Info ] Sequences to process: 1
 [ Info ] Saved results in: random.lz76.json
$ cat random.lz76.json
{"filename":"random.txt","format":"AUTO","sequences":[{"alphabet":[49,48],"alphabet_size":2,"lz76Complexity":30,"lz76EntropyDensity":1.1465784284662086,"lz76RandomShuffleComplexity":{"max_block_size":15,"multi_information":-0.07643856189774723,"value":-1.7198676426993127},"size":200}],"size":1}
```

</div>

**2 — Fast screening pass.** Entropy density only; the shuffle block is the `-1 / 0.0 / 0.0`
placeholder, and `lz76Complexity` is unchanged.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n random.txt -o B.json
$ cat B.json
{"filename":"random.txt","format":"AUTO","sequences":[{"alphabet":[49,48],"alphabet_size":2,"lz76Complexity":30,"lz76EntropyDensity":1.1465784284662086,"lz76RandomShuffleComplexity":{"max_block_size":-1,"multi_information":0.0,"value":0.0},"size":200}],"size":1}
```

</div>

**3 — One report per line.** Trial by trial, epoch by epoch, read by read.

<div class="lz-run" markdown>

```console
$ lzcomplexity -m trials.txt -o E.json
$ python3 -c "import json;d=json.load(open('E.json'));print(d['size'],[s['lz76Complexity'] for s in d['sequences']])"
3 [2, 10, 1]
```

</div>

**4 — Consecutive-line information distance.** `-d` implies `-m`.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -d trials.txt -o N.json
$ python3 -c "import json;print(json.load(open('N.json'))['lz76Distance'])"
{'InformationDistance': [0.8, 0.9]}
```

</div>

**5 — Every EMC summand at a pinned block size.** Use this whenever you compare EMC across
sequences of different lengths, since the auto block size is length-dependent.

<div class="lz-run" markdown>

```console
$ lzcomplexity -e=3:f random.txt -o Q3.json
$ python3 -c "import json;print(json.load(open('Q3.json'))['sequences'][0]['lz76RandomShuffleComplexity'])"
{'max_block_size': 3, 'multi_information': -0.07643856189774723, 'summands': [-0.07643856189774723, 0.07643856189774723, 0.0], 'value': 0.0}
```

</div>

**6 — Dump the factor boundaries without paying for the shuffle.**

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -m -f trials.factors.json trials.txt -o D.json
$ cat trials.factors.json
{"factors":[[0,1,2,49],[0,1,2,4,7,13,19,24,29,37,45,49],[0,1,48]],"filename":"trials.txt","format":14,"size":3}
```

</div>

**7 — Read a spreadsheet column-wise.** The `.csv` extension selects the CSV reader automatically.
The header row is data, and without `-m` only the first column is read.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n table.csv -o K.json
$ python3 -c "import json;print(json.load(open('K.json'))['format'])"
CSV
```

</div>

**8 — Analyse an image row by row.** Each PGM row becomes its own sequence; pixel values 255 and
128 surface as `-1` and `-128`.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -m img.pgm -o J.json
$ python3 -c "import json;print([s['alphabet'] for s in json.load(open('J.json'))['sequences']])"
[[-1, -128, 64, 0], [-1, -128, 64, 0], [40, 30, 20, 10]]
```

</div>

**9 — Force the alphabet used for normalisation.** Remember that `-l` will not do this.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -a 4 random.txt -o G.json
$ python3 -c "import json;print(json.load(open('G.json'))['sequences'][0]['lz76EntropyDensity'])"
0.5732892142331043
```

</div>

**10 — Single-threaded, for reproducible timing.** The numbers are unchanged; only the wall clock
moves.

<div class="lz-run" markdown>

```console
$ lzcomplexity random.txt -o base.json
$ lzcomplexity -j 1 random.txt -o j1.json
$ cmp j1.json base.json && echo "identical to the default-thread run"
identical to the default-thread run
```

</div>

<div class="lz-tickrule"></div>

Next: [Input formats](formats.md) for how a file becomes a sequence,
[`lzdistance`](lzdistance.md) for all-pairs matrices, and
[Reading the numbers](../guide/reading-the-numbers.md) for what to do with these values.
