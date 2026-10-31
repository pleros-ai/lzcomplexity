# Rust measurement functions

These functions borrow sequences and settings. Inputs remain unchanged and all
positions are byte positions. The [sequence reference](sequence.md) covers input
construction; [configuration and results](types.md) defines the public types.

## Factorization and entropy

### `lz76_factorization` { #lz76_factorization }

```text
lz76::lz76_factorization(seq: &Sequence, args: &LzArgs) -> u32
```

Return the LZ76 factor count `C` for `seq`. Use this entry point when you need only
the count. It reports one for empty or constant sequences. For other inputs,
a factor that would extend beyond the input is excluded from the count.
`args.alphabet` and `args.log_base` do not change the factor boundaries or count.

```rust
use lzcomplexity_core::{lz76, LzArgs, Sequence};

let args = LzArgs::default();
assert_eq!(lz76::lz76_factorization(&Sequence::from_str("1000110011101"), &args), 5);
assert_eq!(lz76::lz76_factorization(&Sequence::from_str("aaaa"), &args), 1);
```

### `lz76_factors` { #lz76_factors }

```text
lz76::lz76_factors(seq: &Sequence, args: &LzArgs) -> Lz76Result
```

Return the count, the `epsilon` diagnostic and the boundary vector `lzf`.
Adjacent positions in `lzf` delimit factors using half-open byte intervals.
For `1000110011101`, the intervals give `1·0·001·1001·1101`.

A final repeated tail may have an attempted boundary beyond the input length.
For `banana`, `lzf` is `[0, 1, 2, 3, 7]` and the reported count is three.
Clamp interval endpoints when displaying the tail:

```rust
use lzcomplexity_core::{lz76, LzArgs, Sequence};

let seq = Sequence::from_str("banana");
let result = lz76::lz76_factors(&seq, &LzArgs::default());
let pieces: Vec<&[u8]> = result.lzf.windows(2)
    .map(|pair| &seq.as_bytes()[pair[0] as usize..(pair[1] as usize).min(seq.len())])
    .collect();
assert_eq!(result.factorization, 3);
assert_eq!(pieces, vec![&b"b"[..], &b"a"[..], &b"n"[..], &b"ana"[..]]);
```

Empty and constant inputs use a special result: count one, `epsilon = 0`,
and boundaries `[0, 1, N]`. For an empty sequence that is `[0, 1, 0]`;
handle it before slicing intervals.

### `lz76_entropy_density` { #lz76_entropy_density }

```text
lz76::lz76_entropy_density(seq: &Sequence, args: &LzArgs) -> f64
```

Return entropy density h from the factor count and byte length:
`C * log_b(N) / N`. The result is zero for `N <= 1`.
`args.log_base` selects `b`; automatic selection uses the sequence alphabet,
with a minimum of two. Base two gives bits per input byte. A finite-sequence
estimate is returned as calculated, so it can exceed one even for binary input.

```rust
use lzcomplexity_core::{lz76, LzArgs, Sequence};

let mut args = LzArgs::default();
args.log_base = 2;
assert_eq!(lz76::lz76_entropy_density(&Sequence::from_str("01010101"), &args), 0.75);
```

### `entropy_density_from` { #entropy_density_from }

```text
lz76::entropy_density_from(count: u32, seq: &Sequence, args: &LzArgs) -> f64
```

Apply the same entropy normalization to a count you already computed. This avoids
another factorization. Supply the count for these exact bytes; the function does
not check it. `seq` provides the byte length and automatic logarithm base.

```rust
use lzcomplexity_core::{lz76, LzArgs, Sequence};

let seq = Sequence::from_str("01010101");
let args = LzArgs::default();
let count = lz76::lz76_factorization(&seq, &args);
assert_eq!(lz76::entropy_density_from(count, &seq, &args), 0.75);
```

## Effective Measure Complexity E

### `emc` { #emc }

```rust
emc::emc(seq: &Sequence, args: &LzArgs) -> EmcResult
emc::emc_with(seq: &Sequence, args: &LzArgs, complexity: i32) -> EmcResult
```

