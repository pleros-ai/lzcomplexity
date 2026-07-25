# Information distance (NID)

*How `nid` descends from Kolmogorov complexity, how it relates to NCD, and how it actually behaves.*

`lzcomplexity.nid(x, y)` returns a normalized dissimilarity between two symbolic sequences, built
entirely from LZ76 factor counts. It belongs to a well-defined lineage — Kolmogorov complexity →
information distance → normalized information distance → normalized compression distance — and it
inherits that lineage's intuition but not its theorems. This page gives both halves honestly.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">factorizations per pair</span></div>
<div class="lz-stat"><span class="lz-stat__v">[0, 1]</span><span class="lz-stat__k">observed range</span></div>
<div class="lz-stat"><span class="lz-stat__v">1/C(X)</span><span class="lz-stat__k">typical self-distance</span></div>
<div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">tuning parameters</span></div>
</div>

## What `nid` computes

<div class="lz-formula">
  <p class="lz-math"><i>d</i>(<i>X</i>,<i>Y</i>) = max{ <i>C</i>(<i>XY</i>) − <i>C</i>(<i>X</i>), <i>C</i>(<i>YX</i>) − <i>C</i>(<i>Y</i>) } ⁄ max{ <i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>) }</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count — an integer, not a compressed bit length</dd>
    <dt><i>XY</i></dt><dd>plain byte concatenation of <i>X</i> then <i>Y</i>, with no separator symbol</dd>
    <dt><i>YX</i></dt><dd>the other order; LZ76 is order-sensitive, so <i>C</i>(<i>XY</i>) ≠ <i>C</i>(<i>YX</i>) in general</dd>
  </dl>
  <p class="lz-formula__cite">Implemented at <code>crates/lzcomplexity-core/src/metrics.rs:37</code>. Four independent factorizations per pair.</p>
</div>

