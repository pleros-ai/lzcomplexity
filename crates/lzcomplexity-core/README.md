# lzcomplexity-core

[![crates.io](https://img.shields.io/crates/v/lzcomplexity-core.svg)](https://crates.io/crates/lzcomplexity-core)
[![docs.rs](https://docs.rs/lzcomplexity-core/badge.svg)](https://docs.rs/lzcomplexity-core)
[![License](https://img.shields.io/badge/license-MIT%20%2B%20BSD--3--Clause-blue)](#license)

The algorithmic core of [`lzcomplexity`](https://github.com/pleros-ai/lzcomplexity) —
information-theoretic complexity measures for symbolic sequences, based on
**Lempel–Ziv 76 (LZ76) factorization**.

This crate has no Python or CLI dependencies. It exposes a Rust API and uses
the bundled C `libdivsufsort` implementation through `cdivsufsort`, so building
from source also requires a C compiler. It powers the
`lzcomplexity` Python package (via PyO3) and the `lzcomplexity` / `lzdistance`
standalone binaries, and can be used directly as a Rust library.

## What it computes

- **LZ76 factorization** — the complete-factor count `C(S)` and byte boundaries,
  via a suffix array and longest-previous-factor table. The suffix-array
  dependency has `O(n log n)` worst-case time.
- **Entropy density** — the normalized entropy-rate estimator
  `h = C(S)·log_b(n)/n`. The asymptotic result for stationary ergodic
  finite-alphabet sources is stated as `limsup h_n = h_source` almost surely
  [2, 3]. The log base `b` determines the units; this relation supplies no
  finite-sample accuracy guarantee.
- **Effective measure complexity (EMC)** — excess entropy estimated by
  bidirectional causal-state reconstruction (CSSR), with the mutual
  information of inferred forward/reverse states measured on held-out data.
  It uses no shuffling, Chao–Shen correction, or LZ-rate subtraction. The
  method assumes finite, synchronizable state models in both directions;
  failed checks return an explicit unresolved status. This changes the
  released 2.0.0 estimator; see the [estimator notes](../../docs/emc-estimator.md).
- **Normalized information distance (NID)** — an LZ76-based distance between two
  sequences, implemented as a heuristic phrase-count ratio. Self-distance can
  be nonzero and the triangle inequality can fail.

## Usage

```toml
[dependencies]
lzcomplexity-core = "2.0"
```

This selects the published 2.0 series. To evaluate this branch's unreleased
E estimator, build from its source checkout instead.

```rust
use lzcomplexity_core::{lz76, metrics, LzArgs, Sequence};

let args = LzArgs::new(); // auto-detect alphabet / log base

// Complete-factor count + boundaries (final boundary can exceed length by one).
let r = lz76::lz76_factors(&Sequence::from_str("banana"), &args);
assert_eq!(r.factorization, 3);
assert_eq!(r.lzf, vec![0, 1, 2, 3, 7]);

// Normalized entropy density.
let h = lz76::lz76_entropy_density(&Sequence::from_str("01010101"), &args);
assert!((h - 0.75).abs() < 1e-12);

// Normalized information distance between two sequences.
let d = metrics::lz76_information_distance(
    &Sequence::from_str("abcd"),
    &Sequence::from_str("abce"),
    &args,
);
assert!((d - 0.25).abs() < 1e-12);
```

The [API documentation](https://docs.rs/lzcomplexity-core) and
[project website](https://pleros-ai.github.io/lzcomplexity/) describe published
releases. Match their version to the crate or source revision being used.

## Notes

- Inputs are raw byte sequences (`Sequence::from_str` / `Sequence::from_bytes`),
  so any alphabet works, including non-UTF-8 bytes.
- For nonconstant input, the count omits a trailing copied suffix without an
  innovation symbol. `"banana"` returns 3; its textbook exhaustive history has
  four components. Returned boundaries are byte offsets and may end at `n+1`.
- `factors_stddev`, `lz_normal_errors`, and `lz_poison_errors` are legacy
  diagnostics, not calibrated statistical uncertainty. The `LzExtra` fields
  are phrase-count ratios rather than Shannon information measures.
- `LzArgs::max_context` caps CSSR history depth. A positive `block_size` is a
  deprecated alias for this cap on the E path; shuffle-distance metrics keep
  their separate block-size meaning. A finite reconstruction depth can infer
  a model with arbitrarily long observable memory. It can also miss states
  requiring longer distinguishing suffixes. Check the status and diagnostics;
  a successful estimate is conditional on the model assumptions, not a
  universal accuracy certificate. E is not replaced by state entropy `C_mu`.

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences.
   *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Ziv, J. (1978). Coding theorems for individual sequences.
   *IEEE Transactions on Information Theory*, 24(4), 405–412.
   [doi:10.1109/TIT.1978.1055911](https://doi.org/10.1109/TIT.1978.1055911).
3. Estevez-Rams, E., et al. (2013). On the non-randomness of maximum Lempel
   Ziv complexity sequences of finite size. *Chaos*, 23, 023118.
   [Author preprint, Eq. (4)](https://arxiv.org/abs/1311.0546).

## Authors

- **Efrén Aragón Pérez** — principal author and creator of `lzcomplexity`.
- **Daniel Estévez Moya** — the Rust rewrite.
- **Ernesto Estévez Rams** — research lineage of the original C++ implementation.

## License

Project code is MIT-licensed; the CSSR component includes BSD-3-Clause code
from `sofic`. See the project [LICENSE](../../LICENSE) and
[third-party notice](THIRD_PARTY_LICENSES/sofic-BSD-3-Clause.txt).
