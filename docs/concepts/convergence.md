# Sequence length and convergence

*How much data each measure needs before it means anything, with the bias measured.*

Every convergence result behind this library is asymptotic. `h` converges to the entropy rate of
an ergodic source as *n* → ∞, and the finite-size bias falls off like a power of `1/log n` — so it
is large, it shrinks slowly, and at the lengths real experiments produce it never quite goes away.

This page gives the measured numbers, an honest table of what each measure needs, and one firm
rule about comparing sequences of different lengths.

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">+12.4%</span><span class="lz-stat__k">bias in h at n = 100</span></div>
  <div class="lz-stat"><span class="lz-stat__v">+1.2%</span><span class="lz-stat__k">bias in h at n = 10⁶</span></div>
  <div class="lz-stat"><span class="lz-stat__v">10.4×</span><span class="lz-stat__k">bias shrink over 4 decades</span></div>
  <div class="lz-stat"><span class="lz-stat__v">100×</span><span class="lz-stat__k">what 1/√n would have given</span></div>
</div>

<hr class="lz-tickrule">

## The measured bias of `h`

A fair coin has an entropy rate of exactly 1 bit per symbol. Here is what `lz.h(seq, log_base=2)`
returns on i.i.d. fair-coin sequences, averaged over independent draws at each length
(`random.Random(n)` seeded per row).

| n | draws | `h` (bits) | bias | s.d. over draws |
|---:|---:|---:|---:|---:|
| 100 | 400 | 1.1243 | +0.1243 | 0.0719 |
| 300 | 300 | 1.0818 | +0.0818 | 0.0334 |
| 1 000 | 200 | 1.0522 | +0.0522 | 0.0196 |
| 3 000 | 100 | 1.0360 | +0.0360 | 0.0101 |
| 10 000 | 60 | 1.0269 | +0.0269 | 0.0045 |
| 30 000 | 40 | 1.0209 | +0.0209 | 0.0026 |
| 100 000 | 25 | 1.0171 | +0.0171 | 0.0016 |
| 300 000 | 40 | 1.0142 | +0.0142 | 0.0006 |
| 1 000 000 | 25 | 1.0120 | +0.0120 | 0.0003 |

Four decades of extra data — 100 symbols to a million — shrink the bias by **10.4×**. That is
nowhere near what an ordinary statistical error would do.

| n | bias | observed shrink | if bias ∝ 1/√n | if ∝ 1/log₂n | if ∝ 1/(log₂n)² |
|---:|---:|---:|---:|---:|---:|
| 100 | 0.1243 | 1.00× | 1.00× | 1.00× | 1.00× |
| 1 000 | 0.0522 | 2.38× | 3.16× | 1.50× | 2.25× |
| 10 000 | 0.0269 | 4.63× | 10.00× | 2.00× | 4.00× |
| 100 000 | 0.0171 | 7.27× | 31.62× | 2.50× | 6.25× |
| 1 000 000 | 0.0120 | 10.35× | 100.00× | 3.00× | 9.00× |

The observed column tracks the logarithmic forms and diverges further from `1/√n` at every decade:
by *n* = 10⁶ an ordinary sampling error would have shrunk 100×, and the measured bias has shrunk
10×. Over this window the observed shrink sits a little above the `1/(log n)²` curve. That is a fit
to nine points on one source, not a law — a separate Monte-Carlo run with different seeds and only
20 draws per length lands 4 % away at *n* = 100 and 1 % away at *n* = 10⁶. Treat the columns as
interpolation and do not read a required sample size off them.

!!! danger "Halving the bias costs you squaring n"

    A `1/log n` error law means the sample size you need grows exponentially in the precision you
    want. Fit `1/log₂n` to the fair-coin column and halving the +1.2 % bias at *n* = 10⁶ takes
    *n* ≈ 10¹²; fit the more optimistic `1/(log₂n)²` and it still takes *n* ≈ 3·10⁸. Neither is
    "twice the data". Do not plan an experiment on the assumption that `h` behaves like a sample
    mean.

There is a harder result behind the slow decay: **no distribution-free sample-size answer exists.**
Shields (1993) proved there is no universal redundancy rate over the class of all ergodic sources —
for any *n* and any claimed error bound, some stationary ergodic process violates it. Every number
on this page is a rule of thumb calibrated on particular sources, and is labelled as such.

