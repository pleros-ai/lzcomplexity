# Comparing many sequences

*From a folder of sequences to a distance matrix you can cluster — by CLI or by Python loop.*

There are two routes to a pairwise distance matrix, and they answer different questions.

| | `lzdistance` | Python loop over `lz.nid` |
|---|---|---|
| Input | files and directories on disk | anything already in memory |
| Output | one JSON file, two matrices | whatever you build |
| Matrices | NID **and** the shuffle distance, always both | NID only |
| Shape | full `N × N` (or `first × second`), diagonal included | whatever you loop over |
| Variant matching | `-a` `-b` `-y` `-r` strategies built in | you write it |
| Cost per cell | 4 + 1 + `mm` factorizations | 4 factorizations |

Use the CLI when your data is already one-sequence-per-file and you want the shuffle matrix
too. Use the Python loop when the sequences come out of a preprocessing pipeline, when you
only want NID, or when you need control over which pairs get computed.

<div class="lz-stats">
  <div class="lz-stat"><div class="lz-stat__v">4</div><div class="lz-stat__k">factorizations per NID pair</div></div>
  <div class="lz-stat"><div class="lz-stat__v">2</div><div class="lz-stat__k">matrices always emitted</div></div>
  <div class="lz-stat"><div class="lz-stat__v">N²</div><div class="lz-stat__k">cells lzdistance computes</div></div>
  <div class="lz-stat"><div class="lz-stat__v">1</div><div class="lz-stat__k">sequence read per file</div></div>
</div>

---

## The corpus used on this page

Every matrix below came from this five-file corpus; the timing sections name the larger corpora
they used. The generator is deterministic, so you can reproduce the whole page.

```python title="make_corpus.py"
import pathlib, random

d = pathlib.Path("sequences")
d.mkdir(exist_ok=True)

(d / "s01_periodic.txt").write_text("01" * 60 + "\n")
(d / "s02_periodic.txt").write_text("10" * 60 + "\n")
(d / "s03_period4.txt").write_text("0011" * 30 + "\n")
for name, seed in [("s04_random.txt", 7), ("s05_random.txt", 11)]:
    r = random.Random(seed)
    (d / name).write_text("".join(r.choice("01") for _ in range(120)) + "\n")

(d / "notes.json").write_text("{}")
(d / "run.log").write_text("nothing\n")
```

```text
sequences/
├── notes.json          ← skipped
├── run.log             ← skipped
├── s01_periodic.txt    ← "01" × 60
├── s02_periodic.txt    ← "10" × 60
├── s03_period4.txt     ← "0011" × 30
├── s04_random.txt      ← 120 coin flips, seed 7
└── s05_random.txt      ← 120 coin flips, seed 11
```

<hr class="lz-tickrule">

## Route A — `lzdistance`

### One source: the self-matrix

Point `lzdistance` at the directory. `-L` turns on the progress lines; without it the tool is
silent on success.

<div class="lz-run" markdown>

```console
$ lzdistance -L sequences
 [ Info ] first_dim: 5  second_dim: 5
 [ Info ] Saved results in: sequences.lzdist.json
```

</div>

`first_dim: 5` confirms that `notes.json` and `run.log` were dropped and the five `.txt` files
each contributed one sequence. The output lands **next to the source**, with `.lzdist.json`
**appended** to the source path — not substituted for its extension.

| First source | Output path |
|---|---|
| `sequences` | `sequences.lzdist.json` |
| `sequences/` (trailing slash) | `sequences/.lzdist.json` — a hidden file *inside* the directory |
| `setA.txt` | `setA.txt.lzdist.json` |

The trailing-slash case is real, and shell tab-completion adds that slash for you. Pass `-o`
if you care where the file goes.

### What comes out

The file is a single line of compact JSON with keys in lexicographic order. This is the whole
file, verbatim:

```json
{"directed_matrix":[],"first_data_source":"sequences","first_data_source_format":"AUTO","first_dim":5,"information_distance":[[0.0,0.5,0.6666666666666666,0.8947368421052632,0.95],[0.5,0.0,0.6666666666666666,0.8421052631578947,0.95],[0.6666666666666666,0.6666666666666666,0.0,0.8947368421052632,0.85],[0.8947368421052632,0.8421052631578947,0.8947368421052632,0.05263157894736842,0.7],[0.95,0.95,0.85,0.7,0.05]],"shuffle_information_distance":[[0.2542372881355932,0.231958762886598,0.21126760563380276,0.7125,0.7914572864321608],[0.22388059701492535,0.2325581395348837,0.20270270270270274,0.690537084398977,0.7816377171215881],[0.2061855670103092,0.2568493150684932,0.2017937219730942,0.7317073170731707,0.7109004739336493],[0.7352941176470589,0.75,0.7125890736342043,0.6507592190889371,0.9959758551307847],[0.7816377171215881,0.7682926829268293,0.7627118644067796,1.0323886639676114,0.6759656652360515]]}
```

| Key | Type | Meaning |
|---|---|---|
| `first_data_source` | string | the first source path exactly as typed |
| `first_data_source_format` | string | what `-I` named, or the guess from the source's extension |
| `first_dim` | int | number of sequences loaded from the first source |
| `second_data_source`, `second_data_source_format`, `second_dim` | | present **only** when a second source path was given |
| `information_distance` | `float[][]` | the NID matrix |
| `shuffle_information_distance` | `float[][]` | the shuffle matrix |
| `directed_matrix` | `int[][]` | `[]` unless `-g` was passed with a non-zero threshold |

Row order is the load order of the first source; column order is the load order of the second
source, or of the first source again when there is only one.

The format string is not a report of what was actually parsed. It stays `AUTO` when neither
`-I` nor the extension resolved anything — each file is still sniffed by magic bytes as it is
read — and DNA, RNA and FASTA all record as the empty string `""`.
[Input formats](../cli/formats.md) has the full mapping.

### Directory input: one sequence per file, and only the first

!!! warning "CAREFUL"

    Everything after the first line of each file is silently dropped, so a multi-line file
    contributes a matrix row for its **first line only**. Directory sources are read with
    multi-line mode **off**: `lzdistance` takes the first sequence each file yields — the
    first non-blank line of a text file, the first FASTA record, the first CSV column, the
    whole raster of a PBM/PGM image. Appending a second line to `s01_periodic.txt` above
    leaves the matrix byte-identical.

The rest of the directory rules:

- **Not recursive.** Sub-directories are ignored, not descended into.
- **Sorted by full path**, byte order. That is why `s01…` through `s05…` land in rows 0–4.
- **`.json` and `.log` are skipped**, case-sensitively. A file named `notes.JSON` is *not*
  skipped and will be parsed as data.
- A file that yields no sequence contributes an **empty sequence**, not a skip — it still
  occupies a row and a column.
- `-I` / `-S` apply to **every** file in the directory. Extension-based format detection is
  disabled for directory sources, so a folder of `.fasta` files needs an explicit `-I fasta`.
  Without it each file sniffs as raw binary — no error, only a different matrix.
  [Input formats](../cli/formats.md) lists the accepted names, several of which are not what
  the help text claims.

A single **file** source behaves the opposite way: it is read with multi-line mode **on**, so
each line, column, FASTA record or image row becomes its own sequence.

### Two sources: the cross-matrix

Give a second path and you get a `first_dim × second_dim` rectangle instead of a square. Here
`query.txt` holds two lines — the reverse of `s04_random.txt`, and the bit-flip of
`s01_periodic.txt`:

```python title="make_query.py"
import pathlib

d = pathlib.Path("sequences")
s01 = d.joinpath("s01_periodic.txt").read_text().strip()
s04 = d.joinpath("s04_random.txt").read_text().strip()
flip = str.maketrans("01", "10")
pathlib.Path("query.txt").write_text(s04[::-1] + "\n" + s01.translate(flip) + "\n")
```

<div class="lz-run" markdown>

```console
$ lzdistance sequences query.txt -o cross.json
$ python3 -c "import json; [print(r) for r in json.load(open('cross.json'))['information_distance']]"
[0.9473684210526315, 0.5]
[0.9473684210526315, 0.0]
[0.8947368421052632, 0.6666666666666666]
[0.631578947368421, 0.8421052631578947]
[0.65, 0.95]
```

</div>

