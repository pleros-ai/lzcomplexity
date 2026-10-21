# Entropy density

Entropy density, also called entropy rate, asks: **after accounting for the source's temporal dependencies, how much new uncertainty remains per symbol?** The function `h()` estimates it from a finite sequence using the LZ76 factor count.

## A histogram is not an entropy rate

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

These are assumptions about a source, not properties established by calling the library. A file containing a mixture of operating regimes may not support a single stationary interpretation.

## The finite-sequence calculation

For an input of `N` bytes with LZ76 count `C`, the implementation returns

<div class="lz-formula">
<p><strong>ĥ = C · log<sub>b</sub>(N) / N</strong></p>
</div>

Here `b` is the logarithm base. The implementation returns zero for `N ≤ 1`, avoiding a logarithm/length singularity. The Lempel–Ziv asymptotic approach motivates this estimate for appropriate long source realizations; it does not supply a universal finite-sample accuracy guarantee. See [Ziv (1978)](https://doi.org/10.1109/TIT.1978.1055911) and the [reference list](../project/references.md).

```python
import lzcomplexity as lz

assert lz.factorization("01010101")[0] == 2
assert lz.h("01010101", log_base=2) == 0.75
assert lz.h("01010101", log_base=4) == 0.375
```

This eight-byte periodic string has zero entropy rate in an ideal deterministic infinite-source description, yet its finite LZ estimate is positive. The example illustrates the difference between an estimator applied to a short string and the source's limiting quantity.

## Units and default normalization

| Base | Interpretation of `ĥ` |
|---|---|
| `log_base=2` | Bits per input byte symbol |
| Observed alphabet size `k` | Alphabet-normalized entropy-rate estimate |
| Another explicit integer base `b ≥ 2` | Base-`b` information units per byte symbol |

Python defaults to the number of distinct input bytes, floored at two. An explicit Python `alphabet` argument does **not** change that default base; pass `log_base` to select units. A CLI-declared alphabet has a different legacy default, so set the base explicitly for cross-interface comparisons. [Alphabets and log bases](alphabets.md) gives examples.

For DNA encoded with four distinct bytes, an independent uniform source has theoretical rate `log₂(4) = 2` bits per byte symbol. Base-four normalization expresses that same rate as one. A byte symbol need not be a biological base or a Unicode character unless your encoding makes it so.

## Values above one

The estimator is not clipped to `[0, 1]`. With base two, an alphabet larger than two can have a true entropy rate above one bit per symbol. Even with binary input, finite-size behavior can produce an estimate above one:

```python
assert lz.h("1001110110101000", log_base=2) == 1.5
```

That value is an observed finite-string estimate, not a claim that a binary source emits more than one bit of Shannon entropy per symbol. Do not clip values silently: retain the measurement settings, examine length sensitivity, and state that it is an estimate.

## Bias, uncertainty and interpretation

There is no universal “enough symbols” threshold. Convergence depends on the source's dependencies, alphabet, symbolization and sample length. A small estimate can indicate regularity, a coarse discretization, or a recording that did not explore the source. It is not by itself proof of determinism.

The fields `epsilon`, `normal_error`, `poison_error` and `factors_stddev` are legacy diagnostics. They are not calibrated confidence intervals for `h`, and none provides uncertainty for the shuffle score. [Reading the numbers](../guide/reading-the-numbers.md) gives their actual formulas.

For a practical study, compare matched-length samples, examine several lengths, and use controls appropriate to your scientific question. [Sequence length and convergence](convergence.md) explains that workflow.
