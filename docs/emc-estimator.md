# Excess-entropy estimator on this branch

This note describes the revised estimator in PR #14. The
[published website](https://pleros-ai.github.io/lzcomplexity/) describes the
released shuffle estimator, including historical measurements from earlier
versions. Those E tables, timings, RNG explanations and C++ parity results do
not apply to this algorithm. Record the package version and, for an unreleased
checkout, the commit used for an analysis.

## Quantity and estimator

For a stationary finite-alphabet source, excess entropy is the mutual
information between the infinite past and infinite future:

```text
E = lim I(X[-a:0]; X[0:b])      as a,b -> infinity.
```

At total window length `m`, use `a = floor(m/2)` past symbols and
`b = ceil(m/2)` future symbols. The population quantity is

```text
E_m = H(X[-a:0]) + H(X[0:b]) - H(X[-a:b]).
```

For even `m = 2L` this is `2H(L) - H(2L)`. It is non-negative,
non-decreasing as the two windows expand, and a lower bound on E. Its limit
can be infinite. These are properties of the population quantity; they do
not make a finite-sample estimate an unbiased estimate or a lower bound.
See [Crutchfield and Feldman (2003)](https://arxiv.org/abs/cond-mat/0102181)
for excess entropy and its past/future mutual-information form.

The implementation forms an empirical joint frequency distribution and
computes its mutual information. Both marginals are obtained from exactly
the same observations as the joint counts. It uses no random shuffle,
Chao–Shen coverage correction, or LZ entropy-rate subtraction.

Let `M` be the resolved maximum total window length and `N` the input byte
length. There are `N-M+1` observations, starting at each complete length-`M`
window. All smaller scales use these same starts and a common past/future
boundary. Their balanced windows are nested. Consequently their empirical
mutual informations are also non-negative and non-decreasing, apart from
floating point roundoff. The implementation does not select a ladder peak
or fit an isotonic regression.

The public `h` function remains the separate LZ76 entropy-rate estimate.
Its finite-sample bias is not multiplied by the window length in E.

## Parameters and returned fields

- `max_block_size` is the **combined** past-plus-future length, not the
  length of each side. Auto mode (`<= 0`) chooses the largest `M` satisfying
  `M * sigma**M <= N`, with a minimum of 1, using the effective alphabet
  size `sigma`. An explicit positive cap is clipped to `N`. Empty input
  returns zero and an empty terms vector.
- `alphabet` supplies the effective alphabet size; its default is the number
  of distinct input bytes, floored at 2. Use a common declared alphabet for
  comparisons where not every sample contains all source symbols. The auto
  cap depends on this value and the length, independently of output units.
- `log_base` changes the units. Its default is the effective alphabet size;
  `log_base=2` gives E in **bits** and `h` in **bits per byte**. Values below
  2 are treated as 2.
- `value` is the empirical mutual information at scale `M`.
- `summands[m-1]` is the increase from scale `m-1` to `m`, taking the initial
  value as zero. The first term is zero because scale 1 has an empty past.
  Terms sum to `value`. They are increments of finite-window mutual
  information, not estimates of the old `h(m)-h` summands.
- `multi_information` retains the separate diagnostic
  `H_plugin(1) - h_LZ`, using one-symbol counts over the entire input. It
  can be negative and does not enter the E estimate.

Python always requests `summands`. Rust callers must set
`LzArgs::get_shuffle_terms = true` to obtain the vector. Historical Rust
names such as `LzShuffle` and `lz76_random_shuffle_complexity`, and CLI JSON
keys such as `lz76RandomShuffleComplexity`, remain for compatibility;
they no longer describe how E is calculated. The shuffle-distance metrics
have a separate surrogate calculation.

For example, the two alternating phases of a period-2 sequence share one
bit once each side contains a symbol:

```python
import lzcomplexity as lz

value, terms = lz.emc(b"01010101", max_block_size=3, log_base=2)
assert value == 1.0
assert terms == [0.0, 1.0, 0.0]
```

## Interpretation and limits

The auto cap is a conservative alphabet-occupancy heuristic, not a confidence
guarantee. Overlapping windows are dependent observations. Increasing the
window length introduces many more possible categories and can inflate
empirical mutual information even for an independent source. Conversely,
windows shorter than a source's memory cannot recover all its excess
entropy. Zero is not proof that the source has no memory.

A concrete limit is a stationary process formed by repeating a binary
de Bruijn cycle of order `r`, with a uniformly random initial phase. Its
period is `2**r`, its entropy rate is zero, and its true E is `r` bits:
the infinite past and future both identify the phase. Yet every binary
word of length `m <= r` has the uniform distribution, so the population
`E_m` is zero at every such scale. A cap `M <= r` cannot distinguish this
long-memory periodic process from a fair coin, even with unlimited data.
This illustrates finite-window identifiability, not a numerical defect.

Compare estimates using matched lengths, alphabets, log bases, window caps
and estimator versions. Report the selected cap and `h` alongside E; use
known-source controls or suitable surrogates to assess sampling effects.
The API supplies no calibrated confidence interval for E.

The additional `lz76()` fields `factors_stddev`, `normal_error`, and
`poison_error` are legacy formulas, not statistical uncertainty on E or
calibrated error bars on `h`. The `extras` fields are ratios of LZ factor
counts from the two halves of a sequence, not Shannon information measures
or statistical correlation coefficients.

## Why this differs from the initial PR

Released 2.0.0 constructed block-shuffled surrogates and estimated their
entropy rates with LZ76. The initial PR replaced that step with Chao–Shen
block entropies, subtracted `m * h_LZ`, and truncated the ladder at its peak.
That mixes two estimators with different finite-sample biases; peak selection
does not remove those biases or establish convergence to excess entropy.

The revision instead estimates a clearly defined finite-window mutual
information directly. Matching the joint and marginal observations supplies
the sign and monotonicity properties without a projection. This changes
both the numerical results and the meaning of `summands`; data from the
different estimators must be labelled and analysed separately.

Before a matching website deployment, the documentation branch needs its
method descriptions, API details, examples and performance measurements
updated together. Until then, use this source note and runtime help for this
branch, and the versioned website material for the corresponding release.