### The bias is not always upward

`h` overestimates high-entropy sources and underestimates low-entropy ones. Measured on i.i.d.
Bernoulli sources at *n* = 100 000 with `log_base=2`, mean of 10 draws per row:

| p | true H(p) bits | `h` (bits) | ratio |
|---:|---:|---:|---:|
| 0.50 | 1.0000 | 1.0172 | 1.017 |
| 0.40 | 0.9710 | 0.9848 | 1.014 |
| 0.30 | 0.8813 | 0.8897 | 1.009 |
| 0.20 | 0.7219 | 0.7200 | 0.997 |
| 0.10 | 0.4690 | 0.4563 | 0.973 |
| 0.05 | 0.2864 | 0.2726 | 0.952 |
| 0.01 | 0.0808 | 0.0734 | 0.908 |

At `p = 0.01` the estimate is **9 % low** at a hundred thousand samples. Never state "LZ
overestimates entropy" as a blanket rule; the sign of the error depends on the source.

Correlated sources behave the same way. A two-state chain that flips with probability `p`, same
length and same number of draws:

| p(flip) | true h | `h` (bits) | ratio |
|---:|---:|---:|---:|
| 0.50 | 1.0000 | 1.0174 | 1.017 |
| 0.30 | 0.8813 | 0.8971 | 1.018 |
| 0.20 | 0.7219 | 0.7326 | 1.015 |
| 0.10 | 0.4690 | 0.4712 | 1.005 |
| 0.05 | 0.2864 | 0.2842 | 0.992 |
| 0.02 | 0.1414 | 0.1368 | 0.967 |
| 0.01 | 0.0808 | 0.0774 | 0.958 |

The sign flips at a different place — between `p = 0.05` and `p = 0.1` here, against between
`p = 0.2` and `p = 0.3` for the i.i.d. sources. The crossover is a property of the source, not a
constant of the estimator.

<hr class="lz-tickrule">

## Why short sequences are biased upward

The mechanism is the empty dictionary at the start of the parse. An LZ76 factor can only be as long
as the match it finds in the prefix that precedes it, and at the beginning of a sequence there is no
prefix. Asymptotically the mean factor length approaches `log_k(n)/h`; at small *n* it falls short of
that, so the parse produces too many factors and `h = c·log_k(n)/n` comes out too high.

<div class="lz-formula">
  <p class="lz-math">mean factor length = <i>n</i> ⁄ <i>c</i> &rarr; log<sub><i>k</i></sub> <i>n</i> ⁄ <i>h</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i></dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length in symbols (bytes)</dd>
    <dt><i>k</i></dt><dd>alphabet size</dd>
    <dt><i>h</i></dt><dd>entropy rate — equals 1 for a fair coin measured in bits</dd>
  </dl>
  <p class="lz-formula__cite">Ornstein &amp; Weiss (1993) proved the Wyner–Ziv conjecture: the longest match into the past grows like (log n)/h almost surely, and that match is what an LZ76 factor is.</p>
</div>

For a fair coin the target is exactly `log₂ n`. Measured on single draws, CPython `random.seed(0)`
re-seeded per row:

| n | c | mean factor length | log₂ n | ratio | `h` |
|---:|---:|---:|---:|---:|---:|
| 100 | 18 | 5.56 | 6.64 | 0.836 | 1.1959 |
| 1 000 | 107 | 9.35 | 9.97 | 0.938 | 1.0663 |
| 10 000 | 777 | 12.87 | 13.29 | 0.969 | 1.0325 |
| 100 000 | 6 138 | 16.29 | 16.61 | 0.981 | 1.0195 |
| 1 000 000 | 50 789 | 19.69 | 19.93 | 0.988 | 1.0123 |

`h` is exactly the reciprocal of that ratio — for a binary sequence read in bits, `h = log₂n / (n/c)`
by definition. At *n* = 100 the average factor is 16 % shorter than its asymptotic length and `h` is
20 % too high. These are single draws, so the `h` column sits above the averaged table further up —
at *n* = 100 the across-draw standard deviation of `h` is 0.072, which is itself a reason not to
report a single short-sequence value.