Five rows (the directory), two columns (the file's two lines). Row 1 column 1 is `0.0` because
the bit-flip of `"01"×60` *is* `s02_periodic.txt` — literally the same bytes. Row 3 column 0 is
`0.6316`: NID reads a reversed copy of a random string as only weakly related, because LZ76
matches literal forward copies only, and a reversal leaves nothing longer than the occasional
short palindrome to match. [Strategies](#strategies) fix that, at a price.

### The two matrices

Both are always computed and always written. They are different measures, not two views of one
measure.

<div class="lz-formula">
  <p class="lz-math"><i>d</i><sub>NID</sub>(<i>X</i>,<i>Y</i>) = max{ <i>C</i>(<i>XY</i>) − <i>C</i>(<i>X</i>), <i>C</i>(<i>YX</i>) − <i>C</i>(<i>Y</i>) } ⁄ max{ <i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>) }</p>
  <p class="lz-math"><i>d</i><sub>shuffle</sub>(<i>X</i>,<i>Y</i>) = <i>C</i>(<i>XY</i>) ⁄ mean<sub><i>l</i></sub> <i>C</i>((<i>XY</i>)<sup>RS(<i>l</i>)</sup>)</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count</dd>
    <dt><i>XY</i></dt><dd>the concatenation of <i>X</i> then <i>Y</i></dd>
    <dt>RS(<i>l</i>)</dt><dd>a deterministic block shuffle at block size <i>l</i>, averaged over <i>l</i> = 1 … <i>mm</i></dd>
  </dl>
  <p class="lz-formula__cite">The NID form is the order-aware generalization of the normalized compression distance — Li, Chen, Li, Ma &amp; Vitányi (2004); Cilibrasi &amp; Vitányi (2005).</p>
</div>

| | `information_distance` | `shuffle_information_distance` |
|---|---|---|
| What it uses | `C(X)`, `C(Y)`, `C(XY)`, `C(YX)` | `C(XY)` and `mm` shuffles of `XY` |
| Uses X and Y apart | yes | **no** — they are never factorized separately |
| Symmetric | **yes**, exactly — under the default strategy | **no** |
| Observed range | `[0, 1]`, never violated in testing | no upper bound by construction; 0.20–1.03 across the runs on this page |
| Identical inputs give | `(C(XX) − C(X)) / C(X)`, → 0 as *n* grows | 0.65 and 0.68 for the two random files here, settling near 0.55 by *n* = 10⁴; exactly **1.0** for a constant string |
| Cost per cell | 4 factorizations | 1 + `mm` factorizations |

!!! warning "CAREFUL"

    A `shuffle_information_distance` cell greater than 1 is not a bug and not an overflow —
    nothing in `C(XY) ⁄ mean C((XY)^RS)` caps the ratio at 1. Cell `[4][3]` in the matrix above
    is `1.0323886639676114`, and on a corpus of 20 i.i.d. binary sequences of length 10⁴,
    226 of the 400 cells exceeded 1 (largest 1.013). It is also asymmetric:
    `[0][1] = 0.2320` while `[1][0] = 0.2239`,
    because only `XY` is ever formed, never `YX`. And its value for identical inputs is
    neither 0 nor extremal. Treat it as a joint-structure score for the concatenation, not as
    a distance. If you want a distance, use `information_distance`.

!!! danger "BREAKS"

    Feeding `information_distance` to an algorithm that assumes a metric will produce
    conclusions the data does not support. The measured facts: the **diagonal is not zero**
    (`0.0526` and `0.05` for the two random files above), the **triangle inequality fails** —
    rarely for long incompressible inputs, but at any length once the corpus contains highly
    compressible sequences — and **distinct sequences can sit at distance 0**. It is a
    symmetric dissimilarity, not a metric. [The NID](../concepts/nid.md) has the
    counterexamples.

!!! note "NOTE"

    Distances are quantized. The numerator is an integer, so the resolution of every NID cell
    is `1 / max(C(X), C(Y))`. The `0.6666666666666666` entries above are exactly `2/3`,
    because `C("0011"×30) = 3`. A corpus of near-deterministic sequences yields a matrix drawn
    from a handful of coarse values, and more data does not change that.

### Loading the matrix

```python title="load.py"
import json
import numpy as np

with open("sequences.lzdist.json") as f:
    out = json.load(f)

D = np.asarray(out["information_distance"])
names = ["s01_periodic", "s02_periodic", "s03_period4", "s04_random", "s05_random"]

print(D.shape)          # (5, 5)
print(D.diagonal())     # [0. 0. 0. 0.05263158 0.05 ]
```

The row labels are not in the JSON. For a directory source they are the sorted file names, so
rebuild them with `sorted(pathlib.Path("sequences").glob("*"))` minus the `.json` and `.log`
entries — or keep a manifest next to the corpus.

!!! example "WORKED EXAMPLE"

    Hierarchical clustering of the five-file corpus separates the three periodic sequences
    from the two random ones. `scipy.spatial.distance.squareform` rejects the raw matrix, so
    the diagonal has to be zeroed first.

    ```python
    import json
    import numpy as np
    from scipy.cluster.hierarchy import linkage, fcluster
    from scipy.spatial.distance import squareform

    D = np.asarray(json.load(open("sequences.lzdist.json"))["information_distance"])

    squareform(D)
    # ValueError: Distance matrix 'X' diagonal must be zero.

    np.fill_diagonal(D, 0.0)      # the diagonal is (C(XX)-C(X))/C(X), not 0
    D = (D + D.T) / 2             # no-op for the default strategy; required for -a/-b/-y/-r

    Z = linkage(squareform(D), method="average")
    print(Z)
    print(fcluster(Z, t=0.8, criterion="distance"))
    ```

    ```text
    [[0.         1.         0.5        2.        ]
     [2.         5.         0.66666667 3.        ]
     [3.         4.         0.7        2.        ]
     [6.         7.         0.89692982 5.        ]]
    [1 1 1 2 2]
    ```

    Read the linkage rows in order: the two period-2 files merge at 0.5, the period-4 file
    joins them at 0.667, the two random files merge at 0.7, and the two groups only join at
    0.897. Cutting at `t=0.8` gives `[1 1 1 2 2]` — periodic versus random.

!!! danger "BREAKS"

    `(D + D.T) / 2` is doing real work whenever a strategy flag is in play, and skipping it
    changes your dendrogram. `information_distance` is symmetric only under the default
    strategy: `-a`, `-b`, `-y` and `-r` transform the **second** operand only, so `[i][j]` and
    `[j][i]` minimise over different candidate sets. Measured on this corpus with `-r`,
    `[3][4] = 0.7` but `[4][3] = 0.65`.

### Line ranges

`-i #:#` gates rows (the first source), `-s #:#` gates columns (the second). Both matrices are
gated identically. Use them to split one large matrix across machines.

<div class="lz-run" markdown>

```console
$ lzdistance sequences -i 2:3 -o i23.json
$ python3 -c "import json; [print(r) for r in json.load(open('i23.json'))['information_distance']]"
[0.0, 0.0, 0.0, 0.0, 0.0]
[0.5, 0.0, 0.6666666666666666, 0.8421052631578947, 0.95]
[0.6666666666666666, 0.6666666666666666, 0.0, 0.8947368421052632, 0.85]
[0.0, 0.0, 0.0, 0.0, 0.0]
[0.0, 0.0, 0.0, 0.0, 0.0]
```

</div>

Ranges are **1-based and inclusive**:

| Flag | Rows computed | Note |
|---|---|---|
| *(none)*, `-i :`, `-i 0` | all | |
| `-i 2` | 2 → end | **not** "line 2 only" |
| `-i 2:` | 2 → end | same as `-i 2` |
| `-i 2:2` | 2 only | |
| `-i 2:3` | 2, 3 | |
| `-i :3` | 1 → 3 | |

!!! warning "CAREFUL"

    A gated-out cell is written as `0.0`, indistinguishable from a genuine zero distance, and
    nothing in the JSON records which range was applied. If you shard a matrix across runs,
    track the ranges yourself and mask before merging — do not test for `== 0.0`.

### Strategies { #strategies }

Each cell is the **minimum** distance over a set of transformed copies of the second operand.
The first operand is never transformed. Both matrices minimise independently, so the NID cell
and the shuffle cell in one position may come from different variants.

<div class="lz-scroll" markdown>

| Flag | Strategy | Variants of the second operand `b` | Count |
|---|---|---|---|
| *(none)* | default | `b` | 1 |
| `-r` | reverse | `reverse(b)` **only** — the identity is not included | 1 |
| `-b` | binary | `b`, `reverse(b)`, `flip(b)`, `reverse(flip(b))` | 4 |
| `-a` | DNA | `b`, `reverse(b)`, `A↔T(b)`, `reverse(A↔T(b))`, `C↔G(b)`, `reverse(C↔G(b))` | 6 |
| `-y` | trajectory | for each rotation `tr` in 0 … 7: `rot(b, tr)` and `reverse(rot(b, tr))` | 16 |

</div>

Precedence when several are passed: **`-b` > `-a` > `-y` > `-r` > default.** First match wins;
the rest are ignored silently.

On the cross-matrix above, `-b` recovers both planted relationships:

<div class="lz-run" markdown>

```console
$ lzdistance sequences query.txt -b -o crossb.json
$ python3 -c "import json; [print(r) for r in json.load(open('crossb.json'))['information_distance']]"
[0.8421052631578947, 0.0]
[0.8421052631578947, 0.0]
[0.8947368421052632, 0.6666666666666666]
[0.05263157894736842, 0.8421052631578947]
[0.65, 0.95]
```

</div>

Row 3 column 0 drops `0.6316 → 0.0526`: the reversal is recognised. Row 0 column 1 drops
`0.5 → 0.0`: the bit-flip is recognised. Unrelated cells drop too — row 0 column 0 goes
`0.9474 → 0.8421` — because a minimum over four candidates is systematically smaller. **A
strategy rescales the whole matrix, not only the cells you hoped it would change.** Never
compare a `-b` matrix against a default matrix.

!!! warning "CAREFUL"

    `-a` and `-y` produce plausible-looking numbers on data they do not fit, and they are
    wrong in different directions. `-a` swaps A↔T and C↔G **and lowercases them**, and the two
    swaps are never composed — there is **no reverse-complement variant**. `-y` maps every
    input byte into `'1'`…`'8'`, so on any other alphabet the identity is not among the 16
    variants and distances can only *rise*: `lzdistance sequences -y` on the corpus above
    raises 19 of the 25 cells and lowers none. `-y` is meaningful only for data drawn from
    the digits 1–8. For DNA see
    [Genomic sequences](genomics.md).

### `-g`: the directed graph

`-g <threshold>` adds an integer matrix recording which concatenation order compresses better —
an asymmetry probe, not a distance.

<div class="lz-run" markdown>

```console
$ lzdistance sequences -g 1 -o g1.json
$ python3 -c "import json; [print(r) for r in json.load(open('g1.json'))['directed_matrix']]"
[0, 1, 1, 1, 1]
[1, 0, 1, -2, 1]
[1, 1, 0, 1, 1]
[1, 2, 1, 0, 1]
[1, 1, 1, 1, 0]
```

</div>

The cell convention is easy to misread:

| Cell value | Meaning |
|---|---|
| `1` | the difference is within the threshold — **no significant direction**. Not a distance of 1. |
| `0` | never touched: the diagonal (self-matrix), or both diagonal blocks (cross-matrix) |
| anything else | `[i][j] = C(Xi ‖ Yj) − C(Yj ‖ Xi)`, and `[j][i]` is its negation |

The one significant pair above is `[1][3] = −2`, `[3][1] = +2`. Verify it directly:
`C("10"×60 ‖ s04_random) = 18` and `C(s04_random ‖ "10"×60) = 20`, so `18 − 20 = −2`. Those
are the library's factor counts, which include only **complete** LZ76 components — the textbook
exhaustive-history count is one higher whenever the greedy parse overshoots the end of the
sequence. See [LZ76 factorization](../concepts/lz76.md).

With two sources the matrix is `(first_dim + second_dim)` square, with both diagonal blocks
left at zero.

!!! warning "CAREFUL"

    Bare `-g` and `-g 0` compute the entire `O(N²)` double-concatenation pass and then discard
    it — the key is omitted from the JSON altogether, so you pay full price for nothing. Pass a
    positive threshold to actually get the matrix. Negative thresholds need the `=` form:
    `-g=-1` works, `-g -1` is a parse error and exits 2.

| Invocation | Computed? | `directed_matrix` in the JSON |
|---|---|---|
| *(no `-g`)* | no | present, `[]` |
| `-g` or `-g 0` | yes | **absent** — computed, then discarded |
| `-g 1` | yes | present, populated |
| `-g=-1` | yes | present, populated (raw differences; `0` where they tie) |

The full flag reference, including the flags that do nothing, is on
[`lzdistance`](../cli/lzdistance.md). Two that bite here: **`-v` prints the version and exits**
(verbose is `-L`), and `-p/--partitions` is a no-op.

<hr class="lz-tickrule">

## Route B — the Python loop

There is no batch entry point in the Python API. Write the `O(N²)` loop; it is four lines.

```python title="matrix.py"
import itertools
import pathlib

import numpy as np
import lzcomplexity as lz

paths = sorted(pathlib.Path("sequences").glob("*.txt"))
names = [p.stem for p in paths]
seqs = [p.read_text().split("\n")[0] for p in paths]

n = len(seqs)
D = np.zeros((n, n))
for i, j in itertools.combinations(range(n), 2):
    D[i, j] = D[j, i] = lz.nid(seqs[i], seqs[j])

print(names)
print(np.round(D, 4))
```

```text
['s01_periodic', 's02_periodic', 's03_period4', 's04_random', 's05_random']
[[0.     0.5    0.6667 0.8947 0.95  ]
 [0.5    0.     0.6667 0.8421 0.95  ]
 [0.6667 0.6667 0.     0.8947 0.85  ]
 [0.8947 0.8421 0.8947 0.     0.7   ]
 [0.95   0.95   0.85   0.7    0.    ]]
```

The off-diagonal entries are bit-identical to `information_distance` from the CLI run above —
same core, same code path. The difference is the diagonal: the loop leaves it at `0.0` by
construction, while the CLI computes `lz.nid(s, s)` and gets `0.05263157894736842` and `0.05`
for the two random files. Computing only half the pairs sidesteps the issue entirely.

### Cost

`lz.nid` runs four factorizations per call — `C(X)`, `C(Y)`, `C(XY)`, `C(YX)` — and rayon
overlaps them with a nested `join`. The measured parallel speed-up is **2.0×–3.1×** against a
ceiling of 4×, which is why a NID call costs only **2.2×–4.2× a single factorization** despite
doing six times the suffix-array work.

Wall clock for one `lz.nid` call, both operands length *n*, i.i.d. binary, on the benchmark
laptop (8 cores / 16 threads) — the [Performance](../project/performance.md) figures:

| n | 1 000 | 10 000 | 100 000 | 1 000 000 |
|---|---|---|---|---|
| `lz.nid` | 0.276 ms | 1.72 ms | 16.3 ms | 278 ms |

`lz.nid` is empirically linear in *n* (α ≈ 1.00 ± 0.05), so multiplying by `N(N−1)/2` sizes a
job honestly:

<div class="lz-scroll" markdown>

| Sequences | Pairs | n = 10³ | n = 10⁴ | n = 10⁵ | n = 10⁶ |
|---|---|---|---|---|---|
| 50 | 1 225 | 0.3 s | 2.1 s | 20 s | 5.7 min |
| 100 | 4 950 | 1.4 s | 8.5 s | 1.3 min | 23 min |
| 500 | 124 750 | 34 s | 3.6 min | 34 min | 9.6 h |
| 1 000 | 499 500 | 2.3 min | 14 min | 2.3 h | 39 h |

</div>

Sanity check against the extrapolation, on a second corpus of 20 i.i.d. binary sequences of
length 10⁴ (the one every timing below also uses): 190 pairs, predicted
`190 × 1.72 ms = 0.33 s`. Measured on the same laptop: **0.35 s**. The same corpus through
`lzdistance` — 400 cells, both matrices — takes **2.2 s**. That 6.4× ratio is the cost of the
shuffle matrix plus the full square, not CLI overhead: 2.1× more cells, and each cell adds
`1 + mm` shuffle factorizations on top of NID's four.

!!! tip "FASTER"

    Halving the loop is the cheapest win available: `itertools.combinations` computes
    `N(N−1)/2` pairs where the CLI computes `N²` — 2.1× fewer at N = 20, 2.0× fewer in the
    limit. NID is symmetric under the default strategy, so nothing is lost.

### Do not use threads

!!! danger "BREAKS"

    A `ThreadPoolExecutor` over your pair list runs no faster than the serial loop and freezes
    the rest of your process while it does. **The extension never releases the GIL** — there is
    no `allow_threads` anywhere in the workspace — so every call blocks all other Python
    threads for its whole duration, including a GUI or asyncio event loop, and `Ctrl-C` is not
    serviced until it returns. Measured on the 190-pair job above, ten runs alternating between
    the two: the thread pool took **1.00×–1.06×** the serial time — 0.35 s either way. The
    [published four-way factorization benchmark](../project/performance.md) reports the same
    thing at 0.98×.

Use processes instead, and set `RAYON_NUM_THREADS=1` in the workers so the two levels of
parallelism do not oversubscribe the machine.

```python title="matrix_mp.py"
import os
os.environ.setdefault("RAYON_NUM_THREADS", "1")   # before the first lz call — see below

import itertools
import pathlib
from multiprocessing import Pool

import numpy as np
import lzcomplexity as lz

PATHS = sorted(pathlib.Path("sequences").glob("*.txt"))
SEQS = [p.read_text().split("\n")[0] for p in PATHS]

def pair_nid(ij):
    i, j = ij
    return i, j, lz.nid(SEQS[i], SEQS[j])

if __name__ == "__main__":
    n = len(SEQS)
    pairs = list(itertools.combinations(range(n), 2))
    D = np.zeros((n, n))
    with Pool() as pool:
        for i, j, d in pool.imap_unordered(pair_nid, pairs, chunksize=16):
            D[i, j] = D[j, i] = d
    print(f"{n} sequences, {len(pairs)} pairs, matrix {D.shape}")
```

```text
5 sequences, 10 pairs, matrix (5, 5)
```

`RAYON_NUM_THREADS` is read when rayon builds its global pool, which happens on the **first
call into the extension**, not at import. Setting it any time before that first call works;
setting it at the top of the file, as above, is the habit that cannot go wrong. Measured on the
190-pair job: set before the first `lz.nid`, 0.73 s; set immediately after it, 0.30 s — the
variable was ignored. Results are bit-identical at every thread count, so this is a pure
performance dial, never a numerical one. See [Determinism](../project/determinism.md).

!!! tip "FASTER"

    Process-level parallelism is worth **≈3.2×** on this workload. Measured on the 190-pair,
    n = 10⁴ job on the same laptop: serial loop with default rayon **0.35 s**, serial loop with
    `RAYON_NUM_THREADS=1` **0.81 s**, `multiprocessing.Pool()` with `RAYON_NUM_THREADS=1`
    **0.11 s**. Read the three together: rayon inside `nid` is already buying 2.3×, and
    processes on top of it buy another 3.2×.

    That 0.11 s is the `imap_unordered` pass alone. Building the pool costs another 0.06 s, so
    `matrix_mp.py` as written finishes this job in **0.17 s** — a 2.1× end-to-end win, not 3.2×.

Two caveats on the pool. Process startup and pickling dominate for small jobs — below roughly a
second of serial work, skip it. And the module-level `SEQS` above relies on `fork` start
semantics; on `spawn` platforms each worker re-imports the module and re-reads the files, which
is correct but slower.

<hr class="lz-tickrule">

## Which route

| You want | Use |
|---|---|
| One-sequence-per-file data already on disk | `lzdistance <dir>` |
| The shuffle matrix as well as NID | `lzdistance` — the only route that emits it |
| Reverse / complement / rotation matching | `lzdistance -r` / `-a` / `-y` / `-b` |
| Sequences produced by a preprocessing pipeline | the Python loop |
| Only the upper triangle | the Python loop with `itertools.combinations` |
| To shard a huge matrix across machines | `lzdistance -i` / `-s`, masking the zeros yourself |

Before interpreting any of these numbers, read
[Reading the numbers](../guide/reading-the-numbers.md) and [The NID](../concepts/nid.md). The
single most important caveat: NID measures **shared literal substrings**, not a shared
generating process. Two independent realizations of the same Markov source score the same as
two independent i.i.d. strings — mean NID **0.80 either way**, over 20 pairs at n = 1000
(two-state chain, stay probability 0.8, against fair coin flips). If your hypothesis is "these
came from the same process", this is the wrong instrument.
