# lzcomplexity-core

[![crates.io](https://img.shields.io/crates/v/lzcomplexity-core.svg)](https://crates.io/crates/lzcomplexity-core)
[![docs.rs](https://docs.rs/lzcomplexity-core/badge.svg)](https://docs.rs/lzcomplexity-core)
[![MIT](https://img.shields.io/badge/license-MIT-blue)](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE)

The algorithmic core of [`lzcomplexity`](https://github.com/pleros-ai/lzcomplexity) —
information-theoretic complexity measures for symbolic sequences, based on
**Lempel–Ziv 76 (LZ76) factorization**.

This crate has no Python or CLI dependencies. Its suffix-array dependency
bundles C code, so building from source requires a C compiler. It powers the
`lzcomplexity` Python package (via PyO3) and the `lzcomplexity` / `lzdistance`
standalone binaries, and can be used directly as a Rust library.

## What it computes

- **LZ76 factorization** — the complexity `c(S)` (factor count) and the factor
  boundaries, via a suffix array + longest-previous-factor table.
- **Entropy density** — the normalized entropy-rate estimator
  `h ≈ c(S)·log_k(n)/n`, which converges to the true entropy rate of ergodic
  sources.
- **Effective measure complexity E** — sensitivity to correlations across different
  ranges of a sequence. See the [calculation reference](https://pleros-ai.github.io/lzcomplexity/concepts/emc/).
- **Normalized information distance (NID)** — an LZ76-based distance between two
  sequences.

## Usage

```toml
[dependencies]
lzcomplexity-core = "2.0"
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
- Symbol-changing mutations refresh observed alphabet information. Explicit
  declarations persist until `determine_alphabet` or `clear`.
- E is deterministic for identical input bytes and parameters. Its returned
  summands are fitted profile increments, which sum to the final value.
- Compare E with matched sequence lengths, encodings, log bases and range limits.
- `log_base` defaults to the observed alphabet size, with a minimum of two,
  independently of an explicit `alphabet` argument.

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences.
   *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Kontoyiannis, I., et al. (1998). Nonparametric entropy estimation for
   stationary processes and random fields. *IEEE TIT*, 44(3), 1319–1327.

## Authors

- **Efrén Aragón Pérez** — principal author and creator of `lzcomplexity`.
- **Daniel Estévez Moya** — the Rust rewrite.
- **Ernesto Estévez Rams** — scientific foundations.

## License

MIT — see [LICENSE](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE).

E defaults to Chao–Shen block entropy estimation. Call `emc::emc` with
`LzArgs::default()`, or set `args.emc_method = EmcMethod::Shuffle` to select
the earlier random-shuffle estimator. Results use `EmcResult` and the full
analysis stores E in `LempelZiv.emc`.
