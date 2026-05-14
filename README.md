<div align="center">
  <h2>lzcomplexity</h2>
  <p><em>LZ76-based complexity analysis for symbolic sequences and time-series.</em></p>

[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![Python](https://img.shields.io/badge/Python-3.9+-3776AB.svg?style=flat&logo=python&logoColor=white)](https://www.python.org)
[![Rust](https://img.shields.io/badge/Rust-stable-DEA584.svg?style=flat&logo=rust&logoColor=white)](https://www.rust-lang.org)

</div>

`lzcomplexity` computes information-theoretic measures of symbolic sequences using **Lempel–Ziv 76 (LZ76) factorization** [1]. The LZ76 complexity `c(S)` is the minimum number of factors needed to represent a sequence, where each factor is either a new symbol or the longest previously-seen substring. From `c(S)` you get a non-parametric entropy-rate estimator — `h ≈ c(S)·log_k(n)/n` — that converges to the true entropy rate of ergodic sources [2].

The library exposes:

- **Complexity & entropy** — `lz76`, `factorization`, `factors`, `entropy_density`, `emc`.
- **Information distances** (`metrics`) — `nid` (normalized info distance), `rid` (shuffle-based distance).
- **Spectral analysis** (`spectral`) — `psd`, `entropy`, `semc`.

The core is implemented in Rust; Python bindings are built with [PyO3](https://pyo3.rs/). Common applications: neuroscience time-series, DNA analysis, anomaly detection, structural pattern analysis.

> This is the **Rust implementation**. The original C++/nanobind implementation lives on the `main` branch and is preserved unchanged. Numerical outputs are equivalent up to deterministic shuffle seeding (see [Differences from the C++ version](#differences-from-the-c-version)).

---

## Install

You need **Python ≥ 3.9** and a **Rust toolchain** (`stable`, installed via [rustup](https://rustup.rs)).

From a clone of this repository:

```bash
pip install .
```

`pip` invokes [maturin](https://www.maturin.rs/) (declared in `pyproject.toml`), which compiles the Rust workspace, produces a wheel, and installs it. No CMake, no submodules, no C++ toolchain.

### Development install

If you want to iterate on the Rust code without reinstalling each time:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install maturin
maturin develop --release
```

After this, `import lzcomplexity` picks up your local debug build. Re-run `maturin develop --release` after Rust changes.

---

## Quick start

```python
import lzcomplexity as lz

# Number of LZ76 factors
lz.factorization("01001010101101010101110101010101010000100101011")
# → 9

# Factor count + boundaries
lz.factors("banana")
# → (3, [0, 1, 2, 3, 7])
#       └── factor i spans [positions[i], positions[i+1])

# Normalised entropy density (in [0, 1] by default)
lz.entropy_density("ABRACADABRA")
# → 0.677...   (alphabet auto-detected: 5 distinct symbols)

# Full analysis — complexity, entropy, factors, random-shuffle stats
complexity, entropy, factors, shuffle = lz.lz76("ABRACADABRA")
max_block_size, emc_value, multi_information = shuffle

# Information distance between two sequences
lz.metrics.nid("ABRACADABRA", "ABRACADABRZ")     # → small, similar
lz.metrics.nid("ABRACADABRA", "ZYXWVUTSRQP")     # → large, dissimilar

# Spectral entropy of a signal
import numpy as np
signal = np.sin(2 * np.pi * 5 * np.linspace(0, 1, 1024)).tolist()
lz.spectral.entropy(signal, sample_frequency=1024)
```

### Accepted input types

Every sequence-accepting function accepts any of:

- `str` — symbols are taken from the string bytes directly.
- `bytes` — raw byte sequence.
- `list[str]` — concatenated as-is (e.g. `["A","C","G","T"]`).
- `list[int]` — each element is converted to its decimal string and concatenated. So `[0, 1, 10]` becomes the symbolic string `"0110"` (the multi-digit value collapses). For predictable behaviour with multi-symbol integer data, pre-format to `str` yourself.
- Any iterable of ints — covers NumPy arrays via Python's sequence protocol; same conversion as `list[int]`.

`spectral.psd` / `spectral.entropy` / `spectral.semc` take `list[float]` (or any iterable of floats).

### Alphabet auto-detection

By default, every function leaves `alphabet=None` and `log_base=None`, which means **auto-detect from the input** (number of distinct symbols, minimum 2). This is the right behaviour for almost all inputs:

```python
lz.entropy_density("01010101")    # alphabet inferred as 2
lz.entropy_density("ABRACADABRA") # alphabet inferred as 5
```

If you specifically want entropy in bits, override `log_base`:

```python
lz.entropy_density("ABRACADABRA", log_base=2)
```

Pass `alphabet=N` explicitly only if you know the *true* alphabet differs from what is observed in the input (e.g. a short binary slice that happens to contain only `0`s).

---

## API reference (compact)

Run `help(lzcomplexity.<name>)` from Python for full per-function docs. The complete public surface is:

| Symbol | Signature | Returns |
|---|---|---|
| `lz.lz76(seq, ...)` | full analysis | `(complexity, entropy, factors, (mbs, emc, mi))` |
| `lz.factorization(seq, ...)` | factor count | `int` |
| `lz.factors(seq, ...)` | factor count + boundaries | `(int, list[int])` |
| `lz.entropy_density(seq, ...)` | normalised entropy density | `float` |
| `lz.emc(seq, ...)` | effective measure complexity | `(int, float, float)` |
| `lz.metrics.nid(seq1, seq2, ...)` | normalised info distance | `float` |
| `lz.metrics.rid(seq1, seq2, ...)` | random-shuffle info distance | `float` |
| `lz.spectral.psd(signal, sr, ...)` | power spectral density | `list[float]` |
| `lz.spectral.entropy(signal, sr, ...)` | spectral entropy | `float` |
| `lz.spectral.semc(signal, sr, ...)` | spectral effective complexity | `float` |

Common keyword arguments on the LZ functions:

- `partitions` (int, default 1) — suffix-array partition count; performance knob, no effect on results.
- `alphabet` (int | None, default None) — auto-detect when None.
- `log_base` (int | None, default None) — matches `alphabet` when None.
- `max_block_size` (int, default −1) — shuffle block-size cap; −1 auto-selects.
- `jobs` (int, default 0) — reserved; currently ignored (rayon manages its pool).

Type stubs (`__init__.pyi`) ship with the package, so editors and `mypy`/`pyright` see signatures and types directly.

---

## Repository layout

```
crates/
├── lzcomplexity-core/   Rust crate: algorithms (LZ76, suffix array, LPF,
│                         shuffle, spectral, metrics). No Python types.
└── lzcomplexity-py/     Rust crate: PyO3 bindings. Builds the
                          `lzcomplexity` Python extension module.
python/lzcomplexity/     Python package skin: __init__.py re-exports,
                          __init__.pyi type stubs, py.typed marker.
pyproject.toml           Maturin build config.
```

The Rust workspace builds as one shared object that's installed under the `lzcomplexity` Python package. Running `cargo test --workspace` exercises the algorithmic core (suffix array on `"banana"` / `"test text"`, LCP, LPF, sequence operators, deterministic shuffle).

---

## Differences from the C++ version

| Aspect | C++ (`main` branch) | Rust (this branch) |
|---|---|---|
| Build system | CMake + nanobind | Cargo + maturin |
| Suffix array | CaPS (custom parallel) | `suffix` crate + Kasai LCP |
| FFT | pocketfft | rustfft |
| Parallelism | OpenMP / TBB / Cilk | rayon |
| Shuffle RNG | `std::mt19937` (time-seeded) | `ChaCha8` (seeded from input → **deterministic**) |
| Python surface | older versions exposed many classes (`sequence`, `LZ_Args`, `CaPS`, …) | locked to the 10 names listed above |

Same input ⇒ same outputs across Rust and C++ within float tolerance, *except* shuffle-based metrics (`emc`, `rid`, `spectral.semc`), which are now reproducible run-to-run.

---

## References

1. Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences. *IEEE Transactions on Information Theory*, 22(1), 75–81.
2. Kontoyiannis, I., Algoet, P. H., Suhov, Y. M., & Wyner, A. J. (1998). Nonparametric entropy estimation for stationary processes and random fields. *IEEE Transactions on Information Theory*, 44(3), 1319–1327.

## Citation

```bibtex
@software{lzcomplexity_2025,
  title={lzcomplexity: an entropy measurement library},
  author={Efren Aragon-Perez},
  url={https://github.com/pleros-ai/lzcomplexity},
  year={2025}
}
```

## License

MIT — see [LICENSE](LICENSE).
