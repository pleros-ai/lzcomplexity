# Rust crate

`lzcomplexity-core` exposes the same algorithmic core used by Python and the
two command-line tools. Inputs are owned byte sequences; results are ordinary
Rust structs. The crate requires Rust 1.85 or newer and a C compiler for its
bundled suffix-array dependency.

```toml
[dependencies]
lzcomplexity-core = "2.0"
```

## Start with the high-level functions

```rust
use lzcomplexity_core::{lz76, metrics, shuffle, LzArgs, Sequence};

fn main() {
    let seq = Sequence::from_str("banana");
    let mut args = LzArgs::new();
    args.log_base = 2;
    args.block_size = 8;
    args.get_shuffle_terms = true;

    let factors = lz76::lz76_factors(&seq, &args);
    assert_eq!(factors.factorization, 3);
    assert_eq!(factors.lzf, vec![0, 1, 2, 3, 7]);

    let h = lz76::lz76_entropy_density(&seq, &args);
    let score = shuffle::lz76_random_shuffle_complexity(&seq, &args);
    let full = metrics::lz76(&seq, &args);
    assert_eq!(h, full.entropy_density);
    assert_eq!(score.emc_value, full.random_shuffle_complexity.emc_value);
}
```

| Function | Return | Use |
|---|---|---|
| `lz76::lz76_factorization(seq, args)` | `u32` | Factor count only. |
| `lz76::lz76_factors(seq, args)` | `Lz76Result` | Count, `epsilon` and boundaries. |
| `lz76::lz76_entropy_density(seq, args)` | `f64` | Count-normalized entropy estimate. |
| `lz76::entropy_density_from(count, seq, args)` | `f64` | Reuse a count already computed for exactly these bytes. |
| `shuffle::lz76_random_shuffle_complexity(seq, args)` | `LzShuffle` | EMC-like shuffle score. |
| `shuffle::lz76_random_shuffle_complexity_with(seq, args, count)` | `LzShuffle` | Same score, reusing the original count (`i32`). |
| `metrics::lz76_information_distance(first, second, args)` | `f64` | LZ-based pairwise distance. |
| `metrics::lz76(seq, args)` | `LempelZiv` | Complete analysis; original factorization reused. |
| `metrics::lz76_extras(seq, args)` | `LzExtra` | Half-sequence count ratios. |
| `metrics::lz76_extras_with(seq, args, count)` | `LzExtra` | Same ratios, reusing an original `f64` count. |

The `seq` arguments above are `&Sequence`; configuration is `&LzArgs`.
Reused-count functions trust the supplied value. They do not verify that it
belongs to the sequence or configuration.

## `LzArgs`

```rust
use lzcomplexity_core::{LzArgs, NO_ALPHABET};

let args = LzArgs::default();
assert_eq!(args.alphabet, NO_ALPHABET);
assert_eq!(args.log_base, NO_ALPHABET);
assert_eq!(args.block_size, -1);
assert!(!args.get_shuffle_terms);
```

| Field | Type | Default | Effect |
|---|---|---|---|
| `chunks` | `i32` | `0` | Compatibility field; not used by the current factorization. |
| `max_context` | `i32` | `0` | Compatibility field; not used by the current factorization. |
| `block_size` | `i32` | `-1` | Positive shuffle cap; nonpositive selects the automatic cap. |
| `get_shuffle_terms` | `bool` | `false` | Populate shuffle `summands`. |
| `alphabet` | `u32` | `NO_ALPHABET` | Alphabet parameter in `epsilon`; auto uses sequence metadata. |
| `log_base` | `u32` | `NO_ALPHABET` | Entropy/score units; auto uses sequence alphabet size. |

`NO_ALPHABET` is `u32::MAX`. Normalization floors explicit bases below two to
two. Setting `args.alphabet` does not set `args.log_base`; specify both when
that is the intended convention. `ALPHABET_SIZE` is the default floor, two.

The current `PartialEq` implementation compares `chunks`, `max_context`,
`block_size`, `get_shuffle_terms` and `alphabet`, **omitting `log_base`**.
Compare units explicitly when constructing a cache key or determining whether
measurements used identical settings.

Use a Rayon pool when you need explicit worker control:

```rust
// With rayon also listed in your application's dependencies:
use lzcomplexity_core::{metrics, LzArgs, Sequence};

fn main() -> Result<(), rayon::ThreadPoolBuildError> {
    let seq = Sequence::from_str("0001011100010111");
    let args = LzArgs::default();
    let pool = rayon::ThreadPoolBuilder::new().num_threads(4).build()?;
    let result = pool.install(|| metrics::lz76(&seq, &args));
    println!("{}", result.entropy_density);
    Ok(())
}
```