Return Effective Measure Complexity E. `emc_with` reuses an existing original
LZ76 factor count. The default `args.emc_method = EmcMethod::ChaoShen` uses
coverage-adjusted block entropies from overlapping windows. Set
`EmcMethod::Shuffle` to reproduce the earlier estimator. Both return
nonnegative fitted increments when `args.get_emc_terms` is true.

Automatic Chao–Shen selection uses an alphabet-aware ceiling and stops at the
earliest raw ladder maximum. A positive `block_size` keeps the entire requested
ladder. See the [calculation](../../concepts/implementation.md#6-structural-complexity-e).

```rust
use lzcomplexity_core::{emc, EmcMethod, LzArgs, Sequence};
let seq = Sequence::from_str(&"00010111".repeat(256));
let mut args = LzArgs::new();
args.block_size = 8;
args.get_emc_terms = true;
let default = emc::emc(&seq, &args);
args.emc_method = EmcMethod::Shuffle;
let legacy = emc::emc(&seq, &args);
assert_eq!(default.summands.len(), 8);
assert_eq!(legacy.summands.len(), 8);
```

### `lz76_random_shuffle_complexity` { #lz76_random_shuffle_complexity }

```text
shuffle::lz76_random_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> EmcResult
```

Return the explicit random-shuffle estimate and its range limit. This helper
always uses shuffle, independently of `args.emc_method`. Use `emc::emc` for
the selected estimator.
E is sensitive to correlations across different ranges of the sequence.
`args.block_size > 0` sets the maximum range; a nonpositive value selects it
automatically. `args.log_base` controls units as for h.

Set `args.get_emc_terms = true` to populate `summands` with the nonnegative
increments of the fitted range profile. These increments sum to `emc_value`.
Otherwise, `summands` is empty while E is still computed. Identical bytes and
settings give the same result.

```rust
use lzcomplexity_core::{shuffle, LzArgs, Sequence};

let seq = Sequence::from_bytes(b"00010111".repeat(256));
let mut args = LzArgs::default();
args.log_base = 2;
args.block_size = 8;
args.get_emc_terms = true;
let result = shuffle::lz76_random_shuffle_complexity(&seq, &args);
assert_eq!(result.max_block_size, 8);
assert_eq!(result.summands.len(), 8);
assert!((result.summands.iter().sum::<f64>() - result.emc_value).abs() < 1e-12);
assert!((result.emc_value - 3.2105712890625).abs() < 1e-12);
```

The `multi_information` field is the first-range contrast before fitting and may
be negative. The returned E is nonnegative. Constant sequences and sequences of
at most ten bytes return zero E. See [E](../../concepts/emc.md) for interpretation
and choosing comparable ranges.

### `lz76_random_shuffle_complexity_with` { #lz76_random_shuffle_complexity_with }

```text
shuffle::lz76_random_shuffle_complexity_with(
    seq: &Sequence, args: &LzArgs, complexity: i32
) -> EmcResult
```

Calculate the same E result using an existing original-sequence factor count.
`complexity` is the count for `seq`, not an entropy value. It is trusted without
validation. This is useful after calling `lz76_factors`; convert its `u32` count
to `i32` only when the value fits.

### `lz76_paired_shuffle_complexity` { #lz76_paired_shuffle_complexity }

```text
shuffle::lz76_paired_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> EmcResult
```

Split `seq` at `N / 2`, pair corresponding bytes of the two halves into a derived
sequence, and return E for that paired-state sequence. For odd lengths the
unpaired final byte is omitted. Settings and result fields have the same meaning
as the standard E function. This optional calculation is called explicitly;
`metrics::lz76` does not populate it automatically. The
[paired-state encoding contract](building-blocks.md#merge_sequences) applies.

## Information distance and pairwise measures

### `lz76_information_distance` { #lz76_information_distance }

```text
metrics::lz76_information_distance(
    first: &Sequence, second: &Sequence, args: &LzArgs
) -> f64
```

Return the library's LZ-based information distance, derived from Normalized
Information Distance (NID). Lower values indicate more shared pattern structure.
The function uses the original and both concatenated sequence counts. The result
is symmetric, uses no symbol separator and accepts unequal input lengths.
Use the same byte codebook for both inputs.

`alphabet`, `log_base` and E range settings do not affect the count ratio.
The finite-sequence result is returned without clipping; self-distance can be
nonzero. See [information distance](../../concepts/nid.md) for interpretation.

```rust
use lzcomplexity_core::{metrics, LzArgs, Sequence};

let a = Sequence::from_str("abcd");
let b = Sequence::from_str("abce");
let args = LzArgs::default();
assert_eq!(metrics::lz76_information_distance(&a, &b, &args), 0.25);
assert_eq!(metrics::lz76_information_distance(&b, &a, &args), 0.25);
```

### `mutual_information` { #mutual_information }

```text
metrics::mutual_information(first: &Sequence, second: &Sequence, args: &LzArgs) -> f64
```

Return the additional normalized count-based pairwise score for the concatenated
sequence `first + second`. Let `C` be its factor count, `M` the selected range
limit, and `S` the sum of intermediate counts returned by
`shuffle_factorization`: the result is `1 - M*C/S`, or zero when `S` is zero.
Input order is preserved, so this helper is not guaranteed to be symmetric.
It is distinct from the standard information-distance entry point above.
The returned score is not clipped to a fixed interval.

### `lz76_random_shuffle_distance` { #lz76_random_shuffle_distance }

```text
metrics::lz76_random_shuffle_distance(
    first: &Sequence, second: &Sequence, args: &LzArgs
) -> f64
```

Return `1 - mutual_information(first, second, args)`. It inherits the same input
ordering and range settings; values are not clipped.

### `mutual_information_z` { #mutual_information_z }

```text
metrics::mutual_information_z(first: &Sequence, second: &Sequence, args: &LzArgs) -> f64
```

Apply the `mutual_information` calculation to the paired-state sequence from
`merge_sequences(first, second)` instead of concatenation. The shorter input
sets its length. Pairing requires aligned observations and a shared definition
of the two state alphabets. See [paired-state encoding](building-blocks.md#merge_sequences).

### `lz76_random_shuffle_distance_z` { #lz76_random_shuffle_distance_z }

```text
metrics::lz76_random_shuffle_distance_z(
    first: &Sequence, second: &Sequence, args: &LzArgs
) -> f64
```

Return `1 - mutual_information_z(first, second, args)` using the same paired-state
encoding and settings.

## Full analysis and extras

### `lz76` { #lz76 }

```text
metrics::lz76(seq: &Sequence, args: &LzArgs) -> LempelZiv
```

Return the full analysis: factor count and boundaries, h, E, finite-size
diagnostics and the five extra count ratios. The original factorization is
reused across the calculations. Use it when you need several of these results
for the same sequence.

`emc` contains E. `paired_shuffle_complexity` remains at
its default value; call the paired function separately if needed. Profile
increments are included only when `args.get_emc_terms` is true.
[Result types](types.md#lempelziv) defines every field.

```rust
use lzcomplexity_core::{metrics, LzArgs, Sequence};

let seq = Sequence::from_str("1000110011101");
let result = metrics::lz76(&seq, &LzArgs::default());
assert_eq!(result.complexity, 5);
assert_eq!(result.factors, vec![0, 1, 2, 5, 9, 13]);
assert!((result.entropy_density - 1.4232460454388816).abs() < 1e-12);
```

### `lz76_extras` { #lz76_extras }

```text
metrics::lz76_extras(seq: &Sequence, args: &LzArgs) -> LzExtra
```

Calculate five ratios from the full-sequence count `C`, the first-half count `F`
and second-half count `L`. The split is at `N / 2`; an odd-length second half has
one additional byte. Results are `lz_rajski_distance`, `redundancy`,
`fh_uncertainty`, `lh_uncertainty` and `lz_pearson_coefficient`.
Their [formulas](../rust.md#extra-measures) and [field descriptions](types.md#lzextra)
are part of the reference. All fields are zero if a required count is zero.

### `lz76_extras_with` { #lz76_extras_with }

```text
metrics::lz76_extras_with(seq: &Sequence, args: &LzArgs, count: f64) -> LzExtra
```

Reuse an existing full-sequence factor count while calculating the two half
counts. Supply the count for exactly `seq`; it is accepted as `f64` and is not
validated. Passing zero returns the default all-zero `LzExtra`.