!!! note "Low-entropy sources buy you less data than their length suggests"

    Lesne, Blanc & Pezard (2009) give the rule that a sequence behaves as if it had effective
    length `n_eff = n·h / ln k`. A near-constant sequence of a million symbols carries the
    statistical weight of a much shorter random one, which is why the `p = 0.01` row above is
    still 9 % off at *n* = 10⁵.

<hr class="lz-tickrule">

## `epsilon` — how untrustworthy a short value is

`lz.lz76()` returns `epsilon`, the Lempel–Ziv 1976 finite-size term. It is the most readable
indicator of whether a short-sequence result is worth reporting, and it costs nothing extra: it is
computed during the factorization you already ran.

<div class="lz-formula">
  <p class="lz-math">&epsilon;(<i>n</i>) = 2 &middot; (1 + log<sub><i>b</i></sub> log<sub><i>b</i></sub>(&alpha;&middot;<i>n</i>)) ⁄ log<sub><i>b</i></sub> <i>n</i></p>
  <dl class="lz-formula__key">
    <dt>&alpha;</dt><dd>the <code>alphabet</code> argument, or the auto-detected byte alphabet</dd>
    <dt><i>b</i></dt><dd>the <code>log_base</code> argument, or the same auto-detected value</dd>
    <dt><i>n</i></dt><dd>sequence length</dd>
  </dl>
  <p class="lz-formula__cite">The LZ76 bound c &lt; n ⁄ [(1 − ε) log<sub>σ</sub> n] is stated only for ε &lt; 1.</p>
</div>

Read it as a ceiling on `h`: the bound implies `h < 1/(1 − ε)`, and while `ε ≥ 1` there is no bound
at all. Measured on random binary sequences:

| n | `epsilon` | implied ceiling on `h` | measured `h` |
|---:|---:|---:|---:|
| 100 | 1.1843 | none — bound inapplicable | 1.1959 |
| 361 | 0.9999 | 6 747 | 1.1061 |
| 1 000 | 0.8940 | 9.44 | 1.0663 |
| 10 000 | 0.7280 | 3.68 | 1.0325 |
| 100 000 | 0.6187 | 2.62 | 1.0195 |
| 1 000 000 | 0.5406 | 2.18 | 1.0123 |

For binary input `ε` first drops below 1 at **n = 361**. Even at a million symbols it is still 0.54.
`epsilon` is a bound parameter, not a correction you subtract from `h`.

!!! warning "h above 1 at short lengths is expected, not a bug"

    A normalised `h` greater than 1 does not mean the estimator is broken — the theory permits it
    at every length you will ever handle. `h ≤ 1` is an asymptotic property. The binary ceiling is
    still 2.18 at *n* = 10⁶, and below *n* = 361 there is no ceiling at all. If you see `h` a little
    above 1 on high-entropy data, check `epsilon` before you check your pipeline.

!!! warning "epsilon is 0.0 on single-symbol sequences, and that zero is meaningless"

    `lz.lz76("0" * 10000)["epsilon"]` returns `0.0`, which reads as "the bound is tight here". It
    is not: the true binary ε(10⁴) is 0.7280. Constant sequences take a short-circuit path that
    never computes ε. Only trust `epsilon` when the sequence contains at least two distinct
    symbols.

`epsilon` gets worse as the alphabet grows, because the bound is stated in base-*σ* logarithms. The
length at which it first drops below 1:

<div class="lz-scroll" markdown>

| alphabet k | typical data | first n with ε &lt; 1 | ε(10³) | ε(10⁶) |
|---:|---|---:|---:|---:|
| 2 | binarised spikes, bits | 361 | 0.894 | 0.541 |
| 4 | DNA | 475 | 0.919 | 0.547 |
| 20 | amino acids | 6 117 | 1.214 | 0.683 |
| 26 | lowercase text | 9 882 | 1.273 | 0.711 |
| 64 | coarse-grained bins | 53 630 | 1.487 | 0.814 |
| 256 | raw bytes | 778 351 | 1.840 | 0.984 |

</div>

See [Alphabets and encoding](alphabets.md) for how `k` is detected and why it counts *bytes*.

<hr class="lz-tickrule">

## How much data do you need

