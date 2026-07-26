# Changelog

## [2.0.0](https://github.com/pleros-ai/lzcomplexity/compare/v1.0.2...v2.0.0) (2026-07-26)


### ⚠ BREAKING CHANGES

* emc() and lz76()["emc"] now return values read off a projected block-entropy ladder. The value and every summand are non-negative, the summands are increments of the projected ladder rather than raw differences of independent block-entropy estimates, and values change for any input whose raw ladder was not monotone. multi_information is unchanged.

### Features

* project the EMC ladder onto the cone excess entropy lives in ([fb0a16d](https://github.com/pleros-ai/lzcomplexity/commit/fb0a16d241bde97daff097a9442bb870c97e8f3c))


### Build System

* pin rand and rand_chacha exactly, and sync Cargo.lock ([cecc611](https://github.com/pleros-ai/lzcomplexity/commit/cecc611a1151a3113b9cb7a51f2e0945f32dafbf))

## [1.0.2](https://github.com/pleros-ai/lzcomplexity/compare/v1.0.1...v1.0.2) (2026-07-25)


### Documentation

* credit Efren Aragon Perez as principal author ([748902b](https://github.com/pleros-ai/lzcomplexity/commit/748902b11b33a5381973a303155c4db590b60626))

## [1.0.1](https://github.com/pleros-ai/lzcomplexity/compare/v1.0.0...v1.0.1) (2026-07-25)


### Documentation

* add a README for the core crate ([b1bae5e](https://github.com/pleros-ai/lzcomplexity/commit/b1bae5e39522b15d7c555d541e96d397ca4ad9ab))

## [1.0.0](https://github.com/pleros-ai/lzcomplexity/compare/v0.13.0...v1.0.0) (2026-07-25)


### Features

* declare a stable 1.0 public API ([76ad6c7](https://github.com/pleros-ai/lzcomplexity/commit/76ad6c713fd4460d083eb2511477c82c121a6a18))


### Build System

* add package metadata and mark the package production-stable ([8889072](https://github.com/pleros-ai/lzcomplexity/commit/8889072c0b4e1b111e678ab30727b7f11737d1f8))


### CI

* add Dependabot and publish the core crate to crates.io ([533b285](https://github.com/pleros-ai/lzcomplexity/commit/533b285afeca4e3904749cfdc7a8504b1828fd0a))

## [0.13.0](https://github.com/pleros-ai/lzcomplexity/compare/v0.12.0...v0.13.0) (2026-07-25)


### ⚠ BREAKING CHANGES

* emc() and lz76()["emc"] now return values from the block-entropy estimator, which differ numerically from the previous formula.

### Features

* emc uses the block-entropy estimator ([d16939d](https://github.com/pleros-ai/lzcomplexity/commit/d16939d354ed6ada6eef93b612502fd91784fef6))


### Performance

* linear-time suffix array + reuse/parallelize factorizations ([8f858be](https://github.com/pleros-ai/lzcomplexity/commit/8f858beead0a182dcad0fd8543c84199239591bc))

## [0.12.0](https://github.com/pleros-ai/lzcomplexity/compare/v0.11.0...v0.12.0) (2026-07-24)


### ⚠ BREAKING CHANGES

* `entropy_density` is renamed to `h`; `nid` moves from `lz.metrics.nid` to `lz.nid`; `metrics.information_distance` and the `metrics` submodule are removed.

### Features

* flatten the API — top-level nid and h, drop the metrics submodule ([8fe9ac6](https://github.com/pleros-ai/lzcomplexity/commit/8fe9ac6d725296e76aac981a3e53b459a4a0d93c))


### CI

* fix release binary uploads and make macOS builds runner-independent ([9548a31](https://github.com/pleros-ai/lzcomplexity/commit/9548a3185e03bad2dc20765bf898f0b5d0cb0538))
* publish standalone binaries as artifacts and attach best-effort ([6d73227](https://github.com/pleros-ai/lzcomplexity/commit/6d73227ba1e8ed45641bba82ed2b6d774dbc8188))

## [0.11.0](https://github.com/pleros-ai/lzcomplexity/compare/v0.10.2...v0.11.0) (2026-07-24)


### ⚠ BREAKING CHANGES

* spectral analysis (`psd`, spectral `entropy`, `semc`) is removed and now lives in a separate package; `metrics.rid` and the standalone `factors` function are removed (folded into `factorization`).

### Features

* rewrite the library backend in Rust with a redesigned Python API ([96c583f](https://github.com/pleros-ai/lzcomplexity/commit/96c583ff32189c95ee99ca82a26a97603d53dd9c))


### Documentation

* rewrite the README for the Rust library, binaries, and release flow ([2bf700d](https://github.com/pleros-ai/lzcomplexity/commit/2bf700d0f8ce4b559531f4c0ba34688b0385bb74))


### CI

* build Rust wheels for PyPI and automate releases with release-please ([5781c2b](https://github.com/pleros-ai/lzcomplexity/commit/5781c2bebaa16abb3bbd79cce1faf6d175eaff95))
