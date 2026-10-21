# Authors and citation

`lzcomplexity` is developed by the Pleros group. The repository credits:

- **Efrén Aragón Pérez** — principal author and creator of the original implementation.
- **Daniel Estévez Moya** — author of the Rust rewrite used by the current backend.
- **Ernesto Estévez Rams** — from the research lineage of the original C++ implementation.

The current package metadata records these three authors in that order. The older C++ implementation remains on the repository's `main` branch; current Rust/Python documentation follows `rust-backend`.

## Cite the library paper

The library has a published reference:

> Aragón-Pérez, E., & Estevez-Rams, E. (2025). Lzcomplexity: Entropy Measurement Library. *Revista Cubana de Física*, **42**(2), 80–86.

The [journal record](https://www.revistacubanadefisica.org/index.php/rcf/article/view/31) provides the article and citation exports. It describes the library's research and implementation lineage; use this site's API reference for the current Rust-backed Python interface.

```bibtex
@article{aragon_perez_lzcomplexity_2025,
  author  = {Arag{\'o}n-P{\'e}rez, E. and Estevez-Rams, E.},
  title   = {Lzcomplexity: Entropy Measurement Library},
  journal = {Revista Cubana de F{\'i}sica},
  year    = {2025},
  volume  = {42},
  number  = {2},
  pages   = {80--86},
  url     = {https://www.revistacubanadefisica.org/index.php/rcf/article/view/31}
}
```

## Record the software version too

A paper citation does not uniquely identify the executable that produced a number. Record the installed version and, for a development build, its source commit and backend.

```python
import lzcomplexity as lz
print(lz.__version__)
```

The following software citation uses `2.0.0` as an example. Replace it with the version you actually used:

```bibtex
@software{lzcomplexity_software,
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

For a methods section, add the byte encoding, preprocessing/symbolization, analyzed lengths, explicit logarithm base, shuffle cap and whether results came from Python, Rust or a CLI report. Distinguish the EMC-like shuffle score from theoretical EMC.

## Cite the method relevant to your analysis

The software paper credits the implementation. The underlying methods also have their own research literature: Lempel and Ziv for parsing, entropy-rate work for the normalization, shuffle-based analyses for the qualitative complexity measure, and information-distance literature for sequence comparison. [References](references.md) groups those sources and distinguishes theoretical background from this implementation's definition.

## License

The current repository is MIT licensed. Its [LICENSE](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/LICENSE) records `Copyright (c) 2024 Efren Aragon Perez` and contains the applicable permission and warranty terms.

The supplied [historical manual](manual.md) has its own version and notices. Historical platform, API and performance descriptions should be read in that version's context.

## Contributing and reporting issues

Use the [GitHub issue tracker](https://github.com/pleros-ai/lzcomplexity/issues) for reproducible problems. Include:

- installed version or source commit, OS, architecture and Python/Rust version;
- the exact input bytes or a small generator;
- options, byte length, logarithm base and shuffle cap;
- expected behavior and actual output.

For a documentation correction, link the page and give the conflicting code or reference. Scientific-method changes need an explicit definition and comparison goal, not only a new label for an existing score.
