# DNA and FASTA

*Read FASTA, pick the right alphabet, profile complexity, and build an alignment-free distance matrix.*

Nucleotide sequence is the least fussy input this library takes. There is no symbolisation step and
no thresholding decision: DNA is already a string over a four-letter alphabet. Everything that goes
wrong in genomics goes wrong at the edges — letter case, ambiguity codes, format auto-detection, and
sequence length.

Two functions carry this page. `lz.h` turns one sequence into an entropy density. `lz.nid` turns two
sequences into a distance. The `lzdistance` binary computes the whole matrix over a directory.

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">alphabet, auto-detected</span></div>
  <div class="lz-stat"><span class="lz-stat__v">6</span><span class="lz-stat__k">variants -a tries</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0.856</span><span class="lz-stat__k">d, unrelated 8 kbp pair</span></div>
  <div class="lz-stat"><span class="lz-stat__v">500 bp</span><span class="lz-stat__k">floor to resolve 0.01</span></div>
</div>

## Reading FASTA

### From the shell

Both binaries read FASTA, and both gunzip transparently. The reader trims whitespace from each line
and concatenates what is left, so line wrapping is invisible; `>` and `;` lines both start a new
record and their text is discarded.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n -F fasta genomes/strain_a.fasta
$ cat genomes/strain_a.lz76.json
{"filename":"genomes/strain_a.fasta","format":"","sequences":[{"alphabet":[84,71,67,65],
"alphabet_size":4,"lz76Complexity":1211,"lz76EntropyDensity":0.9813477980453618,
"lz76RandomShuffleComplexity":{"max_block_size":-1,"multi_information":0.0,"value":0.0},
"size":8000}],"size":1}
```

</div>

The JSON is emitted on a single line with no trailing newline; it is wrapped here to fit.
`lzcomplexity` **replaces** the input extension, so `strain_a.fasta` becomes `strain_a.lz76.json`
next to the input, not in the working directory. Under `-n` the shuffle stage is skipped and
reported as `{-1, 0.0, 0.0}` rather than omitted.

Three flags matter for FASTA:

| Flag | Effect |
|---|---|
| `-F fasta` | Forces the FASTA reader. `.fasta`, `.fna` and `.gz` already route there by extension, so this only matters for other filenames. `-F dna` and `-F rna` reach the same reader — there is no U↔T handling and no alphabet validation. |
| `-m` | One sequence per record. Without it, only the **first** record is read. |
| `-n` | Entropy density only; skips the block-shuffle stage. |

Gzip needs no extra flag and gives identical numbers:

<div class="lz-run" markdown>

```console
$ lzcomplexity -n gz/strain_a.fna.gz
$ python3 -c "import json;d=json.load(open('gz/strain_a.fna.lz76.json'));print(d['sequences'][0])"
{'alphabet': [84, 71, 67, 65], 'alphabet_size': 4, 'lz76Complexity': 1211,
 'lz76EntropyDensity': 0.9813477980453618,
 'lz76RandomShuffleComplexity': {'max_block_size': -1, 'multi_information': 0.0, 'value': 0.0},
 'size': 8000}
```

</div>

!!! warning "Decompression keys off the filename, never the contents"

    A gzipped FASTA whose name does not end in `.gz` fails with
    `stream did not contain valid UTF-8`, and a plain-text file named `.gz` fails with
    `invalid gzip header`. The reader checks the path extension and nothing else. Rename the file,
    or decompress it first.

!!! tip "`-n` is 14× faster on a whole genome"

    Measured on a 2 Mbp single-record FASTA, 16 cores: **4.8 s** for the default run and
    **0.33 s** with `-n`. The block-shuffle stage that `-n` skips costs one extra factorization per
    block scale and dominates the runtime. Every profiling and screening workflow on this page wants
    `h` and nothing else, so pass `-n`.

### From Python

The Python API has no FASTA reader; it takes strings. Twelve lines cover the format including gzip,
and let you normalise case at read time — which, as the next section shows, is the decision that
matters most.

```python
import gzip
import lzcomplexity as lz

