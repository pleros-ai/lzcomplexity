# lzcomplexity-core

[![crates.io](https://img.shields.io/crates/v/lzcomplexity-core.svg)](https://crates.io/crates/lzcomplexity-core)
[![docs.rs](https://docs.rs/lzcomplexity-core/badge.svg)](https://docs.rs/lzcomplexity-core)
[![MIT](https://img.shields.io/badge/license-MIT-blue)](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE)

The algorithmic core of [`lzcomplexity`](https://github.com/pleros-ai/lzcomplexity) —
information-theoretic complexity measures for symbolic sequences, based on
**Lempel–Ziv 76 (LZ76) factorization**.

This is pure Rust with no Python or CLI dependencies. It powers the
`lzcomplexity` Python package (via PyO3) and the `lzcomplexity` / `lzdistance`
standalone binaries, and can be used directly as a Rust library.

## What it computes

- **LZ76 factorization** — the complexity `c(S)` (factor count) and the factor
  boundaries, via a linear-time suffix array + longest-previous-factor.
- **Entropy density** — the normalized entropy-rate estimator
  `h ≈ c(S)·log_k(n)/n`, which converges to the true entropy rate of ergodic
  sources.
- **Effective measure complexity (EMC)** — a block-entropy (excess-entropy)
  estimator via random block shuffling.
- **Normalized information distance (NID)** — an LZ76-based distance between two
  sequences.

## Usage

```toml
[dependencies]
lzcomplexity-core = "1.0"
```

```rust
use lzcomplexity_core::{lz76, metrics, LzArgs, Sequence};

let args = LzArgs::new(); // auto-detect alphabet / log base

// Complexity + factor boundaries: the i-th factor spans [lzf[i], lzf[i+1]).
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

See the [API documentation](https://docs.rs/lzcomplexity-core) for the full surface, and the
[project documentation](https://pleros-ai.github.io/lzcomplexity/) for the theory, the recipes and
the command-line tools.

## Notes

- Inputs are raw byte sequences (`Sequence::from_str` / `Sequence::from_bytes`),
  so any alphabet works, including non-UTF-8 bytes.
- The shuffle-based EMC is seeded deterministically from the input, so results
  are reproducible run-to-run.

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences.
   *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Kontoyiannis, I., et al. (1998). Nonparametric entropy estimation for
   stationary processes and random fields. *IEEE TIT*, 44(3), 1319–1327.

## Authors

- **Efrén Aragón Pérez** — principal author and creator of `lzcomplexity`.
- **Daniel Estévez Moya** — the Rust rewrite.
- **Ernesto Estévez Rams** — research lineage of the original C++ implementation.

## License

MIT — see [LICENSE](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE).
