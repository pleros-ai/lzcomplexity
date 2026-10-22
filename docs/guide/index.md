---
title: Documentation
description: Guides, function references, examples and usage cheatsheets for the lzcomplexity library and standalone tools.
---

# Documentation

lzcomplexity analyzes symbolic sequences: text, binary states, categorical events, DNA or a discretized time series. Use it to measure entropy density h, effective measure complexity E and information distance. The Python library and standalone binaries share the same Rust core.

## Start an analysis

[Install lzcomplexity](install.md), then follow [your first factorization](first-factorization.md) for a complete Python example. The [input guide](../api/inputs.md) explains how to represent symbols as bytes, including numeric states and Unicode text.

```python
import lzcomplexity as lz

sequence = b"00010111" * 256
result = lz.lz76(sequence, log_base=2, max_block_size=16)
print(result["h"])
print(result["emc"]["value"])
```

Continue with [reading the numbers](reading-the-numbers.md) to interpret the results and choose comparable lengths, encodings and units.

## Python library

The [Python API reference](../api/python.md) documents every function, its parameters, return values and examples:

- [`factorization`](../api/functions/factorization.md) returns the factor count and byte boundaries.
- [`h`](../api/functions/h.md) measures entropy density.
- [`emc`](../api/functions/emc.md) returns effective measure complexity E and its range profile.
- [`nid`](../api/functions/nid.md) compares two sequences using information distance.
- [`lz76`](../api/functions/lz76.md) returns the complete analysis in one call.

For a compact reminder of the calls and input preparation, keep the [Python usage cheatsheet](quick-reference.md) nearby.

## Standalone binaries

The [command-line guide](../cli/index.md) covers installation, file preparation and output. Use [`lzcomplexity`](../cli/lzcomplexity.md) to analyze sequences and [`lzdistance`](../cli/lzdistance.md) to compare collections. Both have worked examples and an option reference. The separate [CLI usage cheatsheet](../cli/quick-reference.md) collects common commands.

## Apply it to your data

The recipes walk through [neural time series](../recipes/neuro.md), [DNA and FASTA](../recipes/genomics.md), [nonlinear dynamics](../recipes/dynamical-systems.md) and [comparisons across many sequences](../recipes/batch-distance.md). Each explains the input preparation alongside the analysis.

The concept chapters explain [LZ76 factorization](../concepts/lz76.md), [entropy density](../concepts/entropy-density.md), [E](../concepts/emc.md) and [information distance](../concepts/nid.md). For native applications, the [Rust reference](../api/rust.md) covers measurement functions, sequence operations and result types.