## Result structures

All fields below are public. Structs implement `Clone` and `Debug`.

| Struct | Fields |
|---|---|
| `Lz76Result` | `factorization: u32`, `epsilon: f64`, `lzf: Vec<u32>` |
| `LzShuffle` | `max_block_size: i32`, `emc_value: f64`, `multi_information: f64`, `summands: Vec<f64>` |
| `LempelZiv` | `complexity`, `entropy_density`, `random_shuffle_complexity`, `paired_shuffle_complexity`, `lz_normal_errors`, `lz_poison_errors`, `epsilon`, `factors_stddev`, `factors`, `extras` |
| `LzExtra` | `lz_rajski_distance`, `redundancy`, `fh_uncertainty`, `lh_uncertainty`, `lz_pearson_coefficient` (all `f64`) |

Within `LempelZiv`, `complexity` is `u32`, `factors` is `Vec<u32>`, the shuffle
fields are `LzShuffle`, `extras` is `LzExtra`, and the remaining scalars are
`f64`. The full driver leaves `paired_shuffle_complexity` at its default; it
does not compute that optional analysis.

`LzShuffle::default()` has `max_block_size = -1`, zero scalars and an empty
summand vector. A negative default cap therefore means *uncomputed* in a
result, while a negative cap in `LzArgs` means *select automatically*.

The [Python reference](python.md) explains output meanings, legacy diagnostic
fields and boundary conventions. High-level count functions return one for
empty and constant inputs. The lower-level `lz76::factorize` instead returns
`(0, 0.0, Vec::new(), 0.0)` for empty input; choose an entry point deliberately.

## `Sequence`

A `Sequence` owns a `Vec<u8>` and cached alphabet information. It is `Clone`,
`Send` and `Sync`, compares and orders by its data, and accepts every byte
value. String construction uses UTF-8 bytes.

### Construction and access

| Method | Behavior |
|---|---|
| `new()`, `default()` | Empty sequence; automatic alphabet metadata. |
| `with_capacity(n)` | Empty sequence with reserved data capacity. |
| `from_str(text)` | Copy UTF-8 bytes. |
| `from_bytes(data)` | Own the byte vector and determine its alphabet. |
| `from_bytes_with_alphabet(data, size)` | Legacy constructor hint: a mismatch is replaced by the observed size (floored at two). An exact zero/one-symbol hint is retained. |
| `with_alphabet_size(size)` | Empty sequence with an explicit size declaration. |
| `as_bytes()`, `as_bytes_mut()` | Borrow the bytes immutably or mutably. |
| `to_string_lossy()` | Return text with invalid UTF-8 replaced. |
| `len()`, `is_empty()` | Byte length and emptiness. |
| `get(i)`, `first()`, `last()` | Return `Option<u8>` rather than indexing exceptions. |
| `min_byte()`, `max_byte()` | Extremes as `Option<u8>`. |
| `min_range(start, end)`, `max_range(start, end)` | Extremes in `[start, end)`; invalid/empty ranges return `None`. |
| `char_density()` | Sorted byte-to-count `BTreeMap<u8, u32>`. |

### Alphabet metadata and mutation

| Method | Behavior |
|---|---|
| `alphabet()` | Observed distinct bytes, sorted descending. |
| `alphabet_size()` | Explicit declaration if present; otherwise cached observed size, normally floored at two. |
| `set_alphabet_size(size)` | Declare a normalization size without changing bytes or the observed symbol list. |
| `determine_alphabet()` | Rescan, replace an explicit declaration with observed metadata, and return the alphabet. |
| `push(byte)`, `pop()` | Append/remove a byte; return the resulting length. `pop()` on empty data stays empty. |
| `extend_from_slice(bytes)` | Append bytes. |
| `clear()` | Empty data, remove a declaration and restore automatic metadata. |
| `map(function)` | New sequence with each byte transformed. |
| `xor_with(other)` | Set zero where corresponding bytes agree, one where they differ; error `SequenceNoMatchSize` on unequal lengths. |

Symbol-changing operations invalidate the observed cache; the next metadata
query rebuilds it. Explicit declarations survive mutation until
`determine_alphabet()` or `clear()`. Borrowing mutable bytes invalidates the
cache before the borrow, so edits cannot leave stale observed metadata.

