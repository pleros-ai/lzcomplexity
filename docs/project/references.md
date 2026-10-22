# References

Research behind the library's methods, together with applications to language,
brain activity, phase transitions and nonlinear dynamics.

## The library

Aragón-Pérez, E., & Estevez-Rams, E. (2025). **Lzcomplexity: Entropy Measurement Library.** *Revista Cubana de Física*, **42**(2), 80–86. [Article and citation](https://www.revistacubanadefisica.org/index.php/rcf/article/view/31).

For software citations and BibTeX, see [authors and citation](authors.md).
The [reference manual](manual.md) provides an extended introduction.

## Parsing and entropy rate

Lempel, A., & Ziv, J. (1976). **On the complexity of finite sequences.** *IEEE Transactions on Information Theory*, **22**(1), 75–81. [10.1109/TIT.1976.1055501](https://doi.org/10.1109/TIT.1976.1055501).

Introduces the parsing approach used by [LZ76 factorization](../concepts/lz76.md).

Ziv, J. (1978). **Coding theorems for individual sequences.** *IEEE Transactions on Information Theory*, **24**(4), 405–412. [10.1109/TIT.1978.1055911](https://doi.org/10.1109/TIT.1978.1055911).

Coding-theoretic foundations of individual-sequence complexity and its
relationship to entropy rate.

## Structure, memory and entropy growth

Grassberger, P. (1986). **Toward a quantitative theory of self-generated complexity.** *International Journal of Theoretical Physics*, **25**, 907–938. [10.1007/BF00668821](https://doi.org/10.1007/BF00668821).

A foundation for studying complexity through the organization of pattern ensembles.

Crutchfield, J. P., & Feldman, D. P. (2003). **Regularities unseen, randomness observed: Levels of entropy convergence.** *Chaos*, **13**(1), 25–54. [10.1063/1.1530990](https://doi.org/10.1063/1.1530990). [Preprint](https://arxiv.org/abs/cond-mat/0102181).

Examines entropy growth, predictability and memory across observation lengths.

Melchert, O., & Hartmann, A. K. (2015). **Analysis of the phase transition in the two-dimensional Ising ferromagnet using a Lempel-Ziv string-parsing scheme and black-box data-compression utilities.** *Physical Review E*, **91**, 023306. [10.1103/PhysRevE.91.023306](https://doi.org/10.1103/PhysRevE.91.023306).

Applies LZ-based analysis to a phase transition in the Ising model.

## Written language

Estevez-Rams, E., Mesa-Rodriguez, A., & Estevez-Moya, D. (2019). **Complexity-entropy analysis at different levels of organisation in written language.** *PLOS ONE*, **14**(5), e0214863. [10.1371/journal.pone.0214863](https://doi.org/10.1371/journal.pone.0214863).

Studies innovation and context at different organizational levels of text,
providing an application of entropy and structural complexity together.

## Brain activity

Mesa-Rodríguez, A., Estevez-Rams, E., & Kantz, H. (2025). **Entropy measures as indicators of connectivity paths in the human brain.** arXiv preprint, arXiv:2507.04442. [Paper and versions](https://arxiv.org/abs/2507.04442).

Uses entropy, complexity and Lempel–Ziv distance to explore patterns and
connectivity in task-based fMRI during motor, working-memory, emotion-recognition
and language tasks.

## Information distance

Li, M., Chen, X., Li, X., Ma, B., & Vitányi, P. M. B. (2004). **The similarity metric.** *IEEE Transactions on Information Theory*, **50**(12), 3250–3264. [10.1109/TIT.2004.838101](https://doi.org/10.1109/TIT.2004.838101). [Author-hosted paper](https://homepages.cwi.nl/~paulv/papers/similarity.pdf).

Theoretical background for comparing objects through shared information.
The library's formula is given in [information distance](../concepts/nid.md).

## Nonlinear dynamics

Estevez-Rams, E., Estevez-Moya, D., Garcia-Medina, K., & Lora-Serrano, R. (2019). **Computational capabilities at the edge of chaos for one dimensional systems undergoing continuous transitions.** *Chaos*, **29**, 043105. [10.1063/1.5062364](https://doi.org/10.1063/1.5062364).

Estevez-Moya, D., Estevez-Rams, E., & Kantz, H. (2023). **Complexity and transition to chaos in coupled Adler-type oscillators.** *Physical Review E*, **107**, 044212. [10.1103/PhysRevE.107.044212](https://doi.org/10.1103/PhysRevE.107.044212).

These studies use entropy and complexity to examine changes in dynamical behavior.
See the [dynamical systems recipe](../recipes/dynamical-systems.md) for a worked analysis.

## Implementation

- [Rust core](https://github.com/pleros-ai/lzcomplexity/tree/rust-backend/crates/lzcomplexity-core/src).
- [libdivsufsort](https://github.com/y-256/libdivsufsort), used for suffix-array construction.
- [Rayon](https://docs.rs/rayon/), used for task parallelism.

[Inside the implementation](../concepts/implementation.md) follows the calculation
from input bytes to results.
