# References

The current implementation is defined by its code and the [operational shuffle description](../concepts/emc.md). The sources below provide the scientific background, research lineage and implementation context. A reference to an ideal information-theoretic quantity does not establish that every finite factor-count ratio inherits its guarantees.

The [historical manual](manual.md) is the starting reference for these documentation chapters. Its older APIs, construction backend and empirical figures are identified as historical rather than copied into current-release claims.

## Software and its research lineage

### The library paper

Aragón-Pérez, E., & Estevez-Rams, E. (2025). **Lzcomplexity: Entropy Measurement Library.** *Revista Cubana de Física*, **42**(2), 80–86. [Journal record and article](https://www.revistacubanadefisica.org/index.php/rcf/article/view/31).

This is the library publication. The journal record has English metadata and the article is available in Spanish. Cite it together with the actual software version; [Authors and citation](authors.md) provides BibTeX.

### Written-language analysis

Estevez-Rams, E., Mesa-Rodriguez, A., & Estevez-Moya, D. (2019). **Complexity-entropy analysis at different levels of organisation in written language.** *PLOS ONE*, **14**(5), e0214863. [10.1371/journal.pone.0214863](https://doi.org/10.1371/journal.pone.0214863).

A research example of LZ-based entropy and multiscale shuffle contrasts applied to organized symbolic data. It explicitly distinguishes the shuffle quantity from exact excess entropy. For current package arithmetic, use the [EMC-like method page](../concepts/emc.md).

## Parsing and entropy rate

### Lempel–Ziv 76

Lempel, A., & Ziv, J. (1976). **On the complexity of finite sequences.** *IEEE Transactions on Information Theory*, **22**(1), 75–81. [10.1109/TIT.1976.1055501](https://doi.org/10.1109/TIT.1976.1055501).

The foundational finite-sequence parsing paper. The library's [factorization page](../concepts/lz76.md) states its completed-phrase and constant-input conventions explicitly, rather than assuming all implementations return the same short-string count.

### Individual-sequence coding

Ziv, J. (1978). **Coding theorems for individual sequences.** *IEEE Transactions on Information Theory*, **24**(4), 405–412. [10.1109/TIT.1978.1055911](https://doi.org/10.1109/TIT.1978.1055911). [Author's institutional record](https://cris.technion.ac.il/en/publications/coding-theorems-for-individual-sequences/).

Part of the asymptotic coding background motivating normalized LZ complexity. It is distinct from Ziv and Lempel's different 1978 paper on variable-rate coding. An asymptotic source result does not supply a fixed finite-sample accuracy threshold for arbitrary recordings.

## EMC, memory and shuffle-based analysis

### Effective measure complexity

Grassberger, P. (1986). **Toward a quantitative theory of self-generated complexity.** *International Journal of Theoretical Physics*, **25**, 907–938. [10.1007/BF00668821](https://doi.org/10.1007/BF00668821).

Conceptual background for describing complexity using properties of ensembles of patterns. The operational shuffle score in this package is described as qualitatively similar to EMC, rather than identified with an exact source-level value.

### Entropy convergence

Crutchfield, J. P., & Feldman, D. P. (2003). **Regularities unseen, randomness observed: Levels of entropy convergence.** *Chaos*, **13**(1), 25–54. [10.1063/1.1530990](https://doi.org/10.1063/1.1530990). [Author preprint](https://arxiv.org/abs/cond-mat/0102181).

Theoretical relationships among block entropy growth, entropy rate and source memory. These relationships explain why source-level entropy and a finite shuffle intervention need to be distinguished.

### Shuffle-based complexity measurements

Melchert, O., & Hartmann, A. K. (2015). **Analysis of the phase transition in the two-dimensional Ising ferromagnet using a Lempel-Ziv string-parsing scheme and black-box data-compression utilities.** *Physical Review E*, **91**, 023306. [10.1103/PhysRevE.91.023306](https://doi.org/10.1103/PhysRevE.91.023306).

An empirical comparison of LZ-derived and block-entropy observables. It is background for shuffle-based analyses, not a universal calibration of the current package score.

## Information distance

Li, M., Chen, X., Li, X., Ma, B., & Vitányi, P. M. B. (2004). **The similarity metric.** *IEEE Transactions on Information Theory*, **50**(12), 3250–3264. [10.1109/TIT.2004.838101](https://doi.org/10.1109/TIT.2004.838101). [Author-hosted paper](https://homepages.cwi.nl/~paulv/papers/similarity.pdf).

Theoretical normalized information distance and the motivation for practical approximations. The package's [LZ-count formula](../concepts/nid.md) is computable and can have a nonzero self-distance; it must not be assumed to satisfy every theoretical metric axiom.

## Examples from nonlinear dynamics

Estevez-Rams, E., Estevez-Moya, D., Garcia-Medina, K., & Lora-Serrano, R. (2019). **Computational capabilities at the edge of chaos for one dimensional systems undergoing continuous transitions.** *Chaos*, **29**, 043105. [10.1063/1.5062364](https://doi.org/10.1063/1.5062364).

Estevez-Moya, D., Estevez-Rams, E., & Kantz, H. (2023). **Complexity and transition to chaos in coupled Adler-type oscillators.** *Physical Review E*, **107**, 044212. [10.1103/PhysRevE.107.044212](https://doi.org/10.1103/PhysRevE.107.044212).

These studies provide research context for the manual's oscillator examples. A result for those systems does not establish that every observed peak in a different dataset identifies a transition to chaos.

## Implementation sources

For exact current behavior, consult:

- [The core source](https://github.com/pleros-ai/lzcomplexity/tree/rust-backend/crates/lzcomplexity-core/src), including parsing, normalization, sampler boundaries, seeds, fitting and count-based diagnostics.
- [libdivsufsort](https://github.com/y-256/libdivsufsort), the suffix-array implementation wrapped by the Rust dependency.
- [Rayon](https://docs.rs/rayon/), the Rust task-parallelism library.
- [CaPS-SA research](https://doi.org/10.4230/LIPIcs.WABI.2023.16), relevant to the older C++ backend described in the manual, rather than the current Rust suffix-array construction.

[Inside the implementation](../concepts/implementation.md) maps the current stages to source files. Source links follow the maintained branch; record a commit when a study needs an immutable implementation reference.
