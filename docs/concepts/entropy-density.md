# Entropy density (h)

*What `h` estimates, in which units, how fast it converges, and which companion numbers to ignore.*

`h` is the LZ76 factor count rescaled so that it has a limit. It is a per-symbol
quantity: how much new information each symbol carries, given everything before
it. For a stationary ergodic source it converges to the entropy rate. For
anything else it is a descriptive statistic of one string and must not be called
an entropy rate.

## The formula

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>c</i>(<i>S</i>) · log<sub><i>b</i></sub> <i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length, in <b>bytes</b></dd>
    <dt><i>b</i></dt><dd>logarithm base — the <code>log_base</code> argument, defaulting to the auto-detected alphabet size <i>k</i>, floored at 2</dd>
  </dl>
  <p class="lz-formula__cite">Transcribed from <code>entropy_density_from</code> in <code>crates/lzcomplexity-core/src/lz76.rs</code>: <code>div = n / (ln n / ln b); h = c / div</code>.</p>
</div>

`c(S)` is the library's factor count, which counts only **complete** LZ76
components. The trailing component that runs past the end of the sequence is not
counted, so this number is one less than the textbook exhaustive-history count
whenever the sequence ends mid-component — see
[LZ76 factorization](lz76.md) for the exact conversion. The difference is `O(1)`
and does not affect the limit, but it does shift `h` on short sequences.

`n ≤ 1` is guarded: `lz.h("")` and `lz.h("a")` both return exactly `0.0`.

## Units

The log base is the unit. It enters only as a multiplicative constant, so
conversion between conventions is exact, never approximate.

| `log_base` | Unit of `h` | Target range | Conversion from the default |
|---|---|---|---|
| `None` (default → alphabet size `k`) | normalised, dimensionless | `[0, 1]` asymptotically | — |
| `2` | bits per symbol | `[0, log₂ k]` | `h_bits = h_norm · log₂ k` |
| `10` | hartleys per symbol | `[0, log₁₀ k]` | `h_hartley = h_norm · log₁₀ k` |

In the default normalisation a maximally random `k`-ary source sits at **1**.
Encoding one LZ76 factor costs roughly one back-pointer into the prefix already
seen, i.e. about `log_k(n)` base-`k` digits, so `c · log_k(n)` is a code length
and `h` is digits per input symbol. Values below 2 are clamped to 2:
`log_base=0` and `log_base=1` return exactly what `log_base=2` returns.

!!! example "Bits and normalised units differ by exactly log₂ k"

    A 58-nucleotide DNA string over `{A,C,G,T}` (`k = 4`, `c = 18`, `n = 58`):

    ```python
    import math
    import lzcomplexity as lz

    dna = "ACGTACGTTGCAACGTGGATCCGTAAGCTTACGATCGATCGGATCAGCTAGCATCGAT"

    print(lz.h(dna))                 # 0.9089970509680715  normalised, k = 4
    print(lz.h(dna, log_base=2))     # 1.817994101936143   bits per symbol
    print(lz.h(dna) * math.log2(4))  # 1.817994101936143   bit-identical
    ```

    `lz.h(dna) * math.log2(4) == lz.h(dna, log_base=2)` evaluates to `True`.

