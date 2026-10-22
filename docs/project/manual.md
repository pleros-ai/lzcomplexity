# Reference manual

The Pleros manual covers the scientific foundations of the library, its
implementation and applications to symbolic signals and nonlinear dynamics.

[Download the manual (PDF)](../downloads/manual-0.9.pdf){ .md-button .md-button--primary }

The PDF includes the earlier class-based API. Use the API and installation
pages on this site for executable examples, or consult the
[migration reference](cpp-parity.md) when adapting a manual example.

## Reading guide

| Topic | Documentation |
|---|---|
| Installation | [Install](../guide/install.md) |
| Factorization and factor boundaries | [LZ76 factorization](../concepts/lz76.md), [worked example](../guide/first-factorization.md) |
| Entropy density and normalization | [Entropy density](../concepts/entropy-density.md), [alphabets and units](../concepts/alphabets.md) |
| Structural complexity E | [E](../concepts/emc.md), [calculation](../concepts/implementation.md#6-structural-complexity-e) |
| Information distance | [Information distance](../concepts/nid.md) |
| Sequence encoding, construction and transformations | [Python inputs](../api/inputs.md), [Rust API](../api/rust.md) |
| Functions, parameters and result structures | [Python API](../api/python.md), [Rust API](../api/rust.md) |
| Suffix arrays, LCP and longest previous factors | [Inside the implementation](../concepts/implementation.md) |
| Parallel execution | [Execution](determinism.md), [performance](performance.md) |
| Standalone tools and batch processing | [CLI overview](../cli/index.md), [input formats](../cli/formats.md) |
| Interpreting results and finite-size effects | [Reading the numbers](../guide/reading-the-numbers.md), [length and convergence](../concepts/convergence.md) |
| Neural signals and connectivity | [Neural time-series](../recipes/neuro.md) |
| Coupled oscillators and parameter scans | [Dynamical systems](../recipes/dynamical-systems.md) |
| Citation and scientific background | [Authors and citation](authors.md), [references](references.md) |