def read_fasta(path):
    """Yield (header, sequence) pairs. Handles .gz. Upper-cases as it goes."""
    op = gzip.open if path.endswith(".gz") else open
    with op(path, "rt") as fh:
        name, chunks = None, []
        for line in fh:
            line = line.strip()
            if line.startswith((">", ";")):
                if name is not None:
                    yield name, "".join(chunks)
                name, chunks = line[1:], []
            elif line:
                chunks.append(line.upper())
        if name is not None:
            yield name, "".join(chunks)

for name, seq in read_fasta("genomes/strain_a.fasta"):
    c, factors = lz.factorization(seq)
    print(name, len(seq), c, round(lz.h(seq), 6), round(lz.h(seq, log_base=2), 6))
```

```text
strain_a 8000 1211 0.981348 1.962696
```

Biopython's `SeqIO.parse` works equally well; pass `str(record.seq).upper()`. See
[Input types](../api/inputs.md) for the full conversion rules — in particular, a `list[int]` is
**not** what you want for DNA.

!!! note "The factor count is one short of the textbook count, deliberately"

    `complexity` counts only **complete** LZ76 components. The trailing component that runs past
    the end of the sequence is not counted, so the library's number is one less than the textbook
    exhaustive-history count whenever a sequence ends mid-component. The conversion is exact:

    ```python
    c, factors = lz.factorization(seq)
    c_textbook = c + (1 if factors[-1] > len(seq) else 0)
    ```

    For `strain_a` above, `c = 1211` and `factors[-1] = 8001 > 8000`, so the textbook count is
    1212. This shifts `h` by `O(1/n)` and `nid` by at most one factor quantum; it changes no
    ordering. [LZ76 factorization](../concepts/lz76.md) has the derivation.

<hr class="lz-tickrule">

## The alphabet question

`h` is a *normalised* quantity, and the normalisation depends on the alphabet size:

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>c</i>(<i>S</i>) · log<sub><i>b</i></sub> <i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length in bytes</dd>
    <dt><i>b</i></dt><dd>logarithm base — the <code>log_base</code> argument, defaulting to the auto-detected alphabet size <i>k</i></dd>
  </dl>
  <p class="lz-formula__cite">For clean DNA <i>k</i> = 4 and the default is what you want. Every letter that is not A, C, G or T raises <i>k</i>.</p>
</div>

The alphabet is detected as the number of **distinct bytes**. `a` and `A` are different bytes. `N`
is a fifth symbol. A soft-masked, ambiguity-coded FASTA straight off a genome browser therefore has
`k = 8` or more, and the default normalisation quietly rescales every number you compute from it.

### What soft-masking actually costs

A 4600 bp construct: random background with a 300 bp element inserted twice, at 500 and at 2500 —
the classic repeat that LZ76 exists to find. The masked version is the same string with the
**second** copy lower-cased, exactly as RepeatMasker would leave it.

<div class="lz-scroll" markdown>

| Input | `k` | `c(S)` | `lz.h(seq)` | error vs truth |
|---|---|---|---|---|
| upper-case (ground truth) | 4 | 704 | **0.931072** | — |
| soft-masked, default | 8 | 777 | 0.685079 | **26% low** |
| soft-masked, `log_base=4` | 8 | 777 | 1.027618 | **10% high** |
| soft-masked, `.upper()` first | 4 | 704 | 0.931072 | none |

</div>

Two independent failures are stacked here, and they are worth separating:

1. **The normalisation breaks.** `k` jumps from 4 to 8, `log₈ n` is two-thirds of `log₄ n`, and `h`
   falls by a third for no biological reason.
2. **The parse breaks.** `c(S)` rises from 704 to 777. The lower-cased copy of the repeat shares no
   symbol with the upper-case copy, so LZ76 cannot match it and re-parses all 300 bp from scratch —
   73 extra factors, which is what 300 bp of unseen DNA costs. **The repeat becomes invisible.**

`log_base=4` fixes the first failure only. That is why the third row is still 10% off.

!!! danger "Upper-case before you measure, or the numbers describe your masker"

    Soft-masking changes both the alphabet size and the factorization, and nothing in the library
    warns you. `log_base=4` corrects the units but not the parse. The only correct fix is to
    normalise case in the reader:

    ```python
    seq = seq.upper()                       # always
    seq = seq.replace("N", "")              # if ambiguity runs are short and scattered
    ```

    Strip `N` rather than keeping it: 40 scattered `N`s in the same 4600 bp construct push `k` to 5
    and `h` from 0.931072 to **0.820209** — a 12% shift caused by 0.9% of the sequence. Deleting
    them gives 0.935601, within finite-size noise of the truth. If your `N` runs are long (assembly
    gaps, telomeres), split the record on them and analyse the segments separately rather than
    splicing across a gap you have no evidence for.

Pass `log_base=4` explicitly when you have already cleaned the alphabet and want the quaternary
normalisation guaranteed regardless of what the sequence happens to contain. `log_base=2` gives
bits per base and is the more portable choice for a paper: `lz.h(seq, log_base=2)` returned
`1.962696` for `strain_a`, exactly twice the default `0.981348`, because `log₂ 4 = 2`.
[Alphabets and log bases](../concepts/alphabets.md) covers the general case.

!!! warning "`h` above 1 is normal, not a bug"

    The bound `c(S) < n / log n` is asymptotic, so at finite `n` the ratio overshoots and `h`
    exceeds 1 on high-entropy input. In the sliding-window profile below, two windows of clean
    random DNA read 1.0016 and 1.0115. Do not clip the value, and do not treat 1 as a ceiling when
    you set an axis or a threshold. [Sequence length and convergence](../concepts/convergence.md)
    quantifies the bias.

<hr class="lz-tickrule">

## Profiling: repeats, coding, non-coding

LZ76 entered genomics because its factor count measures internal repetition directly: a substring
the parser has already seen costs one factor no matter how long it is. A sliding window turns that
into a track you can plot against genomic coordinate; the `Complexity` web server (Orlov and
Potapov, 2004) uses a 1000 bp default window, which is a reasonable starting point.

!!! example "A tandem repeat is a tenfold drop in a 1 kb window"

    6000 bp of random background with a 50 bp unit repeated 30 times over `[2000, 3500)`.
    Windows of 1000 bp, step 500.

    ```python
    import random
    import lzcomplexity as lz

    rng = random.Random(7)
    bg = "".join(rng.choice("ACGT") for _ in range(6000))
    unit = "".join(rng.choice("ACGT") for _ in range(50))
    seq = bg[:2000] + unit * 30 + bg[3500:]      # 6000 bp, repeat over [2000, 3500)

    for start in range(0, len(seq) - 999, 500):
        w = seq[start:start + 1000]
        c, _ = lz.factorization(w)
        print(f"{start:5d}-{start + 1000:5d}  c={c:4d}  h={lz.h(w):.4f}")
    ```

    ```text
        0- 1000  c= 198  h=0.9866
      500- 1500  c= 198  h=0.9866
     1000- 2000  c= 194  h=0.9667
     1500- 2500  c= 119  h=0.5930
     2000- 3000  c=  18  h=0.0897
     2500- 3500  c=  18  h=0.0897
     3000- 4000  c= 120  h=0.5979
     3500- 4500  c= 201  h=1.0016
     4000- 5000  c= 203  h=1.0115
     4500- 5500  c= 196  h=0.9766
     5000- 6000  c= 195  h=0.9717
    ```

    The two windows wholly inside the repeat drop to `c = 18` and `h = 0.0897`, more than a tenfold
    fall from the background. The half-overlapping windows sit near 0.59. Window boundaries smear the
    edges by half a window: with step 500 the repeat's extent is bracketed to ±500 bp and no better.
    The seed is fixed so the run is reproducible; a different seed moves each `c` by a few counts
    and leaves the shape unchanged.

The literature's headline result is that **exons are on average more complex than introns and
regulatory regions**. That is a population tendency, not a classifier — the distributions overlap
heavily, and a single window's `h` will not tell you whether you are inside coding sequence. Treat
it as a feature or a screen, never as an annotation.

What the measure is reliable for is exactly what the profile above shows: locating low-complexity
and repetitive tracts. That is a detection problem with a large effect size, rather than a
classification problem with a small one.

<hr class="lz-tickrule">

## Sequence distance for phylogeny

`lz.nid(x, y)` is the LZ76 information distance:

<div class="lz-formula">
  <p class="lz-math"><i>d</i>*(<i>X</i>,<i>Y</i>) = max{ <i>C</i>(<i>XY</i>) − <i>C</i>(<i>X</i>), <i>C</i>(<i>YX</i>) − <i>C</i>(<i>Y</i>) } ⁄ max{ <i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>) }</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count — a count of phrases, not a compressed bit length</dd>
    <dt><i>XY</i></dt><dd>the two sequences concatenated, with no separator symbol</dd>
  </dl>
  <p class="lz-formula__cite">This is the <i>d</i>* of Otu &amp; Sayood, <i>Bioinformatics</i> <b>19</b>(16), 2122–2130 (2003) — evaluated with this library's complete-factor convention rather than their <i>c</i>(<i>S</i>), so the values shift by <i>O</i>(1/<i>C</i>).</p>
</div>

The reading is Cilibrasi and Vitányi's: `C(XY) − C(X)` is the cost of describing `Y` given the
dictionary already extracted from `X`. Four factorizations per pair, no alignment, no gene models,
no tunable parameters. [Normalized information distance](../concepts/nid.md) derives it from
Kolmogorov complexity and audits how far it falls short of being a metric.

### In Python

Four 8000 bp sequences: `strain_a`, `strain_b` (1% of sites mutated), `strain_c` (10% mutated), and
an independent `outgroup`.

```python
import lzcomplexity as lz