There is no distribution-free answer. What follows is calibrated on i.i.d. and Markov sources at
the alphabet sizes shown. Correlated sources need more, not less: at *n* = 1 000 a two-state chain
with `p_flip = 0.1` sits **+3.9 %** above its entropy rate while an i.i.d. source of the same
entropy rate sits **+0.7 %** above — roughly five times further out, for the same length and the
same alphabet (mean of 200 draws each).

<div class="lz-scroll" markdown>

<table class="lz-compare">
<thead>
<tr><th>Goal</th><th>k = 2</th><th>k = 4 (DNA)</th><th>k ≈ 20–30 (text, AA)</th><th>k = 256 (bytes)</th></tr>
</thead>
<tbody>
<tr><td>Rank-order comparison, equal length and alphabet</td><td>n ≳ 10³</td><td>n ≳ 10³</td><td>n ≳ 10⁴</td><td>n ≳ 10⁵</td></tr>
<tr><td>ε &lt; 1 — the LZ bound applies at all</td><td>n &gt; 361</td><td>n &gt; 475</td><td>n &gt; ~10⁴</td><td>n &gt; 7.8·10⁵</td></tr>
<tr><td>Absolute <code>h</code> good to ~5 %</td><td>n ≳ 10³–10⁴</td><td>n ≳ 10³</td><td class="is-no">not reached by n = 10⁷</td><td class="is-no">not reached by n = 10⁷</td></tr>
<tr><td>Absolute <code>h</code> good to ~1 %</td><td>n ≳ 10⁶–10⁷</td><td>plateaus at 1.2–1.4 % low, never cleanly reached</td><td class="is-no">not practically reachable</td><td class="is-no">not practically reachable</td></tr>
<tr><td>Absolute <code>h</code> good to ~0.1 %</td><td class="is-no">not practically reachable</td><td class="is-no">—</td><td class="is-no">—</td><td class="is-no">—</td></tr>
</tbody>
</table>

</div>

Those rows come from `lz.h` on maximum-entropy i.i.d. sources, where the true normalised value is
exactly 1.0 at every alphabet size:

<div class="lz-scroll" markdown>

| n | k = 2 | k = 4 | k = 20 | k = 26 | k = 256 |
|---:|---:|---:|---:|---:|---:|
| 10³ | 1.0464 | 0.9982 | 0.8778 | 0.8728 | 0.7334 |
| 10⁴ | 1.0232 | 0.9859 | 0.9105 | 0.9023 | 0.8174 |
| 10⁵ | 1.0170 | 0.9860 | 0.9247 | 0.9077 | 0.8433 |
| 10⁶ | 1.0124 | 0.9869 | 0.9311 | 0.9237 | 0.8448 |
| 10⁷ | 1.0092 | 0.9878 | 0.9388 | 0.9380 | 0.8990 |

</div>

Only the binary column approaches 1 from above; every larger alphabet reads *low*, and at
`k = 256` it is still 10 % low at ten million symbols. The `k = 4` column is not even monotone — it
happens to land within 0.2 % at *n* = 10³ and then drifts *away* to 1.4 % low, so a single
agreement at one length proves nothing. Absolute `h` is not comparable across alphabets; see
[Alphabets and encoding](alphabets.md).

By measure, rather than by alphabet:

<div class="lz-scroll" markdown>

| Measure | Worth reporting from | What it needs, and why |
|---|---:|---|
| `complexity` | any *n* | Exact at every length — it is a count, not an estimate. But it is comparable **only** at fixed *n* and fixed alphabet. |
| `h` | *n* ≳ 10³ to rank, 10⁶ to quote | Bias falls off like a power of `1/log n`, and its sign depends on the source. Report `epsilon` alongside it. |
| `emc` | *n* ≳ 10⁴, and only to rank | Rests on one surrogate per block size, at a block-size ceiling that is itself a function of *n*. Non-negative by construction, so its noise on structureless input is one-sided: over 50 i.i.d. binary draws at *n* = 2 048 the null sits at mean 0.15, s.d. 0.17, up to 0.58, and is exactly `0.0` in 19 of them. Subtract that floor before reading a small value as signal. |
| `nid` | *n* ≳ 10³ for shape, 10⁴–10⁵ for a number | A ratio, so the leading length factors cancel — but both endpoints are compressed toward the middle at short *n* (below). |
| `multi_information` | diagnostic only | Built from the block-size-1 shuffle, which performs *n*/2 single-symbol swaps and so never touches about 37 % of the positions (e⁻¹) at any *n*. |