The numerator terms are conditional-complexity estimates: `C(XY) − C(X)` is the number of extra LZ76
phrases that `Y` costs once the parser has already read `X`. If factor counts are subadditive
(`C(XY) ≤ C(X) + C(Y)`) and monotone (`C(XY) ≥ C(X)`), the numerator cannot exceed the denominator
and `d` lands in `[0, 1]`. Both properties are checked below by exhaustive search rather than proved
for this `C` — see [the axiom audit](#is-it-a-metric-measured-not-assumed).

!!! example "Four parses give a distance of 0.25"

    Every factorization below was produced by `lz.factorization` on the 1.0.0 module.

    ```text
    X  = abcd        factors [0,1,2,3,4]     C = 4    a | b | c | d
    Y  = abce        factors [0,1,2,3,4]     C = 4    a | b | c | e
    XY = abcdabce    factors [0,1,2,3,4,8]   C = 5    a | b | c | d | abce
    YX = abceabcd    factors [0,1,2,3,4,8]   C = 5    a | b | c | e | abcd

    d = max(5 − 4, 5 − 4) / max(4, 4) = 1/4 = 0.25
    ```

    ```pycon
    >>> import lzcomplexity as lz
    >>> lz.nid("abcd", "abce")
    0.25
    ```

    The whole of `X` is available as a dictionary while `XY` is parsed, so `Y` costs exactly one
    extra phrase: the single novel factor `abce`.

!!! note "nid has exactly two meaningful arguments."
    `partitions`, `alphabet`, `log_base` and `jobs` do not change the returned float. Measured on two
    independent 140-symbol DNA strings drawn after `random.seed(7)`, all **64** combinations of
    `partitions ∈ {0,1,4,16}`, `alphabet ∈ {2,4,64,None}` and `log_base ∈ {2,4,256,None}` returned the
    identical `f64` value `0.725`. The factor count is alphabet-agnostic; `alphabet` and `log_base`
    are read only to compute `epsilon`, which `nid` never uses. `jobs` is discarded outright
    (`let _ = jobs;`). See [the Python reference](../api/python.md) for the full signature.

<div class="lz-tickrule"></div>

## Where the formula comes from

### Kolmogorov complexity and conditional complexity

The Kolmogorov complexity `K(x)` of a string is the length of a shortest binary program that computes
`x` on a universal machine; `x*` denotes that shortest program, so `K(x) = |x*|`. The conditional
complexity `K(x | y)` is the length of a shortest program that computes `x` when `y` is supplied as
auxiliary input. Li, Chen, Li, Ma & Vitányi (2004) use prefix complexity throughout, and condition on
`y*` rather than `y`; the two differ by at most a logarithmic term.[^star] The chain rule (their
eq. II.1) is:

<div class="lz-formula">
  <p class="lz-math"><i>K</i>(<i>x</i>, <i>y</i>) = <i>K</i>(<i>x</i>) + <i>K</i>(<i>y</i> | <i>x</i>*) = <i>K</i>(<i>y</i>) + <i>K</i>(<i>x</i> | <i>y</i>*)</p>
  <p class="lz-formula__cite">Li, Chen, Li, Ma &amp; Vitányi, "The Similarity Metric", IEEE T-IT 50(12):3250–3264, 2004, eq. (II.1).</p>
</div>

`K(x)` and `K(x | y*)` are upper semi-computable but not computable.

[^star]: For readability this page writes `K(x|y)`. The primary sources write `K(x|y*)`, and
    `K(x | y*) ≤ K(x | y) + O(1)`.

### Information distance E(x, y)

Bennett, Gács, Li, Vitányi & Zurek (1998) define the algorithmic information distance as the length
of a shortest program that computes `x` from `y` and `y` from `x`, and show (their Section III) that
this equals, up to a logarithmic additive term, the *max distance*:

<div class="lz-formula">
  <p class="lz-math"><i>E</i>(<i>x</i>, <i>y</i>) = max{ <i>K</i>(<i>x</i> | <i>y</i>), <i>K</i>(<i>y</i> | <i>x</i>) }</p>
  <p class="lz-formula__cite">Bennett, Gács, Li, Vitányi &amp; Zurek, "Information Distance", IEEE T-IT 44(4):1407–1423, 1998, §III.</p>
</div>

Their Theorem 4.2 is the reason anyone cares: with an appropriate additive constant, `E` is a
*universal admissible metric*. An admissible distance (their Definition 4.1) is a nonnegative,
symmetric, upper-semicomputable function that is 0 exactly when `x = y`, satisfies the triangle
inequality, and is normalized in the Kraft sense `Σ_{y ≠ x} 2^{−D(x,y)} ≤ 1`. Universality means
`E(x, y) ≤⁺ D(x, y)` for *every* admissible distance `D` — if two objects are close under some
admissible distance, they are at least that close under `E`. Note how much the word *admissible*
is carrying: the Kraft normalization is what rules out degenerate distances, and it is exactly the
condition no practical compression distance can be shown to meet.

`E` is an absolute quantity, so it is dominated by length. The canonical example (Li et al. 2004,
§V) is that an unrelated bacterium of the same size as *H. influenza* would sit closer to it than
*E. coli* does, purely because *E. coli* is longer. Hence normalization.

### Normalized information distance

<div class="lz-formula">
  <p class="lz-math">NID(<i>x</i>, <i>y</i>) = max{ <i>K</i>(<i>x</i> | <i>y</i>*), <i>K</i>(<i>y</i> | <i>x</i>*) } ⁄ max{ <i>K</i>(<i>x</i>), <i>K</i>(<i>y</i>) }</p>
  <p class="lz-formula__cite">Li, Chen, Li, Ma &amp; Vitányi 2004, Definition V.2, eq. (V.3).</p>
</div>

Reading (their p. 3254): if `K(y) ≥ K(x)`, then `NID(x,y) = 1 − I(x : y)/K(y)`, so `1 − NID` is the
number of bits shared between the two strings *per bit of information in the more complex string*.
Remark V.3 explains why the denominator is `max{K(x), K(y)}` and not something else: dividing by
length breaks both the triangle inequality and universality, and dividing by `K(x,y)` gives
`d(x,y) = ½` for every pair of mutually random strings, which is degenerate.

Two results matter for this page.

| Result | Statement |
|---|---|
| Metricity (Lemma V.4, Theorem V.7) | NID satisfies the metric (in)equalities **up to additive precision `O(1/K)`**. Symmetry is exact. Identity is `d(x,x) = O(1/K(x))` — *not* exactly 0. The triangle inequality holds up to `O(1/max{K(x),K(y),K(z)})`. |
| Universality (Theorem VI.2) | NID minorizes every upper semi-computable normalized distance `f`: `NID(x,y) ≤ f(x,y) + O(1/K)`. |

Even universality is narrower than it looks. Cilibrasi & Vitányi (*Clustering by Compression*, §3.2)
note that it "holds only for indefinitely long sequences"; at a definite length `n` it is universal
only with respect to normalized distances computable by programs of length logarithmic in `n`.

!!! danger "NID cannot be computed, and cannot be approximated."
    Terwijn, Torenvliet & Vitányi (*JCSS* 77(4):738–742, 2011) proved that NID is neither upper
    semicomputable nor lower semicomputable "up to any reasonable precision" — strictly worse than
    `K`, which is at least upper semi-computable. Every practical NID-like number, this library's
    included, is a heuristic surrogate with no convergence guarantee and no error bar. Do not report
    `nid` as an estimate *of* NID with a stated accuracy; report it as what it is, a normalized LZ76
    phrase-count ratio.

<div class="lz-tickrule"></div>

## The computable surrogate: NCD

Cilibrasi & Vitányi (2005) replace `K` with a real compressor `Z` and define the **normalized
compression distance**:

<div class="lz-formula">
  <p class="lz-math">NCD<sub><i>Z</i></sub>(<i>x</i>, <i>y</i>) = [ <i>Z</i>(<i>xy</i>) − min{ <i>Z</i>(<i>x</i>), <i>Z</i>(<i>y</i>) } ] ⁄ max{ <i>Z</i>(<i>x</i>), <i>Z</i>(<i>y</i>) }</p>
  <p class="lz-formula__cite">Cilibrasi &amp; Vitányi, "Clustering by Compression", IEEE T-IT 51(4):1523–1545, 2005, eq. (3.1).</p>
</div>

The pivotal move is their Definition 3.3, `C(y | x) := C(xy) − C(x)`, which they call the *amount of
conditional compressed information*. Li et al. (2004, §VII) gloss the same substitution as "the
compression length of `y` based on using the 'dictionary' extracted from `x`". That one replacement
turns the uncomputable NID numerator into something measurable, and it is the justification for this
library's entire numerator.

NCD's guarantees are conditional on the compressor being **normal** (Definition 3.1 in the IEEE
version, Definition 3.2 in arXiv:cs/0312044v2). A compressor is normal when, up to an additive
`O(log n)`:

| # | Axiom | Statement |
|---|---|---|
| 1 | Idempotency | `C(xx) = C(x)` (and, in the IEEE version, `C(λ) = 0`) |
| 2 | Monotonicity | `C(xy) ≥ C(x)` |
| 3 | Symmetry | `C(xy) = C(yx)` |
| 4 | Distributivity | `C(xy) + C(z) ≤ C(xz) + C(yz)` |

If the compressor is normal, NCD is a similarity metric (their Theorem 3.6) and quasi-universal
(Theorem 3.7). If it is not, none of that transfers.

<div class="lz-tickrule"></div>

## This library versus NCD

