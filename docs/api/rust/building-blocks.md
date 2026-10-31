# Rust building blocks

These public functions support custom analysis pipelines. The standard
[measurement functions](analysis.md) already combine the required stages.
All positions, lengths and state codes below refer to bytes.

## Suffix arrays { #suffix-arrays }

### `build_suffix_array` { #build_suffix_array }

```text
build_suffix_array(text: &[u8]) -> Vec<u32>
```

Return one starting position per input byte, ordered lexicographically by the
suffix beginning there. The output contains no extra sentinel suffix. An empty
input returns an empty vector. The function is available both at the crate root
and as `suffix_array::build_suffix_array`.

### `build_lcp` { #build_lcp }

```text
suffix_array::build_lcp(text: &[u8], sa: &[u32]) -> Vec<u32>
```

Build the longest-common-prefix array for `text` and its suffix array `sa`.
`lcp[0]` is zero; `lcp[i]` is the common-prefix length of the suffixes starting at
`sa[i-1]` and `sa[i]`. Empty `sa` returns an empty vector.

Supply the complete suffix array for these exact bytes. An arbitrary ordering,
invalid indices or a mismatched length does not satisfy the input contract and
can cause indexing panics or incorrect output.

### `build` { #build }

```text
suffix_array::build(text: &[u8]) -> SuffixArray
```