</div>

See [Entropy density](entropy-density.md), [Effective measure complexity](emc.md) and
[Normalised information distance](nid.md) for what each of these measures is.

<hr class="lz-tickrule">

## Never compare raw `complexity` or `emc` across lengths

This is the one hard rule on the page. `complexity` grows with *n* for any source; `emc` depends on
a block size that is itself computed from *n*. Comparing either between sequences of different
lengths measures the lengths, not the sequences.

Two sequences from the **same** sticky-Markov source (`p_flip = 0.05`), one of 4 000 symbols and
one of 12 000:

| | n = 4 000 | n = 12 000 | verdict |
|---|---:|---:|---|
| `complexity` | 102 | 252 | **2.5× apart — an artefact of length** |
| `h` (bits) | 0.3051 | 0.2846 | comparable |
| `complexity` after truncating both to 4 000 | 102 | 90 | comparable |
| surrogate-normalised LZc | 0.296 | 0.278 | comparable |

`emc` is worse, because the estimator's largest block size `mm` is derived from *n*. The identical
period-8 square wave, at three lengths, read off `lz.lz76(seq)` (`mm` is
`result["emc"]["max_block_size"]`):

| n | `complexity` | `h` | `emc` | `mm` |
|---:|---:|---:|---:|---:|
| 2 000 | 3 | 0.016449 | +5.2992 | 18 |
| 5 000 | 3 | 0.007373 | +5.7433 | 19 |
| 20 000 | 3 | 0.002143 | +5.4308 | 21 |

Same source, same generating rule, `emc` moves by 8 % — and it does not even move monotonically with
*n*. Widen the range and the drift is larger: the same motif measured from *n* = 1 000 to
*n* = 100 000 falls from 5.9296 to 4.9701, 16 % on a source whose true excess entropy is a constant
3 bits.

!!! danger "`emc` is evaluated on a ladder of block sizes whose top rung is a function of *n*"

    The estimator builds one rung per block size, `Ê(l) = l · g · (C_LZ(shuffled at l) −
    C_LZ(original))` with `g = log_k(n)/n`, and reads the value off the top of the ladder after
    projecting it onto the non-negative non-decreasing cone. Every scale contributes, but the ceiling
    `mm` is derived from *n* and the rungs keep climbing past the block length the sample can support
    — so two lengths mean two different ladders and the totals are not on a common axis. Comparing
    `emc` across lengths compares two different estimators. See
    [Effective measure complexity](emc.md).

    One ambiguity that used to live here is gone. Up to 1.0.2 the total telescoped to its own top
    rung, so a periodic sequence whose period divided `mm` and a structureless random one both landed
    at zero, for opposite reasons. A zero now means only the second thing.

### Fix 1 — truncate to the shortest

The cheapest fix and the one to reach for first. It costs you data and nothing else.

```python
import random
import lzcomplexity as lz

def sticky(n, p_flip, seed):
    """Two-state Markov chain that flips with probability p_flip."""
    rng = random.Random(seed)
    bit, out = "0", ["0"]
    for _ in range(n - 1):
        if rng.random() < p_flip:
            bit = "1" if bit == "0" else "0"
        out.append(bit)
    return "".join(out)

a = sticky(4000, 0.05, 1)
b = sticky(12000, 0.05, 2)

def compare_at_common_length(sequences):
    """Truncate every sequence to the shortest, then take raw complexity."""
    n = min(len(s) for s in sequences)
    return [lz.factorization(s[:n])[0] for s in sequences]

print(lz.factorization(a)[0], lz.factorization(b)[0])   # 102 252   <- not comparable
print(compare_at_common_length([a, b]))                 # [102, 90] <- comparable
```

Truncate from the same end for every sequence and say which end in your methods. If the sequences
are non-stationary — a recording that drifts — truncating to a common *window* is more defensible
than truncating to a common prefix.

### Fix 2 — normalise against shuffled surrogates of the same length