The two formulas differ by exactly one term. Start from the algebraic identity

```text
C(xy) − min{C(x), C(y)}  ≡  max{ C(xy) − C(x),  C(xy) − C(y) }
```

and line the numerators up:

```text
canonical NCD  :  max{ C(XY) − C(X),   C(XY) − C(Y) }
this library   :  max{ C(XY) − C(X),   C(YX) − C(Y) }
                                        ^^^^
```

CompLearn's NCD reuses `C(XY)` where this library computes `C(YX)`. The two are **identically equal**
whenever the compressor satisfies the symmetry axiom. LZ76 factor counts do not, so the two forms
diverge measurably. Cilibrasi & Vitányi's own Remark 3.1 is explicit that using one concatenation is
an engineering shortcut, "justified by the observation that block-coding based compressors are
symmetric almost by definition".

What the library buys with the two extra factorizations:

<div class="lz-cards" markdown>
<div class="lz-card" markdown>

### Exact symmetry

Swapping `X` and `Y` maps the expression to itself, so `d(X,Y) = d(Y,X)` bit-for-bit. Canonical NCD
is not exactly symmetric when `C(xy) ≠ C(yx)`. Measured: 0 bit-level differences in 2,000 random
pairs, length ≤ 200, alphabets 2–4.
<p class="lz-card__api"><code>lz.nid(x, y) == lz.nid(y, x)</code></p>

</div>
<div class="lz-card" markdown>

### The faithful conditional

`C(YX) − C(Y)` is the cost of `X` *after actually compressing `Y` first*, which is what `K(x|y)`
means. The canonical form substitutes a symmetric proxy for it.
<p class="lz-card__api"><code>metrics.rs:37</code></p>

</div>
<div class="lz-card" markdown>

### The price

Four LZ76 factorizations per pair instead of three, each a suffix-array build over strings up to
`|X| + |Y|`. `lzdistance` does not cache the single-sequence counts, so `C(X)` is recomputed for
every pair of an `m × m` matrix.
<p class="lz-card__api"><code>lzdistance</code> → <code>information_distance</code></p>

</div>
</div>

**Measured divergence.** Independent uniform-DNA pairs, `random.seed(1000·s + n)`, five seeds per
length; the seed-0 row is printed in full.

<div class="lz-scroll" markdown>

| n | C(X) | C(Y) | C(XY) | C(YX) | library `d` | canonical NCD form | \|Δ\| (seed 0) | max \|Δ\|, 5 seeds |
|---|---|---|---|---|---|---|---|---|
| 50 | 17 | 18 | 30 | 32 | 0.7778 | 0.7222 | 0.0556 | 0.0556 |
| 200 | 52 | 52 | 92 | 90 | 0.7692 | 0.7692 | 0.0000 | 0.0370 |
| 1000 | 198 | 196 | 357 | 359 | 0.8232 | 0.8131 | 0.0101 | 0.0195 |
| 5000 | 808 | 805 | 1491 | 1484 | 0.8453 | 0.8490 | 0.0037 | 0.0037 |

</div>

The gap is bounded by roughly `|C(XY) − C(YX)| / max{C(X), C(Y)}` and shrinks with length. Neither
form is Cilibrasi & Vitányi's own preferred numerator, `min{C(xy), C(yx)} − min{C(x), C(y)}`; the
library uses the componentwise form, which is the most literal reading of `max{K(x|y), K(y|x)}`.

### Provenance

The formula is not an idiosyncratic variant. It is the `d*` member of the distance family in Otu &
Sayood, *Bioinformatics* 19(16):2122–2130 (2003), and it is printed character-for-character as
equation (3) of Cebrián, Alfonseca & Ortega, *Comm. Inf. Syst.* 5(4):367–384 (2005), there called
NCD.

!!! warning "The library's C is not the classical LZ76 production count c(S)."
    Any comparison with published `d*` numbers is off by the final-production convention.
    `complexity` counts only **complete** LZ76 components: when the greedy parse overshoots the end
    of the sequence, the trailing incomplete component is dropped, so the library's count is one
    *less* than the textbook exhaustive-history count. The exact conversion, verified over 500
    random strings, is

    ```python
    c_textbook = c + (1 if factors[-1] > len(seq) else 0)
    ```

    Otu & Sayood's own worked example makes it concrete. For `ATGTGAATG` the textbook parse is
    `A | T | G | TGA | ATG`, five productions:

    ```pycon
    >>> lz.factorization("ATGTGAATG")
    (4, [0, 1, 2, 3, 6, 10])
    ```

    The last boundary, 10, overshoots `n = 9`, so the library returns 4. Over uniform-random DNA the
    drop happens in roughly two-thirds to nine-tenths of inputs (measured 65 % at `n = 20`, 88 % at
    `n = 5000`, over 200 draws per length), broadly more often at longer lengths. In `nid` the offset partly cancels
    — it appears in all four terms — but not exactly, because the drop is decided independently for
    `X`, `Y`, `XY` and `YX`. See [LZ76 factorization](lz76.md) for the parse rule itself.

<div class="lz-tickrule"></div>

## Is it a metric? Measured, not assumed

**No.** It is a symmetric dissimilarity. Here is the axiom audit against the normal-compressor
definition, with what the LZ76 factor count actually does.

<div class="lz-scroll" markdown>

