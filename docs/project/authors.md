# Authors and citation

*Who wrote lzcomplexity, where it came from, how to cite it, and where to report bugs.*

## The people

- **Efrén Aragón Pérez** — principal author and creator. `lzcomplexity` exists because of him. He
  wrote the original C++ implementation that everything here descends from, and he holds the
  copyright on the MIT licence.
- **Daniel Estévez Moya** — author of the Rust rewrite that is the current production backend: the
  `lzcomplexity-core` crate, the PyO3 bindings, and the two command-line tools shipped in 1.0.
- **Ernesto Estévez Rams** — from the research lineage of the original C++ implementation.

That order is the one recorded in `Cargo.toml`, `pyproject.toml`, `README.md` and the BibTeX entry
below.

!!! note

    The published package metadata on PyPI and crates.io still carries the older two-name author
    list until the next release goes out. The repository is the current record.

## Where the library came from

`lzcomplexity` started as a C++ library for complexity analysis using entropic magnitudes — entropy
density, effective measure complexity and information distance, all derived from Lempel–Ziv 76
factorization. That implementation still lives on the repository's `main` branch, with Python
bindings built on nanobind.

The rewrite landed in 0.11.0 and the API settled at 1.0.0. The Python bindings moved to PyO3, the
CLI was rebuilt on the same core, and the shuffle PRNG stopped drawing from a clock-seeded
`mt19937`: it is now a per-block `ChaCha8Rng` seeded from a hash of the sequence itself, so the same
input returns the same `emc` on every run and at any thread count. There is no user-facing seed
parameter.

Factor counts, factor boundaries and `nid` match the C++ backend exactly; `h` matches at equal log
base, and `emc` now matches too — the C++ backend adopted both the projected-ladder estimator and the
content-seeded ChaCha8 shuffle, so the two draw the same surrogates. See
[Rust vs C++](cpp-parity.md) and [Determinism](determinism.md).

Spectral analysis (`spectral.psd`, `spectral.entropy`, `spectral.semc`) was removed in the same
0.11.0 change and moved to a separate package. It is not part of the 1.0 API, and the CI wheel smoke
test asserts that `spectral` and `metrics` stay absent.

<hr class="lz-tickrule">

## Citing lzcomplexity

Cite the software with the entry below, replacing the version with the one you actually ran.

```bibtex
@software{lzcomplexity_2026,
  title   = {lzcomplexity: LZ76 complexity, entropy rate and information distance
             for symbolic sequences},
  author  = {Arag{\'o}n P{\'e}rez, Efr{\'e}n and
             Est{\'e}vez Moya, Daniel and
             Est{\'e}vez Rams, Ernesto},
  url     = {https://github.com/pleros-ai/lzcomplexity},
  version = {1.0.1},
  year    = {2026}
}
```

The accents are written as BibTeX escapes so the entry survives a classic 8-bit `bibtex` run. With
`biblatex` and a UTF-8 `.bib` file you can write `Aragón Pérez, Efrén` directly.

Plain text:

> Aragón Pérez, E., Estévez Moya, D., & Estévez Rams, E. (2026). *lzcomplexity: LZ76 complexity,
> entropy rate and information distance for symbolic sequences* (version 1.0.1) [Computer software].
> https://github.com/pleros-ai/lzcomplexity

The installed version is on the module, so a methods section can quote the exact build that produced
its numbers:

```pycon
>>> import lzcomplexity as lz
>>> lz.__version__
'1.0.1'
```

Quote the version, not just the name. `emc` values from the C++ line do not compare with 1.x, and
the [Releases](releases.md) page records what changed when.

!!! note

    A citation to this software does not credit the method. The factor count, `h` and the
    information distance come from published results by other people; the library implements them.
    Cite Lempel & Ziv (1976) for the factorization, and the relevant entropy-rate and distance
    papers alongside the software — all of them are on [References](references.md).

## Licence

`lzcomplexity` is MIT licensed. The copyright line reads, verbatim,
`Copyright (c) 2024 Efren Aragon Perez` — unaccented in the file.

The full text is in `LICENSE` at the repository root, and every wheel carries a copy at
`lzcomplexity-<version>.dist-info/licenses/LICENSE`. You may use, modify and redistribute the
library, including commercially, provided the copyright notice and licence text travel with it.
There is no warranty.

## Reporting bugs

Open an issue at **<https://github.com/pleros-ai/lzcomplexity/issues>**.

Both binaries print `Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.` in their
`--help` banner. The issue tracker is the better route: it is public, searchable, and linked from
the package metadata.

A report that can be reproduced gets fixed faster. Include:

| Item | How to get it |
|---|---|
| Version | `lz.__version__`, or `--version` on either binary |
| Platform | OS, architecture, and Python version |
| Input | The sequence, or a script that regenerates it |
| Expected vs actual | The number you got and the number you expected |

The two CLIs report their versions in different formats:

<div class="lz-run" markdown>

```console
$ lzcomplexity --version
 [ Info ] v1.0.1
$ lzdistance --version
 [ Info ] Version of lzdistance: v1.0.1
```

</div>

The discrete part of every computation is deterministic: the same bytes give the same factor count,
the same factor boundaries and the same shuffle permutation, in every process and on every platform.
Floats carry one caveat — `ln` comes from the platform's libm, and the CLI and Python entropy paths
order their arithmetic differently, so the last bit of `h` or `emc` can differ by a few ULP. Report a
discrepancy larger than that. [Determinism](determinism.md) sets out exactly what is guaranteed.

Release notes and the changelog for every version are on [Releases](releases.md).