Divide the complexity by the mean complexity of random permutations of the *same symbols*. The
surrogate has the same length, the same alphabet and the same symbol frequencies, so both the
length dependence and the composition dependence divide out. What is left is temporal structure.

```python
import random, statistics
import lzcomplexity as lz

def lzc_normalised(seq, n_surrogates=20, seed=0):
    """Complexity divided by the mean complexity of shuffles of the same symbols."""
    rng = random.Random(seed)
    c = lz.factorization(seq)[0]
    ref = []
    for _ in range(n_surrogates):
        shuffled = list(seq)
        rng.shuffle(shuffled)
        ref.append(lz.factorization("".join(shuffled))[0])
    return c / statistics.fmean(ref)

print("%.3f  %.3f" % (lzc_normalised(a), lzc_normalised(b)))   # 0.296  0.278
```

!!! tip "The shuffles cost more than the factorizations"

    A single `lz.factorization` on a 100 000-symbol binary sequence takes 6.9 ms
    ([Performance](../project/performance.md)), so 20 surrogates spend 0.14–0.25 s in the library.
    Building the 20 shuffled strings in Python costs **four times** that. End-to-end wall clock for
    the call above measured between 0.7 s and 1.5 s across repeated runs on a shared machine, and
    the split between the two stages was stable at about 4:1 in every one of them. Surrogate
    normalisation is cheap enough to run on every sequence in a study rather than on a subsample —
    and if it is not, replace `random.shuffle` before you blame the factorization.

The normalised value has a natural reading: **1.0 means "indistinguishable from a shuffle of
itself"**, and lower means "carries temporal structure that a shuffle destroys". Measured on five
signals at *n* = 10 000, `random.Random(0)` for the stochastic ones and *x*₀ = 0.4 with 2 000 burn-in
steps for the map:

| signal | LZc | LZc (shuffled) | normalised | `h` (bits) |
|---|---:|---:|---:|---:|
| fair coin (i.i.d.) | 777 | 772.0 | 1.006 | 1.032 |
| biased coin p = 0.1 | 351 | 351.2 | 0.999 | 0.466 |
| sticky Markov p_flip = 0.05 | 229 | 772.9 | 0.296 | 0.304 |
| period-8 square wave | 3 | 772.6 | 0.004 | 0.004 |
| logistic map r = 4.0 | 768 | 773.1 | 0.993 | 1.020 |

The two structureless rows land at 1.006 and 0.999, not at exactly 1.000 — with 20 surrogates the
ratio scatters a few thousandths either side, so treat 1.0 as a neighbourhood rather than a line.

Read the last row against the third. The logistic map at *r* = 4 is fully deterministic, and the
normalised score correctly reports that it looks random to LZ76 — a shuffle of it is no more
complex than the original. Determinism and low complexity are not the same thing; see
[Dynamical systems](../recipes/dynamical-systems.md).

!!! example "Both fixes on one pair, start to finish"

    Run after the two snippets above, which define `sticky`, `a`, `b` and `lzc_normalised`.

    ```python
    print("raw complexity   :", lz.factorization(a)[0], lz.factorization(b)[0])
    print("truncated to 4000:", lz.factorization(a[:4000])[0], lz.factorization(b[:4000])[0])
    print("normalised LZc   : %.3f  %.3f" % (lzc_normalised(a), lzc_normalised(b)))
    print("h (bits)         : %.4f  %.4f" % (lz.h(a, log_base=2), lz.h(b, log_base=2)))
    ```

    ```text
    raw complexity   : 102 252
    truncated to 4000: 102 90
    normalised LZc   : 0.296  0.278
    h (bits)         : 0.3051  0.2846
    ```

    Raw `complexity` says the second sequence is 2.5× more complex. Every length-controlled measure
    says the two came from the same source, which they did.

!!! note "A textbook count is one higher when the parse overruns the end"

    `complexity` counts only **complete** LZ76 components. When the final component runs past the
    end of the sequence it is not counted, so the library's number is one less than the textbook
    exhaustive-history count in exactly that case:

    ```python
    c, factors = lz.factorization(seq)
    c_textbook = c + (1 if factors[-1] > len(seq) else 0)
    ```

    Checked against a brute-force exhaustive-history parser over all 131 070 binary strings of
    length 1 … 16, this is exact for every string containing at least two distinct symbols, and
    wrong for every constant one — `lz.factorization("0000")` returns `(1, [0, 1, 4])`, so the
    formula gives 1 where the textbook gives 2. That is the same short-circuit that zeroes
    `epsilon` above.

    The difference is `O(1)` and irrelevant to `h` asymptotically, but it matters when you are
    checking a short worked example against a paper.