Return `SuffixArray { sa, lcp, n }`, constructing both arrays together and setting
`n` to the input length. The arrays use `u32` positions. The container's
[`new`, `default` and `clear`](types.md#suffixarray) methods support an empty
working value.

```rust
use lzcomplexity_core::{build_suffix_array, suffix_array};

let text = b"banana";
let sa = build_suffix_array(text);
assert_eq!(sa, vec![5, 3, 1, 0, 4, 2]);
let lcp = suffix_array::build_lcp(text, &sa);
assert_eq!(lcp, vec![0, 1, 3, 0, 0, 2]);
let arrays = suffix_array::build(text);
assert_eq!(arrays.sa, sa);
assert_eq!(arrays.lcp, lcp);
assert_eq!(arrays.n, 6);
```

### `lpf` { #lpf }

```text
lpf::lpf(output: &mut [u32], sa: Vec<u32>, lcp: Vec<u32>, n: usize)
```

Populate `output[i]` with the longest match beginning at position `i` that also
starts at an earlier position. Matches can overlap the position being parsed.
The suffix and LCP vectors are consumed; pass arrays built for the same text.
`output` must have at least `n` entries, and both input arrays must describe all
`n` suffixes. Use a zero-initialized output buffer.

For `n = 0`, nothing is written. For `n = 1`, `output[0]` is set to zero.
The function returns `()`; the result is written into the supplied buffer.

```rust
use lzcomplexity_core::{lpf, suffix_array};

let text = b"banana";
let arrays = suffix_array::build(text);
let mut previous = vec![0; text.len()];
lpf::lpf(&mut previous, arrays.sa, arrays.lcp, text.len());
assert_eq!(previous, vec![0, 0, 0, 3, 2, 1]);
```

## Low-level factorization

### `factorize` { #factorize }

```text
lz76::factorize(seq: &Sequence, args: &LzArgs) -> (u32, f64, Vec<u32>, f64)
```

Return `(count, epsilon, boundaries, factor_length_dispersion)` directly.
This builds the suffix/LCP and longest-previous-factor arrays and performs the
factorization walk. The final boundary may exceed the sequence length.

This entry point does not apply the high-level special treatment of constant
inputs. Empty input returns `(0, 0.0, [], 0.0)`, whereas
`lz76_factorization` reports count one for empty input. For ordinary analyses,
use [`lz76_factors`](analysis.md#lz76_factors) so the same counting conventions
are applied throughout your application.

## E range and intermediate counts

### `max_block_size` { #max_block_size }

```text
shuffle::max_block_size(size: usize) -> usize
```

Estimate the range `M` associated with `N = M * 2^M`, using the input byte length
`size` as `N`. Lengths below ten return one. This helper returns the base range
estimate; the explicit shuffle estimator adds ten when the input is longer
than 50 bytes. The default Chao–Shen estimator uses its alphabet-aware ceiling
and peak cutoff instead. A positive `LzArgs::block_size` overrides automatic selection.

### `shuffle_factorization` { #shuffle_factorization }

```text
shuffle::shuffle_factorization(seq: &Sequence, args: &LzArgs) -> (Vec<i32>, usize)
```

Return intermediate factor counts and the maximum range `M` used by the
explicit shuffle estimator. Chao–Shen does not call this helper.
The vector is one-indexed: positions `1..=M` hold the count at each range;
position zero is unused and two trailing entries are reserved. These values
are **factor counts**, despite the internal variable name `h_rand`.

The range is taken directly from a positive `args.block_size`; otherwise it is
selected automatically as described above. The result is deterministic for the
same bytes and range settings. The returned vector can be passed directly to
`shuffle_entropy_calculation`.

### `shuffle_entropy_calculation` { #shuffle_entropy_calculation }

```text
shuffle::shuffle_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    counts: &[i32],
    max_range: usize,
) -> EmcResult
```

Calculate E from an original-sequence factor count and the corresponding
one-indexed intermediate counts. `seq` supplies the input length and automatic
logarithm base. Supply values for these exact bytes and settings; the function
trusts the provided counts.

`max_range` becomes the result's `max_block_size`. When
`args.get_emc_terms` is true, `summands` has `max_range` entries. If the count
slice contains fewer ranges than requested, only available ranges are processed
and remaining increments stay zero. Inputs of length at most one, zero range or
no available counts return zero E. Standard measurements use the complete output
from `shuffle_factorization`:

```rust
use lzcomplexity_core::{lz76, shuffle, LzArgs, Sequence};

let seq = Sequence::from_bytes(b"00010111".repeat(32));
let mut args = LzArgs::default();
args.block_size = 4;
args.get_emc_terms = true;
let count = lz76::lz76_factorization(&seq, &args) as i32;
let (counts, max_range) = shuffle::shuffle_factorization(&seq, &args);
let e = shuffle::shuffle_entropy_calculation(&seq, &args, count, &counts, max_range);
let direct = shuffle::lz76_random_shuffle_complexity(&seq, &args);
assert_eq!(e.emc_value, direct.emc_value);
assert_eq!(e.summands, direct.summands);
```

## Sequence sampling functions { #sequence-sampling-functions }

These helpers are in the `sequence` module. They preserve sequence length and
byte frequencies while exchanging eligible equal-length aligned blocks. Any
suffix outside the eligible blocks remains in place. Zero block size, input
length at most ten, or insufficient eligible blocks leaves the sequence unchanged.
They preserve alphabet metadata because the observed bytes are only permuted.

### `shuffle_in_place` { #shuffle_in_place }

```text
sequence::shuffle_in_place(
    seq: &mut Sequence, block_size: u32, rng: &mut rand_chacha::ChaCha8Rng
)
```

Apply one eligible block exchange in place and return `()`. The caller owns the
random generator and its state; repeated calls continue that stream. Add
`rand_chacha = "=0.3.1"` and `rand = "=0.8.6"` to an application that constructs
the generator directly.

### `shuffle_copy` { #shuffle_copy }

```text
sequence::shuffle_copy(seq: &Sequence, block_size: u32, times: u32) -> Sequence
```

Clone the input and apply `times` exchanges using a deterministic seed derived
from its bytes and `block_size`. The original is unchanged. Zero `times` returns
an unchanged clone.

### `shuffle_copy_seeded` { #shuffle_copy_seeded }

```text
sequence::shuffle_copy_seeded(
    seq: &Sequence, block_size: u32, times: u32, seed: u64
) -> Sequence
```

Return the same kind of sampled copy, using the caller's explicit seed.
Identical input bytes, block size, repetition count and seed produce the same
output. This seed applies to this helper; high-level E selects its own
content-dependent sampling settings.

```rust
use lzcomplexity_core::{sequence, Sequence};

let seq = Sequence::from_str("000101110001011100010111");
let a = sequence::shuffle_copy_seeded(&seq, 2, 12, 42);
let b = sequence::shuffle_copy_seeded(&seq, 2, 12, 42);
assert_eq!(a, b);
assert_eq!(a.len(), seq.len());
assert_eq!(a.char_density(), seq.char_density());
```

## Paired-state encoding

### `merge_sequences` { #merge_sequences }

```text
shuffle::merge_sequences(first: &Sequence, second: &Sequence) -> Sequence
```

Create a sequence of corresponding byte pairs, assigning each observed ordered
pair a byte label in first-seen order. Its length is the shorter input length;
unpaired trailing bytes are omitted. Pair `(a, b)` and pair `(b, a)` are distinct
states when their bytes differ.

The constructor receives the product of the input alphabet sizes as a hint;
normal alphabet-hint rules determine the returned metadata. This helper uses
wrapping byte labels and zero as an internal unassigned marker. For joint
alphabets large enough to reach or wrap that marker, distinct pairs are not
guaranteed distinct labels. Use an explicit joint-state encoding for such data.
Binary or other small joint alphabets fit directly.

```rust
use lzcomplexity_core::{shuffle, Sequence};

let first = Sequence::from_str("0011");
let second = Sequence::from_str("0101");
let paired = shuffle::merge_sequences(&first, &second);
assert_eq!(paired.as_bytes(), b"0123");
assert_eq!(paired.alphabet_size(), 4);
```
