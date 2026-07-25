# lzdistance

*All-pairs LZ76 distance matrices between sequences, files, or whole directories, written as JSON.*

`lzdistance` reads one or two data sources, computes **two** full distance matrices over every pair
of sequences, and writes them to a single compact JSON object. It never prints the matrices —
`-L` prints only the dimensions and the output path.

```
lzdistance [OPTIONS] [first source] [second source]
```

<div class="lz-stats">
  <div class="lz-stat"><div class="lz-stat__v">2</div><div class="lz-stat__k">matrices, always emitted</div></div>
  <div class="lz-stat"><div class="lz-stat__v">4</div><div class="lz-stat__k">factorizations per NID cell</div></div>
  <div class="lz-stat"><div class="lz-stat__v">16</div><div class="lz-stat__k">variants under <code>-y</code></div></div>
  <div class="lz-stat"><div class="lz-stat__v">5</div><div class="lz-stat__k">dead or inert flags</div></div>
</div>

<div class="lz-run" markdown>

```console
$ lzdistance -L -I fasta genomes -o genomes.lzdist.json
 [ Info ] first_dim: 4  second_dim: 4
 [ Info ] Saved results in: genomes.lzdist.json
```

</div>

Matrix cells on this page are written `[row][column]` and are **0-based**, matching the JSON arrays.
The line-range flags `-i` and `-s` are 1-based; that difference is the tool's, not this page's.

---

## Flags