<hr class="lz-tickrule">

## `nid` is a ratio, so it degrades more gracefully — but it still degrades

`nid` is built from a ratio of factor counts, so the `log_k(n)/n` factor that dominates the
finite-size behaviour of `h` cancels. That makes it the least length-sensitive measure in the
library. It is not length-*independent*.

Two independent fair-coin sequences should be maximally distant, `nid ≈ 1`. Five draws at each
length:

| n | `nid` between two independent fair coins |
|---:|---|
| 10 | 0.7500 0.2500 0.4000 0.5000 0.7500 |
| 50 | 0.7000 0.7000 0.7778 0.6364 0.7000 |
| 100 | 0.6667 0.7500 0.6667 0.6667 0.7059 |
| 1 000 | 0.8148 0.8037 0.7944 0.7798 0.8019 |
| 10 000 | 0.8562 0.8501 0.8544 0.8566 0.8588 |
| 100 000 | 0.8862 0.8844 0.8834 0.8826 0.8798 |

The true value is approached from below and is still 0.88 at *n* = 10⁵. The other endpoint moves
too — a sequence compared with **itself** should give exactly 0:

| n | `nid(a, a)` |
|---:|---:|
| 10 | 0.333333 |
| 50 | 0.111111 |
| 100 | 0.062500 |
| 1 000 | 0.009709 |
| 10 000 | 0.001292 |

Both endpoints are pulled toward the middle at short *n*, so short-sequence distances have less
dynamic range than they appear to. The span from "identical" to "unrelated" is 0.06 → 0.69 at
*n* = 100, and 0.001 → 0.86 at *n* = 10⁴.

!!! warning "Below about 100 symbols nid takes a handful of distinct values and stops discriminating"

    `nid` on very short inputs is a ratio of small integers, so it collapses onto a coarse grid:
    `nid("0", "1")` is `1.0`, `nid("01", "10")` is `0.5`, and `nid("abc", "abd")` and
    `nid("0110", "1001")` are both exactly `0.3333333333333333`. Two genuinely different pairs
    landing on the same value is the normal case at that length, not a coincidence. Any tree or
    embedding built from a distance matrix over sequences that short is reading quantization noise.

For clustering and distance matrices: use sequences of comparable length, prefer *n* ≳ 10⁴ before
reading absolute distances, and treat differences of a few hundredths as noise unless you have
checked them against a permutation null.

<hr class="lz-tickrule">

## Checklist before you report a number

<div class="lz-cards" markdown>
<div class="lz-card" markdown>
### Fix the length
Truncate every sequence in the comparison to the shortest, or normalise each against surrogates of
its own length. Never compare raw `complexity` or `emc` at different *n*.
<p class="lz-card__api"><code>seq[:n_min]</code></p>
</div>
<div class="lz-card" markdown>
### Fix the alphabet
Pass `log_base=` explicitly. The default is the alphabet auto-detected *in that sequence*, so two
files drawn from the same four-letter alphabet get different normalisations if one happens to be
missing a letter.
<p class="lz-card__api"><code>lz.h(seq, log_base=4)</code></p>
</div>
<div class="lz-card" markdown>
### Report epsilon
Quote `epsilon` next to any absolute `h`. If it is at or above 1, the LZ bound does not apply to
your sequence and the value is a ranking statistic, nothing more.
<p class="lz-card__api"><code>lz.lz76(seq)["epsilon"]</code></p>
</div>
<div class="lz-card" markdown>
### Build your own error bars
`normal_error` and `poison_error` are heuristic dispersion indicators inherited from the C++
library, not confidence intervals. Use surrogates or sub-block resampling instead.
<p class="lz-card__api"><code>lzc_normalised(seq)</code></p>
</div>
</div>

Continue to [Reading the numbers](../guide/reading-the-numbers.md) for what each field of the
result dictionary means, or to [References](../project/references.md) for the papers behind the
convergence results quoted here.
