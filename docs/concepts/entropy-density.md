# Entropy density

Entropy density, also called entropy rate, asks: **after accounting for the source's temporal dependencies, how much new uncertainty remains per symbol?** The function `h()` estimates it from a finite sequence using the LZ76 factor count.

## Why order matters

A sequence alternating `010101…` and independent fair coin tosses can both contain about half zeros and half ones. Their single-symbol Shannon entropy is therefore about one bit. But once the alternating pattern and its phase are known, its next symbol is predictable; a fresh independent coin toss remains uncertain.

The histogram ignores order. Entropy rate incorporates dependencies across time. LZ parsing detects repeated substrings and uses their frequency of introduction as a route to an entropy-rate estimate.

## The source quantity

For a stationary symbolic process, let `p(w)` be the probability of a particular block `w` of `L` consecutive symbols. Its Shannon block entropy is

```text
H(L) = -sum_w p(w) · log₂(p(w))
```

The sum runs over possible length-`L` blocks, using `0 · log₂(0) = 0`. Its units are bits. The entropy rate is

```text
h = lim_{L→∞} H(L) / L
```

Under the usual finite-alphabet stationary conditions it is also the limit of `H(L) - H(L-1)`. Stationarity means that the probability law does not change when the observation time is shifted. Ergodicity is the additional connection that permits one sufficiently long typical realization to represent the source's statistical behavior.

For a recording that moves between operating regimes, analyze comparable windows separately before assigning a single entropy rate to the whole source.

## The finite-sequence calculation

For an input of `N` bytes with LZ76 count `C`, the implementation returns

<div class="lz-formula">
<p><strong>ĥ = C · log<sub>b</sub>(N) / N</strong></p>
</div>

Here `b` is the logarithm base. The result is zero for `N ≤ 1`. For stationary ergodic sources with a finite alphabet, normalized LZ parsing connects the observed sequence to the source's entropy rate as length increases. See [Ziv (1978)](https://doi.org/10.1109/TIT.1978.1055911) and the [reference list](../project/references.md).

```python
import lzcomplexity as lz

assert lz.factorization("01010101")[0] == 2
assert lz.h("01010101", log_base=2) == 0.75
assert lz.h("01010101", log_base=4) == 0.375
```

The indefinitely repeated `01` pattern has zero entropy rate. The positive estimate for this eight-byte sample illustrates the effect of observing only a short part of that pattern.

## Units and default normalization

| Base | Interpretation of `ĥ` |
|---|---|
| `log_base=2` | Bits per input byte symbol |
| Observed alphabet size `k` | Alphabet-normalized entropy-rate estimate |
| Another explicit integer base `b ≥ 2` | Base-`b` information units per byte symbol |

Python defaults to the number of distinct input bytes, floored at two. An explicit Python `alphabet` argument does **not** change that default base; pass `log_base` to select units. The CLI uses a declared alphabet as its default base, so set the base explicitly for cross-interface comparisons. [Alphabets and log bases](alphabets.md) gives examples.

For DNA encoded with four distinct bytes, an independent uniform source has theoretical rate `log₂(4) = 2` bits per byte symbol. Base-four normalization expresses that same rate as one. A byte symbol need not be a biological base or a Unicode character unless your encoding makes it so.

## Values above one

The estimator is not clipped to `[0, 1]`. With base two, an alphabet larger than two can have a true entropy rate above one bit per symbol. Even with binary input, finite-size behavior can produce an estimate above one:

```python
assert lz.h("1001110110101000", log_base=2) == 1.5
```

This is finite-sample estimation behavior. Keep the reported value and examine how it changes with sequence length; clipping it to one would hide that behavior.

## Bias, uncertainty and interpretation

The required sample length depends on the source's dependencies, alphabet and symbolization. A small estimate can reflect regularity, coarse discretization, or a recording that explored only a narrow part of the source's behavior.

The additional fields `epsilon`, `normal_error`, `poison_error` and `factors_stddev` are algebraic diagnostics. [Reading the numbers](../guide/reading-the-numbers.md) explains their formulas. Estimate sampling uncertainty separately, using observations or resampling methods appropriate to the data.

For a practical study, compare matched-length samples, examine several lengths, and use controls appropriate to your scientific question. [Sequence length and convergence](convergence.md) explains that workflow.
