<div align="center">
  <h2>lzcomplexity</h2>
  <p><em>LZ76-based complexity analysis for symbolic sequences and time-series.</em></p>

[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![Python](https://img.shields.io/badge/Python-3.9+-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org)
[![Rust](https://img.shields.io/badge/Rust-stable-DEA584.svg?style=flat&logo=rust&logoColor=white)](https://www.rust-lang.org)

**[Documentation](https://pleros-ai.github.io/lzcomplexity/)**

</div>

`lzcomplexity` computes information-theoretic measures of symbolic sequences using **Lempel–Ziv 76 (LZ76) factorization** [1]. The LZ76 complexity `c(S)` is the minimum number of factors needed to represent a sequence, where each factor is either a new symbol or the longest previously-seen substring. From `c(S)` you get a non-parametric entropy-rate estimator — `h ≈ c(S)·log_k(n)/n` — that converges to the true entropy rate of ergodic sources [2].

The function named `emc` computes a **shuffle-based complexity measure that is
qualitatively similar to effective measure complexity (EMC)**. It is not an
established numerical estimator of theoretical EMC or excess entropy. The
historical API name is retained. See [the shuffle-measure definition and
limitations](docs/shuffle-measure.md).

The library ships **two things**:

1. A **Python library** (`import lzcomplexity`) built on a Rust core via [PyO3](https://pyo3.rs/).
2. Two **standalone command-line tools** — `lzcomplexity` and `lzdistance`.

The core is implemented in Rust; the two front-ends share it. Common applications: neuroscience time-series, DNA analysis, anomaly detection, structural pattern analysis.

> This is the **Rust implementation**, the current production backend. The original C++/nanobind implementation lives on the `main` branch. Compare backend results only with matched versions, input encodings and parameters (see [Differences from the C++ version](#differences-from-the-c-version)).
>
> **Spectral analysis** (`psd`, spectral `entropy`, `semc`) has been **removed** from this library and now lives in a separate package.

---

## Python library

### Install

You need **Python ≥ 3.9**. From PyPI:

```bash
pip install lzcomplexity
```

Or from a clone of this repository (requires [Rust 1.85 or newer](https://rustup.rs)):

```bash
pip install .
```

`pip` invokes [maturin](https://www.maturin.rs/), which compiles the Rust workspace, produces a wheel, and installs it. A C compiler is also needed by the bundled suffix-array dependency. No CMake, no submodules, no C++ toolchain.

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

# Complexity + factor boundaries — factor i spans [factors[i], factors[i+1])
complexity, factors = lz.factorization("banana")
# → (3, [0, 1, 2, 3, 7])

# Normalised entropy density / entropy-rate estimator — `h`
lz.h("01010101")
# → 0.75

# Shuffle-based EMC-like measure: the score and its fitted increments
emc_value, summands = lz.emc("01001010101101010101110101010101010000100101011")

# Everything in one call (returns a dict)
full = lz.lz76("ABRACADABRA")
full["complexity"], full["h"], full["factors"]
full["emc"]     # {"value", "summands", "max_block_size", "multi_information"}
full["extras"]  # {"rajski_distance", "redundancy", "fh_uncertainty", ...}

# Normalized information distance (NID) between two sequences
lz.nid("ABRACADABRA", "ABRACADABRZ")     # → small, similar
lz.nid("ABRACADABRA", "ZYXWVUTSRQP")     # → large, dissimilar
```

### Public API

| Symbol | Signature | Returns |
|---|---|---|
| `lz.factorization(seq, ...)` | complexity + factor boundaries | `(int, list[int])` |
| `lz.h(seq, ...)` | normalised entropy density (entropy rate) | `float` |
| `lz.emc(seq, ...)` | shuffle-based EMC-like measure | `(float, list[float])` — `(value, summands)` |
| `lz.nid(seq1, seq2, ...)` | normalised information distance | `float` |
| `lz.lz76(seq, ...)` | the full analysis | `dict` (see below) |

`lz.lz76(...)` returns a dict with keys: `complexity`, `h`, `factors`, `emc` (a nested dict), `epsilon`, `factors_stddev`, `normal_error`, `poison_error`, `extras` (a nested dict).

Common keyword arguments:

- `partitions` (int, default 1) — suffix-array partition count; performance knob, no effect on results.
- `alphabet` (int | None, default None) — alphabet parameter for the finite-size `epsilon` diagnostic; auto-detect (distinct input bytes, min 2) when None. An explicit value does not change the default logarithm base.
- `log_base` (int | None, default None) — defaults to the observed input alphabet size, floored at 2. Pass `2` explicitly for base-2 normalization (`h` in bits per input byte); the EMC-like score is not a calibrated bit count.
- `max_block_size` (int, default −1) — shuffle block-size cap for `emc`/`lz76`; nonpositive values auto-select using the existing length-based heuristic, including its `+10` for inputs longer than 50 bytes.
- `jobs` (int, default 0) — reserved; currently ignored (rayon manages its pool).

The `emc` result depends on sequence length, block-size cap, input encoding and
surrogate draws. Compare scores using matched settings and examine their
sensitivity. Its summands are fitted score increments; `multi_information` is
the raw one-symbol shuffle contrast. These are not calibrated Shannon
information quantities.

Run `help(lzcomplexity.<name>)` for full per-function docs. Type stubs (`__init__.pyi`) ship with the package.

### Accepted input types

Every sequence-accepting function accepts any of:

- `str` — symbols taken from the string bytes directly.
- `bytes` — raw byte sequence.
- `list[str]` — concatenated as-is (e.g. `["A","C","G","T"]`).
- `list[int]` — each element becomes its decimal string, concatenated, so `[0, 1, 10]` → `"0110"`.
- Any iterable of ints — covers NumPy arrays; same conversion as `list[int]`.

---

## Standalone binaries

Two command-line tools are built from the same Rust core.

Build them from a clone:

```bash
cargo build --release
# → target/release/lzcomplexity   and   target/release/lzdistance
```

Prebuilt binaries for Linux/macOS/Windows are attached to each [GitHub release](https://github.com/pleros-ai/lzcomplexity/releases).

### `lzcomplexity`

Reads a sequence file and writes a JSON report with the LZ76 complexity, entropy density, and shuffle-based EMC-like score.

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

| Prefix | Use for | Version effect (the project is 1.x) |
|---|---|---|
| `feat:` | a new feature | bumps the **minor** (`1.0.0 → 1.1.0`) |
| `fix:` | a bug fix | bumps the **patch** (`1.0.0 → 1.0.1`) |
| `perf:` | a performance improvement | patch |
| `refactor:`, `docs:`, `build:`, `ci:`, `test:`, `chore:` | everything else | no release on its own |

Add `!` after the type (e.g. `feat!:`) or a `BREAKING CHANGE:` footer to force a **major** bump
(`1.x → 2.0.0`). Before 1.0 a breaking change only bumped the minor; that is no longer the case.

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
| Shuffle RNG | Rust-compatible `ChaCha8` implementation in the current C++ source | pinned `ChaCha8` (seeded from input → **deterministic**) |
| Spectral analysis | included (`psd`, `entropy`, `semc`) | **removed** (moved to a separate package) |
| Python surface | many names, `nid`/`rid`, class exports | `factorization`, `h`, `emc`, `nid`, `lz76` |

The current C++ source includes a Rust-compatible shuffle RNG; the old description of a time-seeded C++ shuffle is stale. Backend comparisons must match the source revisions, RNG behavior, input bytes, block-size cap and logarithm base. The CI listed above checks this Rust workspace and its front-ends; it does not establish blanket numerical parity with every C++ revision.

---

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences. *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Kontoyiannis, I., Algoet, P. H., Suhov, Y. M., & Wyner, A. J. (1998). Nonparametric entropy estimation for stationary processes and random fields. *IEEE Transactions on Information Theory*, 44(3), 1319–1327.

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
  version = {1.0.0},
  year    = {2026}
}
```

## License

MIT — see [LICENSE](LICENSE).
