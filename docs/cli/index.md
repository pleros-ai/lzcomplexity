# Command-line tools

*Two binaries: `lzcomplexity` measures one file, `lzdistance` compares sequences. Conventions, then a cookbook.*

`lzcomplexity` reads one sequence file and writes a JSON report: LZ76 complexity, entropy density,
effective measure complexity, and — on request — the raw factor boundaries.

`lzdistance` reads one or two sources, where a source is a file or a directory, and writes two
distance matrices over the sequences it finds.

Neither tool reads standard input, and neither prints results to standard output. Both take a path
and write a file; stdout carries only `[ Info ]` lines, plus the help text when the tool is called
with no arguments at all.

<div class="lz-cards" markdown>

<div class="lz-card" markdown>
### lzcomplexity

One file in, one JSON report out. Per sequence: the factor count, the normalised entropy density and
the random-shuffle EMC. `-n` skips the shuffle stage, `-m` treats every line, column, record or image
row as its own sequence, `-f` also dumps the factor boundaries.

<p class="lz-card__api"><code>lzcomplexity -v seq.txt</code></p>

[Full flag reference](lzcomplexity.md)
</div>

<div class="lz-card" markdown>
### lzdistance

One source gives the *n* × *n* self matrix; two sources give the cross matrix. Both the LZ76
information distance and the shuffle distance are always computed. Strategy flags let a cell match a
reversed, bit-flipped, base-swapped or rotated copy of the second operand.

<p class="lz-card__api"><code>lzdistance -L setA.txt setB.txt</code></p>

[Full flag reference](lzdistance.md)
</div>

</div>

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">2</span><span class="lz-stat__k">binaries</span></div>
<div class="lz-stat"><span class="lz-stat__v">11</span><span class="lz-stat__k">input formats</span></div>
<div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">prebuilt targets</span></div>
<div class="lz-stat"><span class="lz-stat__v">3</span><span class="lz-stat__k">exit codes</span></div>
</div>

## Side by side

<div class="lz-scroll" markdown>

| | `lzcomplexity` | `lzdistance` |
|---|---|---|
| Consumes | one file | one or two sources; a source is a file **or** a directory |
| Produces | one report, `sequences[]` | two matrices, `information_distance` and `shuffle_information_distance` |
| What counts as a sequence | the whole input, or one per line/column/record/image row with `-m` | always one per line/column/record/image row; one per file for a directory source |
| Default output path | extension **replaced** — `seq.txt` → `seq.lz76.json` | suffix **appended** — `setA.txt` → `setA.txt.lzdist.json` |
| Verbose | `-v` | `-L` |
| Version | `-V` | `-v` |
| Format override | `-F` | `-I` (first source), `-S` (second source) |
| Alphabet override | `-a <k>` | none — always auto-detected |
| EMC block size | `-e <opts>` | none — always auto |
| Threads | `-j` | `-j` |
| Accepted and ignored | `-p`, `-w` | `-p`, `-d`, `-f`, `-t`, `-l` |

</div>

## Getting them

The binaries live in the `lzcomplexity-cli` crate. That crate is not on crates.io and is not part of
the Python wheel, so `pip install lzcomplexity` does **not** give you these tools.

<div class="lz-run" markdown>

```console
$ cargo build --release -p lzcomplexity-cli
$ ./target/release/lzcomplexity --version
 [ Info ] v1.0.0
$ ./target/release/lzdistance --version
 [ Info ] Version of lzdistance: v1.0.0
```

</div>