Every flag, with its real default. Five of them do nothing — see [Dead flags](#dead-flags).

<div class="lz-scroll" markdown>

| Short | Long | Value | Default | Effect |
|---|---|---|---|---|
| — | — | `[first source]` | none | File or directory. Required. |
| — | — | `[second source]` | none | File or directory. Omitted → self-comparison. |
| `-a` | `--adn` | flag | off | DNA dispatch strategy. **Not "alphabet"** — `lzdistance` has no alphabet flag. |
| `-b` | `--binary` | flag | off | Binary dispatch strategy (reverse- and flip-aware). |
| `-d` | `--default` | flag | `true` | **Dead.** Never read. |
| `-f` | `--factors` | `<file_name>` | none | **Dead.** Parsed and never read; no file is written. |
| `-g` | `--get-direction` | `[<threshold>]`, implicit `0` | absent | Directed graph. See [`-g`](#the-directed-graph-g). |
| `-I` | `--first-format` | `<value>` | `AUTO` | Format of the first source. Names in [Input formats](formats.md). |
| `-S` | `--second-format` | `<value>` | `AUTO` | Format of the second source. |
| `-i` | `--first` | `#:#` | unset | Row gate. See [Line ranges](#line-ranges-i-and-s). |
| `-s` | `--second` | `#:#` | unset | Column gate. |
| `-j` | `--jobs` | `<value>` | hardware concurrency | rayon pool size. Wall-clock only; results are identical. |
| `-l` | `--log-base` | `<value>` | alphabet cardinality | **Inert.** No observable effect on the output. |
| `-L` | `--logs` | flag | off | Verbose: prints `first_dim`/`second_dim` and the output path. |
| `-r` | `--reverse` | flag | off | Reverse dispatch strategy. |
| `-o` | `--output` | `<file_name>` | `<first source>.lzdist.json` | Output path. |
| `-p` | `--partitions` | `<value>` | `2` | **No-op.** The core never reads it. |
| `-t` | `--text` | flag | off | **Dead.** Never read. |
| `-y` | `--trajectory` | flag | off | Trajectory (rotation) dispatch strategy. |
| `-v` | `--version` | flag | off | Prints `Version of lzdistance: v1.0.0` and exits 0. |
| `-h` | `--help` | flag | — | Help, exit 0. |

</div>

!!! danger "`lzdistance -v mydata.txt` computes nothing and exits 0."
    `-v` is **version** in `lzdistance` and **verbose** in [`lzcomplexity`](lzcomplexity.md). The
    version check runs before source validation, so a perfectly good path on the command line is
    ignored and no error is printed. Verbose output here is `-L`.

The default output path is the first source with `.lzdist.json` **appended** — `setA.txt` →
`setA.txt.lzdist.json`, `corpus` → `corpus.lzdist.json`. (`lzcomplexity` *replaces* the extension
instead; the two tools disagree deliberately, for C++ parity.)

!!! warning "`lzdistance corpus/` hides its own output inside the corpus."
    A trailing slash makes the default output path `corpus/.lzdist.json` — a dotfile inside the
    directory you analysed. It does not corrupt later runs, because `.json` files are skipped
    when a directory is read, but it is easy to lose. Drop the slash, or pass `-o`.

---

## The two matrices

Both are computed on every run and both are always written.

### `information_distance` — the NID

<div class="lz-formula">
  <p class="lz-math"><i>d</i><sub>NID</sub>(<i>X</i>,<i>Y</i>) = max{ <i>C</i>(<i>XY</i>) − <i>C</i>(<i>X</i>), <i>C</i>(<i>YX</i>) − <i>C</i>(<i>Y</i>) } ⁄ max{ <i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>) }</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count — complete components only</dd>
    <dt><i>XY</i></dt><dd>concatenation of X then Y</dd>
    <dt><i>YX</i></dt><dd>concatenation of Y then X — the greedy parse is not order-symmetric</dd>
  </dl>
  <p class="lz-formula__cite">The LZ76 estimator of the normalized information distance.</p>
</div>

Four factorizations per cell — per candidate variant, once a
[dispatch strategy](#dispatch-strategies) is in play. `C(·)` is the same count
[`lzcomplexity`](lzcomplexity.md) reports as
`lz76Complexity`: it counts only **complete** LZ76 components, so a trailing component that runs
past the end of the sequence is dropped. Every distance on this page inherits that convention.

### `shuffle_information_distance` — the shuffle-based distance

<div class="lz-formula">
  <p class="lz-math"><i>d</i><sub>shuffle</sub>(<i>X</i>,<i>Y</i>) = <i>m</i> · <i>C</i>(<i>XY</i>) ⁄ Σ<sub><i>l</i>=1..<i>m</i></sub> <i>C</i>((<i>XY</i>)<sup>RS(<i>l</i>)</sup>)</p>
  <dl class="lz-formula__key">
    <dt><i>m</i></dt><dd>largest block size, and the number of surrogates. Chosen from |<i>XY</i>| alone: a fixed-point estimate of the <i>M</i> solving |<i>XY</i>| = <i>M</i>·2<sup><i>M</i></sup> (capped at 100 iterations, so it need not converge), plus 10 once |<i>XY</i>| &gt; 50. For the 200 + 200-symbol pairs below, <i>m</i> = 16.</dd>
    <dt>(<i>XY</i>)<sup>RS(<i>l</i>)</sup></dt><dd>the concatenation after |<i>XY</i>|/2 block swaps at block size <i>l</i></dd>
  </dl>
  <p class="lz-formula__cite">Equivalently 1 − MI(X,Y), the shuffle-surrogate mutual-information estimator.</p>
</div>

Read it as "how much less complex is `XY` than its own block-shuffled surrogates". It is a property
of the concatenation, not a comparison of two separate objects: `X` and `Y` are never factorized
apart. Calling it a mutual information is generous — nothing in it isolates what the two operands
share.

!!! danger "`shuffle_information_distance` is asymmetric and is not bounded by 1."
    Only `X‖Y` is ever formed, so `[i][j]` and `[j][i]` are different quantities computed from
    different strings. Values above 1 are ordinary, not pathological: the cross-matrix run below
    produced `1.023839`, `1.003841` and `1.015228` on 200-symbol random binary lines. Do not feed
    this matrix to code that assumes a symmetric, `[0,1]`-bounded dissimilarity.

### Properties

<div class="lz-scroll" markdown>

| | `information_distance` | `shuffle_information_distance` |
|---|---|---|
| Inputs used | `C(X)`, `C(Y)`, `C(XY)`, `C(YX)` | `C(XY)` and `m` shuffles of `XY` |
| Symmetric as a *function* | **yes**, exactly | **no** |
| Symmetric as a *matrix* | only under the conditions below | **no** |
| Range | `[0, 1]` in all testing | `(0, ∞)`; routinely above 1, never exactly 0 |
| Diagonal | `(C(XX) − C(X)) / C(X)` — usually `1/C(X)`; 0 only when `C(XX) = C(X)` | not 0, and no fixed value |
| Triangle inequality | **fails** — see [Information distance (NID)](../concepts/nid.md) | not a metric |
| Cost per cell | 4 factorizations **per variant** | 1 + `m` factorizations **per variant** |
| Deterministic | yes | yes — shuffle seeds are derived from the content |

</div>

!!! warning "The `information_distance` matrix is symmetric only in the plainest case."
    The formula is symmetric in `X` and `Y`, but the matrix is symmetric only when all three hold:
    one source, the default strategy, and no `-i`/`-s`. Every [dispatch strategy](#dispatch-strategies)
    transforms the second operand and not the first, so `[i][j]` and `[j][i]` compare different
    strings; gating writes zeros into whole rows. `lzdistance setA.txt` gives a symmetric 4×4
    matrix; `lzdistance setA.txt -b` on the same file gives `[0][2] = 0.700000` against
    `[2][0] = 0.750000`.

### Dimensions

| Sources | Shape | Keys emitted |
|---|---|---|
| One source, `n` sequences | `n × n` self matrix | `first_data_source`, `first_data_source_format`, `first_dim` |
| Two sources, `n` and `m` sequences | `n × m` cross matrix | the three above, plus `second_data_source`, `second_data_source_format`, `second_dim` |

The `second_*` keys are emitted whenever the second **path string** was non-empty — independently of
whether it yielded any sequences.

!!! warning "An empty second directory silently gives you the self matrix."
    `lzdistance tiny.txt emptydir` writes `"second_dim":0` and a **3×3 matrix of `tiny.txt` against
    itself**, because an empty second source falls back to the first. `-L` compounds it by printing
    the *effective* dimension: `first_dim: 3  second_dim: 3`. Trust the JSON's `second_dim`, not the
    log line.

---

## Sources

### File source

A file is read with multi-line semantics **always on**. What "one sequence" means depends on the
format, exactly as documented in [Input formats](formats.md):

| Format | One sequence per… |
|---|---|
| RAWTXT | non-blank line |
| CSV / TSV | column |
| FASTA / DNA / RNA | record |
| PBM / PGM | image row |
| RAWBIN | the whole file — one sequence |

### Directory source

A directory is read non-recursively and yields **one sequence per file**:

- Files are sorted by path in byte order, and that order is the matrix row/column order.
- `.json` and `.log` files are skipped. The match is case-sensitive — `.JSON` is *not* skipped.
- Sub-directories are ignored, not descended into.
- Only the **first** sequence of each file is used: the first non-blank line of a text file, the
  first FASTA record, the first CSV column, the whole raster of an image.
- A file that yields no sequences contributes an **empty sequence**, not a skip.
- `-I` / `-S` apply to *every* file in the directory. Extension-based auto-detection is disabled for
  directory sources, so a directory of `.fasta` files still needs an explicit `-I fasta`.

!!! example "A directory of four genomes, one FASTA record each"
    Four 3 000 bp FASTA files. `strain_a` is the reference; `strain_b` differs from it at 0.8 % of
    sites, `strain_c` at 7.4 %, and `unrelated` is an independent random sequence (76 % of sites
    differ). `notes.json` and `run.log` sit in the same directory and are skipped.

    ```console
    $ lzdistance -L -I fasta genomes -o genomes.lzdist.json
     [ Info ] first_dim: 4  second_dim: 4
     [ Info ] Saved results in: genomes.lzdist.json
    ```

    `information_distance`, printed with the labels the JSON does not carry:

    ```text
                strain_a   strain_b   strain_c   unrelated
    strain_a    0.001949   0.046693   0.385057   0.842412
    strain_b    0.046693   0.001946   0.423372   0.840467
    strain_c    0.385057   0.423372   0.001916   0.831418
    unrelated   0.842412   0.840467   0.831418   0.001946
    ```

    The near mutant sits about eight times closer to the reference than the far one, and both sit
    far from the unrelated sequence. The diagonal is **not zero**: here `C(XX) = C(X) + 1` for all
    four, so it is `1/C(X)` — `C` is 513, 514, 522 and 514 respectively, giving 0.001949, 0.001946,
    0.001916 and 0.001946. That floor shrinks as sequences get longer, but it never vanishes.

    This run also writes `"first_data_source_format":""`, not `"fasta"` — see
    [Key reference](#key-reference).

---

## Line ranges `-i` and `-s`

`-i` gates the **rows** (the first source), `-s` gates the **columns** (the second source, or the
first again in a self-comparison). Both take a `#:#` range and both are **1-based**.

| Flag | Selects |
|---|---|
| *(absent)*, `-i :`, `-i 0` | everything |
| `-i 2` | lines **2 to the end** — *not* "line 2 only" |
| `-i 2:` | identical to `-i 2` |
| `-i 2:2` | line 2 only |
| `-i 2:3` | lines 2 and 3 |
| `-i :3` | lines 1 to 3 |
| `-i 3:2` | nothing — a reversed range selects no line |

The gating rule, for a 1-based line number `L`, an unset end meaning "no upper bound":

1. Both ends unset → accept.
2. `L == init` and (`end` unset or `init == end`) → accept.
3. Otherwise accept iff (`init` unset or `init ≤ L`) **and** (`end` unset or `end ≥ L`).

Rule 2 is redundant — every line it accepts, rule 3 accepts too.

!!! danger "Gated-out cells are written as `0.0`, not omitted."
    On `information_distance` an excluded cell is byte-identical to a cell whose distance genuinely
    is zero, and zero is a common value there — over 1 440 cells of random binary fixtures it came
    up 128 times. Nothing in the JSON records which range was applied. Put the range in the output
    filename, or reconstruct the mask yourself before you read the matrix.

    `shuffle_information_distance` is the one place you get a free check: its quotient
    `m · C(XY) / Σ C((XY)^RS(l))` has a numerator of at least `m`, so it is strictly positive and a
    `0.0` there can *only* be a gated cell. It was never 0 in those same 1 440 cells.

    Both matrices are gated identically. A rejected row is written as an all-zero row; a rejected
    column is skipped inside every surviving row.

!!! example "Row and column gates on a 4×3 cross matrix"
    `setA.txt` holds four 200-symbol random binary lines, `setB.txt` three. Take rows 1–2 and
    columns 2–3:

    ```console
    $ lzdistance setA.txt setB.txt -i 1:2 -s 2:3 -o gated.json
    ```

    ```text
    information_distance              shuffle_information_distance
    [0.0, 0.750000, 0.733333]         [0.0, 0.987406, 0.970923]
    [0.0, 0.821429, 0.733333]         [0.0, 1.023839, 0.995025]
    [0.0, 0.0,      0.0     ]         [0.0, 0.0,      0.0     ]
    [0.0, 0.0,      0.0     ]         [0.0, 0.0,      0.0     ]
    ```

    Column 0 and rows 2–3 were never computed. The `1.023839` in the shuffle matrix is a real value,
    not an artifact of the gating.

---

## Dispatch strategies

A strategy generates a list of transformed copies of the **second** operand. The first operand is
never transformed. The reported cell is the **minimum** distance over all variants.

<div class="lz-scroll" markdown>

| Flag | Strategy | Variants of the second operand `b` | Count |
|---|---|---|---|
| *(none)* | Default | `b` | 1 |
| `-r` | Reverse | `reverse(b)` **only** — the identity is not a candidate | 1 |
| `-b` | Binary | `b`, `reverse(b)`, `flip(b)`, `reverse(flip(b))` | 4 |
| `-a` | DNA | `b`, `reverse(b)`, `at(b)`, `reverse(at(b))`, `cg(b)`, `reverse(cg(b))` | 6 |
| `-y` | Trajectory | for each `t` = 0…7: `reverse(rot_t(b))` and `rot_t(b)` | 16 |

</div>

Precedence when several are given: **`-b` > `-a` > `-y` > `-r` > default**, first match wins.
Verified: `lzdistance setA.txt setB.txt -b -r -y -a` produces a byte-identical file to `-b` alone.

The two matrices minimise **independently**, so the NID cell and the shuffle cell in the same
position may come from different variants.

### What the transforms actually do

`flip`
:   `'0' → '1'`, and **every other byte → `'0'`**. Only meaningful on ASCII `0`/`1` data.

`at` / `cg`
:   `at` swaps A↔T **and lower-cases them**, leaving C and G untouched. `cg` swaps C↔G and
    lower-cases them, leaving A and T untouched. The two are never composed, so **there is no
    reverse-complement variant**; a true complement is matched only when the plain `reverse`
    candidate happens to match.

`rot_t`
:   `x → '1' + ((x − '0') + t − 1) mod 8`. The output is always one of `'1'`…`'8'`, whatever the
    input byte was.

!!! example "`-b` recovers a reversal and a bit-flip"
    `setA.txt` is four independent 200-symbol random binary lines. `setB[0] = reverse(setA[1])`,
    `setB[1] = flip(setA[0])`, `setB[2]` is unrelated random.

    ```console
    $ lzdistance setA.txt setB.txt -o plain.json
    $ lzdistance setA.txt setB.txt -r -o rev.json
    $ lzdistance setA.txt setB.txt -b -o bin.json
    ```

    `information_distance` under each:

    ```text
    default   [0.724138, 0.750000, 0.733333]
              [0.655172, 0.821429, 0.733333]
              [0.758621, 0.785714, 0.700000]
              [0.724138, 0.750000, 0.733333]

    -r        [0.714286, 0.750000, 0.677419]
              [0.035714, 0.785714, 0.709677]
              [0.750000, 0.785714, 0.709677]
              [0.785714, 0.785714, 0.741935]

    -b        [0.714286, 0.035714, 0.666667]
              [0.035714, 0.714286, 0.709677]
              [0.750000, 0.750000, 0.677419]
              [0.689655, 0.750000, 0.700000]
    ```

    Cell `[1][0]` falls 0.655172 → 0.035714 once reversal is a candidate; cell `[0][1]` falls
    0.750000 → 0.035714 once the bit-flip is. Unrelated cells drop too — `[0][0]` goes 0.724138 →
    0.714286 under `-b` — because a minimum over four candidates is always ≤ the single default
    candidate. Compare rows within one strategy, never across strategies.

!!! warning "`-y` raises distances on anything that is not 1–8 data."
    `rot_t` rewrites every input byte into `'1'`…`'8'`, so on other alphabets the identity is
    **not** among the 16 variants: the second operand ends up in an alphabet the untransformed
    first operand barely shares, and every concatenation looks more complex. On the binary fixtures
    above all twelve cells rose — `[0][0]` went 0.724138 → 0.965517, `[1][1]` went 0.821429 →
    1.000000.

    On genuine trajectory data drawn from `'1'`…`'8'` it behaves as intended. Two 200-symbol
    trajectories, where `trajB[0]` is `trajA[0]` rotated by +3 and `trajB[1]` is `trajA[1]`
    reversed:

    ```text
    default   [0.797297, 0.810811]      -y   [0.013514, 0.756757]
              [0.783784, 0.783784]           [0.756757, 0.013514]
    ```

!!! warning "`-a` never forms a reverse complement."
    Because `at` and `cg` are separate one-pair swaps, a sequence and its full complement are not
    related by any of the six variants. With fixtures built to match the implementation exactly
    (`base = "TGCA"×50`, `at = "aGCt"×50`, `cg = "TcgA"×50`), `-a` does collapse both single-pair
    swaps to zero:

    ```text
    default   [[0.5], [0.5], [0.0]]
    -a        [[0.0], [0.0], [0.0]]
    ```

    It does not collapse a complement. A random 300 bp sequence against its complement, its
    reverse complement and itself:

    ```text
                complement   revcomp   identity
    default     0.8116       0.7733    0.0145
    -a          0.7733       0.7733    0.0145
    ```

    `-a` moves the complement column only as far as the plain `reverse` candidate already reaches.
    For complement-aware genomics, reverse-complement your sequences yourself before calling the
    tool. See [DNA and FASTA](../recipes/genomics.md).

---

## The directed graph `-g`

`-g` builds an integer matrix of "which sequence explains which". For each pair it computes
`diff = C(Y‖X) − C(X‖Y)` and compares `|diff|` against the threshold.

### Compute-versus-save asymmetry

The matrix is **computed** when a threshold was supplied, and **written** when the threshold is
non-zero. Those two conditions do not agree:

| Invocation | Threshold | Computed | `directed_matrix` in the JSON |
|---|---|---|---|
| *(no `-g`)* | none | no | present, **`[]`** |
| `-g` (bare) | `0` | **yes** | **absent** — computed, then discarded |
| `-g 0` | `0` | yes | **absent** — computed, then discarded |
| `-g 1` | `1` | yes | present, populated |
| `-g=-1` | `-1` | yes | present, populated |

!!! danger "Bare `-g` and `-g 0` pay the full O(n²) cost and then throw the result away."
    Both compute every pairwise double concatenation and then omit the key entirely, so you get a
    slower run and less output. Use `-g 1` for a threshold of one factor, or `-g=-1` for raw signed
    differences everywhere.

    A default run always carries an empty `"directed_matrix":[]`. That empty array means "not
    requested" — never "no structure found".

    Negative thresholds need the `=` form. `-g -1` is a clap usage error (`unexpected argument '-1'
    found`, exit 2), **not** a bare `-g`.

### Cell convention

- `1` means **"no significant direction"** — `|diff| ≤ threshold`.
- Otherwise the pair is antisymmetric: `−diff` at `[i][j]` and `+diff` at `[j][i]`.
- Untouched cells stay `0`: the diagonal in the one-source case, and both diagonal blocks in the
  two-source case.

So `1` is a result and `0` is an absence, which is the opposite of what most adjacency conventions
assume. A matrix of all `1`s off the diagonal is a valid answer meaning *nothing exceeded the
threshold*. Under `-g=-1` a cell can also be `0` because `diff` happened to be zero, which is then
indistinguishable from an untouched cell.

### Shape

| Sources | Shape | Layout |
|---|---|---|
| One source, `n` sequences | `n × n` | upper triangle computed, mirrored antisymmetrically; diagonal `0` |
| Two sources, `n` and `m` | `(n + m) × (n + m)` | the two off-diagonal blocks are filled; both diagonal blocks stay `0` |

!!! example "Both regimes on the same fixtures"
    One source, four 200-symbol binary lines, threshold 1 — nothing exceeds it:

    ```console
    $ lzdistance setA.txt -g 1 -o directed.json
    ```
    ```json
    "directed_matrix":[[0,1,1,1],[1,0,1,1],[1,1,0,1],[1,1,1,0]]
    ```

    The same file with `-g=-1`, so `|diff| > -1` is always true and every computed cell carries the
    raw difference:

    ```console
    $ lzdistance setA.txt -g=-1 -o raw.json
    ```
    ```json
    "directed_matrix":[[0,0,0,0],[0,0,0,1],[0,0,0,1],[0,-1,-1,0]]
    ```

    Two sources (4 + 3 sequences) with threshold 1 — a 7×7 block matrix:

    ```console
    $ lzdistance setA.txt setB.txt -g 1 -o directed-cross.json
    ```
    ```text
    [0,  0, 0, 0,  1,  1, -4]
    [0,  0, 0, 0, -2,  1, -2]
    [0,  0, 0, 0,  1,  1, -2]
    [0,  0, 0, 0,  1,  1, -2]
    [1,  2, 1, 1,  0,  0,  0]
    [1,  1, 1, 1,  0,  0,  0]
    [4,  2, 2, 2,  0,  0,  0]
    ```

    Rows and columns 0–3 are the first source, 4–6 the second. Cell `[0][6]` reads `-4` and `[6][0]`
    reads `+4`: `setB[2]` costs four fewer factors when prefixed by `setA[0]` than the other way
    round.

---

## Output JSON

One object, written compact — no whitespace, no trailing newline. Keys come out in **alphabetical**
order, not insertion order, because the writer serialises a `BTreeMap`.

### One source

Three short lines: `abcdabcdabcd`, `abcdabceabcd`, `wxyzwxyzwxyz`.

```console
$ lzdistance tiny.txt
```

```json
{"directed_matrix":[],"first_data_source":"tiny.txt","first_data_source_format":"AUTO","first_dim":3,"information_distance":[[0.0,0.2,1.0],[0.2,0.2,1.0],[1.0,1.0,0.0]],"shuffle_information_distance":[[0.48,0.5769230769230769,0.6486486486486487],[0.6666666666666666,0.6206896551724138,0.6923076923076923],[0.6153846153846154,0.6585365853658537,0.48]]}
```

Three things to read off it. `information_distance` is symmetric, and its diagonal is
`[0.0, 0.2, 0.0]` — the middle sequence has a non-zero self-distance.
`shuffle_information_distance` is not symmetric: `[0][1] = 0.5769…` against `[1][0] = 0.6666…`.
And `directed_matrix` is present but empty, because `-g` was not given.

### Two sources

`tiny.txt` (3 sequences) against `tiny2.txt`, which holds `abcdabcdabcd` and `zyxwzyxwzyxw`:

```console
$ lzdistance tiny.txt tiny2.txt -o tiny-cross.json
```

```json
{"directed_matrix":[],"first_data_source":"tiny.txt","first_data_source_format":"AUTO","first_dim":3,"information_distance":[[0.0,1.0],[0.2,1.0],[1.0,0.75]],"second_data_source":"tiny2.txt","second_data_source_format":"AUTO","second_dim":2,"shuffle_information_distance":[[0.48,0.631578947368421],[0.6666666666666666,0.7105263157894737],[0.6153846153846154,0.6774193548387096]]}
```

Both matrices are 3×2, and the three `second_*` keys have appeared.

### Key reference

| Key | Type | Present when |
|---|---|---|
| `first_data_source` | string | always — the path exactly as typed |
| `first_data_source_format` | string | always — but see the name table below |
| `first_dim` | integer | always — sequences read from the first source |
| `second_data_source` | string | the second path string was non-empty |
| `second_data_source_format` | string | as above |
| `second_dim` | integer | as above — **sequences read**, which may be 0 while the matrix is not empty |
| `information_distance` | array of arrays of float | always |
| `shuffle_information_distance` | array of arrays of float | always |
| `directed_matrix` | array of arrays of integer | always **except** under bare `-g` / `-g 0` |

The two `*_format` values are *not* the names you pass to `-I`/`-S`. They are the C++ tool's
serialised enum names, and three of them are lossy:

| `-I` value | Written as |
|---|---|
| *(unset, no extension rule matched)*, **or any unrecognised name** | `AUTO` |
| `text` / `txt` / `rawtxt` | `PNM_RAWTXT` |
| `raw` / `bin` / `rawbin` | `PNM_RAWBIN` |
| `pbm`, `pbmtxt`, `pgm`, `pgmtxt` | `PNM_P4`, `PNM_P1`, `PNM_P5`, `PNM_P2` |
| `csv` **and** `tcsv` | `CSV` — the two are indistinguishable |
| `fasta`, `dna`, `rna` | `""` — the empty string |

Two consequences. `-I fastaa` is not an error: it resolves to `AUTO` and the run proceeds with
content sniffing. And `AUTO` in the JSON means "the format was never resolved from the name or the
extension", not "no format was applied" — a file peeked as raw text at read time still records
`AUTO`. The format actually used is not recoverable from the JSON alone; record the `-I` value you
passed.

---

## Dead flags

These are accepted, documented in `--help`, and do nothing. All five were checked by byte-comparing
the output file against a default run on the same input:

| Flag | Status |
|---|---|
| `-d`, `--default` | Never read. Output byte-identical. |
| `-t`, `--text` | Never read. Output byte-identical. |
| `-f`, `--factors` | Parsed, never read. **No file is created.** |
| `-l`, `--log-base` | Reaches only a quantity `lzdistance` does not emit. Output byte-identical. |
| `-p`, `--partitions` | The core never reads it. Output byte-identical. |

`-j` is not dead, but it changes wall-clock time only — results are bit-identical at any thread
count. See [Determinism](../project/determinism.md).

---

## Errors and exit codes

| Situation | Message | Exit |
|---|---|---|
| `-v` | `[ Info ] Version of lzdistance: v1.0.0` on stdout | 0 |
| No arguments at all | `--help` on stdout | 0 |
| Flags but no first source | `[ Error ] Input data source is missing` | 1 |
| First source does not exist | `[ Error ] First data source doesn't exist` | 1 |
| Second source does not exist | `[ Error ] No such file or directory (os error 2)` | 1 |
| Unknown flag, or `-g -1` | clap usage error on stderr | 2 |

The second source is **never validated**, which is why a typo there surfaces as a raw IO error
instead of a named diagnostic. Messages carry hard-coded ANSI colour codes whether or not the stream
is a terminal, so piping to a file keeps the escape sequences.

---

## Worked command lines

**All-pairs genome distance from a directory of FASTA files.** One record per file, sorted by
filename; `-I fasta` is required because directory sources skip extension detection.

```bash
lzdistance -L -I fasta genomes -o genomes.lzdist.json
```

**Cross matrix between two sets, blind to reversal and bit-flips.**

```bash
lzdistance setA.txt setB.txt -b -o ab.lzdist.json
```

**One row against everything.** Row 3 (1-based) of the first source, all columns:

```bash
lzdistance setA.txt setB.txt -i 3:3 -o row3.json
```

**A sub-block of a large matrix**, for splitting one job across machines. Every other cell comes
back as `0.0`, so keep the range in the filename:

```bash
lzdistance corpus -i 1:500   -o corpus.rows-1-500.json
lzdistance corpus -i 501:1000 -o corpus.rows-501-1000.json
```

**Directed graph with a one-factor threshold.** Use `-g 1`, never bare `-g`:

```bash
lzdistance setA.txt -g 1 -o directed.json
```

**Space-delimited table, column-wise.** `lzdistance` has no auto-rule for `.tsv`, so an unforced
`.tsv` file is read as raw text lines instead; force the format:

```bash
lzdistance table.tsv -I tcsv -o table.lzdist.json
```

**Single-threaded, for a reproducible timing baseline.** Results are identical to the parallel run:

```bash
lzdistance -j 1 corpus -o corpus.lzdist.json
```

---

## See also

- [Information distance (NID)](../concepts/nid.md) — what the numbers mean, and why the triangle
  inequality fails.
- [lzcomplexity](lzcomplexity.md) — the single-sequence tool.
- [Input formats](formats.md) — format names, auto-detection, and the parsing traps.
- [Comparing many sequences](../recipes/batch-distance.md) — building and reading a distance matrix.
- [DNA and FASTA](../recipes/genomics.md) — the genomics workflow end to end.
