# Reading the numbers

*What `complexity`, `h`, `emc` and `nid` can support as conclusions — and how each is misread.*

Every number this library returns is a property of **one finite string**, not of the process that
produced it. Three of the four are also strong functions of the sequence length. If you take one
thing from this page: compare like with like, and normalise against a shuffled surrogate before you
interpret anything.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">1.012</span><span class="lz-stat__k">h of a fair coin at n = 10⁶</span></div>
<div class="lz-stat"><span class="lz-stat__v">1.001</span><span class="lz-stat__k">normalised LZc of a biased coin</span></div>
<div class="lz-stat"><span class="lz-stat__v">0.17</span><span class="lz-stat__k">mean emc of structureless input at n = 2048</span></div>
<div class="lz-stat"><span class="lz-stat__v">0.860</span><span class="lz-stat__k">nid of two unrelated sequences at n = 20 000</span></div>
</div>

## The four numbers at a glance

| Measure | Call | Attainable range | Low means | High means | Compare across lengths? |
|---|---|---|---|---|---|
| Complexity | `lz.factorization(s)[0]` | integer, `1` … `≈ n/log_k n` | long stretches reused from earlier in the same string | little reuse | **No** |
| Entropy density | `lz.h(s)` | `log_k(n)/n` … `1.82` observed | ordered, compressible | as incompressible as i.i.d. uniform | Cautiously — same alphabet, lengths within an order of magnitude |
| EMC | `lz.emc(s)[0]` | `0` … unbounded, **never negative** | no multi-scale structure the shuffle could destroy — but check against a null, the floor is above zero | correlations destroyed across many block scales | **No** |
| Information distance | `lz.nid(a, b)` | `[0, 1]` in all testing | much shared literal content | little shared content | Only within a length band |

<div class="lz-tickrule"></div>

## `complexity` — the LZ76 factor count

The number of complete LZ76 components. An exact integer computed from a suffix array — no sampling,
no tuning parameter. It is also the primitive every other measure is built from.

### Range

The floor is `1` (a constant string). The ceiling grows like `n / log_k n`. Measured on i.i.d. fair
coin flips (CPython `random.seed(0)`, one draw per row):

| `n` | `c` (fair coin) | `n / log₂ n` | `c` (constant string) |
|---:|---:|---:|---:|
| 100 | 18 | 15 | 1 |
| 1 000 | 107 | 100 | 1 |
| 10 000 | 777 | 753 | 1 |
| 100 000 | 6 138 | 6 021 | 1 |
| 1 000 000 | 50 789 | 50 172 | 1 |

### Misreading 1 — "this is the number in my textbook"

`complexity` counts only **complete** LZ76 components. When the greedy parse extends the final
factor past the end of the sequence, that trailing component is not counted, so the library's count
is **one less** than the textbook exhaustive-history count `|E(u)|` whenever the sequence ends
mid-component. Over 400 random strings (lengths 2–120, alphabets `01` / `012` / `ACGT`) the library
was one lower in 302 cases and equal in 98.

The conversion is mechanical, with one exception noted below:

```python
c, factors = lz.factorization(seq)
c_textbook = c + (1 if factors[-1] > len(seq) else 0)
```

```python
lz.factorization("banana")
# (3, [0, 1, 2, 3, 7])          factors[-1] = 7 > 6, so c_textbook = 4
lz.factorization("ABRACADABRA")
# (5, [0, 1, 2, 3, 5, 7, 12])   factors[-1] = 12 > 11, so c_textbook = 6
```

!!! warning "The conversion has one exception, and it is the constant string"

    A sequence with a single distinct symbol reports `c = 1` where the textbook exhaustive history
    has two components (`0 | 000000000`), and the rule above does not catch it:
    `lz.factorization("0"*10)` returns `(1, [0, 1, 10])`, and `factors[-1] == len(seq)`. The
    factorization short-circuits before the parser runs when fewer than two distinct symbols are
    present. The conversion reproduced the textbook count in 399 of the 400 strings above; the one
    failure was a constant string.

### Misreading 2 — "A has c = 400, B has c = 90, so A is more complex"