| Axiom | Required | LZ76 factor count | Evidence |
|---|---|---|---|
| Idempotency `C(xx) = C(x)` | up to `O(log n)` | **Holds to within 1** at every length | `C(XX) − C(X) ∈ {0, 1}` in 200/200 random DNA strings, `n ∈ [50, 2000]`; 156 gave +1, 44 gave +0 |
| Monotonicity `C(xy) ≥ C(x)` | up to `O(log n)` | **Holds** in every trial | 0 negatives across all 1,044,484 exhaustive ordered binary pairs of length ≤ 9 |
| Symmetry `C(xy) = C(yx)` | up to `O(log n)` | **Violated**; the absolute gap grows, the relative gap shrinks | over 50 DNA pairs per length, \|C(XY) − C(YX)\| reached 4 at n = 50 and 16 at n = 5000 — but as a fraction of `max{C(X), C(Y)}` that falls from 0.21 to 0.02 |
| Distributivity | up to `O(log n)` | not audited; the derived subadditivity `C(xy) ≤ C(x)+C(y)` held | 0 violations in all 1,044,484 exhaustive pairs of length ≤ 9 |
| `C(λ) = 0` | required (IEEE version) | **Violated**: `C("") = 1` | the constant-sequence short-circuit returns 1 for any input with fewer than 2 distinct symbols |

</div>

The deeper problem is not any single axiom. Cilibrasi & Vitányi's Definition 2.3 requires `C` to be a
*bit length* produced by a lossless prefix code. The LZ76 factor count is a **phrase count**.
Converting it would take roughly `c(S)·(log₂ c(S) + log₂ α)` bits for the pointer/length/symbol
triples, and the library never does that. So the metric proofs do not apply verbatim; you inherit
only the axioms that happen to hold.

### What `d` actually does

| Property | Verdict | Evidence |
|---|---|---|
| Range | `[0, 1]`, never exceeded in testing | max exactly 1.0, min 0, over all 260,100 ordered pairs of binary strings of length ≤ 8 (exhaustive) |
| Symmetry | **Exact**, by construction | 2,000/2,000 random pairs bit-identical under swap |
| `d(X, X) = 0` | **Usually not** | `d(X,X) = (C(XX) − C(X))/C(X)`, which is `1/C(X)` in the common case. Over all 8,190 binary strings of length ≤ 12, 6,264 (76.5 %) give a strictly positive self-distance |
| Identity of indiscernibles | **Fails both ways** | 243 of 521,731 distinct unordered binary pairs of length ≤ 9 sit at distance 0 — for instance `lz.nid("a","aa")` and `lz.nid("aba","bab")` both return `0.0` |
| Triangle inequality | **Fails at every length tested** | see below |

!!! danger "The triangle inequality fails, and not only on toy inputs."
    Treat `nid` as a dissimilarity: do not feed it to any algorithm that requires a metric — metric
    trees, metric-space embeddings, or proofs that rely on `d(x,z) ≤ d(x,y) + d(y,z)`.

    | Population | Triples | Violations | Worst gap `d(X,Z) − d(X,Y) − d(Y,Z)` |
    |---|---|---|---|
    | **Exhaustive**: all binary strings of length ≤ 7 | 16,387,064 | 178,792 (1.091 %) | +1.0000 |
    | Random binary, len 50–300 | 5,000 | 0 | 0 |
    | Periodic binary, 18 distinct periods, all len 200 | 5,832 | 46 (0.789 %) | +0.5000 |

    The periodic pool is 18 patterns (`a`, `b`, `ab`, `ba`, `aab`, `abb`, `aabb`, `abab`, …) each
    repeated and truncated to exactly 200 symbols, so length is held constant and only structure
    varies.

    Violations become rare for *incompressible* long inputs, because large `C` gives fine
    quantization. They do **not** stop. As soon as the population contains highly compressible
    sequences — periodic, constant, near-deterministic Markov — they reappear at any length.

    Worst case in the exhaustive sweep, and the smallest counterexample overall:

    ```pycon
    >>> lz.nid("aba", "ababa")
    1.0
    >>> lz.nid("aba", "bab"), lz.nid("bab", "ababa")
    (0.0, 0.0)
    ```

    `1.0 > 0.0 + 0.0` — the maximum possible violation. It is not a short-string artefact; the
    same failure survives at length 200:

    ```pycon
    >>> X, Y, Z = "ab" * 100, "abaab" * 40, "aaaab" * 40
    >>> lz.nid(X, Z), lz.nid(X, Y), lz.nid(Y, Z)
    (1.0, 0.25, 0.25)
    ```

    `1.0 > 0.25 + 0.25` — violated by 0.5.

!!! warning "The DNA, binary and trajectory modes of lzdistance break the triangle inequality by design."
    Each matrix cell is the **minimum** over a set of transformed variants of the second operand —
    4 variants for `--binary` (as-is, reversed, bit-flipped, flipped+reversed), 6 for `--adn`, 16
    for `--trajectory`. A minimum over variants is a dissimilarity, never a metric, independently of
    everything above. See [`lzdistance`](../cli/lzdistance.md).

<div class="lz-tickrule"></div>

## Length: the floor, the ceiling, and the grid

This is the part that decides whether your numbers mean anything.

### The resolution is one over max C

The numerator is an integer, so `d` lives on a grid:

<div class="lz-formula">
  <p class="lz-math"><i>d</i>(<i>X</i>,<i>Y</i>) ∈ { <i>k</i> ⁄ max{<i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>)} : <i>k</i> = 0, 1, 2, … }</p>
  <dl class="lz-formula__key">
    <dt>max <i>C</i> = 2</dt><dd>only {0, 0.5, 1} are attainable</dd>
    <dt>max <i>C</i> = 3</dt><dd>only {0, 0.333, 0.667, 1}</dd>
    <dt>max <i>C</i> ≈ 200</dt><dd>a 1 kbp DNA string — about 200 distinguishable levels</dd>
  </dl>
</div>

Two near-deterministic Markov chains (p_stay = 0.999, n = 2000) can have `C` as low as 1 or 2, so
their pairwise distances are drawn from a handful of coarse values, and which value you get is
draw-dependent noise rather than signal.