Or download the prebuilt assets: every GitHub release carries 8 files — both tools across four
targets (Linux x86-64, macOS arm64, macOS x86-64, Windows x86-64). There is no Linux aarch64 build
and no musl build. [Install](../guide/install.md#the-standalone-binaries) has the asset table and the
note about releases where the binaries exist only as workflow artifacts.

<hr class="lz-tickrule">

## Conventions both tools share

### Format detection runs in three steps

1. The `-F` / `-I` / `-S` name, matched case-insensitively. An unrecognised name silently becomes
   `AUTO` — no error, no warning.
2. The filename extension: `fna`, `fasta`, `gz` → FASTA; `csv` → CSV. `tsv` → TSV **in
   `lzcomplexity` only**; `lzdistance` has no rule for that extension.
3. A peek at the first four bytes of the file, which separates PNM images, raw text and a raw
   bitstream.

The `format` field in the JSON reports steps 1–2, not step 3, so a plain `.txt` file always reads
back as `"format":"AUTO"` even though it was parsed as raw text.

!!! warning

    Step 3 reads text only if the **first three characters** of the first line are all alphanumeric,
    space or tab. Two things break that test: a first line shorter than three characters (`01\n`, or
    a blank line) leaves a NUL in the third slot, and a leading `#` comment or FASTA `>` fails on the
    first slot. Either way the file falls through to RAWBIN and is expanded into `(bytes − 1) × 8`
    bit symbols — a 12-byte file reports `"size": 88`, and a 14-byte FASTA record reports
    `"size": 104` instead of its 10 bases. Pass `-F txt` (or `-F fasta`) for those files.
    [Input formats](formats.md) lists every detection rule and every reader's quirks.

### The default output path replaces or appends, and never uses the cwd

The output always lands next to the input.

| Tool | Input | Default output |
|---|---|---|
| `lzcomplexity` | `periodic.txt` | `periodic.lz76.json` |
| `lzcomplexity` | `sub/nested.dat` | `sub/nested.lz76.json` |
| `lzcomplexity` | `noext` | `noext.lz76.json` |
| `lzcomplexity` | `archive.tar.gz2` | `archive.tar.lz76.json` |
| `lzdistance` | `setA.txt` | `setA.txt.lzdist.json` |
| `lzdistance` | `corpus` | `corpus.lzdist.json` |

Only the final extension is dropped, which is why `archive.tar.gz2` becomes `archive.tar.lz76.json`.
Pass `-o` whenever the derived name matters to a downstream script.

!!! warning

    A trailing slash on a directory source hides the result inside the directory.
    `lzdistance corpus/` writes `corpus/.lzdist.json` — a dotfile in the corpus itself, not
    `corpus.lzdist.json` beside it. Later runs skip it (`.json` files are excluded from directory
    sources), so nothing is corrupted, but you will not find the file where you looked.

### Verbose, version, and no arguments

The two tools disagree on `-v`, and the disagreement is silent.

| Behaviour | `lzcomplexity` | `lzdistance` |
|---|---|---|
| Verbose progress lines | `-v` | `-L` |
| Print version, exit 0 | `-V` | `-v` |

!!! warning

    `lzdistance -v data.txt` computes nothing and writes no matrix. In `lzdistance`, `-v` means
    *version*: it prints one line and exits 0 before touching the input, so a script that passes
    `-v` for verbosity looks like it succeeded and leaves no output file behind. Use `-L`.

Run either tool with no arguments at all and it prints its full help to stdout and exits 0. Pass
other arguments but no input path and it prints an error to stderr and exits 1.

<div class="lz-run" markdown>

```console
$ lzcomplexity | head -4
LempelZiv-76 complexity engine. Suited for complexity analysis of time series.
Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.

Usage: lzcomplexity [OPTIONS] [file]
$ lzcomplexity > /dev/null; echo $?
0
$ lzcomplexity -v
 [ Error ] Input file is missing
$ echo $?
1
```

</div>

### Exit codes

| Code | Meaning | Example |
|---|---|---|
| `0` | Report written, or help / version printed | `lzcomplexity seq.txt` · `lzcomplexity --version` |
| `1` | Missing input, or an I/O error | `lzcomplexity nope.txt` → `[ Error ] File doesn't exist: nope.txt` |
| `2` | Command-line usage error, raised by the argument parser | `lzcomplexity -Z seq.txt` |

`lzdistance` never validates its **second** source, so a typo there surfaces as a code 1 carrying the
raw message `[ Error ] No such file or directory (os error 2)` rather than a named diagnostic.

### Output is compact JSON; messages are always coloured

The JSON file is a single line with no trailing newline, and its keys come out in **alphabetical**
order rather than the order the tool computes them. Pipe it through a formatter to read it:
`python3 -m json.tool seq.lz76.json`.

Terminal messages carry hard-coded ANSI escapes with no TTY check, so redirecting stdout to a file
keeps the colour codes:

<div class="lz-run" markdown>

```console
$ lzcomplexity -v -n random.txt -o p.json | cat -v
^[[1;32m [ Info ] ^[[0mSequences to process: 1
^[[1;32m [ Info ] ^[[0mSaved results in: p.json
```

</div>

### Flags that are accepted and do nothing

`-p/--partitions` is inert in both tools — the core never reads the field it sets. `-w` in
`lzcomplexity` is inert because the Rust port emits no warnings. In `lzdistance`, `-d`, `-f`, `-t`
and `-l` change nothing in the output: `-d` and `-t` are dead flags, `-f fac.json` writes no file,
and `-l` reaches the core but no distance formula consumes a log base — `lzdistance -l 7 setA.txt`
and `lzdistance setA.txt` compare byte-identical under `cmp`.

`-j/--jobs` is real, but it affects wall time only. Verified with `cmp`: `-j 1` and the default
produce byte-identical output files.

<hr class="lz-tickrule">

## The entropy-density quirk

`lz76EntropyDensity` in the JSON is the factor count normalised by the alphabet:

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>C</i> · ln&thinsp;<i>n</i> ⁄ (<i>n</i> · ln&thinsp;<i>k</i>)</p>
  <dl class="lz-formula__key">
    <dt><i>C</i></dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length in symbols</dd>
    <dt><i>k</i></dt><dd>the <code>-a</code> value, or the detected alphabet size; floored at 2</dd>
  </dl>
</div>

The log base cancels out of that expression, so `-l/--log-base` **never moves the entropy density**.
It moves the shuffle stage only, which uses it directly. `-a` moves both, because it also sets the
default log base. Neither flag changes `lz76Complexity` — the factor count does not depend on the
alphabet. Measured on one 200-symbol binary sequence, *C* = 28, detected alphabet size 2:

<div class="lz-scroll" markdown>

| Flags | `lz76Complexity` | `lz76EntropyDensity` | shuffle `value` |
|---|---|---|---|
| *(none)* | 28 | 1.0701398665684614 | −1.1465784284662093 |
| `-a 4` | 28 | 0.5350699332842307 | −0.5732892142331046 |
| `-l 4` | 28 | 1.0701398665684614 | −0.5732892142331046 |
| `-a 4 -l 2` | 28 | 0.5350699332842307 | −1.1465784284662093 |

</div>

Two further things about that column. `alphabet_size` in the report is always the **detected** value,
never your `-a` override, so the report does not record which normalisation produced the number —
keep the command line with the result. And *h* > 1 is normal at these lengths: 1.07 for a fair coin
over two symbols is finite-size bias, not an error.
[Alphabets and log bases](../concepts/alphabets.md) explains which of the two flags you want;
[Sequence length and convergence](../concepts/convergence.md) covers the bias.

<hr class="lz-tickrule">

## Cookbook — `lzcomplexity`

| Command | What it produces |
|---|---|
| `lzcomplexity -v seq.txt` | `seq.lz76.json`: complexity, entropy density and EMC for the whole file |
| `lzcomplexity -n big.txt -o big.json` | the same minus the shuffle stage — the fast screening pass |
| `lzcomplexity -m lines.txt` | one entry in `sequences[]` per non-blank line |
| `lzcomplexity -d lines.txt` | adds `lz76Distance.InformationDistance` between consecutive lines; implies `-m` |
| `lzcomplexity -e=f seq.txt` | adds the per-scale EMC `summands` array |
| `lzcomplexity -e 5:f seq.txt` | pins the EMC block size to 5 — required to compare EMC across lengths |
| `lzcomplexity -f seq.factors.json seq.txt` | also writes the raw factor-boundary vectors |
| `lzcomplexity -m table.csv` | one sequence per column; the header row is data |
| `lzcomplexity -F fasta genes.dat` | forces the FASTA reader when the extension does not say so |

!!! example

    A default run over a 200-symbol random binary file, start to finish.

    ```console
    $ lzcomplexity -v random.txt
     [ Info ] Sequences to process: 1
     [ Info ] Saved results in: random.lz76.json
    $ python3 -m json.tool random.lz76.json
    ```

    ```json
    {
        "filename": "random.txt",
        "format": "AUTO",
        "sequences": [
            {
                "alphabet": [49, 48],
                "alphabet_size": 2,
                "lz76Complexity": 28,
                "lz76EntropyDensity": 1.0701398665684614,
                "lz76RandomShuffleComplexity": {
                    "max_block_size": 15,
                    "multi_information": 0.07643856189774723,
                    "value": -1.1465784284662093
                },
                "size": 200
            }
        ],
        "size": 1
    }
    ```

    `alphabet` is `[49, 48]` — the ASCII bytes of `1` and `0`, sorted descending. `max_block_size`
    was chosen from the sequence length. Under `-n` the whole `lz76RandomShuffleComplexity` object is
    emitted as the placeholder `{"max_block_size": -1, "multi_information": 0.0, "value": 0.0}`, so
    downstream schemas stay stable.

!!! tip

    `-n` cut a 1,000,000-symbol run from 12.3 s of CPU time to 0.18 s; wall clock on a 16-core box
    went 1.74 s → 0.19 s in the same measurement (medians of five runs, release build). The shuffle
    stage runs `max_block_size` extra full factorizations — 26 of them at that length — spread over
    every core, and in a default run it is the only parallel stage. `lz76Complexity` and
    `lz76EntropyDensity` are unaffected and still reported in full.

The factor boundaries written by `-f` are the raw vector, and its last element can run past the end
of the sequence. On a 10-symbol line the boundaries `[0, 1, 2, 11]` give `lz76Complexity` = 2:

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -f multi.factors.json multi.txt
$ cat multi.factors.json
{"factors":[[0,1,2,11]],"filename":"multi.txt","format":14,"size":1}
```

</div>

!!! danger

    The reported complexity is one less than the textbook count whenever a sequence ends
    mid-component. `lzcomplexity` counts only **complete** LZ76 components; the trailing component
    that runs past the end of the sequence is not counted. Convert with
    `c_textbook = c + (1 if factors[-1] > len(seq) else 0)` — no mismatches over 988 random binary
    strings of length 2–80. The rule has one exception: a sequence of a single repeated symbol
    short-circuits to `c = 1` with boundaries `[0, 1, n]`, so the conversion returns 1 where the
    textbook count is 2. Check which convention a published figure used before comparing against it.
    [LZ76 factorization](../concepts/lz76.md) works through both.

!!! note

    The `summands` array from `-e=f` is worth reading scale by scale, but the total is not a sum over
    scales in any meaningful sense. The EMC sum telescopes, reducing exactly to
    `mm · g · (C_LZ(shuffled at mm) − C_LZ(original))`, so only the largest block size survives into
    `value` — the other `mm − 1` factorizations are computed and then cancelled out.

    A `value` of 0.0 therefore means "the block shuffle did not change the factor count", which is
    not the same as "no structure". On 1,000-symbol inputs (`mm` = 17) a period-17 sequence returns
    `8.9e-16`, but a period-2 sequence returns `2.033`, a period-4 sequence `4.235` and a random
    binary sequence `0.339`. Exactly the periods that divide `mm` collapse.
    [Effective measure complexity](../concepts/emc.md) derives it.

Every flag, the complete JSON schema and the `-e` parsing rules: [`lzcomplexity`](lzcomplexity.md).

## Cookbook — `lzdistance`

| Command | What it produces |
|---|---|
| `lzdistance -L setA.txt` | the *n* × *n* self matrix over the file's lines |
| `lzdistance setA.txt setB.txt` | the cross matrix, `first_dim` × `second_dim` |
| `lzdistance -L -I fasta genomes` | all-pairs over a directory — one sequence per file, sorted by path, `.json` and `.log` skipped |
| `lzdistance setA.txt setB.txt -b` | each cell is the minimum over `b`, its reverse, its bit-flip and both |
| `lzdistance setA.txt setB.txt -a` | the same idea for DNA: A↔T and C↔G swaps, one pair at a time |
| `lzdistance setA.txt -g 1` | adds `directed_matrix`, the signed complexity asymmetry above threshold 1 |
| `lzdistance setA.txt -i 2:2` | computes row 2 only |

<div class="lz-run" markdown>

```console
$ lzdistance -L setA.txt -o selfd.json
 [ Info ] first_dim: 4  second_dim: 4
 [ Info ] Saved results in: selfd.json
$ python3 -c "import json;[print(r) for r in json.load(open('selfd.json'))['information_distance']]"
[0.0, 0.6666666666666666, 0.5, 0.5]
[0.6666666666666666, 0.3333333333333333, 0.6666666666666666, 0.6666666666666666]
[0.5, 0.6666666666666666, 0.0, 1.0]
[0.5, 0.6666666666666666, 1.0, 0.0]
```

</div>

The four lines of `setA.txt` were `0101010101`, `1100110011`, `0000000000` and `1111111111`. Look at
the diagonal: `information_distance[i][i]` is `(C(XX) − C(X)) / C(X)`, which is 0 for three of them
and 0.333… for the second. This is the LZ76 estimator of the normalized information distance, with
`C_LZ` standing in for Kolmogorov complexity — not a metric with a guaranteed zero diagonal.
[Normalized information distance](../concepts/nid.md) states how far the guarantees actually go.

!!! warning

    Row and column gates leave uncomputed cells at exactly `0.0`, which reads as a perfect match.
    `-i` and `-s` restrict which cells are computed; every other cell keeps its initialised zero,
    indistinguishable from a genuine zero distance. And `-i 2` means *lines 2 to the end*, not
    "line 2" — write `-i 2:2` for a single line.

!!! warning

    Bare `-g` and `-g 0` pay for the directed matrix and then discard it. The matrix is computed when
    a threshold is supplied but written only when the threshold is non-zero, and those two conditions
    disagree at zero. Use `-g 1`, or `-g=-1` for raw differences — `-g -1` is a usage error, exit 2.
    Without `-g` at all, the output still carries an empty `"directed_matrix":[]`.

Strategies, matrix symmetry, directory rules and the full schema: [`lzdistance`](lzdistance.md). For
the workflow around a whole corpus, see [Comparing many sequences](../recipes/batch-distance.md).

<hr class="lz-tickrule">

## Where next

<div class="lz-cards" markdown>

<div class="lz-card" markdown>
### lzcomplexity

Every flag, the `-e` grammar, the JSON schema, and what `-n`, `-m` and `-d` change.

<p class="lz-card__api"><code>lzcomplexity -e 5:f seq.txt</code></p>

[Open the reference](lzcomplexity.md)
</div>

<div class="lz-card" markdown>
### lzdistance

Both matrices, the four transform strategies, directory sources and the directed graph.

<p class="lz-card__api"><code>lzdistance -b setA.txt setB.txt</code></p>

[Open the reference](lzdistance.md)
</div>

<div class="lz-card" markdown>
### Input formats

Magic-number detection, CSV headers as data, the dropped last byte of RAWBIN, PBM and PGM rasters.

<p class="lz-card__api"><code>lzcomplexity -F tcsv table.dat</code></p>

[Read the rules](formats.md)
</div>

<div class="lz-card" markdown>
### Python instead

The same core, five functions, no JSON round-trip. Usually the better choice inside an analysis.

<p class="lz-card__api"><code>import lzcomplexity as lz</code></p>

[Python API](../api/python.md)
</div>

</div>
