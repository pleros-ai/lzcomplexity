# The reference manual

The comprehensive manual supplied by the Pleros Group is the starting reference for this documentation. It is labelled **version 0.9.0 (Beta)** and describes the original C++/Python implementation, its scientific foundations and applications.

[Download the original manual (PDF)](../downloads/manual-0.9.pdf){ .md-button .md-button--primary }

This site carries the manual's explanations forward into documentation for the current Rust backend. The PDF remains useful for the historical class tables, derivations, figures and research context; use this site's API and CLI pages for commands that match current code.

## A map from the manual to this site

| Manual topic | Current documentation |
|---|---|
| Introduction; software overview | [Home](../index.md), [reading the numbers](../guide/reading-the-numbers.md) |
| Installation of CLI, Python and source | [Install](../guide/install.md) |
| How to cite the library | [Authors and citation](authors.md), [references](references.md) |
| Standalone invocation, flags and batch processing | [CLI overview](../cli/index.md), [`lzcomplexity`](../cli/lzcomplexity.md), [`lzdistance`](../cli/lzdistance.md) |
| Text, CSV/TSV and PNM inputs; output schemas | [Input formats](../cli/formats.md), [CLI references](../cli/index.md) |
| Random-shuffle options, cap and reported terms | [EMC-like measure](../concepts/emc.md), [Python API](../api/python.md), [CLI flags](../cli/lzcomplexity.md) |
| Sequence class, constructors, mutation, slicing and transformations | [Rust API](../api/rust.md), [Python input types](../api/inputs.md), [migration guide](cpp-parity.md) |
| `LempelZiv76`; factor counts and boundaries | [LZ76 factorization](../concepts/lz76.md), [first factorization](../guide/first-factorization.md), [implementation](../concepts/implementation.md) |
| CaPS-SA, suffix arrays and LCP | [Implementation](../concepts/implementation.md), [Rust API](../api/rust.md), [backend differences](cpp-parity.md) |
| `LZ_Result`, `SA_Args`, `LZ_Args`, `LZ_SuffixArray`, `LZ_Shuffle`, `LempelZiv` | [Rust structures](../api/rust.md), [current Python return values](../api/python.md) |
| API functions: `lz76`, factorization, factors, entropy, shuffle complexity, distance | [Python API](../api/python.md), [Rust API](../api/rust.md) |
| Parallel annotations and partitions | [Determinism and execution](determinism.md), [performance](performance.md) |
| Shannon/block entropy, entropy density and LZ normalization | [Entropy density](../concepts/entropy-density.md), [alphabets and units](../concepts/alphabets.md) |
| Theoretical EMC/excess entropy and the shuffle-based magnitude | [EMC-like measure](../concepts/emc.md) |
| Kolmogorov complexity and information distance | [Information distance](../concepts/nid.md) |
| Brain-connectivity case study and symbolic signal preprocessing | [Sampled signals and neural time-series](../recipes/neuro.md) |
| Coupled nonlinear oscillators and parameter scans | [Dynamical systems](../recipes/dynamical-systems.md) |
| Even-process / two-state symmetric examples and finite-length comparisons | [Length and convergence](../concepts/convergence.md), [reading the numbers](../guide/reading-the-numbers.md) |
| License and bibliography | [MIT license](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE), [references](references.md) |

## What must be read historically

**Installation.** The manual's CMake, nanobind, oneTBB, Linux-repository and Homebrew instructions belong to its C++ environment. The current build uses Cargo and maturin, and current installation instructions are in the [install guide](../guide/install.md).

**Python APIs.** Old names such as `entropy_density` and `factors`, C++-style objects and tuple unpacking from `lz76()` do not describe the current five-function Python API. The [migration guide](cpp-parity.md) gives direct replacements.

**Algorithms and parallelism.** CaPS-SA and its partition tuning are part of the original implementation. The current core uses comparison sorting for small suffix arrays and `cdivsufsort` for larger ones, with Rayon for parallel tasks. A retained argument name is not proof that the old algorithm still consumes it.

**Scientific interpretation.** The manual itself distinguishes its shuffle quantity from exact excess entropy and discusses qualitative trends. The current score is likewise an EMC-like magnitude. The manual's historical aggregation, scaled validation figures and source-specific observations are not a numerical acceptance oracle for a different version of the implementation.

**Benchmarks and applications.** Historical timings belong to the stated hardware, software, data and settings. Brain-activity figures illustrate a particular study; they are not clinical thresholds supplied by the library. Oscillator figures require their original simulation and symbolization protocols to reproduce.

**License.** The introductory GPL wording conflicts with the manual's final MIT license section. The current repository's `LICENSE` is MIT and is the source to consult for the distributed code.

## Preserve the measurement, not just the label

When reproducing an older analysis, retain its source revision, input bytes, preprocessing, parameters and numerical definition. If you migrate an analysis to a new backend, establish the comparison on your actual inputs before pooling old and new results. See [versions](releases.md) and [determinism](determinism.md).