names = ["outgroup", "strain_a", "strain_b", "strain_c"]
seqs = [dict(read_fasta(f"genomes/{n}.fasta"))[n] for n in names]

for i, n in enumerate(names):
    print(f"{n:9s}", [round(lz.nid(seqs[i], seqs[j]), 4) for j in range(4)])
```

```text
outgroup  [0.0008, 0.8557, 0.8492, 0.8567]
strain_a  [0.8557, 0.0008, 0.0379, 0.4758]
strain_b  [0.8492, 0.0379, 0.0, 0.5029]
strain_c  [0.8567, 0.4758, 0.5029, 0.0008]
```

1% divergence reads 0.0379, 10% reads 0.4758, and an unrelated sequence reads 0.855. The measure is
strongly non-linear in sequence divergence and saturates well before the sequences are unrelated, so
use it as a ranking rather than as a rate.

Note the diagonal: `nid(x, x)` is `0.0008`, not `0`, and for `strain_b` it lands on exactly `0.0`.
Neither is a rounding artefact.

!!! danger "Distance 0 does not mean identical, and values do not transfer between lengths"

    `nid` fails the identity axiom at finite `n`: `d(X, X) = (C(XX) − C(X)) / C(X)`, which is
    typically `1/C(X)` and occasionally exactly 0. Both the floor and the ceiling move with
    sequence length:

    | Length | typical self-distance | `d` for an unrelated pair |
    |---|---|---|
    | 500 bp | 0.0089 | ≈ 0.80 |
    | 1 kbp | 0.0051 | ≈ 0.82 |
    | 10 kbp | 0.0007 | ≈ 0.86 |
    | 100 kbp | 0.00008 | ≈ 0.89 |

    A distance computed on 500 bp windows and a distance computed on 50 kbp contigs are different
    quantities. Keep every sequence in one matrix inside a narrow length band, or report the length
    distribution alongside the matrix. The ceiling tracks `(log₂ n − 1) / (log₂ n + 1)` and is
    independent of alphabet size, so 0.86 means "as different as two independent random sequences of
    this length", **not** "86% of the way to maximally different".

**Minimum useful length.** The self-distance is your noise floor. To resolve differences of size `δ`
you need `C(X) ≳ 1/δ` — for DNA, roughly **500 bp for δ = 0.01** and **6 kbp for δ = 0.001**. Below
`C(X) ≈ 30`, about 100 bp of DNA, the measure is quantised more coarsely than the differences you
are trying to see and should be read qualitatively at most.

### From the shell, over a whole directory

`lzdistance` takes a directory and emits the full matrix: one sequence per file, files in sorted
path order, `.json` and `.log` skipped, sub-directories ignored, and **only the first record of each
file used**.

<div class="lz-run" markdown>

```console
$ lzdistance -L -I fasta genomes -o genomes.lzdist.json
 [ Info ] first_dim: 4  second_dim: 4
 [ Info ] Saved results in: genomes.lzdist.json
