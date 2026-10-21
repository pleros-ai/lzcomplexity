# Reading the numbers

`lz76()` returns a dictionary containing several kinds of result. The factor count, entropy-rate estimate, shuffle contrast and legacy diagnostics answer different questions. This page gives the actual field meanings and formulas.

```python
import lzcomplexity as lz

result = lz.lz76("00010111" * 256, log_base=2, max_block_size=8)
```

## The main results

| Python key | Meaning | How to read it |
|---|---|---|
| `complexity` | LZ76 count `C` | Integer count under this implementation's final-phrase convention |
| `factors` | Parser boundary positions | Byte offsets; the final position can exceed the input length |
| `h` | `C · log_b(N) / N` | Finite entropy-rate estimate, in the chosen log-base units per byte symbol |
| `emc` | Shuffle analysis dictionary | Operational EMC-like score, its fitted profile increments and raw first-scale contrast |
| `epsilon` | Finite-size formula | A legacy normalization diagnostic, not a measured uncertainty |
| `factors_stddev` | Factor-dispersion formula | A legacy formula whose normalization differs from ordinary sample standard deviation |
| `normal_error` | Formula using `h` and factor dispersion | Not a calibrated confidence interval |
| `poison_error` | `h / N` for `N > 1` | Historical spelling; not a calibrated Poisson error bar |
| `extras` | Five count-based ratios | Comparisons between the two halves of this one sequence |

The current key is `h`, not `entropy_density`. The [Python API](../api/python.md) lists signatures and return types; the [CLI](../cli/lzcomplexity.md) uses a different JSON naming scheme.

## Factor count and boundaries

For `banana`, the result is count `3` with boundaries `[0, 1, 2, 3, 7]`. The input has six bytes. The final attempted phrase goes beyond its end and is excluded from the completed count. Do not infer the count from the vector length alone or use the last boundary as the input length.

Empty and constant inputs take a special path with count one. An empty input's `[0, 1, 0]` vector is a legacy sentinel-like result, not an ordered partition. See [the full convention](../concepts/lz76.md).

## Entropy density

For `N > 1`, `h` is `C · log_b(N) / N`; otherwise it is zero. The base defaults to observed alphabet size, floored at two. It is an estimator, so finite-string values need not lie inside the limiting entropy bounds. [Entropy density](../concepts/entropy-density.md) explains the source assumptions and units.

## The `emc` dictionary

| Key | Definition |
|---|---|
| `value` | Final level of the fitted nonnegative, nondecreasing shuffle-contrast profile |
| `summands` | Successive fitted increments; nonnegative and summing to `value` |
| `max_block_size` | Number of included block-size scales, `M` |
| `multi_information` | Raw first-scale contrast `r_1`, before fitting |

The historical `emc` name denotes a magnitude qualitatively similar to EMC. `multi_information` is not calibrated Shannon mutual information. Neither the fitted score nor its increments are established measurements of an exact number of shared-information bits. Its [method page](../concepts/emc.md) gives the complete calculation.

## Legacy finite-size and error fields

These fields are useful for inspecting implementation output and reproducing earlier analyses. Their names do not establish statistical calibration.

### `epsilon`

For an ordinary input containing at least two distinct bytes, define `k` as the requested alphabet parameter (automatic when omitted), `b` as the selected logarithm base and `N` as byte length. Values of `k` and `b` are floored at two in the formula:

```text
epsilon = 2 · (1 + log_b(log_b(k · N))) / log_b(N)
```

The empty/constant short path reports zero. `epsilon` is calculated from length and parameters, not estimated from resampling the recording. The implementation does not subtract it from `h`, use it to correct the shuffle score, or return it as a confidence interval.

### `factors_stddev`

Let the boundary vector be `B = [b_0, …, b_(q-1)]`. Define successive nonnegative differences `d_i = max(b_i - b_(i-1), 0)` for `i = 1…q-1`. The complete analysis computes

```text
mu = (last_boundary - 1) / q
s  = sqrt(sum_i((d_i - mu)^2) / max_i(d_i))
```

If no positive difference exists, it returns zero. The denominator is the **largest interval**, and `mu` is derived as shown rather than as the ordinary mean of the intervals. Therefore this historical field should not be interpreted as the conventional population or sample standard deviation of the visible factors. An overshooting final boundary also enters the formula.

### `normal_error` and `poison_error`

For `N > 1`, writing `s = factors_stddev`:

```text
normal_error = h^(3/2) · s / sqrt(N / log_b(N))
poison_error = h / N
```

Both are zero for `N ≤ 1`. These arithmetic formulas are retained for compatibility. They do not provide a validated confidence level, establish normal/Poisson sampling of your data, or quantify uncertainty in the EMC-like score.

## The five `extras`

The full analysis splits the **single** input at `N // 2`. If `N` is odd, the second half has one extra byte. Let

```text
A = C(first half)
B = C(second half)
C = C(whole input)
Q = A + B - C
```

The returned fields are:

| Key | Implemented formula |
|---|---|
| `rajski_distance` | `2 - (A + B) / C` |
| `redundancy` | `Q / (A + B)` |
| `fh_uncertainty` | `Q / A` |
| `lh_uncertainty` | `Q / B` |
| `pearson_coefficient` | `Q / sqrt(A · B)` |

A zero whole/half count produces default zero extras in the Rust function. The public factorization short path normally gives count one for empty and constant data, so even these degenerate inputs can produce nonzero extras.

These are analogies formed by substituting factor counts into information-style ratios. Factor counts are not Shannon entropies; textbook entropy bounds and correlation interpretations need not hold. In particular, `pearson_coefficient` is **not** a sample Pearson correlation of numeric time series.

### `banana`, worked through

The split is `ban | ana`:

```text
A = C("ban") = 3
B = C("ana") = 2
C = C("banana") = 3
Q = 3 + 2 - 3 = 2
```

The extras are approximately `0.333333` (Rajski), `0.4` (redundancy), `0.666667` and `1.0` (the two uncertainty fields), and `0.816497` (the legacy coefficient).

Some low-complexity inputs leave the textbook ranges suggested by these names. For example:

```python
extra = lz.lz76("aaaabaaaabaaaaba")["extras"]
assert extra["rajski_distance"] == -1.0
assert extra["fh_uncertainty"] == 2.0
```

Do not clip or reinterpret such results as ordinary correlations. They are the output of the factor-count formulas above. Passing `X + Y` also does not compare the original datasets unless the midpoint happens to coincide with their join.

## Report the result you actually measured

A useful result record contains input encoding, byte length, preprocessing, explicit log base, actual shuffle cap and package/source revision. Report whether a plotted quantity is `h`, the EMC-like score, a fitted increment, `nid`, the CLI shuffle matrix, or an extra ratio.

For scientific uncertainty, use a study-specific sampling analysis and suitable controls; these legacy fields do not replace one. [Length and convergence](../concepts/convergence.md) explains practical sensitivity checks.