### The floor: self-distance is your noise level

<div class="lz-scroll" markdown>

| n | C(X) | C(XX) | `nid(X, X)` | 1/C(X) |
|---|---|---|---|---|
| 10 | 6 | 6 | 0.000000 | 0.166667 |
| 50 | 17 | 17 | 0.000000 | 0.058824 |
| 100 | 30 | 31 | 0.033333 | 0.033333 |
| 500 | 112 | 113 | 0.008929 | 0.008929 |
| 1 000 | 198 | 199 | 0.005051 | 0.005051 |
| 10 000 | 1 489 | 1 490 | 0.000672 | 0.000672 |
| 100 000 | 11 887 | 11 888 | 0.000084 | 0.000084 |

</div>

Uniform DNA, `random.seed(n)`. The two zero rows are **not** a length effect — `C(XX) = C(X)` occurs
at every length and happened here to land on the two shortest draws. Quote `1/C(X)` as the typical
floor and note that it is occasionally 0.

!!! tip "No sliding window means no discontinuity: the self-distance floor keeps falling, to 8×10⁻⁵ at 100 kbp."
    The longest-previous-factor search runs over the entire prefix from a global suffix array. There
    is no window, no block size, no dictionary reset. The floor tracks `1/C(X)` and so falls with
    length — with draw-to-draw jitter, and the occasional exact 0, but no cliff anywhere. Compare the
    same self-distance experiment on real compressors (Cebrián et al. 2005, Calgary Corpus): gzip
    `--best` sits at `NCD(x,x) ≈ 0.0–0.1` until the file exceeds its 32 KiB window, then **jumps to
    0.9 and saturates at 1**; bzip2 `--best` runs 0.2–0.3 in its usable region and 0.25–0.9 outside
    it. Borbely (2016) reports the same class of failure on multi-megabyte files: bz2-based 1-NN
    Android-malware family identification scores 29.8 % over the full corpus (APKs up to 15.4 MB) and
    89.7 % when restricted to files under 200 KB. This library's practical limit is CPU and RAM for
    suffix-array construction, not a fixed window.

### The ceiling: `d` never reaches 1 for long sequences

For two independent random sequences of equal length, using `C(x) ≈ n / log_α n`:

<div class="lz-formula">
  <p class="lz-math"><i>d</i><sub>max</sub>(<i>n</i>) ≈ ( log<sub>2</sub> <i>n</i> − 1 ) ⁄ ( log<sub>2</sub> <i>n</i> + 1 )</p>
  <p class="lz-formula__cite">Independent of alphabet size — the α-dependence cancels between numerator and denominator.</p>
</div>

<div class="lz-scroll" markdown>

| n | mean `nid`, 10 independent DNA pairs | (log₂ n − 1)/(log₂ n + 1) |
|---|---|---|
| 100 | 0.7376 | 0.7384 |
| 500 | 0.7928 | 0.7993 |
| 1 000 | 0.8107 | 0.8176 |
| 5 000 | 0.8484 | 0.8495 |
| 10 000 | 0.8583 | 0.8600 |
| 50 000 | 0.8810 | 0.8796 |

</div>

Pairs `i = 0…9` at each length are drawn after `random.seed(1000·i + n)`.

So the usable range at length `n` is approximately `[log_α(n)/n, (log₂n − 1)/(log₂n + 1)]` — about
`[0.005, 0.82]` at 1 kbp and `[0.0007, 0.86]` at 10 kbp. **A value of 0.82 means "as different as two
independent random strings of this length", not "82 % of the way to maximally different".**

!!! danger "Never compare nid values computed on sequences of very different length."
    Both the floor and the ceiling move with `n`. At 500 bp the floor is 0.0089 and the observed
    ceiling about 0.80; at 50 kbp the floor is 0.00016 and the ceiling about 0.88. The short pair's
    noise floor is roughly **56× higher**. `lzdistance` applies no length normalization whatsoever.
    Keep every sequence in a distance matrix inside a narrow length band, or report the length
    distribution alongside the matrix.

With **unequal** lengths the denominator is set by the longer sequence, and the measure reads as
"fraction of the longer sequence not explained by the shorter". Here `X` is 1000 bp of uniform DNA
(`random.seed(1000)`) with `C(X) = 198`:

| Y | C(Y) | `nid(X, Y)` |
|---|---|---|
| first 10 % of X | 30 | 0.8535 |
| first 25 % | 64 | 0.6818 |
| first 50 % | 110 | 0.4495 |
| first 90 % | 181 | 0.0909 |
| X itself | 198 | 0.0051 |
| X repeated 10× | 199 | 0.0050 |

A perfect prefix half as long already sits at `d ≈ 0.45`, while ten copies of `X` cost nothing.
`nid` measures shared **vocabulary**, not shared length.

### Short strings are degenerate

| X | Y | C(X) | C(Y) | C(XY) | C(YX) | `nid` |
|---|---|---|---|---|---|---|
| `"a"` | `"b"` | 1 | 1 | 2 | 2 | 1.000 |
| `"aaaa"` | `"bbbb"` | 1 | 1 | 2 | 2 | 1.000 |
| `"aaaa"` | `"aaaa"` | 1 | 1 | 1 | 1 | 0.000 |
| `"ab"` | `"ba"` | 2 | 2 | 3 | 3 | 0.500 |
| `"abcd"` | `"abce"` | 4 | 4 | 5 | 5 | 0.250 |
| `"abcd"` | `"dcba"` | 4 | 4 | 6 | 6 | 0.500 |
| `"abcd"` | `"wxyz"` | 4 | 4 | 8 | 8 | 1.000 |
| `"banana"` | `"banana"` | 3 | 3 | 4 | 4 | 0.333 |
| `""` | `""` | 1 | 1 | 1 | 1 | 0.000 |
| `""` | `"abc"` | 1 | 3 | 3 | 3 | 0.667 |