```

</div>

```text
             outgroup  strain_a  strain_b  strain_c
outgroup       0.0008    0.8557    0.8492    0.8567
strain_a       0.8557    0.0008    0.0379    0.4758
strain_b       0.8492    0.0379    0.0000    0.5029
strain_c       0.8567    0.4758    0.5029    0.0008
```

Identical to the Python matrix, to the last bit. The JSON holds two matrices:
`information_distance` is the one above, and `shuffle_information_distance` is a different statistic
that is asymmetric and can exceed 1 — do not feed it to a tree builder. `directed_matrix` is an
empty list unless you pass `-g` with a non-zero threshold; bare `-g` (threshold 0) computes the
matrix and then drops the key from the JSON entirely. [The `lzdistance`
reference](../cli/lzdistance.md) covers the rest of the surface.

!!! warning "Every distance changes if you omit `-I fasta` on a directory"

    A directory source disables extension-based format detection, so `.fasta` files fall through to
    magic-byte detection — and a file whose first byte is `>` is classified as **raw binary**. The
    same 8000-base record is then read as 65 144 bits over a 2-letter alphabet. The run succeeds,
    prints no warning, and produces a plausible-looking matrix with different numbers throughout:
    the `outgroup`/`strain_a` cell moves from 0.8557 to 0.8372. Always pass `-I fasta` for
    directories. A directory of `.fasta.gz` files read with `-I fasta` gives a matrix identical to
    the uncompressed one.

Output paths follow `lzdistance`'s own rule — the suffix is **appended**, so `genomes` yields
`genomes.lzdist.json`. A trailing slash (`genomes/`) writes the hidden file `genomes/.lzdist.json`
*inside* the directory. Pass `-o` and the question does not arise.

### What `-a` does, exactly

`-a` / `--adn` is the DNA strategy. It builds six variants of the **second** operand, and the matrix
cell is the **minimum** distance over all six. The first operand is never transformed.

| # | Variant | Definition |
|---|---|---|
| 1 | `b` | unchanged |
| 2 | `reverse(b)` | sequence reversed |
| 3 | `at(b)` | A↔T swapped, **and those two bases lower-cased**; C and G untouched |
| 4 | `reverse(at(b))` | variant 3, reversed |
| 5 | `cg(b)` | C↔G swapped, **and those two bases lower-cased**; A and T untouched |
| 6 | `reverse(cg(b))` | variant 5, reversed |

Measured — one upper-case query record against six targets, 3000 bp each:

<div class="lz-run" markdown>

```console
$ lzdistance -L -a -I fasta -S fasta query.fasta targets.fasta -o strand.lzdist.json
 [ Info ] first_dim: 1  second_dim: 6
 [ Info ] Saved results in: strand.lzdist.json
