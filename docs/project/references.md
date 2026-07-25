# References

*Every source the documentation leans on, grouped by topic, with what each one establishes.*

This is the bibliography for the whole site. Each entry gives the full citation, one line on what it establishes, and — where one exists — the page that rests on it. "Cited on X" means page X names the work; "background for X" means the page uses the result without carrying the citation inline.

Volume numbers, page ranges and DOIs were checked against Crossref or the publisher's own record wherever one exists. The older conference papers and books here carry no DOI and were taken from the citing literature instead. Where a paper could not be read directly — several are paywalled — the entry says so rather than hiding it.

!!! note
    A reference you expected and cannot find here was left out on purpose. The rule for this page is that a citation appears only once its bibliographic details are confirmed; anything whose authors, volume or pages could not be pinned down is absent rather than guessed at. That is why a few results discussed in the recipes are described without a citation attached.

!!! warning
    Copying these citations from secondary sources gives you wrong volumes and wrong page ranges — five known traps are listed at the [bottom of this page](#citation-traps). Four of the five are misprints in the primary literature's own reference lists; the fifth was made here.

<hr class="lz-tickrule">

## LZ76 complexity and its normalisations

The library computes the Lempel–Ziv 76 exhaustive history with self-referential (overlapping) copying, and it counts only **complete** components. When the parse overruns the end of the sequence the trailing component is dropped, so the reported count is one less than the textbook `c(S)` printed in these papers:

```pycon
>>> import lzcomplexity as lz
>>> seq = "0101010101010101"
>>> c, factors = lz.factorization(seq)
>>> c, factors
(2, [0, 1, 2, 17])
>>> c + (1 if factors[-1] > len(seq) else 0)   # textbook c(S)
3
```

That conversion was checked against the Kaspar–Schuster counter over 500 random binary strings and exhaustively over every binary string of length 2 to 16. It matched every one of them **except the constant strings**: a sequence with a single distinct symbol takes a separate code path and returns `1` whatever its length, where the textbook count for length ≥ 2 is `2`. Read this section with that offset in mind — see [LZ76 factorization](../concepts/lz76.md) and [Rust vs C++](cpp-parity.md).

Lempel & Ziv (1976)
:   Lempel, A., & Ziv, J. (1976). On the complexity of finite sequences. *IEEE Transactions on Information Theory*, **22**(1), 75–81. [10.1109/TIT.1976.1055501](https://doi.org/10.1109/TIT.1976.1055501)
:   The founding paper: the drop operator, reproducibility, producibility, the exhaustive history, `c(S)`, and the finite-`n` bound whose slack term the library reports as `epsilon`. Underpins [LZ76 factorization](../concepts/lz76.md) and [Entropy density](../concepts/entropy-density.md). Paywalled; what these docs say about its contents comes from Estevez-Rams et al. (2013) and Constantinescu & Ilie (2007), which quote it.

Ziv & Lempel (1977)
:   Ziv, J., & Lempel, A. (1977). A universal algorithm for sequential data compression. *IEEE Transactions on Information Theory*, **23**(3), 337–343. [10.1109/TIT.1977.1055714](https://doi.org/10.1109/TIT.1977.1055714)
:   LZ77 — a sliding-window *codec*, not a complexity measure. Cited on [LZ76 factorization](../concepts/lz76.md) only to say what this library does not implement.

Ziv & Lempel (1978)
:   Ziv, J., & Lempel, A. (1978). Compression of individual sequences via variable-rate coding. *IEEE Transactions on Information Theory*, **24**(5), 530–536. [10.1109/TIT.1978.1055934](https://doi.org/10.1109/TIT.1978.1055934)
:   LZ78 — incremental dictionary parsing. Same role on [LZ76 factorization](../concepts/lz76.md): a different measure that produces different numbers on the same string.

Kaspar & Schuster (1987)
:   Kaspar, F., & Schuster, H. G. (1987). Easily calculable measure for the complexity of spatiotemporal patterns. *Physical Review A*, **36**(2), 842–848. [10.1103/PhysRevA.36.842](https://doi.org/10.1103/PhysRevA.36.842)
:   The paper that moved LZ76 into physics and fixed the binary normaliser `b(n) = n / log₂ n` that `lz.h` divides by. Paywalled, with no published abstract; cite it for the idea and Aboy et al. (2006) for the α-ary formulas. Cited on [LZ76 factorization](../concepts/lz76.md); background for [Entropy density](../concepts/entropy-density.md).

Aboy, Hornero, Abásolo & Álvarez (2006)
:   Aboy, M., Hornero, R., Abásolo, D., & Álvarez, D. (2006). Interpretation of the Lempel-Ziv complexity measure in the context of biomedical signal analysis. *IEEE Transactions on Biomedical Engineering*, **53**(11), 2282–2288. [10.1109/TBME.2006.883696](https://doi.org/10.1109/TBME.2006.883696)
:   Reproduces the chain from the LZ76 bound to `b(n) = n / log_α n` to `C(n) = c(n)/b(n)` explicitly (its eqs. 3–6), and reports the median as the prevailing binarisation threshold. Cited on [LZ76 factorization](../concepts/lz76.md) and [EEG and neural time-series](../recipes/neuro.md).

Hu, Gao & Principe (2006)
:   Hu, J., Gao, J., & Principe, J. C. (2006). Analysis of biomedical signals by the Lempel-Ziv complexity: the effect of finite data size. *IEEE Transactions on Biomedical Engineering*, **53**(12 Pt 2), 2606–2609. [10.1109/TBME.2006.883825](https://doi.org/10.1109/TBME.2006.883825)
:   Closed-form LZ complexity for regular and random sequences, derived so that a finite-`n` normalisation can be built. The standard citation for "the finite-size effect is real"; background for [Sequence length and convergence](../concepts/convergence.md).

Constantinescu & Ilie (2007)
:   Constantinescu, S., & Ilie, L. (2007). The Lempel–Ziv complexity of fixed points of morphisms. *SIAM Journal on Discrete Mathematics*, **21**(2), 466–481. [10.1137/050646846](https://doi.org/10.1137/050646846). Conference version: MFCS 2006, LNCS **4162**, 280–291. [10.1007/11821069_25](https://doi.org/10.1007/11821069_25)
:   The cleanest combinatorial statement of the exhaustive history: innovative versus reproductive components, uniqueness, and Lemma 3 — the exhaustive history is the *shortest* history. That minimality is what makes `c(S)` canonical rather than an artefact of greedy parsing. Cited on [LZ76 factorization](../concepts/lz76.md).

Zhang, Hao, Zhou & Chang (2009)
:   Zhang, Y., Hao, J., Zhou, C., & Chang, K. (2009). Normalized Lempel-Ziv complexity and its application in bio-sequence analysis. *Journal of Mathematical Chemistry*, **46**, 1203–1212. [10.1007/s10910-008-9512-2](https://doi.org/10.1007/s10910-008-9512-2)
:   Alphabet-aware normalised LZ complexity for biological sequences — the reference for why the log base has to match the alphabet. Background for [Alphabets and log bases](../concepts/alphabets.md).

Estevez-Rams, Lora Serrano, Aragón Fernández & Brito Reyes (2013)
:   Estevez-Rams, E., Lora Serrano, R., Aragón Fernández, B., & Brito Reyes, I. (2013). On the non-randomness of maximum Lempel-Ziv complexity sequences of finite size. *Chaos*, **23**(2), 023118. [10.1063/1.4808251](https://doi.org/10.1063/1.4808251). Preprint: [arXiv:1311.0546](https://arxiv.org/abs/1311.0546)
:   States the LZ76 bound and its slack term `ε(N)` in usable form, shows that `ε(N)` does not reach 0.1 until `N ≈ 4 × 10⁵⁰` — reproducible from the library's own `epsilon` field — and proves that maximum-complexity strings are neither random nor stationary. Source of the worked example `010011101101100 → 0.1.00.11.101.101100` used on [LZ76 factorization](../concepts/lz76.md); background for the bias discussion on [Sequence length and convergence](../concepts/convergence.md).

Nunes, Estevez-Rams, Aragón Fernández & Lora Serrano (2013)
:   Nunes, C. A. J., Estevez-Rams, E., Aragón Fernández, B., & Lora Serrano, R. (2013). *Properties of maximum Lempel-Ziv complexity strings.* [arXiv:1311.0822](https://arxiv.org/abs/1311.0822) (preprint; no journal version located)
:   Length profile and noise sensitivity of maximum-complexity strings, and the restatement of Ziv's theorem these docs rely on. Its §2 worked example for `11011101000011` prints a factorisation that violates the paper's own condition — the count `C = 6` is right, the parse shown is not — which is why the docs use the 2013 *Chaos* example instead.

<hr class="lz-tickrule">

## Entropy-rate estimation

`lz.h` returns `c(S) · log_k(n) / n`. The theorem that licenses reading it as an entropy rate is Ziv (1978), not Kontoyiannis et al. (1998) — the latter covers a different family of estimators under a stronger hypothesis. Both are listed here; [Entropy density](../concepts/entropy-density.md) explains the distinction.

Ziv (1978)
:   Ziv, J. (1978). Coding theorems for individual sequences. *IEEE Transactions on Information Theory*, **24**(4), 405–412. [10.1109/TIT.1978.1055911](https://doi.org/10.1109/TIT.1978.1055911)
:   The convergence result behind `lz.h`: for the output of an ergodic source, `lim sup_N C[u(1,N)] / (N / log N) = h` almost surely. Note the `lim sup`, and that it is asymptotic. Paywalled; the docs rely on the restatement in Nunes et al. (2013). Do not confuse it with its sibling, Ziv & Lempel (1978) above.

Wyner & Ziv (1989)
:   Wyner, A. D., & Ziv, J. (1989). Some asymptotic properties of the entropy of a stationary ergodic data source with applications to data compression. *IEEE Transactions on Information Theory*, **35**(6), 1250–1258. [10.1109/18.45281](https://doi.org/10.1109/18.45281)
:   Match lengths grow as `L_N / log N → 1/H`; proved in probability here, with the almost-sure version conjectured. Background for [Sequence length and convergence](../concepts/convergence.md).

Shields (1993)
:   Shields, P. C. (1993). Universal redundancy rates do not exist. *IEEE Transactions on Information Theory*, **39**(2), 520–524. [10.1109/18.212281](https://doi.org/10.1109/18.212281). Follow-up: Shields, P. C., & Weiss, B. (1995). Universal redundancy rates for the class of B-processes do not exist. *Ibid.*, **41**(2), 508–512. [10.1109/18.370156](https://doi.org/10.1109/18.370156)
:   Why "how many samples do I need?" has no distribution-free answer: for every claimed error bound there is a stationary ergodic process that violates it. The reason every sample-size figure on [Sequence length and convergence](../concepts/convergence.md) is labelled a rule of thumb.

Ornstein & Weiss (1993)
:   Ornstein, D. S., & Weiss, B. (1993). Entropy and data compression schemes. *IEEE Transactions on Information Theory*, **39**(1), 78–83. [10.1109/18.179344](https://doi.org/10.1109/18.179344)
:   The return-time theorem `(log R_n)/n → H` almost surely, and the LZ block-size statement — most of a length-`n` string parses into blocks of size about `(log n)/h`, which is `c(n) · log n / n → h` in one line. Cited alongside Ziv (1978) on [Entropy density](../concepts/entropy-density.md).

Wyner & Wyner (1995)
:   Wyner, A. D., & Wyner, A. J. (1995). Improved redundancy of a version of the Lempel–Ziv algorithm. *IEEE Transactions on Information Theory*, **41**(3), 723–731. [10.1109/18.382018](https://doi.org/10.1109/18.382018)
:   Sliding-window LZ has redundancy `O(log log n / log n)`; a modified version reaches `O(1/log n)`. One of the results behind the slow-convergence story.

Schürmann & Grassberger (1996)
:   Schürmann, T., & Grassberger, P. (1996). Entropy estimation of symbol sequences. *Chaos*, **6**(3), 414–427. [10.1063/1.166191](https://doi.org/10.1063/1.166191). Preprint: [arXiv:cond-mat/0203436](https://arxiv.org/abs/cond-mat/0203436)
:   Writes the estimator as `h = lim log N / ⟨L(w)⟩`, finds convergence "very slowly with `N`" in every non-trivial case, and proposes the extrapolation ansatz `ĥ_N ≈ h + c · log N / N^γ` (their eq. 28). Background for [Sequence length and convergence](../concepts/convergence.md).

Savari (1997)
:   Savari, S. A. (1997). Redundancy of the Lempel–Ziv incremental parsing rule. *IEEE Transactions on Information Theory*, **43**(1), 9–21. [10.1109/18.567642](https://doi.org/10.1109/18.567642)
:   The LZ78 counterpart of the redundancy results above: `O(log log n / log n)` in general, `O(1/ln n)` for memoryless and unifilar sources.

Kontoyiannis, Algoet, Suhov & Wyner (1998)
:   Kontoyiannis, I., Algoet, P. H., Suhov, Yu. M., & Wyner, A. J. (1998). Nonparametric entropy estimation for stationary processes and random fields, with applications to English text. *IEEE Transactions on Information Theory*, **44**(3), 1319–1327. [10.1109/18.669425](https://doi.org/10.1109/18.669425)
:   Consistency for Cesàro averages of match lengths — a **different** estimator family, whose proof needs Doeblin mixing on top of stationarity and ergodicity. The repository README cites it for the `lz.h` convergence claim; that attribution is wrong, and [Entropy density](../concepts/entropy-density.md) says so. Cite it for the competing match-length estimators.

Politis & Romano (1994)
:   Politis, D. N., & Romano, J. P. (1994). The stationary bootstrap. *Journal of the American Statistical Association*, **89**(428), 1303–1313.
:   The resampling scheme to reach for when you need an actual confidence interval on an entropy-rate estimate. Neither `normal_error` nor `poison_error` is a substitute; see [Python API](../api/python.md).

Amigó, Szczepański, Wajnryb & Sanchez-Vives (2004)
:   Amigó, J. M., Szczepański, J., Wajnryb, E., & Sanchez-Vives, M. V. (2004). Estimating the entropy rate of spike trains via Lempel–Ziv complexity. *Neural Computation*, **16**(4), 717–736. [10.1162/089976604322860677](https://doi.org/10.1162/089976604322860677)
:   LZ76 as an entropy-rate estimator on neural spike trains. Background for [EEG and neural time-series](../recipes/neuro.md).

Amigó & Kennel (2006)
:   Amigó, J. M., & Kennel, M. B. (2006). Variance estimators for the Lempel–Ziv entropy rate estimator. *Chaos*, **16**(4), 043102. [10.1063/1.2347102](https://doi.org/10.1063/1.2347102)
:   The variance formula `σ̂ = ĥ^{3/2} · s / sqrt(N · log_k N)`, quoted here from Lesne, Blanc & Pezard (2009) eq. (25), which attributes it to this paper. The library's `normal_error` divides by `sqrt(N / log_k N)` instead: the inner ratio is inverted, which makes the field larger than the published formula by a factor of `log_k N`, and the `s` it uses is not a standard deviation. That is why [Python API](../api/python.md) documents the field as a heuristic and not as an error bar. Paywalled; not read directly.

Gao, Kontoyiannis & Bienenstock (2008)
:   Gao, Y., Kontoyiannis, I., & Bienenstock, E. (2008). Estimating the entropy of binary time series: methodology, some theory and a simulation study. *Entropy*, **10**(2), 71–99. [10.3390/entropy-e10020071](https://doi.org/10.3390/entropy-e10020071). Preprint: [arXiv:0802.4363](https://arxiv.org/abs/0802.4363)
:   Open access, and the most useful practical source on this list: bias is `O(1/log n)`, variance is `O(1/k)` — `k` being the number of match positions, not the alphabet size — and "the main source of error is the bias" for every estimator tested. The published `O(1/log n)` law that [Sequence length and convergence](../concepts/convergence.md) checks its own measured bias against.

Lesne, Blanc & Pezard (2009)
:   Lesne, A., Blanc, J.-L., & Pezard, L. (2009). Entropy estimation of very short symbolic sequences. *Physical Review E*, **79**, 046208. [10.1103/PhysRevE.79.046208](https://doi.org/10.1103/PhysRevE.79.046208)
:   Studies `N ≤ 10³` deliberately, concludes that LZ76 "is to be preferred in the first step and remains the best estimator for highly correlated sequences" while block entropy wins at high entropy, and gives the effective-length rule `N_eff = N · h / ln k`. Cited on [Sequence length and convergence](../concepts/convergence.md) and [The route to chaos](../recipes/dynamical-systems.md).

Nemenman, Shafee & Bialek (2002)
:   Nemenman, I., Shafee, F., & Bialek, W. (2002). Entropy and inference, revisited. *Advances in Neural Information Processing Systems 14*, MIT Press, 471–478. [10.7551/mitpress/1120.003.0065](https://doi.org/10.7551/mitpress/1120.003.0065)
:   The NSB estimator, for the undersampled plug-in regime. An alternative rather than a competitor: it estimates block entropies, not the rate.

Paninski (2003)
:   Paninski, L. (2003). Estimation of entropy and mutual information. *Neural Computation*, **15**(6), 1191–1253. Companion: Paninski, L. (2004). Estimating entropy on `m` bins given fewer than `m` samples. *IEEE Transactions on Information Theory*, **50**(9), 2200–2203.
:   The reference for how badly plug-in block entropies are biased — background for the "why not count blocks directly?" question on [Effective measure complexity](../concepts/emc.md).

Willems, Shtarkov & Tjalkens (1995)
:   Willems, F. M. J., Shtarkov, Y. M., & Tjalkens, T. J. (1995). The context-tree weighting method: basic properties. *IEEE Transactions on Information Theory*, **41**(3), 653–664. [10.1109/18.382012](https://doi.org/10.1109/18.382012)
:   Context-tree weighting — the estimator to use when the source is plausibly a finite-depth tree process and you need a calibrated number rather than a comparative one. Gao et al. (2008) put its bias at `O(log n / n)` against LZ76's `O(1/log n)` — but that rate is derived for tree processes only, so it is not a general statement.

Cover & Thomas (2006)
:   Cover, T. M., & Thomas, J. A. (2006). *Elements of Information Theory* (2nd ed.). Wiley.
:   The textbook definitions of entropy rate, the asymptotic equipartition property and the source-coding theorem that the concept pages assume you have somewhere to look up.

<hr class="lz-tickrule">

## Excess entropy, effective measure complexity and predictive information

These are four community names for one quantity, and the equivalence is a theorem chain rather than a convention — Crutchfield & Feldman (2003), Propositions 6, 7 and 8.

Carry one implementation fact into all of these papers: the library's `emc` sum telescopes. `Σ_l [(H_l − H_{l−1}) − ĥ]` collapses exactly to `mm · g · (C_LZ(shuffled at mm) − C_LZ(original))`, where `g = log_k(N)/N`, so only the largest block size contributes to the total. The per-scale `summands` stay informative; the scalar total does not depend on the intermediate scales at all. See [Effective measure complexity](../concepts/emc.md).

Crutchfield & Packard (1983)
:   Crutchfield, J. P., & Packard, N. H. (1983). Symbolic dynamics of noisy chaos. *Physica D*, **7**, 201–223.
:   Coinage of "excess entropy".

Shaw (1984)
:   Shaw, R. (1984). *The Dripping Faucet as a Model Chaotic System.* Aerial Press.
:   The "stored information" name for the same quantity.

Grassberger (1986)
:   Grassberger, P. (1986). Toward a quantitative theory of self-generated complexity. *International Journal of Theoretical Physics*, **25**(9), 907–938. [10.1007/BF00668821](https://doi.org/10.1007/BF00668821)
:   Defines effective measure complexity as `EMC = Σ_k (h_k − h)`, the quantity `lz.emc` estimates. It also defines a *second*, different quantity — true measure complexity, which is the statistical complexity `C_μ`, not the excess entropy. Basis of [Effective measure complexity](../concepts/emc.md).

Grassberger (1989/1991)
:   Grassberger, P. (1991). Randomness, information, and complexity. In *Proceedings of the 5th Mexican School on Statistical Physics (EMFE 5)*, Oaxtepec 1989, ed. F. Ramos-Gómez, World Scientific. Corrected version: [arXiv:1208.3459](https://arxiv.org/abs/1208.3459)
:   The author's own restatement of EMC, §F and Eqs. (25)–(27). Cite it as 1989/1991; 2012 is only the arXiv posting date.

Lindgren & Nordahl (1988)
:   Lindgren, K., & Nordahl, M. G. (1988). Complexity measures and cellular automata. *Complex Systems*, **2**(4), 409–440.
:   Early use of "effective measure complexity" outside Grassberger's own papers.

Csordás & Szépfalusy (1989)
:   Csordás, A., & Szépfalusy, P. (1989). Singularities in Rényi information as phase transitions in chaotic states. *Physical Review A*, **39**(9), 4767–4777. Companion: Kaufmann, Z. (1991). Characteristic quantities of multifractals — application to the Feigenbaum attractor. *Physica D*, **54**, 75–84.
:   The same quantity again, under the name "reduced Rényi entropy of order 1".

Li (1991)
:   Li, W. (1991). On the relationship between complexity and entropy for Markov chains and regular languages. *Complex Systems*, **5**(4), 381–399.
:   The early statement that `E` is not a function of `h_μ` — the complexity–entropy plane is filled, not a curve. The reason [Effective measure complexity](../concepts/emc.md) prints a one-hump figure but refuses to read it as a function `E(h)`.

Bialek, Nemenman & Tishby (2001)
:   Bialek, W., Nemenman, I., & Tishby, N. (2001). Predictability, complexity, and learning. *Neural Computation*, **13**(11), 2409–2463. [10.1162/089976601753195969](https://doi.org/10.1162/089976601753195969). Preprint: [arXiv:physics/0007070](https://arxiv.org/abs/physics/0007070)
:   Predictive information: "predictability is a deviation from extensivity". Also the source of the naming equivalence, and of the statement that EMC diverges logarithmically at the onset of chaos while staying finite on either side — the behaviour [The route to chaos](../recipes/dynamical-systems.md) sets out to show.

Crutchfield & Feldman (2003)
:   Crutchfield, J. P., & Feldman, D. P. (2003). Regularities unseen, randomness observed: levels of entropy convergence. *Chaos*, **13**(1), 25–54. Preprint: [arXiv:cond-mat/0102181](https://arxiv.org/abs/cond-mat/0102181)
:   The canonical modern treatment. Eq. (48) defines `E = Σ_L [h_μ(L) − h_μ]`; Props. 6–8 prove the three definitions coincide; Prop. 10 gives `E = log₂ p` for a period-`p` source and Prop. 11 gives `E = H(R) − R h_μ` for order-`R` Markov. Every reference value on [Effective measure complexity](../concepts/emc.md) comes from here.

Feldman, McTague & Crutchfield (2008)
:   Feldman, D. P., McTague, C. S., & Crutchfield, J. P. (2008). The organization of intrinsic computation: complexity-entropy diagrams and the diversity of natural information processing. *Chaos*, **18**(4), 043106. [10.1063/1.2991106](https://doi.org/10.1063/1.2991106)
:   The complexity–entropy diagram, the bound `E ≤ R(1 − h_μ)` for binary order-`R` Markov processes, and an explicit refusal to assume that complexity must peak at intermediate entropy.

Estevez-Rams, Lora-Serrano, Nunes & Aragón-Fernández (2015)
:   Estevez-Rams, E., Lora-Serrano, R., Nunes, C. A. J., & Aragón-Fernández, B. (2015). Lempel-Ziv complexity analysis of one dimensional cellular automata. *Chaos*, **25**, 123106. [10.1063/1.4936876](https://doi.org/10.1063/1.4936876). Preprint: [arXiv:1511.08657](https://arxiv.org/abs/1511.08657)
:   The methodological ancestor of this library. Its Eq. (5) is `lz.h` verbatim, Eq. (3) is the LZ76 bound, and Eq. (7) is the Gaussian error estimate that the library's `normal_error` field departs from. Background for [Entropy density](../concepts/entropy-density.md) and [The route to chaos](../recipes/dynamical-systems.md).

Melchert & Hartmann (2015)
:   Melchert, O., & Hartmann, A. K. (2015). Analysis of the phase transition in the two-dimensional Ising ferromagnet using a Lempel-Ziv string-parsing scheme and black-box data-compression utilities. *Physical Review E*, **91**, 023306. Preprint: [arXiv:1406.1354](https://arxiv.org/abs/1406.1354)
:   Independent work applying LZ string parsing across a phase transition. Not by the same group as the papers around it, despite frequently appearing beside them in reference lists.

Estevez-Rams, Mesa Rodriguez & Estevez-Moya (2019)
:   Estevez-Rams, E., Mesa Rodriguez, A., & Estevez-Moya, D. (2019). Complexity-entropy analysis at different levels of organisation in written language. *PLOS ONE*, **14**(5), e0214863. [10.1371/journal.pone.0214863](https://doi.org/10.1371/journal.pone.0214863). Preprint: [arXiv:1903.07416](https://arxiv.org/abs/1903.07416)
:   The direct antecedent of the shuffle-surrogate estimator in this library, and the source of the block-shuffle construction. Its Eq. (4) sums `h_LZ(S_(M)) − h_LZ(S)`; the library multiplies by `M` before taking the discrete derivative, which recovers Grassberger's summand exactly. That is a correction rather than a reimplementation — and it is also what makes the library's sum telescope, which is worked out on [Effective measure complexity](../concepts/emc.md).

von Wegner, Wiemers, Hermann, Tödt, Tagliazucchi & Laufs (2024)
:   von Wegner, F., Wiemers, M., Hermann, G., Tödt, I., Tagliazucchi, E., & Laufs, H. (2024). Complexity measures for EEG microstate sequences: concepts and algorithms. *Brain Topography*, **37**(2), 296–311. [10.1007/s10548-023-01006-2](https://doi.org/10.1007/s10548-023-01006-2)
:   An applied survey computing entropy rate, excess entropy and LZ complexity side by side, and stating plainly that entropy rate and LZ complexity measure randomness while excess entropy peaks at intermediate randomness. Independent support for the framing on [Effective measure complexity](../concepts/emc.md).

<hr class="lz-tickrule">

## Information distance and compression-based distances

`lz.nid` computes `max(C(XY) − C(X), C(YX) − C(Y)) / max(C(X), C(Y))` — the `d*` member of the Otu–Sayood family, which is the normalized information distance template with the Cilibrasi–Vitányi conditional estimator substituted in both directions. See [Information distance](../concepts/nid.md).

Bennett, Gács, Li, Vitányi & Zurek (1998)
:   Bennett, C. H., Gács, P., Li, M., Vitányi, P. M. B., & Zurek, W. H. (1998). Information distance. *IEEE Transactions on Information Theory*, **44**(4), 1407–1423. [10.1109/18.681318](https://doi.org/10.1109/18.681318)
:   Defines `E(x,y) = max{K(x|y), K(y|x)}` and proves it is a universal admissible metric — the object every compression distance is trying to approximate.

Otu & Sayood (2003)
:   Otu, H. H., & Sayood, K. (2003). A new sequence distance measure for phylogenetic tree construction. *Bioinformatics*, **19**(16), 2122–2130. [10.1093/bioinformatics/btg295](https://doi.org/10.1093/bioinformatics/btg295)
:   The LZ-native distance family `d, d*, d1, d1*, d1**`, applied to alignment-free phylogeny. `lz.nid` is their `d*` — evaluated with this library's factor-count convention, not with their `c(S)`. The companion patent (Sayood, K., Otu, H. H., & Hinrichs, S., US 2007/0225918 A1, published 27 Sep 2007) prints all five formulas explicitly. Cited on [Information distance](../concepts/nid.md) and [DNA and FASTA](../recipes/genomics.md).

Li, Chen, Li, Ma & Vitányi (2004)
:   Li, M., Chen, X., Li, X., Ma, B., & Vitányi, P. M. B. (2004). The similarity metric. *IEEE Transactions on Information Theory*, **50**(12), 3250–3264. [10.1109/TIT.2004.838101](https://doi.org/10.1109/TIT.2004.838101)
:   Definition V.2 is NID: `max{K(x|y*), K(y|x*)} / max{K(x), K(y)}`. Remark V.3 explains why the denominator is `max{K(x), K(y)}` and not the length or `K(x,y)`; Lemma V.4 gives `d(x,x) = O(1/K(x))`, which is why `lz.nid(x, x)` is not zero. Also the source of the mammalian-mitochondrial-genome and 52-language experiments.

Keogh, Lonardi & Ratanamahatana (2004)
:   Keogh, E., Lonardi, S., & Ratanamahatana, C. A. (2004). Towards parameter-free data mining. *Proceedings of the 10th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD '04)*, 206–215. [10.1145/1014052.1014077](https://doi.org/10.1145/1014052.1014077)
:   The compression-based dissimilarity measure `CDM(x,y) = C(xy) / (C(x) + C(y))` — another member of the family, and not what `nid` computes.

Cilibrasi, Vitányi & de Wolf (2004)
:   Cilibrasi, R., Vitányi, P., & de Wolf, R. (2004). Algorithmic clustering of music based on string compression. *Computer Music Journal*, **28**(4), 49–67.
:   The clustering-quality bar to aim at, and the honest limit that comes with it: the tree distortion stays low only while `n ≤ 15`, and by `n ≥ 35` the tree-benefit score `S(T)` falls below 0.9. Background for [Comparing many sequences](../recipes/batch-distance.md).

Cebrián, Alfonseca & Ortega (2005)
:   Cebrián, M., Alfonseca, M., & Ortega, A. (2005). Common pitfalls using the normalized compression distance: what to watch out for in a compressor. *Communications in Information and Systems*, **5**(4), 367–384.
:   Their Eq. (3) is character-for-character the formula this library implements. Also the sliding-window pathology: once an input exceeds gzip's 32 KiB window, `NCD(x,x)` jumps to about 0.9 and the measure becomes unusable. The strongest argument for LZ76 factor counts over an off-the-shelf compressor, on [Information distance](../concepts/nid.md).

Cilibrasi & Vitányi (2005)
:   Cilibrasi, R., & Vitányi, P. M. B. (2005). Clustering by compression. *IEEE Transactions on Information Theory*, **51**(4), 1523–1545. [10.1109/TIT.2005.844059](https://doi.org/10.1109/TIT.2005.844059). Preprint: [arXiv:cs/0312044](https://arxiv.org/abs/cs/0312044)
:   NCD, the normal-compressor axioms (idempotency, monotonicity, symmetry, distributivity), and Definition 3.3 — `C(y|x) := C(xy) − C(x)` — which is the justification for the numerator of `lz.nid`. Also the source of the narrow-band warning: measured distances often fall in 0.85–1.2, so a sensitive clustering method is needed and a minimum spanning tree is not one. The definition and equation numbers differ between the arXiv and IEEE versions, so state which you mean.

Sculley & Brodley (2006)
:   Sculley, D., & Brodley, C. E. (2006). Compression and machine learning: a new perspective on feature space vectors. *Proceedings of the Data Compression Conference (DCC 2006)*, 332–341. [10.1109/DCC.2006.13](https://doi.org/10.1109/DCC.2006.13)
:   Compression-based similarity is a similarity measure in an implicit feature space, not a parameter-free method. The parameters moved into the compressor and into your symbolisation. Quoted on [Information distance](../concepts/nid.md), which calls `nid` parameter-light rather than parameter-free.

Wehner (2007)
:   Wehner, S. (2007). Analyzing worms and network traffic using compression. *Journal of Computer Security*, **15**(3), 303–320.
:   NCD clustering of polymorphic worms found in network traffic: worm species cluster by type, and an unknown binary can be flagged as a later version of a known one. The template for a reference-profile design.

Terwijn, Torenvliet & Vitányi (2011)
:   Terwijn, S. A., Torenvliet, L., & Vitányi, P. M. B. (2011). Nonapproximability of the normalized information distance. *Journal of Computer and System Sciences*, **77**(4), 738–742. [10.1016/j.jcss.2010.06.018](https://doi.org/10.1016/j.jcss.2010.06.018). Preprint: [arXiv:0910.4353](https://arxiv.org/abs/0910.4353)
:   NID is neither upper nor lower semicomputable "up to any reasonable precision". Every practical NID-like number, this library's included, is a heuristic surrogate with no convergence guarantee — stated as such on [Information distance](../concepts/nid.md).

Borbely (2016)
:   Borbely, R. S. (2016). On normalized compression distance and large malware: towards a useful definition of normalized compression distance for the classification of large files. *Journal of Computer Virology and Hacking Techniques*, **12**(4), 235–242. Preprint: [arXiv:1509.00689](https://arxiv.org/abs/1509.00689)
:   Real compressors fail idempotency badly on 6–51 MB files. With bz2, 1-NN Android-malware family accuracy falls from 89.7 % when both reference and test files stay under 200 KB to 29.8 % over the full corpus — against 25 % for random guessing. Cited on [Information distance](../concepts/nid.md) as the failure mode this library does not have.

Simons & Abásolo (2017)
:   Simons, S., & Abásolo, D. (2017). Distance-based Lempel–Ziv complexity for the analysis of electroencephalograms in patients with Alzheimer's disease. *Entropy*, **19**(3), 129. [10.3390/e19030129](https://doi.org/10.3390/e19030129)
:   Introduces `dLZC`, which is **not** this library's `nid`: it subtracts `c(PP)` rather than `c(P)`, sums the two directions instead of taking a maximum, and normalises by the analytic bound. The paper exists partly because its authors found the Otu–Sayood normalisation unsuitable for EEG. Their median-threshold binarisation is the one [EEG and neural time-series](../recipes/neuro.md) recommends; the paper itself is a caution, not validation of `nid`. Companion: Simons, S., Abásolo, D., & Sauseng, P. (2015). Volume conduction effects on bivariate Lempel-Ziv complexity of Alzheimer's disease electroencephalograms. *EMBC 2015*, PMID 26738005 — with current-source-density preprocessing the significant group differences disappeared.

<hr class="lz-tickrule">

## Algorithms

One factorization is a suffix array, an LCP array, a longest-previous-factor array, and one left-to-right walk. Index construction is essentially all of the cost; the LZ76 walk itself is never more than 0.5 % of the runtime. See [Performance](performance.md) and [Rust crate](../api/rust.md).

Itoh & Tanaka (1999)
:   Itoh, H., & Tanaka, H. (1999). An efficient method for in memory construction of suffix arrays. *SPIRE/CRIWG 1999*, IEEE Press, 81–88.
:   Origin of the A/B suffix classification that divsufsort uses — not Ko & Aluru, despite the resemblance.

Manber & Myers (1993)
:   Manber, U., & Myers, G. (1993). Suffix arrays: a new method for on-line string searches. *SIAM Journal on Computing*, **22**(5), 935–948. Conference version: SODA 1990.
:   The data structure, with an `O(N log N)` prefix-doubling construction. The property the library actually depends on is simpler than anything in the paper: the suffix array is *unique*, so the comparison-sort path below 2048 bytes and the divsufsort path at 2048 and up are result-identical.

Kasai, Lee, Arimura, Arikawa & Park (2001)
:   Kasai, T., Lee, G., Arimura, H., Arikawa, S., & Park, K. (2001). Linear-time longest-common-prefix computation in suffix arrays and its applications. *CPM 2001*, LNCS **2089**, 181–192.
:   The LCP algorithm the library implements verbatim: iterate over text positions, carry `h`, and `h` drops by at most 1 per step. Linear time, but cache-hostile — which is why the LCP phase catches up with suffix sorting at `n = 10⁷` on [Performance](performance.md).

Kärkkäinen & Sanders (2003)
:   Kärkkäinen, J., & Sanders, P. (2003). Simple linear work suffix array construction. *ICALP 2003*, LNCS **2719**, 943–955. Journal version: Kärkkäinen, J., Sanders, P., & Burkhardt, S. (2006). Linear work suffix array construction. *Journal of the ACM*, **53**(6), 918–936. [10.1145/1217856.1217858](https://doi.org/10.1145/1217856.1217858)
:   The skew/DC3 algorithm — one of three independent linear-time constructions published in 2003. Context for why the library's dependency is *not* one of them.

Ko & Aluru (2003)
:   Ko, P., & Aluru, S. (2003). Space efficient linear time construction of suffix arrays. *CPM 2003*, LNCS **2676**, 200–210. Companion: Kim, D. K., Sim, J. S., Park, H., & Park, K. (2003). Linear-time construction of suffix arrays. *Ibid.*, 186–199.
:   S-type/L-type classification and induced sorting, the intellectual ancestor of SA-IS.

Crochemore & Ilie (2008)
:   Crochemore, M., & Ilie, L. (2008). Computing longest previous factor in linear time and applications. *Information Processing Letters*, **106**(2), 75–80. [10.1016/j.ipl.2007.10.006](https://doi.org/10.1016/j.ipl.2007.10.006)
:   Given a suffix array and an LCP array, the longest-previous-factor array costs `O(n)` via a monotone stack. `LPF` is what makes the parse self-referential: a match may start before position `i` and run past it.

Crochemore, Ilie & Smyth (2008)
:   Crochemore, M., Ilie, L., & Smyth, W. F. (2008). A simple algorithm for computing the Lempel–Ziv factorization. *DCC 2008*, 482–488.
:   Proves the `O(√n)` stack bound for the on-line variant. The library omits the branch that bound depends on — output-identical, but its stack is `O(n)` in the worst case.

Nong, Zhang & Chan (2009)
:   Nong, G., Zhang, S., & Chan, W. H. (2009). Linear suffix array construction by almost pure induced-sorting. *DCC 2009*, 193–202. Journal version: (2011). Two efficient algorithms for linear time suffix array construction. *IEEE Transactions on Computers*, **60**(10), 1471–1484. Companion: (2009). Linear time suffix array construction using D-critical substrings. *CPM 2009*, LNCS **5577**, 54–67.
:   SA-IS — the linear-time algorithm that stuck, and the one divsufsort is usually benchmarked against.

Kärkkäinen, Manzini & Puglisi (2009)
:   Kärkkäinen, J., Manzini, G., & Puglisi, S. J. (2009). Permuted longest-common-prefix array. *CPM 2009*, LNCS **5577**, 181–192.
:   The Φ-algorithm, reported to be the fastest LCP construction when the suffix array is already available. The known remedy for the cache-bound Kasai pass that [Performance](performance.md) measures at 43–45 % of runtime at `n = 10⁷`. Not implemented here, and the size of the win has not been measured here either.

Crochemore, Ilie, Iliopoulos, Kubica, Rytter & Waleń (2013)
:   Crochemore, M., Ilie, L., Iliopoulos, C. S., Kubica, M., Rytter, W., & Waleń, T. (2013). Computing the longest previous factor. *European Journal of Combinatorics*, **34**(1), 15–26. [10.1016/j.ejc.2012.07.011](https://doi.org/10.1016/j.ejc.2012.07.011)
:   Source of the `LPF-on-line` pseudocode the Rust implementation follows, including the sanctioned "extend the suffix array to rank `n`" simplification it uses in place of a final drain loop.

Shun & Zhao (2013)
:   Shun, J., & Zhao, F. (2013). Practical parallel Lempel–Ziv factorization. *Data Compression Conference (DCC 2013)*, 123–132.
:   The lemma relating `LPF` to nearest-smaller neighbours in the suffix array, and the `LPFtoLZ` walk. Note that its walk advances by `max(1, LPF[i])`, which is the stringology LZ77 factorization; LZ76 advances by `LPF[i] + 1`. That extra symbol is the innovation, and it is the whole difference. Explained on [LZ76 factorization](../concepts/lz76.md).

Fischer & Kurpicz (2017)
:   Fischer, J., & Kurpicz, F. (2017). Dismantling DivSufSort. *Prague Stringology Conference 2017*. Preprint: [arXiv:1710.01896](https://arxiv.org/abs/1710.01896)
:   The first academic description of divsufsort, which the library uses through the `cdivsufsort` bindings for inputs of 2048 bytes and up. The correction it supports — divsufsort is `O(n log n)` worst case, not linear, as its own README states — is the one [Performance](performance.md) makes against the library's own in-source comment.

Larsson & Sadakane (2007)
:   Larsson, N. J., & Sadakane, K. (2007). Faster suffix sorting. *Theoretical Computer Science*, **387**(3), 258–272.
:   The doubling and rank-refining scheme that divsufsort's tie-resolution stage resembles.

<hr class="lz-tickrule">

## Applications

### Neuroscience

Casali, Gosseries, Rosanova, Boly, Sarasso, Casali, Casarotto, Bruno, Laureys, Tononi & Massimini (2013)
:   Casali, A. G., Gosseries, O., Rosanova, M., Boly, M., Sarasso, S., Casali, K. R., Casarotto, S., Bruno, M.-A., Laureys, S., Tononi, G., & Massimini, M. (2013). A theoretically based index of consciousness independent of sensory processing and behavior. *Science Translational Medicine*, **5**(198), 198ra105. [10.1126/scitranslmed.3006294](https://doi.org/10.1126/scitranslmed.3006294)
:   PCI — LZ76 complexity of a binarised TMS-evoked cortical response matrix, normalised by the source entropy `H(p)·L/log₂L` rather than by the bare `L/log₂L`. 208 measurements in 52 subjects; alert wakefulness 0.55 ± 0.05 against 0.23 ± 0.04 for loss of consciousness. The highest-impact application of LZ76 that exists, and the reason [EEG and neural time-series](../recipes/neuro.md) leads with the choice of normaliser.

Sarasso, Boly, Napolitani, Gosseries, Charland-Verville & Casarotto et al. (2015)
:   Sarasso, S., Boly, M., Napolitani, M., Gosseries, O., Charland-Verville, V., Casarotto, S., et al. (2015). Consciousness and complexity during unresponsiveness induced by propofol, xenon, and ketamine. *Current Biology*, **25**(23), 3099–3105.
:   PCI tracks consciousness, not responsiveness: ketamine-unresponsive subjects score 0.44, comparable to wakefulness, while propofol scores 0.24 and xenon 0.17. The cleanest demonstration that two states with the same behaviour can have very different complexity.

Schartner, Seth, Noirhomme, Boly, Bruno, Laureys & Barrett (2015)
:   Schartner, M., Seth, A., Noirhomme, Q., Boly, M., Bruno, M.-A., Laureys, S., & Barrett, A. (2015). Complexity of multi-dimensional spontaneous EEG decreases during propofol induced general anaesthesia. *PLOS ONE*, **10**(8), e0133532. [10.1371/journal.pone.0133532](https://doi.org/10.1371/journal.pone.0133532)
:   The canonical spontaneous-EEG pipeline: notch filter, downsample to 250 Hz, surface Laplacian, detrend, Hilbert amplitude, binarise at each channel's own mean, serialise column by column, then normalise by a *shuffled surrogate* rather than by the analytic bound. Every step of the recipe on [EEG and neural time-series](../recipes/neuro.md) comes from here.

Casarotto, Comanducci, Rosanova, Sarasso, Fecchio, Napolitani, Pigorini, Casali, Trimarchi & Boly et al. (2016)
:   Casarotto, S., Comanducci, A., Rosanova, M., Sarasso, S., Fecchio, M., Napolitani, M., Pigorini, A., Casali, A. G., Trimarchi, P. D., Boly, M., et al. (2016). Stratification of unresponsive patients by an independently validated index of brain complexity. *Annals of Neurology*, **80**, 718–729. [10.1002/ana.24779](https://doi.org/10.1002/ana.24779)
:   The clinical validation: a 150-subject benchmark yields the empirical cutoff `PCI* = 0.31` at 100 % sensitivity and specificity, and identifies 9 of 43 behaviourally vegetative patients as having high complexity.

Schartner, Carhart-Harris, Barrett, Seth & Muthukumaraswamy (2017)
:   Schartner, M. M., Carhart-Harris, R. L., Barrett, A. B., Seth, A. K., & Muthukumaraswamy, S. D. (2017). Increased spontaneous MEG signal diversity for psychoactive doses of ketamine, LSD and psilocybin. *Scientific Reports*, **7**, 46421. [10.1038/srep46421](https://doi.org/10.1038/srep46421)
:   Complexity moves *above* baseline under psychedelics — 86 %, 100 % and 93 % of participants for psilocybin, ketamine and LSD respectively, though the psilocybin differences reached significance for no measure. The result that stops LZ complexity being read as a sedation-depth meter.

Abásolo, James & Hornero (2007)
:   Abásolo, D., James, C. J., & Hornero, R. (2007). Non-linear analysis of intracranial electroencephalogram recordings with approximate entropy and Lempel-Ziv complexity for epileptic seizure detection. *Annual International Conference of the IEEE Engineering in Medicine and Biology Society*, **2007**, 1953–1956. PMID 18002366.
:   LZ complexity increases during seizures at the focal electrodes. A conference paper, not a journal article, and easy to miscite as one — an earlier draft of these docs attributed the same PMID to a 1998 journal paper by different authors. Cited on [EEG and neural time-series](../recipes/neuro.md).

Höhn, Hahn, Lendner & Hoedlmoser (2024)
:   Höhn, C., Hahn, M. A., Lendner, J. D., & Hoedlmoser, K. (2024). Spectral slope and Lempel–Ziv complexity as robust markers of brain states during sleep and wakefulness. *eNeuro*, **11**(3), ENEURO.0259-23.2024. [10.1523/ENEURO.0259-23.2024](https://doi.org/10.1523/ENEURO.0259-23.2024)
:   LZ complexity as a sleep-state marker. The defensible reading of this literature is narrow: LZ complexity separates deep NREM from every other stage, while the finer orderings — wake against N1, REM against wake — are not reliable, and it indexes state rather than the content of experience.

Szczepański, Amigó, Wajnryb & Sanchez-Vives (2004)
:   Szczepański, J., Amigó, J. M., Wajnryb, E., & Sanchez-Vives, M. V. (2004). Characterizing spike trains with Lempel-Ziv complexity. *Neurocomputing*, **58–60**, 79–84. [10.1016/j.neucom.2004.01.026](https://doi.org/10.1016/j.neucom.2004.01.026)
:   LZ76 applied to spike trains rather than to field potentials.

### Genomics

Orlov & Potapov (2004)
:   Orlov, Y. L., & Potapov, V. N. (2004). Complexity: an internet resource for analysis of DNA sequence complexity. *Nucleic Acids Research*, **32**(Web Server issue), W628–W633. [10.1093/nar/gkh466](https://doi.org/10.1093/nar/gkh466)
:   Sliding-window complexity profiles — linguistic, entropy, Wootton–Federhen and Lempel–Ziv — across 140 complete genomes, with a 1000 bp default window. The source of the window size recommended on [DNA and FASTA](../recipes/genomics.md).

Höhl, Rigoutsos & Ragan (2007)
:   Höhl, M., Rigoutsos, I., & Ragan, M. A. (2007). Pattern-based phylogenetic distance estimation and tree reconstruction. *Evolutionary Bioinformatics Online*, **2**. PMC2674673.
:   The benchmark that keeps the genomics claims honest. On 700 synthetic trees the LZ distance ranks 15th and 16th of 19; on empirical BAliBASE data it ranks 10th and 15th of 22 — mid-field, and statistically indistinguishable from the other alignment-free distances. Alignment-based methods beat all of them at fine resolution. Cited on [DNA and FASTA](../recipes/genomics.md).

Menconi, Benci & Buiatti (2008)
:   Menconi, G., Benci, V., & Buiatti, M. (2008). *Data compression and genomes: a two dimensional life domain map.* [arXiv:0803.0465](https://arxiv.org/abs/0803.0465)
:   Archaea, Bacteria and Eukarya separate in a two-dimensional space built from compression statistics over noncoding regions. A preliminary analysis of 15 genomes, not a large-scale result.

Pirogov, Pfaffelhuber, Börsch-Haubold & Haubold (2019)
:   Pirogov, A., Pfaffelhuber, P., Börsch-Haubold, A., & Haubold, B. (2019). High-complexity regions in mammalian genomes are enriched for developmental genes. *Bioinformatics*, **35**(11), 1813–1819. [10.1093/bioinformatics/bty922](https://doi.org/10.1093/bioinformatics/bty922)
:   Sequence complexity used as a genome annotation signal rather than as a summary statistic.

### Physics, dynamical systems and materials

Estevez-Rams, Welzel, Pentón-Madrigal & Mittemeijer (2008)
:   Estevez-Rams, E., Welzel, U., Pentón-Madrigal, A., & Mittemeijer, E. J. (2008). Stacking and twin faults in close-packed crystal structures: exact description of random faulting statistics for the full range of faulting probabilities. *Acta Crystallographica Section A*, **64**(5), 537–548. [10.1107/S0108767308016826](https://doi.org/10.1107/S0108767308016826)
:   Close-packed stacking sequences are one-dimensional symbolic sequences and stacking faults are typos in them, which is what makes `h` and `E` the natural disorder measures for a polytype.

Estevez-Rams & González-Férez (2009)
:   Estevez-Rams, E., & González-Férez, R. (2009). On the concept of long range order in solids: the use of algorithmic complexity. *Zeitschrift für Kristallographie*, **224**(3), 179–184. [10.1524/zkri.2009.1146](https://doi.org/10.1524/zkri.2009.1146)
:   Algorithmic complexity captures long-range order without appealing to the nature of the diffraction pattern — and zero complexity of the diffraction pattern does not imply zero complexity of the atomic arrangement.

Varn, Canright & Crutchfield (2002)
:   Varn, D. P., Canright, G. S., & Crutchfield, J. P. (2002). Discovering planar disorder in close-packed structures from X-ray diffraction: beyond the fault model. *Physical Review B*, **66**, 174110. Preprint: [arXiv:cond-mat/0203290](https://arxiv.org/abs/cond-mat/0203290)
:   The sibling programme: reconstruct an ε-machine for the stacking process from the diffraction pattern, then read excess entropy off the machine. A useful contrast — same physics, model-based rather than compression-based.

Rodriguez-Horta, Estevez-Rams, Neder & Lora-Serrano (2017)
:   Rodriguez-Horta, E., Estevez-Rams, E., Neder, R., & Lora-Serrano, R. (2017). Close-packed structures with finite-range interaction: computational mechanics of layer pair interaction. *Acta Crystallographica Section A*, **73**(4), 357–369. [10.1107/S2053273317006945](https://doi.org/10.1107/S2053273317006945)
:   Statistical complexity, entropy density and excess entropy as functions of faulting probability.

Nagaraj & Balasubramanian (2017)
:   Nagaraj, N., & Balasubramanian, K. (2017). *Dynamical complexity of short and noisy time series.* European Physical Journal Special Topics. Preprint: [arXiv:1609.01924](https://arxiv.org/abs/1609.01924)
:   On the logistic map at length `L = 200` with 4 bins, LZ complexity correlates with the Lyapunov exponent at Pearson `r = 0.8889` while Shannon entropy manages `0.2721`. The quantitative case for LZ over a histogram estimator on short series. Their binning is not the one [The route to chaos](../recipes/dynamical-systems.md) uses, so those two numbers are not a prediction for that page's sweep.

Mesa-Rodríguez, Estevez-Rams & Kantz (2025)
:   Mesa-Rodríguez, A., Estevez-Rams, E., & Kantz, H. (2025). *Entropy measures as indicators of connectivity paths in the human brain.* [arXiv:2507.04442](https://arxiv.org/abs/2507.04442)
:   Current work from the library's own research lineage, using exactly the `h`, `E` and LZ-distance toolkit this package exposes. See [Authors and citation](authors.md) for the provenance.

### Anomaly detection

Kulkarni & Bush (2006)
:   Kulkarni, A., & Bush, S. (2006). Detecting distributed denial-of-service attacks using Kolmogorov complexity metrics. *Journal of Network and Systems Management*, **14**, 69–80. [10.1007/s10922-005-9016-3](https://doi.org/10.1007/s10922-005-9016-3)
:   The joint complexity of correlated streams is lower than the sum of their individual complexities, and varies inversely with the amount of correlation. That is the observation `lz.nid` is built on, applied to traffic flows. Background for [Comparing many sequences](../recipes/batch-distance.md).

<hr class="lz-tickrule">

## Reference implementations

Not literature, but load-bearing: these are the implementations the library's output was differentially tested against.

`libdivsufsort`
:   Mori, Y. *libdivsufsort — a lightweight suffix-sorting library.* MIT licence. <https://github.com/y-256/libdivsufsort>
:   `O(n log n)` worst case in `5n + O(1)` bytes, per its own README. Reached through Wenger, A., `cdivsufsort` 2.0.0 (<https://github.com/fasterthanlime/stringsearch>), which vendors the original C rather than porting it — so a C compiler is needed at build time, and an input over 2 147 483 646 bytes (≈ 2 GiB) panics rather than returning an error.

`antropy` and `NeuroKit2`
:   `antropy` (`_lz_complexity`) and `NeuroKit2` (`_complexity_lempelziv_count`), both on GitHub master.
:   The two Python implementations users are most likely to be migrating from. Both use the classical Kaspar–Schuster loop, which counts the trailing incomplete component; this library does not. The difference is 0 or +1, and outside the constant strings it is exactly the conversion at the top of this page. See [Rust vs C++](cpp-parity.md).

<hr class="lz-tickrule">

## Citation traps

!!! warning
    Five citations in this bibliography have been printed incorrectly somewhere downstream, so copying any of them second-hand gives you a wrong volume, a wrong page range or a wrong year. The first four are misprints in published reference lists; the last was made in this project's own notes and is recorded here for the same reason.

    | Wrong, and where it appears | Correct |
    |---|---|
    | Lempel & Ziv 1976 as *IEEE Trans. Inf. Theory* **92**(1) — Constantinescu & Ilie's reference list | **22**(1), 75–81 |
    | Szczepański et al. 2004 as *Neurocomputing* 58–60, **77**–84 — Estevez-Rams et al. (2013) | **79**–84 |
    | Wyner & Wyner 1995 as *IEEE Trans. Inf. Theory* **35**(3) — the arXiv version of Gao et al. (2008) | **41**(3), 723–731 |
    | Crutchfield & Feldman 2003 as *Chaos* **15** — Feldman, McTague & Crutchfield's bibliography | **13**(1), 25–54 |
    | Shun & Zhao 2013 as "Shun (2017)" — an earlier draft of these docs' own research notes | 2013, and Fuyao Zhao is a joint first author |

    One more to check for yourself: two of the sources behind this page disagree on the initial of the fourth author of Estevez-Rams et al. (2013) — one gives `I. Brito Reyes`, the other `D. Brito Reyes`. The entry above prints `I.`; verify against the publisher record before you paste it.

Two further distinctions are not misprints but are equally easy to get wrong. **Ziv (1978)**, *Coding theorems for individual sequences* (24(4), 405–412), is not **Ziv & Lempel (1978)**, *Compression of individual sequences via variable-rate coding* (24(5), 530–536) — same journal, same year, adjacent issues, and they get cited interchangeably for the `lz.h` convergence result. And **Melchert & Hartmann (2015)** is independent work rather than a paper by the Estevez-Rams group, though it usually appears beside theirs in reference lists.

For how to cite the software itself rather than the method, see [Authors and citation](authors.md).
