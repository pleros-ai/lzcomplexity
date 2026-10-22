<div align="center">
  <h2>lzcomplexity</h2>
  <p><em>LZ76-based complexity analysis for symbolic sequences and time-series.</em></p>

[![CI](https://github.com/pleros-ai/lzcomplexity/actions/workflows/ci.yml/badge.svg?branch=rust-backend)](https://github.com/pleros-ai/lzcomplexity/actions/workflows/ci.yml)
[![Documentation](https://img.shields.io/badge/docs-online-0071e3)](https://pleros-ai.github.io/lzcomplexity/)
[![PyPI](https://img.shields.io/pypi/v/lzcomplexity?color=0071e3)](https://pypi.org/project/lzcomplexity/)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23531093.svg)](https://zenodo.org/doi/10.5281/zenodo.23531093)

[![Python](https://img.shields.io/badge/Python-3.9%2B-5e6b7a?logo=python&logoColor=white)](https://pypi.org/project/lzcomplexity/)
[![Rust](https://img.shields.io/badge/Rust-1.85%2B-5e6b7a?logo=rust&logoColor=white)](https://crates.io/crates/lzcomplexity-core)
[![MIT](https://img.shields.io/badge/license-MIT-5e6b7a)](LICENSE)

**[Documentation](https://pleros-ai.github.io/lzcomplexity/)**

</div>

`lzcomplexity` measures randomness, structure across ranges and similarity in
symbolic sequences using Lempel–Ziv 76 factorization. A Rust core powers the
Python library and two command-line tools.

| Measure | What it captures | Python function |
|---|---|---|
| Entropy density **h** | Unpredictability and the production of new information | `lz.h(sequence)` |
| Structural complexity **E** | Correlations across different ranges of a sequence | `lz.emc(sequence)` |
| Information distance **d** | Differences in recurring patterns between sequences | `lz.nid(first, second)` |

Applications include neural time series, written language, genomic sequences
and nonlinear dynamics. Read the [documentation](https://pleros-ai.github.io/lzcomplexity/)
for concepts, worked examples and complete API references.

## Python library

### Install

Requires Python 3.9 or newer:

```sh
pip install lzcomplexity
```

To build from source, install Rust 1.85 or newer and a C compiler, then run
`pip install .` from the repository. The build uses [maturin](https://www.maturin.rs/).

### Quick start

```python
import lzcomplexity as lz

# LZ76 count and factor boundaries
count, boundaries = lz.factorization("banana")
# (3, [0, 1, 2, 3, 7])

# Entropy density, with base-two logarithms
h = lz.h("01010101", log_base=2)
# 0.75

# Structural complexity E and its increments across ranges
sequence = "00010111" * 256
E, increments = lz.emc(sequence, log_base=2, max_block_size=16)

# Information distance between two sequences
d = lz.nid(sequence, "01010101" * 256)

# Complete analysis in one call
result = lz.lz76(sequence, log_base=2, max_block_size=16)
print(result["complexity"], result["h"], result["emc"]["value"])
```

### Public API

| Function | Returns |
|---|---|
| `factorization(seq, ...)` | LZ76 count and byte boundaries: `(int, list[int])` |
| `h(seq, ...)` | Entropy density: `float` |
| `emc(seq, ...)` | E and fitted increments: `(float, list[float])` |
| `nid(seq1, seq2, ...)` | Normalized information distance: `float` |
| `lz76(seq, ...)` | Complete result dictionary |

Strings are analyzed as UTF-8 bytes. Pass `bytes` to preserve one byte per
symbol; integer sequences are joined as decimal text. For example,
`bytes([0, 1, 10])` contains three symbols, while `[0, 1, 10]` becomes `"0110"`.
See [input types](https://pleros-ai.github.io/lzcomplexity/api/inputs/) for
NumPy arrays and categorical codebooks.

Set `log_base=2` for h in bits per input byte. The default base is the observed
alphabet size, with a minimum of two. `max_block_size` controls the maximum
range used for E; nonpositive values select it automatically. Compare E with
matched lengths, encodings and parameters.

The [Python reference](https://pleros-ai.github.io/lzcomplexity/api/python/)
documents every parameter and result field. Type stubs and runtime help ship
with the package.

## Command-line tools

Prebuilt binaries are available from [GitHub Releases](https://github.com/pleros-ai/lzcomplexity/releases).
To build them locally:

```sh
cargo build --release -p lzcomplexity-cli --bins
```

### Analyze a file

```sh
lzcomplexity input.txt -F text -l 2
lzcomplexity sequences.txt -F text -m -d
lzcomplexity input.txt -F text -n
```

`lzcomplexity` writes a JSON report containing the LZ76 count, h and E.
Use `-m` for one sequence per line, `-d` to add consecutive-sequence distances,
or `-n` for entropy density alone.

### Compare sequences

```sh
lzdistance sequences.txt
lzdistance first.txt second.txt
```

`lzdistance` computes pairwise distance matrices from files or directories.
The [CLI reference](https://pleros-ai.github.io/lzcomplexity/cli/) covers
selection ranges, transformations, input formats and output schemas.

## Development

For an editable Python extension build:

```sh
python3 -m venv .venv
source .venv/bin/activate
pip install maturin
maturin develop --release
```

Rebuild with `maturin develop --release` after changing Rust source.

```sh
cargo test --workspace
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
```

Use Conventional Commit titles such as `feat:`, `fix:` and `docs:`. Release
versions and changelogs are managed by release-please. CI tests the Rust
workspace, standalone tools and Python wheels on Linux, macOS and Windows.
See [documentation contributions](https://pleros-ai.github.io/lzcomplexity/project/contributing/)
for local site previews and checks.

## Repository layout

```text
crates/lzcomplexity-core/   Algorithms and Rust API
crates/lzcomplexity-py/     PyO3 bindings
crates/lzcomplexity-cli/    Command-line tools
python/lzcomplexity/       Python package and type stubs
docs/                     Documentation and browser playground
```

## Authors

- **Efrén Aragón Pérez** — creator and principal author of the library.
- **Daniel Estévez Moya** — Rust implementation.
- **Ernesto Estévez Rams** — scientific foundations.

## Citation

Aragón-Pérez, E., & Estevez-Rams, E. (2025). **Lzcomplexity: Entropy Measurement
Library.** *Revista Cubana de Física*, **42**(2), 80–86.
[Article and citation](https://www.revistacubanadefisica.org/index.php/rcf/article/view/31).

Software DOI: [10.5281/zenodo.23531093](https://zenodo.org/doi/10.5281/zenodo.23531093).
See [authors and citation](https://pleros-ai.github.io/lzcomplexity/project/authors/)
for BibTeX and the [references](https://pleros-ai.github.io/lzcomplexity/project/references/)
for the research behind the measures.

## License

[MIT](LICENSE).