```

</div>

<div class="lz-scroll" markdown>

| Target record | default | `-a` |
|---|---|---|
| identical to the query | 0.0020 | 0.0020 |
| reversed | 0.8379 | **0.0020** |
| A↔T swapped and lower-cased (the form `at` emits) | 0.9882 | 0.9882 |
| C↔G swapped and lower-cased (the form `cg` emits) | 0.9882 | 0.9882 |
| **reverse complement** | 0.8457 | **0.8422** |
| unrelated | 0.8314 | 0.8314 |

</div>

Plain upper-case swaps behave no better: an upper-case A↔T-swapped target reads 0.8501 by default
and 0.8438 under `-a`, an upper-case C↔G-swapped target 0.8442 and 0.8223.

Read that table carefully, because the flag does considerably less than its name suggests.

!!! danger "`-a` does not find reverse complements"

    The reverse complement of the query stays at **0.8422** under `-a` — further away than the
    *unrelated* sequence at 0.8314, and nowhere near the 0.0020 a match scores. The two base swaps
    are never composed, so a full complement is not among the six variants; only `reverse` is, and
    reversal alone does not make a strand match.

    The swaps also lower-case their output, so variants 3–6 can only match a first operand that is
    *already* in that half-swapped lower-case form. Verified: a query record written as
    A↔T-swapped-and-lower-cased sits at 0.9882 by default and collapses to **0.0020** under `-a`.
    With ordinary upper-case FASTA on both sides, `-a` reduces to
    `min(d(a, b), d(a, reverse(b)))`.

    **If you need strand independence, reverse-complement the sequences yourself** before calling
    `lz.nid`, and take the minimum over the two orientations in your own code.

One further consequence of the minimum: taking a min over variants breaks the triangle inequality,
so `-a` output is a dissimilarity, not a metric. Cells that have nothing to do with strand symmetry
can also shrink, because the minimum has six chances to go low — the upper-case C↔G row above falls
from 0.8442 to 0.8223 for no biological reason. The unrelated cell happened not to move here, so
the effect is a bias you should expect rather than one you can predict per cell.

### Getting to a tree

`information_distance` is already a symmetric square matrix with a near-zero diagonal, which is what
every clustering routine wants. Convert to a condensed vector and hand it to a method that is
sensitive to small differences:

```python
import json
import numpy as np
from scipy.cluster.hierarchy import linkage
from scipy.spatial.distance import squareform

