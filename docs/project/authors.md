# Authors and citation

*Who wrote lzcomplexity, where it came from, how to cite it, and where to report bugs.*

## The people

- **Efrén Aragón Pérez** — principal author and creator. He wrote the original C++ implementation that everything here descends from, he is the copyright holder on the MIT licence, and he is the first author listed in the package metadata.
- **Ernesto Estévez Rams** — second author in the 1.0 package metadata, from the research lineage of the original C++ implementation.
- **Daniel Estévez Moya** — author of the Rust rewrite that is the current production backend: the `lzcomplexity-core` crate, the PyO3 bindings, and the two command-line tools shipped in 1.0.

The three names are not recorded uniformly across the repository. What each file actually says, verbatim and unaccented as stored:

| Where | Names recorded |
|---|---|
| `Cargo.toml`, `workspace.package.authors` | Efren Aragon Perez, Ernesto Estevez Rams |
| `pyproject.toml`, `project.authors` | Efren Aragon, Ernesto Estevez Rams |
| Wheel `METADATA`, `Author-email` | Efren Aragon, Ernesto Estevez Rams |
| `LICENSE`, copyright line | Efren Aragon Perez |
| `README.md`, BibTeX entry | Efren Aragon-Perez |

Daniel Estévez Moya is not named in any of those files. His contribution is recorded in the git history of the `rust-backend` branch, starting at commit `96c583f`, *rewrite the library backend in Rust with a redesigned Python API*.

!!! note
    The C++ releases named only one author: `setup.py` on the `main` branch sets `author="Efren Aragon"`. Ernesto Estévez Rams first appears in package metadata with the Rust rewrite, not before it.

## Where the library came from

`lzcomplexity` started as a C++ library for complexity analysis using entropic magnitudes — entropy density, effective measure complexity and information distance, all derived from Lempel–Ziv 76 factorization. That implementation still lives on the repository's `main` branch, with Python bindings built on nanobind.

The rewrite landed in 0.11.0 and the API settled at 1.0.0. The Python bindings moved to PyO3, the CLI was rebuilt on the same core, and the shuffle PRNG stopped drawing from a shared `std::random_device`-seeded `mt19937`: it is now a per-block `ChaCha8Rng` seeded from a hash of the sequence itself, so the same input returns the same `emc` on every run and at any thread count. There is no user-facing seed parameter.

Factor counts, factor boundaries and `nid` match the C++ backend exactly; `h` matches at equal log base, and `emc` deliberately does not match, because the two backends draw different surrogates. See [Rust vs C++](cpp-parity.md) and [Determinism](determinism.md).

Spectral analysis (`spectral.psd`, `spectral.entropy`, `spectral.semc`) was removed in the same 0.11.0 change and moved to a separate package. It is not part of the 1.0 API, and the CI wheel smoke test asserts that `spectral` and `metrics` stay absent.

<hr class="lz-tickrule">

## Citing lzcomplexity

There is no `CITATION.cff` in the repository. The only citation the project publishes is the BibTeX entry in `README.md`, reproduced here verbatim:

```bibtex
@software{lzcomplexity_2025,
  title={lzcomplexity: an entropy measurement library},
  author={Efren Aragon-Perez},
  url={https://github.com/pleros-ai/lzcomplexity},
  year={2025}
}
```

Plain text:

> Aragon-Perez, E. (2025). *lzcomplexity: an entropy measurement library* [Computer software]. https://github.com/pleros-ai/lzcomplexity

That entry names one author and predates the 1.0.0 release, so it carries neither Ernesto Estévez Rams — who is in the package metadata — nor a version field. Two safe additions, neither of which changes an authorship claim:

| Field | Value | Why |
|---|---|---|
| `version` | the string from `lz.__version__` | Pins the build that produced your numbers |
| `note` | `Rust backend` | Distinguishes 1.0 from the C++ line, whose `emc` differs |

If the author list matters for your submission, ask the maintainers on the issue tracker rather than reconstructing one from the metadata. Names in `Cargo.toml` and `README.md` disagree today.

The installed version is on the module, so a methods section can quote the exact build:

```pycon
>>> import lzcomplexity as lz
>>> lz.__version__
'1.0.0'
```

!!! note
    A citation to this software does not credit the method. The factor count, `h` and the information distance come from published results by other people; the library implements them. Cite Lempel & Ziv (1976) for the factorization, and the relevant entropy-rate and distance papers alongside the software. The [References](references.md) page is still being written; `README.md` lists Lempel & Ziv (1976) and Kontoyiannis et al. (1998) today.

## Licence

`lzcomplexity` is MIT licensed. The copyright line reads, verbatim, `Copyright (c) 2024 Efren Aragon Perez` — unaccented in the file.

The full text is in `LICENSE` at the repository root, and every wheel carries a copy at `lzcomplexity-<version>.dist-info/licenses/LICENSE`. You may use, modify and redistribute the library, including commercially, provided the copyright notice and licence text travel with it. There is no warranty.

## Reporting bugs

Open an issue at **<https://github.com/pleros-ai/lzcomplexity/issues>**.

Both binaries print `Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.` in their `--help` banner. The issue tracker is the better route: it is public, searchable, and linked from the package metadata.

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
 [ Info ] v1.0.0
$ lzdistance --version
 [ Info ] Version of lzdistance: v1.0.0
```

</div>

The discrete part of every computation is deterministic: the same bytes give the same factor count, the same factor boundaries and the same shuffle permutation, in every process and on every platform. Floats carry one caveat — `ln` comes from the platform's libm, and the CLI and Python entropy paths order their arithmetic differently, so the last bit of `h` or `emc` can differ by a few ULP. Report a discrepancy larger than that. [Determinism](determinism.md) sets out exactly what is guaranteed.

Release notes and the changelog for every version are on [Releases](releases.md).