!!! warning "The empty string comes back 0.667 away from abc, neither 1 nor 0."
    `C("") = 1`, because the constant-sequence short-circuit fires for any input with fewer than two
    distinct symbols. The empty sequence therefore behaves like a one-phrase sequence — and so does a
    constant string of *any* length, since `C("a" × 10⁶) = 1`. Consequently `nid` between two
    different constant sequences is always exactly 1, and `nid` between a constant and anything else
    is close to 1, with no length information in the result at all.

**A hard floor to work with.** Below `C(X) ≈ 30` — roughly `n < 100` symbols over a four-letter
alphabet — the quantization of `d` is coarser than 0.03 and the self-distance exceeds 0.03; treat
such values as qualitative only. Below `C ≈ 10`, `d` takes about ten distinct values and is
essentially meaningless. If you need to resolve differences of size `δ`, you need `C(X) ≳ 1/δ`: for
DNA and `δ = 0.01` that means `n ≳ 500 bp`; for `δ = 0.001`, `n ≳ 6 kbp`.

<div class="lz-tickrule"></div>

## What `nid` does and does not detect

!!! danger "nid measures shared literal substrings, not a shared generating process."
    Two independent realizations of the *same* Markov chain are statistically indistinguishable from
    two independent iid strings. Over 100 independent pairs at n = 1000 (`random.Random(7000 + i)`),
    with `p` the stay probability:

    | Population | two Markov(p = .9) | two Markov(p = .7) | two independent iid |
    |---|---|---|---|
    | mean `nid` | 0.8037 | 0.7950 | 0.7995 |
    | s.d. across pairs | 0.0485 | 0.0275 | 0.0161 |

    All three means sit inside one standard deviation of each other, even though the underlying
    complexities are wildly different (`C ≈ 25` for `p = .9`, `C ≈ 106` for iid). (`p = .5` is
    omitted: a stay probability of one half *is* the iid process.)

    If your scientific question is "did these two recordings come from the same process?", `nid`
    does not answer it. If your question is "do these two sequences share literal material?", it
    does.

What it tracks well is edit distance and shared literal content. Mutating a random subset of an iid
binary string of length 2000:

<div class="lz-scroll" markdown>

| Mutated symbols | 0 | 2 (0.1 %) | 10 (0.5 %) | 20 (1 %) | 40 (2 %) | 100 (5 %) | 200 (10 %) | 500 (25 %) | 1000 (50 %) |
|---|---|---|---|---|---|---|---|---|---|
| `nid` | 0.0052 | 0.0157 | 0.0576 | 0.1099 | 0.2042 | 0.4660 | 0.7016 | 0.7958 | 0.8083 |

</div>

Monotone and informative up to about 25 % divergence, then flat at the unrelated plateau.

### The join

Concatenation is raw byte concatenation with no separator symbol. A single LZ76 factor straddles the
join in 306 of 400 concatenations of random DNA (lengths 20–400), so the parse genuinely crosses the
boundary — although the parse of the `X` half is unaffected: in all 400 trials the number of factor
boundaries at or before position `|X|` was exactly `C(X)`.

Inserting a fresh separator byte changes `d` a large fraction of the time at *any* length. What
shrinks with `n` is the **magnitude**, of order one factor quantum, `1/max{C(X), C(Y)}`.

| n | draws where a separator changed `d` | max \|Δd\| |
|---|---|---|
| 200 | 10 / 20 | 0.0192 |
| 1 000 | 14 / 20 | 0.0101 |
| 5 000 | 14 / 20 | 0.0025 |

For random binary strings of length 2–12 a separator changed `d` in 1,739 of 2,000 pairs. Do not
insert separators for long sequences — it wastes a factor and biases `d` upward. Do be aware that if
`X` ends with and `Y` begins with the same motif, the join creates a spurious long factor present in
neither sequence alone. With biological data, reading-frame alignment at the join can matter.

### Parameter-light, not parameter-free

Sculley & Brodley (*DCC 2006*) argued that compression-based similarity measures are concrete
similarity measures over an implicit feature space, not a parameter-free method: the parameters have
moved inside the compressor. The critique lands here. `nid` is a deterministic similarity over the
LZ76 phrase feature space, and its dominant hidden parameter is the one you control — **how you
symbolized your data**. See [Alphabets and symbolization](alphabets.md).

<div class="lz-tickrule"></div>

## The shuffle-based distance

`lzdistance` writes a second matrix under the JSON key `shuffle_information_distance`. It is a
different animal, and its name is misleading.

<div class="lz-formula">
  <p class="lz-math"><i>d</i><sub>shuffle</sub>(<i>X</i>,<i>Y</i>) = 1 − MI(<i>X</i>,<i>Y</i>) = <i>C</i>(<i>XY</i>) ⁄ mean<sub><i>l</i> = 1…<i>m</i></sub> <i>C</i>( shuffle<sub><i>l</i></sub>(<i>XY</i>) )</p>
  <dl class="lz-formula__key">
    <dt><i>XY</i></dt><dd>the concatenation — <b>the only thing this measure looks at</b></dd>
    <dt><i>m</i></dt><dd>number of block sizes; <code>lzdistance</code> always derives it from the length (18 at <i>n</i> = 2000)</dd>
    <dt>shuffle<sub><i>l</i></sub></dt><dd>block-swap surrogate of <i>XY</i> at block size <i>l</i>, deterministically seeded from the sequence content</dd>
  </dl>
  <p class="lz-formula__cite"><code>metrics.rs:40-54</code>. Surfaced only by <code>lzdistance</code>; there is no Python binding.</p>
