<div align="center">
  <h2>lzcomplexity</h2>
  <p><em>LZ76-based complexity analysis for symbolic sequences and time-series.</em></p>

[![License](https://img.shields.io/badge/license-MIT%20%2B%20BSD--3--Clause-blue)](#license)
[![Python](https://img.shields.io/badge/Python-3.9+-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org)
[![Rust](https://img.shields.io/badge/Rust-stable-DEA584.svg?style=flat&logo=rust&logoColor=white)](https://www.rust-lang.org)

**[Released documentation](https://pleros-ai.github.io/lzcomplexity/)**

</div>

`lzcomplexity` analyses symbolic byte sequences using **Lempel–Ziv 76 (LZ76) factorization** [1]. Each complete factor extends the longest match starting earlier in the sequence by one additional symbol; the match may overlap the current factor. For nonconstant input, the returned count `C(S)` counts complete factors only, omitting a trailing copied suffix with no additional symbol. For example, `"banana"` returns 3 although its full exhaustive history has four components: `b | a | n | ana`.

The entropy-rate estimate is `h = C(S)·log_b(n)/n`, with log base `b` controlling the units. The asymptotic LZ76 result for stationary ergodic finite-alphabet sources is stated as `limsup h_n = h_source` almost surely [2, 3]. This is not a finite-sample accuracy guarantee: estimates can be biased and can exceed the alphabet's maximum entropy.

`emc` estimates **excess entropy**, the mutual information between the infinite past and future, by reconstructing predictive and retrodictive causal states with CSSR and measuring their mutual information on held-out data. It uses no random shuffling, Chao–Shen correction, or LZ entropy-rate subtraction. This method assumes finite, synchronizable causal-state models in both directions; insufficient or unstable fits produce an explicit unresolved result. See [the estimator notes](docs/emc-estimator.md) for assumptions, diagnostics and migration details.

The library ships **two things**:

1. A **Python library** (`import lzcomplexity`) built on a Rust core via [PyO3](https://pyo3.rs/).
2. Two **standalone command-line tools** — `lzcomplexity` and `lzdistance`.

The core is implemented in Rust; the two front-ends share it. Common applications: neuroscience time-series, DNA analysis, anomaly detection, structural pattern analysis.

> This is the **Rust implementation**. The original C++/nanobind implementation lives on `main`. This branch changes the E estimator from released 2.0.0; the separately maintained website describes the released shuffle estimator. See [the estimator and migration notes](docs/emc-estimator.md) for this branch's method and limitations.
>
> **Spectral analysis** (`psd`, spectral `entropy`, `semc`) has been **removed** from this library and now lives in a separate package.

---

## Python library

### Install

You need **Python ≥ 3.9**. From PyPI:

```bash
pip install lzcomplexity
```

Or from a clone of this repository (requires a [Rust toolchain](https://rustup.rs) and a C compiler for the bundled suffix-array dependency):

```bash
pip install .
```

`pip` invokes [maturin](https://www.maturin.rs/), which compiles the workspace, produces a wheel, and installs it.

#### Development install

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install maturin
maturin develop --release
```

Re-run `maturin develop --release` after Rust changes.

### Quick start

```python
import lzcomplexity as lz

# Complete-factor count + boundaries (byte offsets; final boundary may be n+1)
complexity, factors = lz.factorization("banana")
# → (3, [0, 1, 2, 3, 7])

# Normalised entropy density / entropy-rate estimator — `h`
lz.h("01010101")
# → 0.75

# Excess entropy: use enough observations for state reconstruction.
# Terms are per-forward-state contributions that sum to the estimate.
emc_value, summands = lz.emc(b"01" * 4096, max_history=6, log_base=2)

# Everything in one call (returns a dict)
full = lz.lz76("ABRACADABRA")
full["complexity"], full["h"], full["factors"]
full["emc"]     # value is None when E cannot be resolved; h remains available
full["extras"]  # {"rajski_distance", "redundancy", "fh_uncertainty", ...}

# LZ76 phrase-count dissimilarity between two sequences
lz.nid("ABRACADABRA", "ABRACADABRZ")     # → small, similar
lz.nid("ABRACADABRA", "ZYXWVUTSRQP")     # → large, dissimilar
```

### Public API

| Symbol | Signature | Returns |
|---|---|---|
| `lz.factorization(seq, ...)` | complexity + factor boundaries | `(int, list[int])` |
| `lz.h(seq, ...)` | normalised entropy-rate estimate | `float` |
| `lz.emc(seq, ...)` | causal-state excess-entropy estimate | `(float, list[float])` — `(value, summands)`; raises `ValueError` if unresolved |
| `lz.nid(seq1, seq2, ...)` | LZ76 phrase-count dissimilarity | `float` |
| `lz.lz76(seq, ...)` | the full analysis | `dict` (see below) |

`lz.lz76(...)` returns a dict with keys: `complexity`, `h`, `factors`, `emc` (a nested dict), `epsilon`, `factors_stddev`, `normal_error`, `poison_error`, `extras` (a nested dict).

`lz76()["emc"]` includes a status, explanation and model diagnostics. Its `value` is `None` when the fit fails the data, model-stability or synchronization checks; the other LZ76 results remain available. A successful status is evidence from those checks, not a universal guarantee of source-model accuracy.

`factors_stddev`, `normal_error`, and `poison_error` are legacy diagnostic formulas, not a standard deviation or calibrated uncertainty on `h`. The `extras` are phrase-count ratios, not Shannon mutual information or statistical correlation coefficients. Likewise, `nid` is a heuristic dissimilarity: self-distance can be nonzero and the triangle inequality can fail.

Common keyword arguments:

- `partitions` (int, default 1) — reserved compatibility argument; currently ignored.
- `alphabet` (int | None, default None) — auto-detect (distinct symbols, min 2) when None.
- `log_base` (int | None, default None) — matches the effective alphabet when None; pass `2` for `h` in bits per byte and `emc` in bits.
- `max_history` (int | None, default None) — maximum suffix-history depth (2–16) tested by CSSR in `emc`/`lz76`; `None` selects a data-dependent resource cap. This is a reconstruction parameter, not a cutoff on the inferred process memory.
- `max_block_size` (int, default −1) — deprecated alias for a positive `max_history`. Supplying both with different positive values raises `ValueError`. Its meaning has changed from the released shuffle estimator.
- `jobs` (int, default 0) — reserved; currently ignored (rayon manages its pool).

Run `help(lzcomplexity.<name>)` for full per-function docs. Type stubs (`__init__.pyi`) ship with the package.

### Accepted input types

Every sequence-accepting function accepts any of:

- `str` — symbols taken from the string bytes directly.
- `bytes` — raw byte sequence.
- `list[str]` — concatenated as-is (e.g. `["A","C","G","T"]`).
- `list[int]` — each element becomes its decimal string, concatenated, so `[0, 1, 10]` → `"0110"`.
- Integer sequences such as tuples, ranges, and one-dimensional NumPy arrays — same conversion as `list[int]`. Generators and iterators are not accepted.

For categorical data, prefer `bytes` with one byte per symbol. Decimal concatenation is ambiguous: `[1, 23]` and `[12, 3]` both become `"123"`. Python strings use UTF-8 bytes, so non-ASCII characters can span several symbols.

---

## Standalone binaries

Two command-line tools are built from the same Rust core.

Build them from a clone:

```bash
cargo build --release -p lzcomplexity-cli
# → target/release/lzcomplexity   and   target/release/lzdistance
```

Prebuilt binaries for Linux/macOS/Windows are attached to each [GitHub release](https://github.com/pleros-ai/lzcomplexity/releases). Unzip the platform archive to obtain both executables and their license notices.

### `lzcomplexity`

Reads a sequence file and writes a JSON report with the complete LZ76 factor count, entropy density, and effective measure complexity estimate.

```bash
lzcomplexity input.txt                 # → input.lz76.json
lzcomplexity input.txt -m -d           # multi-line + distance between consecutive lines
lzcomplexity input.txt -n              # entropy density only
lzcomplexity input.txt -a 4 -l 2       # explicit alphabet / log base
```

Key flags: `-a/--alphabet`, `-l/--log-base`, `-p/--partitions`, `-m/--multi-line`, `-d/--dlz`, `-n/--entropy-density`, `-f/--factors <file>`, `-F/--format <fmt>`, `-o/--output`, `-v/--verbose`, `-V/--version`.

### `lzdistance`

Computes pairwise information-distance and shuffle-distance matrices between one or two data sources (files or directories).

```bash
lzdistance sequences.txt               # self-distance matrix (first_dim × first_dim)
lzdistance A.txt B.txt                  # cross matrix
lzdistance genomes/ -a                  # a directory of files, DNA complement-aware
lzdistance A.txt B.txt -g 5             # also emit the directed graph
```

Key flags: `-a/--adn` (DNA), `-b/--binary`, `-r/--reverse`, `-y/--trajectory`, `-g/--get-direction [threshold]`, `-I/--first-format`, `-S/--second-format`, `-i/--first #:#`, `-s/--second #:#`, `-p/--partitions`, `-o/--output`, `-L/--logs`, `-v/--version`.

### Input formats

Both tools auto-detect the format (or accept `-F`/`-I`/`-S`): raw **text** and **binary**, **CSV**/**TSV**, **PBM/PGM** images (P1/P2/P4/P5), and **FASTA/DNA/RNA** (including `.gz`). Use `-m` (or a directory) to treat each line/file as a separate sequence.

---

## Contributing & releases

This repo uses **[Conventional Commits](https://www.conventionalcommits.org/)** together with **[release-please](https://github.com/googleapis/release-please)** so that versioning and PyPI publishing happen **automatically** — you never bump a version or push a tag by hand.

### How to commit

Prefix every commit **title** with a type:

| Prefix | Use for | Version effect (after 1.0) |
|---|---|---|
| `feat:` | a new feature | bumps the **minor** (`1.0.0 → 1.1.0`) |
| `fix:` | a bug fix | bumps the **patch** (`1.0.0 → 1.0.1`) |
| `perf:` | a performance improvement | patch |
| `refactor:`, `docs:`, `build:`, `ci:` | maintenance with a visible changelog section | patch |
| `test:`, `chore:` | other maintenance | no release on its own |

Add `!` after the type (e.g. `feat!:`) or a `BREAKING CHANGE:` footer to force a **major** bump
(`2.x → 3.0.0`, for example).

> Keep commit messages **ASCII-only**. release-please failed to parse a message containing Unicode
> maths symbols and silently dropped it from the changelog.

```
feat: add spectral-free NID batch mode
fix: correct off-by-one in PGM reader
docs: document the lzdistance directed graph
```

### What happens automatically

The Python package is released from the **`rust-backend`** branch; the C++ `main`
branch is kept only as a verification reference and is not published.

1. You merge conventional commits into `rust-backend`.
2. **release-please** opens (and keeps updating) a "release PR" that bumps the version in `Cargo.toml` **and** `pyproject.toml`, and updates `CHANGELOG.md`.
3. When you merge that release PR, release-please tags the release (`vX.Y.Z`) and creates a GitHub release.
4. The [`Release`](.github/workflows/release.yml) workflow then builds the wheels with maturin, **publishes them to PyPI** via OIDC trusted publishing (no token needed), and attaches the standalone binaries to the GitHub release.

So the entire flow is: **write conventional commits → merge the release PR → PyPI updates itself.**

> One-time setup on PyPI: the *trusted publisher* for the `lzcomplexity` project
> (Settings → Publishing) is keyed on the repository and the **workflow filename**
> (plus an optional environment) — **not** on a branch. Point its *Workflow name*
> at **`release.yml`** (the previous C++ setup used `wheels.yml`). No API token
> or secret is stored in the repo.

### CI

Every push and PR runs [`CI`](.github/workflows/ci.yml): `cargo fmt --check`, `cargo clippy -D warnings`, `cargo test`, a smoke test of both standalone binaries, and a wheel build + Python API smoke test on Linux/macOS/Windows.

---

## Repository layout

```
crates/
├── lzcomplexity-core/   Rust crate: algorithms (LZ76, suffix array, LPF,
│                        shuffle, metrics). No Python or CLI types.
├── lzcomplexity-py/     PyO3 bindings → the `lzcomplexity` Python extension.
└── lzcomplexity-cli/    The `lzcomplexity` and `lzdistance` binaries.
python/lzcomplexity/     Python package skin: __init__.py, __init__.pyi, py.typed.
pyproject.toml           Maturin build config.
```

`cargo test --workspace` exercises the algorithmic core.

---

## Differences from the C++ version

| Aspect | C++ (`main` branch) | Rust (this branch) |
|---|---|---|
| Build system | CMake + nanobind | Cargo + maturin |
| Suffix array | CaPS (custom parallel) | comparison sort below 2048 bytes, `cdivsufsort` above, + Kasai LCP |
| Parallelism | OpenMP / TBB / Cilk | rayon |
| Shuffle RNG | content-seeded ChaCha8 compatibility implementation | content-seeded `ChaCha8` for shuffle-distance metrics |
| Spectral analysis | included (`psd`, `entropy`, `semc`) | **removed** (moved to a separate package) |
| Python surface | many names, `nid`/`rid`, class exports | `factorization`, `h`, `emc`, `nid`, `lz76` |

The factorization convention descends from the C++ implementation. Entropy values require matching log bases. This branch changes the E estimator, so its values must not be mixed with the C++ or released 2.0.0 shuffle estimator. Historical parity measurements do not establish parity for a changed algorithm.

---

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences. *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Ziv, J. (1978). Coding theorems for individual sequences. *IEEE Transactions on Information Theory*, 24(4), 405–412. [doi:10.1109/TIT.1978.1055911](https://doi.org/10.1109/TIT.1978.1055911).
3. Estevez-Rams, E., et al. (2013). On the non-randomness of maximum Lempel Ziv complexity sequences of finite size. *Chaos*, 23, 023118. [Author preprint, Eq. (4)](https://arxiv.org/abs/1311.0546).

## Authors

- **Efrén Aragón Pérez** — principal author and creator of `lzcomplexity`. The library exists
  because of him; he wrote the original implementation this work descends from.
- **Daniel Estévez Moya** — the Rust rewrite that is the current production backend.
- **Ernesto Estévez Rams** — from the research lineage of the original C++ implementation.

## Citation

```bibtex
@software{lzcomplexity_2026,
  title   = {lzcomplexity: LZ76 complexity, entropy rate and information distance
             for symbolic sequences},
  author  = {Arag{\'o}n P{\'e}rez, Efr{\'e}n and
             Est{\'e}vez Moya, Daniel and
             Est{\'e}vez Rams, Ernesto},
  url     = {https://github.com/pleros-ai/lzcomplexity},
  version = {2.0.0},
  year    = {2026}
}
```

## License

Project code is MIT-licensed; the CSSR component includes BSD-3-Clause code
from `sofic`. See [LICENSE](LICENSE) and the
[third-party notice](crates/lzcomplexity-core/THIRD_PARTY_LICENSES/sofic-BSD-3-Clause.txt).