!!! warning "`alphabet=` silently does nothing to `h`; only `log_base=` rescales it"

    Passing `alphabet=2` to `lz.h` returns the same float as passing nothing —
    the entropy formula never reads that argument. `alphabet` feeds
    [`epsilon`](#epsilon-the-finite-size-slack) and nothing else.

    ```python
    lz.h(dna, alphabet=2)   # 0.9089970509680715 — unchanged
    lz.h(dna, log_base=2)   # 1.817994101936143  — changed
    ```

    The `lzcomplexity` CLI is the exact mirror image: its `lz76EntropyDensity`
    is controlled by `-a` and is **never** affected by `-l`, on any code path.
    See [the CLI reference](../cli/lzcomplexity.md).

The default base is auto-detected per sequence from the distinct **byte** values
present. Two sequences over the same nominal alphabet get different
normalisations if one of them happens to be missing a symbol. Pass `log_base`
explicitly for any cross-sequence comparison — see [Alphabets](alphabets.md).

## Why it converges

Three results are routinely conflated in the literature and in this project's
own README history. What each actually proves:

<div class="lz-scroll" markdown>

| Result | What it proves | Assumptions |
|---|---|---|
| **Lempel & Ziv (1976)** | The deterministic bound `c(S) < n / ((1 − ε_n) · log_k n)`, and the definition of `ε_n`. A combinatorial statement about strings; no probabilistic content. | Finite alphabet. Holds for **every** string. Vacuous unless `ε_n < 1`. |
| **Ziv (1978)**, *Coding theorems for individual sequences* | `c(S)·log_k(n)/n → h` almost surely, `h` the entropy rate. **This is the theorem behind `lz.h`.** | Stationary, ergodic, finite alphabet. |
| **Ornstein & Weiss (1993)** | For a stationary ergodic process the LZ parse cuts the string into blocks of mean length `≈ (log n)/h`, hence `c(n) ≈ n·h/log n`. Also the return-time theorem `(log R_n)/n → h` almost surely. | Stationary, ergodic, finite alphabet. |
| **Kontoyiannis, Algoet, Suhov & Wyner (1998)** | Pointwise and mean consistency of a **different** family of estimators — Cesàro averages of longest match lengths — not of the LZ76 factor count. | Stationary, ergodic, finite alphabet, **plus a Doeblin-type mixing condition**. |

</div>

Kontoyiannis et al. (1998) is not a consistency proof for `c·log n/n`. It is the
standard reference for the competing match-length estimators, and its proof
needs a strictly stronger hypothesis than the LZ76-count result does. Cite Ziv
(1978) and Ornstein & Weiss (1993) for `h`; full entries are in
[References](../project/references.md).

!!! danger "Without stationarity and ergodicity there is no entropy rate to estimate"

    `h` still returns a number — it is a deterministic function of the bytes —
    but that number describes one string, not a population parameter.
    Ergodicity is the entire licence for reading a single trajectory as a stand-in
    for the ensemble. Drifting recordings, concatenated experimental conditions
    and spliced files break the theorem, not merely the error bar.

## Calibration against known entropy rates

Measured with `log_base=2`, so the target is the true rate in bits per symbol.
Every table below is generated with CPython's `random` module at a fixed seed;
the exact protocol is stated with each one so the numbers can be re-run.

**i.i.d. Bernoulli(p), `n = 100 000`, one realisation per row (`random.seed(11)`
once, then rows drawn in the order shown):**

| `p` | true `H(p)` bits | `h` (bits) | ratio |
|---|---|---|---|
| 0.50 | 1.0000 | 1.0137 | 1.014 |
| 0.40 | 0.9710 | 0.9825 | 1.012 |
| 0.30 | 0.8813 | 0.8921 | 1.012 |
| 0.20 | 0.7219 | 0.7197 | 0.997 |
| 0.10 | 0.4690 | 0.4593 | 0.979 |
| 0.05 | 0.2864 | 0.2704 | 0.944 |
| 0.01 | 0.0808 | 0.0726 | 0.898 |

**Two-state symmetric Markov chain with flip probability `p_flip`, `n = 200 000`,
`random.seed(5)` per row.** The true entropy rate of this chain is `H(p_flip)`.

| `p_flip` | true `h` | `h` (bits) |
|---|---|---|
| 0.50 | 1.0000 | 1.0149 |
| 0.30 | 0.8813 | 0.8947 |
| 0.20 | 0.7219 | 0.7293 |
| 0.10 | 0.4690 | 0.4707 |
| 0.05 | 0.2864 | 0.2833 |
| 0.02 | 0.1414 | 0.1351 |
| 0.01 | 0.0808 | 0.0776 |

`h` tracks the true rate over a twelvefold span of entropy — 1.0 down to 0.08
bits per symbol — to within a few percent in the middle of the range.

The sign of the error is
source-dependent: high-entropy sources come out **too high**, low-entropy
sources **too low** (10 % low at `H = 0.08` bits). Do not tell yourself "LZ
overestimates". It does both.

## Convergence is slow, and the bias dominates

Fair coin, true `h = 1` bit/symbol. Mean over 5 realisations
(2 at `n ≥ 300 000`), `random.seed(1000 + rep)` per realisation:

| `n` | `h` (bits) | bias |
|---|---|---|
| 100 | 1.1560 | +0.1560 |
| 300 | 1.0917 | +0.0917 |
| 1 000 | 1.0484 | +0.0484 |
| 3 000 | 1.0403 | +0.0403 |
| 10 000 | 1.0301 | +0.0301 |
| 30 000 | 1.0228 | +0.0228 |
| 100 000 | 1.0173 | +0.0173 |
| 300 000 | 1.0148 | +0.0148 |
| 1 000 000 | 1.0122 | +0.0122 |

Four decades of extra data cut the bias by a factor of 13.

Over this range the bias tracks `1/(log₂ n)²` rather than `1/log₂ n`:
`bias · (log₂ n)²` sits between 4.6 and 5.3 from `n = 10³` to `n = 10⁷`, while
`bias · log₂ n` falls steadily from 0.48 to 0.24. That is an **empirical fit over
the lengths tested on a fair coin**, not a theorem, and not a rate you should
extrapolate to another source — see the Shields result below. Stated
operationally for this one case: squaring `n` buys you a factor of 4.

Meanwhile the across-realisation standard deviation falls at least as fast as
`1/√n` (measured `sd·√n` = 0.47, 0.52, 0.36, 0.29 at `n = 10³ … 10⁶`). At
`n = 10⁶` the bias is `+0.012` and the standard deviation is `0.0003` — forty
times smaller. The error in `h` is **bias, not noise**, so averaging over more
sequences does not remove it.

There is also no distribution-free sample-size guarantee. Shields (1993) proved
that universal redundancy rates do not exist over the class of ergodic sources:
for any `n` and any claimed error bound there is a stationary ergodic process
that violates it. Every sample-size rule of thumb, including the ones in
[Convergence and bias](convergence.md), is calibrated on a particular family of
sources and labelled as such.

!!! warning "`h > 1` on binary data is normal, not a bug"

    On i.i.d. binary input the mean `h` exceeds 1 at every length tested. The
    table above is still at 1.0122 at a million symbols, and at `n = 10⁷`
    (3 realisations, `random.seed(7000 + rep)`) the mean is 1.0092. Theory
    only bounds `h < 1/(1 − ε_n)`, and `ε_n` is still 0.54 at `n = 10⁶`. `h ≤ 1`
    is an asymptotic property, not a finite-`n` one.

    `h < 0` is impossible: `c ≥ 1`, `log_b(n) > 0` for `n ≥ 2`, and `n ≤ 1`
    returns `0.0`. A negative `h` would indicate a bug.

## `epsilon` — the finite-size slack

`lz76()` returns `epsilon`, the Lempel–Ziv 1976 term that appears in the
finite-`n` bound.

<div class="lz-formula">
  <p class="lz-math">ε<sub><i>n</i></sub> = 2 · ( 1 + log<sub><i>b</i></sub> log<sub><i>b</i></sub>(α · <i>n</i>) ) ⁄ log<sub><i>b</i></sub> <i>n</i></p>
  <dl class="lz-formula__key">
    <dt>α</dt><dd>the <code>alphabet</code> argument, or the auto-detected alphabet size; floored at 2</dd>
    <dt><i>b</i></dt><dd>the <code>log_base</code> argument, or the auto-detected alphabet size; floored at 2</dd>
  </dl>
  <p class="lz-formula__cite">Accompanies <i>c</i>(<i>S</i>) &lt; <i>n</i> ⁄ ((1 − ε<sub><i>n</i></sub>) · log<sub><i>k</i></sub> <i>n</i>), which gives <i>h</i> &lt; 1 ⁄ (1 − ε<sub><i>n</i></sub>) — valid only where ε<sub><i>n</i></sub> &lt; 1.</p>
</div>

Apart from the constant-sequence fast path below, `epsilon` depends only on `n`,
`α` and `b` — never on the sequence content. It is
a **bound-slack parameter, not a correction term**. Do not subtract it from `h`
and do not divide by it. Its only job is to tell you whether the Lempel–Ziv
ceiling on `h` says anything at all at your length.

| alphabet `k` | first `n` with `ε_n < 1` | `ε(10³)` | `ε(10⁶)` |
|---|---|---|---|
| 2 | 361 | 0.894 | 0.541 |
| 4 | 475 | 0.919 | 0.547 |
| 20 | 6 117 | 1.214 | 0.683 |
| 26 | 9 882 | 1.273 | 0.711 |
| 64 | 53 630 | 1.487 | 0.814 |
| 256 | 778 351 | 1.840 | 0.984 |

Below the crossover the bound is not weak, it is **inapplicable**: `1 − ε_n ≤ 0`
makes the right-hand side negative or infinite. Above it, the ceiling is
generous — `1/(1 − ε)` is 2.18 for binary data at `n = 10⁶`.

!!! warning "`epsilon` is 0.0 for constant sequences, and that zero means nothing"

    A sequence with a single distinct symbol takes a fast path that returns
    `epsilon: 0.0` without evaluating the formula. `lz.lz76("0"*10000)["epsilon"]`
    is `0.0`, while `lz.lz76("01"*5000)["epsilon"]` is `0.7279963473405144` at the
    same length. Read `epsilon` only when the sequence has at least two distinct
    symbols.

## `normal_error`, `poison_error` and `factors_stddev`

`lz76()` returns three dispersion numbers alongside `h`. None of them is a
standard error on `h`.

<div class="lz-scroll" markdown>

| Key | Exact formula | What it is |
|---|---|---|
| `factors_stddev` (`σ_f`) | `sqrt( Σ_i (ℓ_i − mean)² / max_i ℓ_i )`, over the factor lengths `ℓ_i`, with `mean = (factors[-1] − 1) / len(factors)` — `factors` being the returned **boundary** array | An internal dispersion statistic. It divides by the **longest factor**, not by the factor count, so it is **not a standard deviation**. |
| `normal_error` | `h^{3/2} · σ_f · sqrt( log_b(n) / n )` | Heuristic inherited from the C++ library: "error assuming a normal distribution of factor lengths". |
| `poison_error` | `h / n` | Heuristic inherited from the C++ library. `poison` is **sic** — a long-standing misspelling of *Poisson*, now frozen into the public API at 1.0. The C++ Doxygen comments say "Poisson"; the identifier, and one source comment, say "poison". |

</div>

Both errors are `0.0` when `n ≤ 1`, and both are affected by `log_base` (through
`h`, and for `normal_error` also through `log_b(n)`).

!!! danger "Do not publish these as error bars — neither is a derived standard error"

    The honest yardstick is the across-realisation spread of `h` itself. On
    i.i.d. binary input at `n = 10⁶` that standard deviation is `3·10⁻⁴`
    (12 realisations). Against it, `normal_error` (0.417) is **~1 400× too
    large** and `poison_error` (`1.0·10⁻⁶`) is **~290× too small**. No
    derivation exists in this codebase or its C++ ancestor, and no reference is
    cited for either expression.

    Measured on i.i.d. binary sequences (CPython `random.seed(0)` re-seeded per row):

    | `n` | `c` | `h` | `factors_stddev` | true sample sd of factor lengths | `normal_error` | `poison_error` |
    |---|---|---|---|---|---|---|
    | 100 | 18 | 1.19589 | 3.066 | 2.26 | 1.033506 | 1.196e-02 |
    | 1 000 | 107 | 1.06634 | 6.988 | 2.78 | 0.768171 | 1.066e-03 |
    | 10 000 | 777 | 1.03246 | 15.723 | 2.65 | 0.601275 | 1.032e-04 |
    | 100 000 | 6 138 | 1.01950 | 35.124 | 2.50 | 0.465978 | 1.019e-05 |
    | 1 000 000 | 50 789 | 1.01230 | 91.678 | 2.47 | 0.416874 | 1.012e-06 |

    Reading the columns:

    - `factors_stddev` grows without bound (3.1 → 91.7) while the **true** sample
      standard deviation of the same factor lengths stays near 2.5. Dividing the
      sum of squares by the longest factor instead of by the factor count
      inflates it by `sqrt(count / max_i ℓ_i)`: measured down the rows above,
      `σ_f / σ_true` is 1.36, 2.51, 5.94, 14.07, 37.05 against a predicted
      1.38, 2.52, 5.95, 14.07, 37.05.
      The factor count grows with `n` while the longest factor grows like
      `log n`, so the inflation grows without limit.
    - `normal_error` inherits that and shrinks only 2.5-fold across four decades
      of `n`. It would claim a 41 % relative uncertainty on `h` at a million
      symbols, where the measured standard error is `3·10⁻⁴`.
    - `poison_error` is exactly `h/n`, so `poison_error / h ≡ 1/n` by
      construction. A naive Poisson standard error for a count of `c` factors
      would be `h/√c`, which is `4.5e-03` on the last row — about 4 400× the
      `1.0e-06` shipped. No consistent estimator of an entropy rate achieves
      `1/n` relative error.

    The numbers above are correct outputs of the documented formulas: recomputing
    each closed form from `h`, `σ_f`, `n` and `log_base` reproduces the shipped
    value bit-for-bit, provided you match the implementation's operation order
    (`sqrt(h*h*h) * σ_f / sqrt(n / log_b(n))`; `sqrt(h**3)` can differ in the
    last bit). The formulas are the problem, not the arithmetic.

### What to use instead

For an honest uncertainty on `h`, resample: surrogates for the bias, a block
bootstrap for the variance.

- **Surrogates.** Compute `h(x)` and `h(shuffle(x))` over uniform random
  permutations of the same string. The shuffle preserves length, alphabet and
  symbol histogram, so both sides carry a finite-size bias of similar size and
  the ratio `h(x) / ⟨h(shuffle(x))⟩` cancels part of it. Do not assume it
  cancels cleanly: the Bernoulli table above shows the bias changes **sign**
  between high- and low-entropy sources, and a shuffle raises the entropy rate,
  so the two biases are not the same number. Shuffle in Python
  (`random.shuffle`, `numpy.random.permutation`): the library's internal
  block-swap shuffle used by [`emc`](emc.md) deliberately preserves within-block
  structure and is the wrong surrogate for this.
- **Block bootstrap.** Resample contiguous blocks of the sequence and recompute
  `h`. The spread across resamples is a defensible variance estimate — it still
  says nothing about the bias, which the tables above show is the larger error.

The surrogate protocol is worked end to end in
[Neuroscience recipes](../recipes/neuro.md); the practical reading rules for all
of these numbers are in [Reading the numbers](../guide/reading-the-numbers.md).

<div class="lz-tickrule"></div>

!!! note "These two fields diverge from the C++ backend"

    `factors_stddev` and `normal_error` are the only `lz76()` outputs that do not
    match the legacy C++ implementation numerically. The C++ `FoundStddev`
    reduces over a vector that was value-initialised with zeros and *then*
    appended to, so its sum of squares includes one spurious zero-length entry
    per boundary. The Rust port iterates only the real lengths.

    Recomputing the C++ expression by hand from the same boundary arrays puts its
    `σ_f` larger by 1.26–1.78× on four short test sequences. That range comes
    from reading the C++ source and re-deriving the formula, **not** from running
    the C++ build, so treat it as indicative rather than measured. See
    [C++ parity](../project/cpp-parity.md).