</div>

!!! danger "It is not a mutual information between X and Y, and it is not bounded by 1."
    `X` and `Y` are never factorized apart. The measure is a property of the single string `XY`:
    how much less complex `XY` is than its own block-shuffled surrogates. Consequences:

    - **Asymmetric.** It depends on `XY` and never on `YX`.
    - **Not bounded by 1.** It exceeded 1 for 30.7 % of all ordered binary pairs with
      `|X|, |Y| ≤ 7` (19,800 of 64,516, exhaustive) and for about half of random pairs with
      `|seq| ∈ [500, 3000]` (1,820 of 3,540 off-diagonal cells). The largest value seen anywhere in
      this testing was **2.0**; no upper bound is proved.
    - **Not zero for identical inputs.** For two identical constant sequences it returns exactly
      1.0 — a middling value, not the maximum.
    - **Sensitive to `block_size`,** unlike `nid`, which ignores every optional argument. Neither
      `lzdistance` nor the Python API exposes that knob, so in practice you are stuck with the
      length-derived default.

A run on the worst-case witness, `X = "aaba"` and `Y = "aaaaaab"`, shows both matrices at once:

<div class="lz-run" markdown>

```console
$ printf 'aaba\naaaaaab\n' > pair.txt
$ lzdistance pair.txt -o pair.json
$ cat pair.json   # one physical line; wrapped here for width
{"directed_matrix":[],"first_data_source":"pair.txt","first_data_source_format":"AUTO",
 "first_dim":2,
 "information_distance":[[0.5,1.0],[1.0,0.0]],
 "shuffle_information_distance":[[1.0,2.0],[0.5714285714285714,0.5]]}
```

</div>

Read those two matrices side by side — they are the whole story of this page:

| Cell | `information_distance` | `shuffle_information_distance` |
|---|---|---|
| `("aaba", "aaba")` — identical | 0.5, not 0 | 1.0 |
| `("aaaaaab", "aaaaaab")` — identical | 0.0 | 0.5 |
| `("aaba", "aaaaaab")` | 1.0 | 2.0 |
| `("aaaaaab", "aaba")` | 1.0 — symmetric | 0.5714 — **not** symmetric |

<div class="lz-scroll" markdown>

| | `information_distance` | `shuffle_information_distance` |
|---|---|---|
| Inputs used | `C(X)`, `C(Y)`, `C(XY)`, `C(YX)` | `C(XY)` and `m` shuffles of `XY` only |
| Symmetric | Yes, exactly | No |
| Uses X and Y separately | Yes | No |
| Range | `[0, 1]`, never violated in testing | no proved bound; routinely above 1, max seen 2.0 |
| Identical inputs | `(C(XX) − C(X))/C(X)` → 0 as n grows | ≈ 0.56–0.59 for random, 1.0 for constant |
| Randomized | No | Yes — deterministically seeded shuffles |
| Cost | 4 factorizations | `1 + m` factorizations; `m = 18` at n = 2000 |
| Depends on `block_size` | No | Yes (not settable from `lzdistance`) |
| Python binding | `lz.nid` | none |

</div>

Use it as a structure statistic on the concatenation, not as a distance. If you want a distance, read
`information_distance`.

<div class="lz-tickrule"></div>

## The five extras from `lz76()`

`lz.lz76(seq)["extras"]` returns five information-theoretic-looking numbers. They are computed by
splitting the **single** input sequence in half.

!!! danger "The extras compare the two halves of one sequence. They are not a comparison of two datasets."
    `lz76_extras` takes one sequence, splits it at `len // 2`, and forms the two-variable information
    diagram with `H(A) → C(first half)`, `H(B) → C(second half)`, `H(A,B) → C(whole)`. Every one of
    the five numbers answers *"how much does the second half of this sequence let me compress the
    first half, and vice versa?"* — a self-similarity and long-memory diagnostic.

    Users read `rajski_distance` in `lz76(seq)["extras"]` as "distance between `seq` and something
    else". It is not.

    Passing `X + Y` in order to get a pairwise comparison is also wrong unless `|X| == |Y|`. For
    `|X| = 300` and `|Y| = 700` the split lands at 500, so the "first half" is all of `X` plus the
    first 200 symbols of `Y`. Nothing in the API reports where the split fell. For odd `n` the second
    half is one symbol longer: `"abcde"` splits into `"ab"` and `"cde"`.

Writing `fh = C(first half)`, `lh = C(second half)`, `c = C(whole)` and `mi = fh + lh − c`:

<div class="lz-scroll" markdown>

| Python key | Formula | Classical analogue | Theoretical range |
|---|---|---|---|
| `rajski_distance` | `2 − (fh+lh)/c` ≡ `1 − mi/c` | Rajski distance `1 − I(A;B)/H(A,B)` (Rajski 1961) | `[0, 1]`; 0 iff bijectively related, 1 iff independent |
| `redundancy` | `mi/(fh+lh)` | relative redundancy `I/(H(A)+H(B))` | **`[0, 0.5]`** — half the sum-normalized NMI; there is no factor of 2 |
| `fh_uncertainty` | `mi/fh` | uncertainty coefficient / Theil's U, `I(A;B)/H(A)` | `[0, 1]` |
| `lh_uncertainty` | `mi/lh` | the same, conditioned the other way | `[0, 1]` |
| `pearson_coefficient` | `mi/√(fh·lh)` | geometric-mean NMI, `I/√(H(A)H(B))` (Strehl & Ghosh 2002) | `[0, 1]` |

</div>