with open("genomes.lzdist.json") as fh:
    D = np.array(json.load(fh)["information_distance"])

D = (D + D.T) / 2          # symmetric by construction; this only kills float asymmetry
np.fill_diagonal(D, 0.0)   # the self-distance floor is not biological signal
condensed = squareform(D)  # default checks=True: validates symmetry and zero diagonal
Z = linkage(condensed, method="average")   # UPGMA

print(np.round(condensed, 4))
print(np.round(Z, 4))
```

```text
[0.8557 0.8492 0.8567 0.0379 0.4758 0.5029]
[[1.     2.     0.0379 2.    ]
 [3.     4.     0.4894 3.    ]
 [0.     5.     0.8539 4.    ]]
```

The condensed vector is the upper triangle in row order: `(outgroup, strain_a)`,
`(outgroup, strain_b)`, `(outgroup, strain_c)`, `(strain_a, strain_b)`, `(strain_a, strain_c)`,
`(strain_b, strain_c)`. The linkage joins `strain_a` with `strain_b` first, then `strain_c`, then
the outgroup — the correct topology.

Zeroing the diagonal is the one liberty you have to take, and you should: the diagonal is
`(C(XX) − C(X)) / C(X)`, a finite-size artefact, and `squareform` raises
`Distance matrix 'X' diagonal must be zero` if you leave it in. Passing `checks=False` silences
that error but does not fix the matrix — it only skips the validation.

For real phylogeny prefer neighbour-joining over UPGMA — UPGMA assumes a molecular clock that LZ
distances certainly do not obey. Whatever you use, pick a method that exploits small differences:
compression distances concentrate in a narrow band near their ceiling, and minimum spanning trees or
single-linkage discard most of the signal.

!!! note "What the benchmarks say"

    Otu and Sayood (2003) recovered mammalian phylogenies from complete mitochondrial genomes
    without alignment. The independent benchmark — Höhl, Rigoutsos and Ragan (2007) — is less
    flattering and worth knowing about: on 700 synthetic trees `d_LZ` ranked 15th and 16th of 19
    methods, and on the empirical BAliBASE data 10th and 15th of 22. That paper's own conclusion is
    that most alignment-free methods are statistically indistinguishable from one another, and that
    on ordinary collinear sequences the alignment-based estimates beat every alignment-free method
    that is not pattern-based. Two caveats in the other direction: the benchmark ran on protein
    sequences throughout, and on their shuffled-domain data the ordering reverses — `d_LZ` there
    reconstructed more trees correctly than three of the four alignment-based pipelines.

    Use LZ distance for what it is good at: fast, automatic, coarse grouping over sequences too long
    or too divergent to align. Do not use it to resolve a contested node.

<hr class="lz-tickrule">

## Cost of an all-pairs matrix

Each `information_distance` cell costs four LZ76 factorizations over strings up to `|X| + |Y|` long,
and `C(X)` is recomputed for every pair — the single-sequence counts are not cached. On top of that,
`lzdistance` always fills the `shuffle_information_distance` matrix as well, and there is no flag to
turn it off: the wall times below cover both matrices. An all-pairs `lz.nid` loop in Python over the
same 8 genomes takes 1.3 s against `lzdistance`'s 2.2 s, and that gap is what the second matrix
costs you.

<div class="lz-scroll" markdown>

| Genomes `m` | Length each | Wall time, 16 cores |
|---|---|---|
| 8 | 50 kbp | 2.2 s |
| 16 | 50 kbp | 9.9 s |
| 32 | 50 kbp | 39.0 s |

</div>

Best of three runs on one 16-core box; individual runs on the same input varied by up to 30%, so
read these as a shape, not as a spec. The shape is the `O(m²)` one — 4.5× and 3.9× per doubling of
`m`, with the small-`m` row noisiest because there are fewer matrix rows than cores. Extrapolating,
128 genomes of 50 kbp is roughly 10 minutes and 1000 would be about 11 hours. Sequence length enters
on top of that, roughly linearly per pair.

Three things to do before starting a large run:

- **Trim to a common length band.** It removes the length confound *and* bounds the cost.
- **Size the thread pool with `-j`** if you share the machine. It changes wall time only, never the
  numbers.
- **Split the job.** `lzdistance A_dir B_dir` computes a rectangular cross matrix, so a large
  all-pairs run can be tiled into blocks and reassembled.

[Comparing many sequences](batch-distance.md) covers the batch workflow, the line-range gates and
the reassembly in detail.

<hr class="lz-tickrule">

## Checklist

<div class="lz-cards" markdown>
<div class="lz-card" markdown>

### Before you measure

Upper-case every sequence. Strip or split on `N`. Confirm `alphabet_size` is 4 in the JSON before
trusting a single number.
<p class="lz-card__api"><code>seq.upper().replace("N", "")</code></p>

</div>
<div class="lz-card" markdown>

### Reading FASTA

`-I fasta` is mandatory for `lzdistance` on a directory, where extension detection is switched off.
`lzcomplexity` needs `-F fasta` only for filenames that are not `.fasta`, `.fna` or `.gz`.
<p class="lz-card__api"><code>lzdistance -I fasta genomes</code></p>

</div>
<div class="lz-card" markdown>

### Distances

Keep lengths in one band, zero the diagonal, cluster with a sensitive method. Never compare `d`
across length scales.
<p class="lz-card__api"><code>lz.nid(x, y)</code></p>

</div>
<div class="lz-card" markdown>

### Strand symmetry

`-a` gives you reversal, not reverse complement. Build the complement yourself and take the minimum
over orientations.
<p class="lz-card__api"><code>min(lz.nid(x, y), lz.nid(x, rc(y)))</code></p>

</div>
</div>

Full citations for Otu &amp; Sayood, Orlov &amp; Potapov and Höhl et al. are in
[References](../project/references.md). For the interpretation rules that apply to every measure on
this page, read [Reading the numbers](../guide/reading-the-numbers.md).
