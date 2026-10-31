# Reading the numbers

`lz76()` returns the factor count, entropy density h, effective measure complexity E and additional diagnostics. This page explains the returned fields and their formulas.

```python
import lzcomplexity as lz

result = lz.lz76("00010111" * 256, log_base=2, max_block_size=8)
```

## The main results

| Python key | Meaning | How to read it |
|---|---|---|
| `complexity` | LZ76 count `C` | Integer count under this implementation's final-phrase convention |
| `factors` | Parser boundary positions | Byte offsets; the final position can exceed the input length |
| `h` | $C\log_b N/N$ | Finite entropy-rate estimate, in the chosen log-base units per byte symbol |
| `emc` | Effective measure complexity E | E, its fitted profile increments and first-range contrast |
| `epsilon` | Finite-size diagnostic | Calculated from sequence length, alphabet and logarithm base |
| `factors_stddev` | Factor-dispersion diagnostic | The interval-based formula defined below |
| `normal_error` | Formula using h and factor dispersion | A descriptive error field; see its definition below |
| `poison_error` | `h / N` for `N > 1` | A length-dependent diagnostic |
| `extras` | Five count-based ratios | Comparisons between the two halves of this one sequence |

The [Python API](../api/python.md) lists signatures and return types. The [CLI reference](../cli/lzcomplexity.md) documents JSON field names.

## Factor count and boundaries

For `banana`, the result is count `3` with boundaries `[0, 1, 2, 3, 7]`. The input has six bytes. The final attempted phrase goes beyond its end and is excluded from the completed count. Do not infer the count from the vector length alone or use the last boundary as the input length.

Empty and constant inputs return count one. Empty input returns the boundary vector `[0, 1, 0]`; handle it separately when displaying pieces. See [LZ76 factorization](../concepts/lz76.md).

## Entropy density

For input length $N>1$ and factor count $C$, entropy density is

$$
\hat{h} = \frac{C\log_b N}{N}.
$$

For shorter inputs it is zero. The base $b$ defaults to observed alphabet size, floored at two. Finite-string estimates can fall outside the limiting entropy bounds. [Entropy density](../concepts/entropy-density.md) explains the units and interpretation.

## Effective measure complexity E { #the-emc-dictionary }

| Key | Definition |
|---|---|
| `method` | `"chao-shen"` (default) or `"shuffle"` |
| `value` | E: the final level of the fitted nonnegative, nondecreasing range profile |
| `summands` | Successive fitted increments; nonnegative and summing to `value` |
| `max_block_size` | Maximum range included in the calculation, `M` |
| `multi_information` | First-range contrast, before fitting |

These fields are returned under `result["emc"]`. Read [effective measure complexity E](../concepts/emc.md) for the meaning of the range profile and its increments.

## Finite-size and error diagnostics { #legacy-finite-size-and-error-fields }

The following fields describe properties of the finite sequence and its factorization. To estimate sampling uncertainty, use repeated observations or a resampling procedure suited to your data.

### `epsilon`

For an ordinary input containing at least two distinct bytes, define $k$ as the requested alphabet parameter (automatic when omitted), $b$ as the selected logarithm base and $N$ as byte length. Values of $k$ and $b$ are floored at two in the formula:

$$
\epsilon = \frac{2\left(1+\log_b\left(\log_b(kN)\right)\right)}{\log_b N}
$$

`epsilon` is reported separately from h and E. Empty and constant inputs return zero.

### `factors_stddev`

Let the boundary vector be $B=[b_0,\ldots,b_{q-1}]$. Define successive nonnegative differences $d_i=\max(b_i-b_{i-1},0)$ for $i=1,\ldots,q-1$. The complete analysis computes

$$
\mu = \frac{b_{q-1}-1}{q}, \qquad
s = \sqrt{\frac{\sum_{i=1}^{q-1}(d_i-\mu)^2}{\max_i d_i}}
$$

The denominator is the **largest interval**. This normalization differs from a conventional sample standard deviation. An overshooting final boundary enters the formula; if all intervals are zero, the result is zero.

### `normal_error` and `poison_error`

For $N>1$, writing $s$ for `factors_stddev`:

$$
\mathrm{normal\_error} = \frac{h^{3/2}s}{\sqrt{N/\log_b N}}, \qquad
\mathrm{poison\_error} = \frac{h}{N}
$$

Both fields are zero for `N ≤ 1`. They are descriptive formulas, so use a separate sampling analysis when you need confidence intervals.

## The five `extras`

The full analysis splits the **single** input at `N // 2`. If $N$ is odd, the second half has one extra byte. Let $A$ and $B$ be the factor counts of the first and second halves, $C$ the whole-input count, and

$$
Q = A+B-C.
$$

The returned fields are:

| Key | Implemented formula |
|---|---|
| `rajski_distance` | $2-(A+B)/C$ |
| `redundancy` | $Q/(A+B)$ |
| `fh_uncertainty` | $Q/A$ |
| `lh_uncertainty` | $Q/B$ |
| `pearson_coefficient` | $Q/\sqrt{AB}$ |

A zero whole/half count produces default zero extras in the Rust function. The public factorization short path normally gives count one for empty and constant data, so even these degenerate inputs can produce nonzero extras.

These ratios compare the factor counts of the two halves with the whole. `pearson_coefficient` uses the count-based formula above rather than sample covariance between numeric observations.

### `banana`, worked through

The split is `ban | ana`:

```text
A = C("ban") = 3
B = C("ana") = 2
C = C("banana") = 3
Q = 3 + 2 - 3 = 2
```

The extras are approximately `0.333333` (Rajski), `0.4` (redundancy), `0.666667` and `1.0` (the two uncertainty fields), and `0.816497` (the count-based coefficient).

The ratios can extend beyond zero to one. For example:

```python
extra = lz.lz76("aaaabaaaabaaaaba")["extras"]
assert extra["rajski_distance"] == -1.0
assert extra["fh_uncertainty"] == 2.0
```

Keep the values returned by these formulas when plotting or comparing results. To compare two separate sequences, use `nid(X, Y)`; the midpoint split above is intended for one sequence.

## Record your analysis settings { #report-the-result-you-actually-measured }

Keep the input encoding, byte length, preprocessing, logarithm base, maximum range and package version with your results. Label plots with the quantity shown: h, E, a profile increment, NID or an additional diagnostic. [Length and convergence](../concepts/convergence.md) explains how to check sensitivity to sample size.