```rust
use lzcomplexity_core::Sequence;

let mut seq = Sequence::new();
seq.extend_from_slice(b"ACGT");
assert_eq!(seq.alphabet_size(), 4);
seq.as_bytes_mut().fill(b'A');
assert_eq!(seq.alphabet_size(), 2); // observed size floored at two
seq.set_alphabet_size(4);
seq.push(b'C');
assert_eq!(seq.alphabet_size(), 4); // explicit declaration retained
```

### Substrings and permutations

| Method/operator | Behavior |
|---|---|
| `take(n)` | First `n` bytes, clamped to length. |
| `drop(n)` | Bytes after the first `n`; empty when `n >= len`. |
| `split_at(n)` | Pair of prefix/suffix sequences, split position clamped. |
| `reverse()` | Reverse in place and return `&mut Self`. |
| `reverse_copy()` | Reversed clone. |
| `right_shift(n)`, `left_shift(n)` | Circular byte rotation, returning `&mut Self`. |
| `pi()` | Remove the last byte and return `&mut Self`. |
| `&first + &second` | New concatenated sequence. |
| `granularity(group_size)` | Wrapping byte sum for each complete group; discard an incomplete trailing group. Zero group size returns empty. |

Subsequence constructors use the legacy alphabet-hint rules; do not assume
an explicit declaration propagates unchanged through every derived sequence.
`granularity` is not a collision-free encoding of byte tuples: distinct groups
can have the same sum. Define your scientific coarse-graining independently
when distinct states must stay distinct.

## Extra measures

Let `C` be the original count, `F` the first-half count, `L` the second-half
count, and `I = F + L - C`. The current formulas are:

| Rust field | Formula |
|---|---|
| `lz_rajski_distance` | `2 - (F + L) / C` |
| `redundancy` | `I / (F + L)` |
| `fh_uncertainty` | `I / F` |
| `lh_uncertainty` | `I / L` |
| `lz_pearson_coefficient` | `I / sqrt(F * L)` |

They are count-based diagnostics. The names do not establish Shannon
information identities or a Pearson correlation of the original observations.
If any required count is zero, the implementation returns default zero extras.

## Advanced shuffle routines

`sequence::shuffle_in_place` performs one aligned-block swap using a supplied
`ChaCha8Rng`. `sequence::shuffle_copy` repeats swaps on a clone with a
content-derived seed; `shuffle_copy_seeded` accepts an explicit seed. The
standalone copy helper and the EMC-like pipeline use different seed-derivation
helpers; a bare `shuffle_copy` call is not a promise of the same surrogate as
the measurement pipeline.

`shuffle::shuffle_factorization` returns `(counts, M)`, where indices `1..=M`
hold surrogate counts and index zero is unused; storage has `M + 3` entries.
`shuffle_entropy_calculation` consumes those counts and the original count.
The public `shuffle::max_block_size(N)` helper does not itself include the
additional ten scales applied by the automatic measurement pipeline for
`N > 50`.

`metrics::mutual_information` and `lz76_random_shuffle_distance` operate on
concatenation. Their `_z` variants use `shuffle::merge_sequences` to pair
corresponding positions; pairing stops at the shorter input. The paired
shuffle-complexity routine splits one sequence into halves before pairing.
These historical names denote operational shuffle contrasts, not calibrated
Shannon mutual information. Pair labels are encoded as bytes with wrapping,
so the helper does not preserve an arbitrary large alphabet of distinct pairs.
Use it only after validating that representation for the intended inputs.

## Suffix arrays and lower-level building blocks

| Item | Purpose |
|---|---|
| `build_suffix_array(bytes)` | Sorted suffix starting indices as `Vec<u32>`; no sentinel suffix. |
| `suffix_array::build_lcp(bytes, sa)` | LCP array for a matching suffix array. |
| `suffix_array::build(bytes)` | `SuffixArray { sa, lcp, n }`. |
| `SuffixArray::new()`, `clear()` | Empty/default arrays and length. |
| `lpf::lpf(output, sa, lcp, n)` | Populate an existing longest-previous-factor buffer, consuming SA/LCP vectors. |
| `lz76::factorize(seq, args)` | Low-level tuple `(count, epsilon, boundaries, dispersion)`. |

Small suffix arrays use comparison sorting; larger ones use the bundled
`cdivsufsort` dependency, with Kasai LCP construction. This Rust crate does not
expose the historical C++ `CaPS_SA` class. Low-level routines expect matching
lengths and arrays; prefer high-level functions unless maintaining an
algorithm implementation.

Generate the complete item-level documentation from this checkout with
`cargo doc -p lzcomplexity-core --no-deps --open`.

Source: [core modules](https://github.com/pleros-ai/lzcomplexity/tree/rust-backend/crates/lzcomplexity-core/src).
