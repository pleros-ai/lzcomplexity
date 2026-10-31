# Rust configuration and result types

These types are re-exported from `lzcomplexity_core` and their fields are public.
They can be inspected directly or stored alongside your analysis records.

## `LzArgs` { #lzargs }

`LzArgs::new()` and `LzArgs::default()` construct the same configuration:

```rust
use lzcomplexity_core::{EmcMethod, LzArgs, NO_ALPHABET};

let args = LzArgs::default();
assert_eq!(args.chunks, 0);
assert_eq!(args.max_context, 0);
assert_eq!(args.block_size, -1);
assert_eq!(args.emc_method, EmcMethod::ChaoShen);
assert!(!args.get_emc_terms);
assert_eq!(args.alphabet, NO_ALPHABET);
assert_eq!(args.log_base, NO_ALPHABET);
```

| Field | Type | Default | Meaning |
|---|---|---|---|
| `chunks` | `i32` | `0` | Accepted configuration field; does not affect the current calculations. |
| `max_context` | `i32` | `0` | Accepted configuration field; does not limit the current factorization. |
| `emc_method` | `EmcMethod` | `ChaoShen` | Select `ChaoShen` or the earlier `Shuffle` estimator. |
| `block_size` | `i32` | `-1` | Maximum range for E; positive values are explicit, nonpositive values select automatically. |
| `get_emc_terms` | `bool` | `false` | Include the E profile increments in `EmcResult::summands`. |
| `alphabet` | `u32` | `NO_ALPHABET` | Alphabet parameter for the `epsilon` diagnostic; automatic uses sequence metadata. |
| `log_base` | `u32` | `NO_ALPHABET` | Base for entropy density and E; automatic uses sequence metadata. |

The normalization base and alphabet parameter are independent. Setting
`alphabet = 4` does not select base-four units; set `log_base = 4` for that.
Explicit normalization bases below two are floored to two.

The type implements `Clone`, `Debug`, `Default` and `PartialEq`. Its equality
comparison includes all fields **except `log_base`**. Compare `log_base`
separately when checking that two results use the same units or building a cache
key from settings.

### Constants

`NO_ALPHABET: u32` equals `u32::MAX` and selects automatic alphabet metadata or
logarithm-base selection in `LzArgs`. `ALPHABET_SIZE: u32` equals two and is the
automatic normalization floor. Neither constant restricts input byte values.

## Result structures { #result-structures }

### `Lz76Result` { #lz76result }

