<div align="center">
  <h2>lzcomplexity</h2>
  <p>Randomness, structure and similarity in symbolic sequences.</p>

[![CI](https://github.com/pleros-ai/lzcomplexity/actions/workflows/ci.yml/badge.svg?branch=rust-backend)](https://github.com/pleros-ai/lzcomplexity/actions/workflows/ci.yml)
[![Documentation](https://img.shields.io/badge/docs-online-0071e3)](https://pleros-ai.github.io/lzcomplexity/guide/)
[![PyPI](https://img.shields.io/pypi/v/lzcomplexity?color=0071e3)](https://pypi.org/project/lzcomplexity/)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23531093.svg)](https://zenodo.org/doi/10.5281/zenodo.23531093)
[![Python](https://img.shields.io/badge/Python-3.9%2B-5e6b7a?logo=python&logoColor=white)](https://pypi.org/project/lzcomplexity/)
[![MIT](https://img.shields.io/badge/license-MIT-5e6b7a)](LICENSE)

**[Documentation](https://pleros-ai.github.io/lzcomplexity/guide/)** ·
**[Python API](https://pleros-ai.github.io/lzcomplexity/api/python/)** ·
**[Command line](https://pleros-ai.github.io/lzcomplexity/cli/)** ·
**[Playground](https://pleros-ai.github.io/lzcomplexity/#try-it)**

</div>

Explore how a sequence balances unpredictability and organization, or compare
the patterns shared by different sequences. `lzcomplexity` brings Lempel–Ziv
analysis to Python and standalone command-line tools, powered by a Rust core.

![Binary LZ76 factorization: 1·0·001·1001·1101. Curved arrows span each factor; the highlighted prefix 100 crosses earlier factor boundaries.](docs/assets/sequence-workflow.svg)

| Measure | What you can explore |
|---|---|
| Entropy density **h** | Unpredictability, the production of new information and freedom of choice within a sequence. |
| Effective measure complexity **E** | Organization and correlations across different ranges of a sequence. |
| Information distance **d** | Similarity between sequences through the patterns they share. |

Use h and E together to distinguish changes in randomness from changes in
organization. Use d to compare recordings, texts or symbolic trajectories.

## Start in Python

```sh
pip install lzcomplexity
```

```python
import lzcomplexity as lz

sequence = "00010111" * 256
other = "01010101" * 256

h = lz.h(sequence, log_base=2)                       # Randomness
E, increments = lz.emc(sequence, max_block_size=16)  # Structure across ranges
d = lz.nid(sequence, other)                         # Distance between sequences
```

Pass text or bytes directly. For categorical data, assign one byte to each
symbol; for continuous signals, first choose a symbolic representation.
The [Python reference](https://pleros-ai.github.io/lzcomplexity/api/python/)
documents each function. The separate [usage cheatsheet](https://pleros-ai.github.io/lzcomplexity/guide/quick-reference/)
collects the common calls.

## From the terminal

Download the standalone tools from [GitHub Releases](https://github.com/pleros-ai/lzcomplexity/releases).

```sh
lzcomplexity recording.txt -F text -l 2  # Analyze a sequence; write a JSON report
lzdistance sequences.txt -I TXT        # Compare one sequence per line
```

The [command-line guide](https://pleros-ai.github.io/lzcomplexity/cli/)
covers file formats, batch analysis and distance matrices, with a separate
[usage cheatsheet](https://pleros-ai.github.io/lzcomplexity/cli/quick-reference/).

## Research applications

These studies show the kinds of scientific questions that entropy, structure
and distance can help investigate:

| Application | Explore |
|---|---|
| **Nonlinear dynamics** | Transitions between collective patterns and chaos in [coupled Adler-type oscillators](https://doi.org/10.1103/PhysRevE.107.044212) (*Physical Review E*, 2023). |
| **Brain activity** | Patterns and connectivity across brain regions during cognitive tasks in [task-based fMRI](https://arxiv.org/abs/2507.04442) (arXiv preprint, 2025). |
| **Written language** | The balance between innovation and context at [different levels of text organization](https://doi.org/10.1371/journal.pone.0214863) (*PLOS ONE*, 2019). |

See the [worked examples](https://pleros-ai.github.io/lzcomplexity/recipes/dynamical-systems/)
and [research references](https://pleros-ai.github.io/lzcomplexity/project/references/)
to develop an analysis for your own data.

## Authors & citation

- **Efrén Aragón Pérez** — creator and principal author.
- **Daniel Estévez Moya** — Rust implementation.
- **Ernesto Estévez Rams** — scientific foundations.

Aragón-Pérez, E., & Estevez-Rams, E. (2025). **Lzcomplexity: Entropy Measurement
Library.** *Revista Cubana de Física*, **42**(2), 80–86.
[Article](https://www.revistacubanadefisica.org/index.php/rcf/article/view/31) ·
[Software DOI](https://zenodo.org/doi/10.5281/zenodo.23531093) ·
[BibTeX](https://pleros-ai.github.io/lzcomplexity/project/authors/)

Released under the [MIT license](LICENSE).