Only if `|A| == |B|`. `c` grows roughly like `h · n / log_k n`, so length dominates everything else.
A period-2 sequence of 10 000 symbols has `c = 2`; 100 fair coin flips have `c = 18`. See
[Never compare raw numbers across lengths](#never-compare-raw-numbers-across-lengths).

<div class="lz-tickrule"></div>

## `h` — normalised entropy density

<div class="lz-formula">
  <p class="lz-math"><i>h</i> ≈ <i>c</i>(<i>S</i>) · log<sub><i>k</i></sub>&thinsp;<i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length, in symbols</dd>
    <dt><i>k</i></dt><dd>log base — by default the auto-detected alphabet size, minimum 2</dd>
  </dl>
  <p class="lz-formula__cite">In the default base the target range is [0, 1]; pass <code>log_base=2</code> for bits per symbol.</p>
</div>

`h ≈ 1` means "as incompressible as an i.i.d. uniform draw over this alphabet". `h ≈ 0` means
"highly ordered, dominated by long repeats". Neither endpoint is attainable at finite `n`.

### The floor is not zero

Because `c ≥ 1`, the minimum attainable value is `log_k(n)/n`. At `n = 10 000`:

| Sequence | `c` | `h` |
|---|---:|---:|
| `"0" * 10000` | 1 | 0.001329 |
| `"01" * 5000` | 2 | 0.002658 |
| `"01101001" * 1250` | 5 | 0.006644 |

### `h > 1` is expected at finite `n` and is not a bug

The Lempel–Ziv bound is `c < n / ((1 − ε(n)) · log_k n)`, i.e. `h < 1/(1 − ε(n))` — and `ε(n) → 0`
glacially. For a binary alphabet `ε(n)` does not drop below 1 until **n = 361** (the value the
library reports in `lz.lz76(seq)["epsilon"]`), so below that length the bound is not weak, it is
inapplicable. Over all binary strings of length ≤ 20 the estimator reaches `h = 1.8232` bits, at
`"010010100011110"`.

Measured convergence for a fair coin, whose true entropy rate is exactly 1 bit/symbol
(`log_base=2`; mean of 5 draws, CPython `random.seed(0)` … `random.seed(4)`):

| `n` | `h` (bits) | bias |
|---:|---:|---:|
| 100 | 1.1427 | +0.1427 |
| 300 | 1.0972 | +0.0972 |
| 1 000 | 1.0624 | +0.0624 |
| 3 000 | 1.0419 | +0.0419 |
| 10 000 | 1.0311 | +0.0311 |
| 30 000 | 1.0228 | +0.0228 |
| 100 000 | 1.0178 | +0.0178 |
| 300 000 | 1.0147 | +0.0147 |
| 1 000 000 | 1.0122 | +0.0122 |

The bias falls like `1/log n`. Over the four decades of `n` in the table it shrinks by a factor of
12, and it is still **+1.2 % at a million symbols**. Halving it costs you roughly squaring `n`.

!!! danger "`h` is not bounded by 1 at any length you will handle — do not clip it"

    Clipping or rejecting `h > 1` throws away real signal and biases every downstream comparison. On
    binary data the mean `h` of an i.i.d. source is above 1 at *every* length tested: the mean over
    three draws at `n = 10⁷` is still **1.0095**. If you need a quantity that lives in `[0, 1]`, use
    the surrogate ratio below, not a clamp.

### Calibration against sources with a known entropy rate

i.i.d. Bernoulli(`p`), `n = 100 000`, `log_base=2`, mean of 3 draws (seeds 0–2):

| `p` | true `H(p)` bits | `h` (bits) | ratio |
|---:|---:|---:|---:|
| 0.50 | 1.0000 | 1.0176 | 1.018 |
| 0.40 | 0.9710 | 0.9857 | 1.015 |
| 0.30 | 0.8813 | 0.8887 | 1.008 |
| 0.20 | 0.7219 | 0.7212 | 0.999 |
| 0.10 | 0.4690 | 0.4565 | 0.973 |
| 0.05 | 0.2864 | 0.2728 | 0.952 |
| 0.01 | 0.0808 | 0.0742 | 0.918 |

Two-state Markov chain, `n = 200 000`, `log_base=2`, mean of 3 draws (seeds 0–2):

| `p_flip` | true `h` | `h` (bits) |
|---:|---:|---:|
| 0.50 | 1.0000 | 1.0148 |
| 0.30 | 0.8813 | 0.8951 |
| 0.20 | 0.7219 | 0.7322 |
| 0.10 | 0.4690 | 0.4711 |
| 0.05 | 0.2864 | 0.2838 |
| 0.02 | 0.1414 | 0.1371 |
| 0.01 | 0.0808 | 0.0777 |

**The sign of the bias is source-dependent** — positive for high-entropy sources, negative for
low-entropy ones. Do not tell yourself "LZ overestimates" or "LZ underestimates". It does both, and
which one depends on the very quantity you are trying to measure.

### Misreading — "h = 0.92, so my source carries 8 % redundancy"

Not on a large alphabet. On i.i.d. **uniform** data — the maximum-entropy case, where the true
normalised value is exactly 1 — `h` sits well below 1 and climbs very slowly. Mean of 3 draws (seeds 0–2), default `log_base`:

| `k` \ `n` | 10⁴ | 10⁵ |
|---:|---:|---:|
| 2 | 1.0289 | 1.0182 |
| 4 | 0.9890 | 0.9864 |
| 8 | 0.9492 | 0.9568 |
| 20 | 0.9112 | 0.9242 |
| 64 | 0.8525 | 0.8895 |

A 64-letter alphabet with 100 000 symbols scores 0.89 **when there is nothing to compress at all**.
That 11 % is finite-size bias, not redundancy. Absolute `h` values are worth quoting only for
`k ≤ 4` with `n ≳ 10⁶`, and then only to about 1 %. Everywhere else, compare against a surrogate.

!!! warning "`h` ignores the `alphabet` argument entirely — the number will not move"

    Passing `alphabet=` to `lz.h()` returns the same float you got without it, to the last bit; the
    entropy formula reads `log_base` only. A 58-nt DNA string returns `0.9089970509680715` for
    `alphabet ∈ {None, 2, 4, 8, 64}`, and `1.817994101936143` for `log_base=2`. To change the
    normalisation, pass `log_base=`. The `lzcomplexity` CLI is the exact mirror image: there `-a`
    moves the reported entropy density and `-l` does not. See
    [Alphabets and log bases](../concepts/alphabets.md).

<div class="lz-tickrule"></div>

## `emc` — effective measure complexity

`emc` is a **surrogate-data structure score**, not an estimate of `I(past; future)` in bits. Read it
as: how much more compressible the sequence is than versions of itself with all correlations beyond
each of `mm` block scales destroyed.

### One rung per block size, then a projection

The estimator computes `mm` block-shuffled surrogates. Each gives a **rung** — the excess entropy
accumulated up to that scale:

<div class="lz-formula">
  <p class="lz-math"><i>Ê</i>(<i>l</i>) = <i>l</i> · (log<sub><i>k</i></sub>&thinsp;<i>N</i> ⁄ <i>N</i>) · ( <i>C</i><sub>LZ</sub>(shuffled at <i>l</i>) − <i>C</i><sub>LZ</sub>(original) )</p>
  <dl class="lz-formula__key">
    <dt><i>l</i></dt><dd>block size, 1 … <i>mm</i></dd>
    <dt><i>mm</i></dt><dd>largest block size, derived automatically from <i>N</i></dd>
    <dt><i>N</i></dt><dd>sequence length</dd>
    <dt><i>C</i><sub>LZ</sub></dt><dd>LZ76 factor count</dd>
  </dl>
  <p class="lz-formula__cite">The true ladder is non-negative and non-decreasing in <i>l</i>, because excess entropy is a mutual information between past and future. The raw rungs are neither — each rests on its own surrogate draw — so they are projected onto that shape (isotonic regression) before the value and the per-scale terms are read off.</p>
</div>

**All `mm` block sizes contribute to the total.** `summands[l-1]` is the fitted increment at scale
`l`, every entry is non-negative, and they sum to the value. Their running sum is the excess entropy
captured up to each scale — the curve worth plotting. A `0.0` entry means two neighbouring scales
were pooled by the projection, not that the scale was skipped.

### Range, sign, and the noise floor

`emc` is unbounded above and **cannot be negative**. The catch is at the other end: because the value
cannot go below zero, the estimator's noise on structureless input piles up *above* zero, so the null
distribution has a positive mean. Twenty i.i.d. binary sequences at `n = 2048` (seeds 0–19):

```text
[0.100, 0.000, 0.000, 0.387, 0.416, 0.148, 0.000, 0.000, 0.174, 0.277,
 0.387, 0.000, 0.231, 0.000, 0.000, 0.483, 0.193, 0.378, 0.290, 0.000]

mean = 0.1732   sd = 0.1721   exactly zero in 8 of 20   max = 0.4834
```

The null is **not centred at zero**, so "positive" is not evidence of structure. At `n = 2048`
anything below about `mean + 2 sd = 0.52` is indistinguishable from noise. There is one surrogate per
block size and no ensemble, so no error bar comes with the number. The `normal_error` and
`poison_error` fields of `lz.lz76()` are estimates for `h`, not for `emc`.

!!! note "An exact `0.0` means 'no monotone structure found' — and it is now the only thing it means"

    Up to 1.0.2 the total collapsed to its largest-block-size rung alone, so a source whose period
    divided `mm` was left completely invariant by the aligned block shuffle and scored exactly zero.
    That ambiguity is gone; a periodic sequence now reports what the other scales found:

    ```python
    lz.emc("0011" * 100)[0]   # n = 400   → 2.7471255453127803   (was 0.0)
    lz.emc("0011" * 128)[0]   # n = 512   → 2.70263671875        (was 0.0)
    lz.emc("0011" * 256)[0]   # n = 1024  → 4.150390625          (unchanged)
    lz.emc("01"   * 1024)[0]  # n = 2048  → 1.0196126302083333   (was 0.0)
    lz.emc("0"    * 2048)[0]  # constant  → 0.0                  (unchanged)
    ```

    A `0.0` now means the whole ladder sat at or below zero, which is what a structureless input
    looks like — 8 of the 20 i.i.d. sequences above return it. A constant sequence returns it too,
    and correctly: it has nothing to predict. The remaining trap is the *small positive* value, not
    the zero.

Sequences of ten symbols or fewer are never shuffled at all, so `emc` is identically `0.0` there.

### Misreading — "emc rose between conditions, so the signal got more structured"

Check that both conditions have the **same length** first. `mm` is derived from `n`, so two
recordings of different duration are scored at different block sizes and their values are not on the
same axis. `emc` also scales with `1/log(log_base)`, so it is not comparable across different
`log_base` settings either. Use it to rank sequences of identical length, and nothing more.

<div class="lz-tickrule"></div>

## `nid` — normalized information distance

`nid(X, Y) = max(C(XY) − C(X), C(YX) − C(Y)) / max(C(X), C(Y))`. Symmetric by construction, and
exactly so in floating point: `nid(X, Y) == nid(Y, X)` for all 32 131 unordered pairs of binary
strings of length ≤ 7.

### Range and resolution

It stayed in `[0, 1]` across every test run: exhaustively over all 260 100 ordered binary pairs of
length ≤ 8, and on random pairs out to `n = 20 000`. The numerator is an **integer**, so the
resolution of the distance is `1 / max(C(X), C(Y))`. With `max(C) = 2` the only attainable values
are `{0, 0.5, 1}`. Very compressible inputs give very coarse distances.

```python
lz.nid("abcd", "abce")       # 0.25
lz.nid("abcd", "wxyz")       # 1.0
lz.nid("banana", "banana")   # 0.3333333333333333
```

### `nid(X, X)` is usually not zero

Self-distance is `(C(XX) − C(X)) / C(X)`, and `C(XX) − C(X)` is 0 or 1. Over all 8 190 binary
strings of length ≤ 12, **6 264 (76.5 %) have a non-zero self-distance**. It shrinks with length —
`0.009346` at `n = 1000`, `0.000696` at `n = 20 000` — but it is a floor, not a zero.

### The "unrelated" baseline is not 1, and it moves with `n`

Two independent i.i.d. binary strings, mean over 8 pairs (3 at `n = 20 000`):

| `n` | mean `nid` | min–max |
|---:|---:|---|
| 100 | 0.711 | 0.667–0.765 |
| 1 000 | 0.791 | 0.759–0.813 |
| 20 000 | 0.860 | 0.851–0.867 |

There is no fixed "unrelated" value to threshold against. Compute your own baseline from independent
or shuffled sequences **of the same length**.

### It is not a metric

The triangle inequality fails:

```python
lz.nid("aba", "abababbbb")   # 1.0
lz.nid("aba", "bab")         # 0.0
lz.nid("bab", "abababbbb")   # 0.3333333333333333
# 1.0 > 0.0 + 0.3333  — violated by 0.667
```

| Population | Triangle checks | Violations |
|---|---:|---:|
| all binary strings of length ≤ 6, every triple | 976 500 | 11 074 (1.13 %) |
| random and periodic binaries, `n` = 64 … 4096 | 5 460 | 0 |

Violations are easy to find on short strings and were not observed in the longer populations tested
— but "not observed" is not "cannot happen": nothing in the definition guarantees the triangle
inequality at any length. Treat `nid` as a **symmetric dissimilarity**. Handing it to a clustering or
embedding method that assumes a metric is unsafe; check that method's assumptions.

!!! danger "`nid` measures shared literal substrings, not a shared generating process"

    Two independent realisations of the *same* Markov source score no closer than two *unrelated
    i.i.d.* strings. Mean over 20 pairs at `n = 1000`: two sticky Markov(`p_flip` = 0.05) draws give
    0.813, two Markov(0.30) draws 0.796, two unrelated i.i.d. strings 0.799. The sticky chain is the
    most structured of the three and lands *furthest* from zero. If your hypothesis is "these two
    recordings come from the same regime", `nid` cannot test it.

    What `nid` *does* track is edit-style divergence between two versions of one string. Mutating a
    random subset of an i.i.d. binary string of length 2000 gives `nid` = 0.005 / 0.104 / 0.198 /
    0.464 / 0.693 at 0 %, 1 %, 2 %, 5 % and 10 % of symbols flipped, then saturation near 0.80
    beyond about 25 %.

<div class="lz-tickrule"></div>

## Always normalise against a shuffled surrogate

Raw complexity confounds two things: how often each symbol occurs (the marginal histogram) and how
the symbols are arranged in time (the temporal structure). Almost every scientific question is about
the second. A random permutation of the same string preserves the histogram and the length, destroys
the temporal structure, and — critically — carries **the same finite-size bias**. Divide by it and
most of that bias cancels.

Five signals, each `n = 8000` binary symbols, one draw per row from `random.Random(0)`, 20 shuffles
each:

| Signal | LZc | mean LZc(shuffled) | normalised | `h` |
|---|---:|---:|---:|---:|
| fair coin (i.i.d.) | 640 | 634.1 | 1.009 | 1.037 |
| biased coin, p = 0.1 | 290 | 289.7 | **1.001** | 0.470 |
| sticky Markov, p_flip = 0.05 | 191 | 634.0 | **0.301** | 0.310 |
| period-8 square wave | 3 | 635.6 | 0.005 | 0.005 |
| logistic map, r = 4.0 | 638 | 634.8 | 1.005 | 1.034 |

Read rows 2 and 3 together. **Both have low raw complexity.** The biased coin normalises to 1.001 —
its low LZc comes entirely from having mostly zeros, and once you account for that it is as
temporally structureless as a fair coin. The sticky Markov chain normalises to 0.301: its low LZc
survives the correction, because it really is structured in time. Raw LZc ranks the two as broadly
similar (290 vs 191), and so does raw `h` (0.470 vs 0.310). The normalised score separates them by a
factor of 3.3.

Note the third column. The surrogate mean is **not** a constant: it tracks the symbol histogram, so
the biased coin's surrogate sits at 290, not at 634. That is the whole mechanism — the surrogate
carries the marginal distribution over with it, and dividing removes it.

```python
import random, statistics
import lzcomplexity as lz

def normalised_lzc(seq, reps=20, seed=0):
    """LZc(x) / mean LZc(shuffled x) — the standard surrogate normalisation."""
    c, _ = lz.factorization(seq)
    rng = random.Random(seed)
    symbols = list(seq)
    surrogate = []
    for _ in range(reps):
        rng.shuffle(symbols)
        surrogate.append(lz.factorization("".join(symbols))[0])
    return c / statistics.mean(surrogate)

# `sticky_markov` and `biased_coin` are rows 3 and 2 of the table above.
normalised_lzc(sticky_markov)   # 0.3012855903462418
normalised_lzc(biased_coin)     # 1.0010355540214015
```

!!! tip "Twenty surrogates is enough, and each costs one linear-time factorization"

    Among the four signals above with a balanced histogram the surrogate mean sits between 634.0 and
    635.6 — a spread of 1 part in 400 — so the mean is stable long before 20 draws. Twenty shuffles
    cost about 20× one `factorization` call, which at `n = 8000` is milliseconds.

!!! warning "Do not use `lz.emc` as your surrogate — its shuffle is not a permutation of the symbols"

    `emc` swaps **aligned blocks**, which preserves every within-block correlation by design. That is
    what makes it a multiscale measure, and what makes it unsuitable as the memoryless surrogate this
    recipe needs. Shuffle in Python with `random.shuffle` or `numpy.random.permutation` and call
    `lz.factorization` on the result.

<div class="lz-tickrule"></div>

## Never compare raw numbers across lengths

**Comparing raw `complexity` or raw `emc` between sequences of different lengths is invalid.** Not
noisy, not approximate — invalid. Both quantities are defined in a way that depends explicitly on
`n`:

- `complexity` is a count that grows like `h · n / log_k n`.
- `emc` is evaluated at a block size `mm` derived from `n`, and the value scales with `mm`.

One source, two lengths:

| Sequence | `n` | `c` | `h` (bits) |
|---|---:|---:|---:|
| fair coin | 1 000 | 107 | 1.0663 |
| fair coin | 200 | 30 | 1.1466 |

Same generator, and the raw counts differ by 3.6×. Meanwhile a period-2 sequence of 10 000 symbols
has `c = 2` while 100 fair coin flips have `c = 18` — the raw count ranks the highly ordered long
sequence far below the short random one.

What to do instead:

1. **Truncate or window every sequence to a common length** before comparing. This is the only fully
   safe option for `emc`.
2. Use `h`, which divides out most of the length dependence — but only across sequences whose
   lengths are within the same order of magnitude, and only with `log_base=` set explicitly, so the
   normalisation does not shift when one sequence happens to be missing a symbol.
3. Use the surrogate ratio above. It is dimensionless and cancels the bulk of the length bias.

`nid` is scale-free by construction — the denominator is `max(C(X), C(Y))` — and tolerates a moderate
length mismatch. Its "unrelated" baseline still drifts upward with length, so pairs drawn from very
different length regimes are not on a common axis either.

<div class="lz-tickrule"></div>

## Things this does not tell you

<div class="lz-cards" markdown>

<div class="lz-card" markdown>
### Whether two sequences share a generating process

`nid` scores two independent draws from the same Markov chain no closer than two entirely unrelated
strings — 0.80 either way at `n = 1000`. It compares literal content, not models.
</div>

<div class="lz-card" markdown>
### An entropy rate with a defensible error bar

The bias is `O(1/log n)` and is still about 1 % on i.i.d. binary data at `n = 10⁷`, and there is no
distribution-free sample-size guarantee at all. `normal_error` and `poison_error` are legacy
heuristic dispersion indicators inherited from the C++ implementation — neither is derived from a
published statistical model, and neither is a confidence interval.
</div>

<div class="lz-card" markdown>
### Anything, if the process is non-stationary

If the source is non-stationary or non-ergodic, the entropy rate does not exist and nothing here
estimates it. You can still report LZ complexity as a descriptive statistic of the string — but do
not call it `h`, and do not compare it across epochs whose statistics differ.
</div>

<div class="lz-card" markdown>
### Which scale the structure lives at

Read `summands`, not the value. The scalar aggregates every block size into one number; the vector
is the per-scale profile, and its running sum shows where the accumulated structure flattens off.
The value alone cannot tell an order-1 source from a hierarchical one.
</div>

<div class="lz-card" markdown>
### Whether two conditions differ significantly

Nothing here is a hypothesis test. Build the null distribution yourself — surrogates, permutation
across trials, or a block bootstrap — and test against that, not against a nominal value such as
`h = 1` or `nid = 1`.
</div>

<div class="lz-card" markdown>
### Direction, lag, or causality

Every measure on this page is a property of a single string, or of an unordered pair. None is
directed, none carries a time lag, and `nid` folds its two concatenation orders together with a
`max` rather than keeping them apart.
</div>

</div>

## Where to go next

- [Sequence length and convergence](../concepts/convergence.md) — how long a sequence has to be
  before `h` means anything, per alphabet size.
- [Entropy density](../concepts/entropy-density.md) and [EMC](../concepts/emc.md) — the derivations
  behind the numbers above.
- [Normalized information distance](../concepts/nid.md) — what `nid` approximates, and where the
  approximation breaks down.
- [Alphabets and log bases](../concepts/alphabets.md) — why the same string can get two different
  `h` values.
- [EEG and neural time-series](../recipes/neuro.md) — the surrogate normalisation applied end to end.