Two identities are worth knowing. `rajski_distance = 1 − mi/c` exactly in ℝ, but not bit-exactly in
`f64` — the code evaluates `2 − (fh+lh)/c`, so `lz76("banana")["extras"]["rajski_distance"]` is
`0.33333333333333326` where `1 − 2/3` is `0.33333333333333337`; the maximum deviation over 300 random
sequences was 1.1e-16. And `pearson_coefficient = √(fh_uncertainty · lh_uncertainty)` whenever
`mi ≥ 0`, which was never violated in testing.

!!! example "The banana extras, by hand and from the library"

    The halves are `"ban"` and `"ana"`.

    ```text
    fh = C("ban")    = 3
    lh = C("ana")    = 2
    c  = C("banana") = 3
    mi = 3 + 2 − 3   = 2

    rajski      = 2 − 5/3    = 0.33333333333333326
    redundancy  = 2/5        = 0.4
    fh_unc      = 2/3        = 0.6666666666666666
    lh_unc      = 2/2        = 1.0
    pearson     = 2/sqrt(6)  = 0.8164965809277261
    ```

    ```pycon
    >>> lz.lz76("banana")["extras"]
    {'rajski_distance': 0.33333333333333326, 'redundancy': 0.4,
     'fh_uncertainty': 0.6666666666666666, 'lh_uncertainty': 1.0,
     'pearson_coefficient': 0.8164965809277261}
    ```

    All five move together: `rajski_distance` decreases with self-similarity, the other four
    increase.

!!! warning "The extras leave their textbook ranges on short, low-complexity inputs."
    You will see a negative `rajski_distance`, a `redundancy` above 0.5, an uncertainty coefficient
    of 3, or a `pearson_coefficient` of 1.41. Exhaustively over all 131,070 binary strings of length
    1–16:

    | Field | Theoretical range | Observed range | Out of range |
    |---|---|---|---|
    | `rajski_distance` | `[0, 1]` | `[−1, 1]` | 168 strings (0.128 %) |
    | `redundancy` | `[0, 0.5]` | `[0, 0.6667]` | 168 (0.128 %) |
    | `fh_uncertainty`, `lh_uncertainty` | `[0, 1]` | `[0, 3]` | 1,382 (1.054 %) |
    | `pearson_coefficient` | `[0, 1]` | `[0, 1.5]` | 1,178 (0.899 %) |

    Cause: `mi = fh + lh − c` can exceed `c`, or exceed `fh`, because LZ76 factor counts are not
    entropies and obey neither `I ≤ min(H(A), H(B))` nor `I ≤ H(A,B)`. The halves lose the dictionary
    context the whole sequence has. A witness:

    ```pycon
    >>> lz.lz76("aaaabaaaabaaaaba")["extras"]
    {'rajski_distance': -1.0, 'redundancy': 0.6666666666666666,
     'fh_uncertainty': 2.0, 'lh_uncertainty': 1.0,
     'pearson_coefficient': 1.414213562373095}
    ```

    The opposite direction — `mi < 0`, giving `rajski_distance > 1` — was never observed, in 131,070
    exhaustive strings or in 300 mixed random sequences; LZ76 subadditivity `c ≤ fh + lh` prevents
    it. This is a small-`n`, low-complexity effect: it did not occur at all for `n ∈ [20, 3000]`.

Two more behaviours worth knowing.

**Degenerate inputs report perfect self-similarity.** `lz76("")["extras"]` and `lz76("a")["extras"]`
both return `rajski_distance = 0.0`, `redundancy = 0.5`, both uncertainties `1.0`, and
`pearson_coefficient = 1.0`. Both halves hit the `C(·) = 1` short-circuit, so `fh = lh = c = 1` and
`mi = 1`. `lz76("ab")` and `lz76("abc")` return the opposite extreme: `rajski_distance = 1.0` and the
other four exactly 0.

**There is a finite-size floor.** A genuinely iid binary sequence of length 10,000 gives
`rajski_distance ≈ 0.91` and `pearson_coefficient ≈ 0.16` — not 1 and 0. Over five seeds the values
spanned 0.909–0.921 and 0.147–0.168. The residual `mi` is the ordinary subadditivity gain of LZ76,
not real structure. Interpret against that baseline, not against the theoretical `rajski = 1`, and do
not quote the floor to more than two decimals.

<div class="lz-tickrule"></div>

## Practical checklist

1. **Symbolize both sequences with the same rule, over the same epoch length**, before calling
   `nid`. This is the dominant parameter and it is entirely yours.
2. **Keep lengths comparable** — same order of magnitude, ideally the same band.
3. **Check `C(X)` before you trust a difference.** You need `C ≳ 1/δ` to resolve `δ`.
4. **Interpret against the plateau, not against 1.** "Unrelated" is about 0.81 at 1 kbp and 0.86 at
   10 kbp.
5. **Do not assume a metric.** Use clustering methods that tolerate a dissimilarity — neighbour
   joining, Fitch–Margoliash, the quartet method. The NCD literature reports naive minimum spanning
   trees as not sensitive enough, because compression distances concentrate in a narrow band
   (Cilibrasi & Vitányi give a typical range of 0.85–1.2).
6. **Expect hierarchical clustering to degrade past roughly 40 objects.** That is the size up to
   which Cilibrasi & Vitányi report their NCD quartet-tree heuristic still recovering a
   high-fidelity tree; exact quartet methods they cite stop at about 30.

## See also

- [LZ76 factorization](lz76.md) — the parse, and the complete-components convention.
- [Alphabets and symbolization](alphabets.md) — the choice that dominates every result on this page.
- [Reading the numbers](../guide/reading-the-numbers.md) — what each returned field means.
- [Python API reference](../api/python.md) — `lz.nid` signature, input coercion, return type.
- [`lzdistance`](../cli/lzdistance.md) — matrix mode, variant strategies, JSON keys.
- [References](../project/references.md) — full bibliography for every paper cited here.