Returned by [`lz76_factors`](analysis.md#lz76_factors). It implements `Clone`,
`Debug` and `Default`.

| Field | Type | Meaning |
|---|---|---|
| `factorization` | `u32` | LZ76 factor count `C`. |
| `epsilon` | `f64` | Finite-size diagnostic using the selected alphabet and logarithm base. |
| `lzf` | `Vec<u32>` | Factor boundary byte positions. |

`Lz76Result::default()` has zero scalar values and an empty vector. A default
value is an uncomputed container, unlike the result of analyzing an empty
sequence, which has count one and boundaries `[0, 1, 0]`.

For an ordinary sequence of length `N > 1`, the diagnostic is
`epsilon = 2 * (1 + log_b(log_b(A * N))) / log_b(N)`, where `A` is the
selected alphabet parameter and `b` is the logarithm base, each floored at two.
The high-level function returns zero `epsilon` for empty and constant inputs.

### `EmcResult` { #emcresult }

Returned by the [E functions](analysis.md#effective-measure-complexity-e).
It implements `Clone`, `Debug` and `Default`.

| Field | Type | Meaning |
|---|---|---|
| `max_block_size` | `i32` | Maximum range actually used. |
| `emc_value` | `f64` | Effective measure complexity E. |
| `multi_information` | `f64` | First-range contrast before fitting; it may be negative. |
| `summands` | `Vec<f64>` | Nonnegative increments of the fitted profile, included when requested. |

With `get_emc_terms = true`, the increments sum to `emc_value`. Without it,
the vector is empty and E is still calculated.

```text
EmcResult::new(max_block_size: i32, emc_value: f64, multi_information: f64) -> EmcResult
```

This constructor sets the three scalar fields and leaves `summands` empty.
`EmcResult::default()` is `new(-1, 0.0, 0.0)`. Thus a negative range in a result
means the container has not been computed; a negative range in `LzArgs` requests
automatic range selection.

### `LempelZiv` { #lempelziv }

Returned by [`metrics::lz76`](analysis.md#lz76). It implements `Clone`, `Debug`
and `Default`.

| Field | Type | Meaning |
|---|---|---|
| `complexity` | `u32` | Full-sequence LZ76 count. |
| `entropy_density` | `f64` | h in the selected logarithm units per byte. |
| `emc` | `EmcResult` | E and the requested range profile. |
| `paired_shuffle_complexity` | `EmcResult` | Optional paired-state E result; the full driver leaves it at its default. |
| `lz_normal_errors` | `f64` | Error diagnostic formed from h, length and factor-length dispersion. |
| `lz_poison_errors` | `f64` | Error diagnostic `h / N` for `N > 1`, otherwise zero. The field spelling is part of the API. |
| `epsilon` | `f64` | Finite-size diagnostic from factorization. |
| `factors_stddev` | `f64` | Factor-length dispersion calculated from the returned boundaries. |
| `factors` | `Vec<u32>` | Boundary byte positions, including the attempted terminal boundary. |
| `extras` | `LzExtra` | Ratios of whole- and half-sequence counts. |

`LempelZiv::default()` contains zero scalar values, empty boundaries and default
nested results. It does not analyze an empty sequence.

The diagnostic fields describe this factorization; they are not confidence
intervals from repeated observations. For comparisons across windows or subjects,
keep the encoding, lengths, logarithm base and E range selection consistent.

For callers using the diagnostic values directly, the calculation uses the
boundary list `q`, `mean = (q.last() - 1) / q.len()`, the nonnegative adjacent
lengths `d`, and `s = sqrt(sum((d - mean)^2) / max(d))`. `factors_stddev` is `s`
(or zero when `max(d)` is zero). For `N > 1`, `lz_normal_errors` is
`sqrt(h^3) * s / sqrt(N / log_b(N))`; otherwise it is zero.

### `LzExtra` { #lzextra }

Returned by [`lz76_extras`](analysis.md#lz76_extras) and included in `LempelZiv`.
All five fields are `f64`. It implements `Clone`, `Debug` and `Default`, with
all-zero default fields.

| Field | Meaning |
|---|---|
| `lz_rajski_distance` | Normalized whole-versus-halves count contrast. |
| `redundancy` | Shared count contribution divided by the sum of half counts. |
| `fh_uncertainty` | Shared contribution divided by the first-half count. |
| `lh_uncertainty` | Shared contribution divided by the second-half count. |
| `lz_pearson_coefficient` | Shared contribution divided by the geometric mean of the half counts. |

The [Rust overview](../rust.md#extra-measures) gives their exact formulas. These
are count-based diagnostics; the coefficient is not the sample correlation of
the observations.

## `SuffixArray`

This container is re-exported at the crate root. It implements `Clone`, `Debug`,
`Default`, `PartialEq` and `Eq`.

| Field | Type | Meaning |
|---|---|---|
| `sa` | `Vec<u32>` | Suffix starting positions in lexicographic order. |
| `lcp` | `Vec<u32>` | Common-prefix lengths of adjacent ordered suffixes, starting with zero. |
| `n` | `u32` | Input byte length. |

`SuffixArray::new()` and `SuffixArray::default()` return empty vectors and length
zero. `clear(&mut self)` clears both vectors and resets `n` to zero. Use
[`suffix_array::build`](building-blocks.md#build) to construct populated arrays
for a sequence.
