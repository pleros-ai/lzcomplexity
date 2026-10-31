---
title: Rust API
description: Complete Rust reference for measurement functions, sequence operations, configuration, result types and lower-level building blocks.
---

# Rust API

`lzcomplexity-core` provides byte sequences, LZ76 factorization, entropy density h,
effective measure complexity E and information distance. Use the measurement
functions directly or combine the lower-level building blocks in your own analysis.

```toml
[dependencies]
lzcomplexity-core = "2.0"
```

The crate uses Rust 1.85 or newer and a C compiler for its bundled suffix-array
dependency. All input positions and lengths refer to **bytes**.

## Start with the high-level functions

```rust
use lzcomplexity_core::{emc, lz76, metrics, LzArgs, Sequence};

let seq = Sequence::from_str("1000110011101");
let mut args = LzArgs::default();
args.log_base = 2;
args.block_size = 8;
args.get_emc_terms = true;

let factors = lz76::lz76_factors(&seq, &args);
assert_eq!(factors.factorization, 5);
assert_eq!(factors.lzf, vec![0, 1, 2, 5, 9, 13]);

let h = lz76::lz76_entropy_density(&seq, &args);
let e = emc::emc(&seq, &args);
let full = metrics::lz76(&seq, &args);
assert_eq!(h, full.entropy_density);
assert_eq!(e.emc_value, full.emc.emc_value);
```

Choose an entry point for the result you need:

| Task | Function | Detailed reference |
|---|---|---|
| Count factors | `lz76::lz76_factorization` | [Factor count](rust/analysis.md#lz76_factorization) |
| Obtain factor boundaries | `lz76::lz76_factors` | [Factor boundaries](rust/analysis.md#lz76_factors) |
| Measure entropy density | `lz76::lz76_entropy_density` | [Entropy density](rust/analysis.md#lz76_entropy_density) |
| Measure effective measure complexity | `emc::emc` | [E](rust/analysis.md#emc) |
| Compare two sequences | `metrics::lz76_information_distance` | [Information distance](rust/analysis.md#lz76_information_distance) |
| Obtain the complete analysis | `metrics::lz76` | [Full result](rust/analysis.md#lz76) |

The [measurement reference](rust/analysis.md) documents every measurement function,
including variants that reuse a factor count and the additional pairwise measures.
Functions receive borrowed `&Sequence` inputs and `&LzArgs` settings. They return
ordinary Rust values synchronously; the input sequences are not modified.

## `Sequence`

A [`Sequence`](rust/sequence.md) owns bytes and tracks its alphabet. Construct it
from text with `Sequence::from_str`, or from encoded states with
`Sequence::from_bytes`. The sequence reference covers every constructor, accessor,
mutation and transformation, including how alphabet declarations propagate.

```rust
use lzcomplexity_core::Sequence;

let text = Sequence::from_str("ACGT");
let states = Sequence::from_bytes(vec![0, 1, 2, 3]);
assert_eq!(text.len(), states.len());
assert_eq!(states.alphabet_size(), 4);
```

Text is encoded as UTF-8. A multi-byte character therefore occupies several
positions. Rust accepts all 256 possible byte values, including zero and invalid
UTF-8. Assign the same state codebook to every sequence that will be compared.

## `LzArgs`

[`LzArgs`](rust/types.md#lzargs) controls logarithm units, the maximum range used
for E and whether its profile increments are returned. `LzArgs::new()` and
`LzArgs::default()` produce identical settings.

By default, the logarithm base follows the sequence's alphabet size, with a
minimum of two. Set `log_base = 2` for base-two units. Set `block_size` to a
positive value for an explicit maximum range and `get_emc_terms = true` to
obtain E's increments. The [configuration reference](rust/types.md#lzargs)
explains every field, sentinel and default.

## Result structures

The [result reference](rust/types.md#result-structures) describes the complete
fields of `Lz76Result`, `EmcResult`, `LempelZiv` and `LzExtra`, their constructors,
default values and the meaning of the diagnostics.

Factor boundaries may contain an endpoint beyond the input length. Such a final
unfinished factor is excluded from the returned count. Preserve the reported
count rather than reconstructing it from the number of boundaries. See the
[factorization reference](rust/analysis.md#lz76_factors) for examples.

## Extra measures

`metrics::lz76_extras` and `metrics::lz76_extras_with` summarize the factor counts
of a sequence and its two halves. Let `C` be the full count, `F` the first-half
count, `L` the second-half count, and `I = F + L - C`:

| `LzExtra` field | Value |
|---|---|
| `lz_rajski_distance` | `2 - (F + L) / C` |
| `redundancy` | `I / (F + L)` |
| `fh_uncertainty` | `I / F` |
| `lh_uncertainty` | `I / L` |
| `lz_pearson_coefficient` | `I / sqrt(F * L)` |

These are ratios of factor counts. In particular, `lz_pearson_coefficient` is not
the sample Pearson correlation of the original observations. The
[extras reference](rust/analysis.md#lz76_extras) explains the split and reuse
variant; [result types](rust/types.md#lzextra) describes the fields.

## Additional Rust helpers { #advanced-shuffle-routines }

The [building-block reference](rust/building-blocks.md) documents the public
sequence sampling functions, range selection, intermediate count calculations
and paired-state encoding. Their signatures retain the names used in the crate.
For a standard E measurement, use
[`emc`](rust/analysis.md#emc).

## Suffix arrays and lower-level building blocks

The crate exposes suffix-array construction, longest-common-prefix arrays,
longest-previous-factor arrays and the low-level factorization tuple. Each helper's
input contract and return layout appears in the
[building-block reference](rust/building-blocks.md#suffix-arrays).

For example, a suffix array lists where each lexicographically ordered suffix
begins:

```rust
use lzcomplexity_core::build_suffix_array;

assert_eq!(build_suffix_array(b"banana"), vec![5, 3, 1, 0, 4, 2]);
```

## Parallel execution

The core uses Rayon for independent work. Use a local Rayon pool when your
application needs a fixed worker count; add `rayon` to your application's
dependencies as well:

```rust
use lzcomplexity_core::{metrics, LzArgs, Sequence};

let seq = Sequence::from_str("0001011100010111");
let pool = rayon::ThreadPoolBuilder::new().num_threads(4).build().unwrap();
let result = pool.install(|| metrics::lz76(&seq, &LzArgs::default()));
assert!(result.entropy_density >= 0.0);
```

See [determinism](../project/determinism.md) for reproducible analysis settings.
You can also generate local item documentation with
`cargo doc -p lzcomplexity-core --no-deps --open`.
