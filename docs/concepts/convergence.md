# Sequence length and convergence

More data can reveal longer dependencies, but length is not a certificate of accuracy. The source, its alphabet, the symbolization and the requested statistic determine what a finite recording can support.

## What the limiting theory says

Entropy rate describes an indefinitely continuing source. Lempel–Ziv results connect normalized parsing complexity to source information under the relevant asymptotic assumptions. They do not say that every signal of a given length is well estimated, or that every finite normalized count lies within theoretical entropy bounds. The [entropy-density page](entropy-density.md) distinguishes the formula from its limiting motivation.

The EMC-like score is a finite multiscale shuffle contrast. These docs do not assert that it converges to exact effective measure complexity. Its automatic maximum block size also changes with length, so changing `N` changes both the observed sample and the set of scales used in the score.

## Why short samples are fragile

A short sequence may contain few examples of a long pattern. LZ factor counts are integers; one extra factor can be a large relative change when the total is small. At a large shuffle block size, there may be few eligible blocks to rearrange. Some scales leave the data unchanged.

There are additional explicit conventions:

- Empty or constant input takes a special factorization path with count one.
- `h` is zero for lengths zero and one.
- Inputs of at most ten bytes are unchanged by the block-swap shuffle, so their shuffle score is zero.
- A final repeated phrase can extend beyond the input in the boundary vector and be excluded from the reported count.

These behaviors are useful to know when checking code, but they do not make tiny inputs scientifically informative.

## A practical length study

Use a source-specific plan rather than a universal minimum length:

1. **Choose the representation first.** Keep the symbolization, encoding and logarithm base consistent.
2. **Examine several lengths.** Use well-defined windows or prefixes, recording whether the samples overlap.
3. **Repeat across realizations or epochs.** Prefixes of one recording alone do not reveal between-recording variation.
4. **Match shuffle caps as well as lengths.** An explicit fixed cap helps isolate length effects; a second analysis with the automatic cap shows the actual default behavior.
5. **Use suitable controls.** A shuffled control, an independent source with matched symbol frequencies, or a domain-specific null can test different hypotheses. Their interpretation depends on how they were constructed.
6. **Report sensitivity.** A conclusion that changes with a modest window or cap adjustment needs that qualification.

Repeated calls on identical input do not generate independent shuffle realizations. The content-derived seeds make those calls reproducible, so repeating them alone cannot produce an uncertainty interval.

## Separate four sources of variation

| Source | What changes | What to record |
|---|---|---|
| Sampling | Which recording/window was observed | Participants, trials, epochs or generator seeds |
| Representation | Which distinctions become symbols | Thresholds, bins, encoding and missing-data rule |
| Measurement settings | Units and included scales | Log base, length and block-size cap |
| Implementation | Which algorithm and conventions ran | Package version, source revision and backend |

A numerical difference can arise from any of these. Match what you can, and keep the rest visible in the result record.

## What the error-looking fields mean

`normal_error`, `poison_error` and `factors_stddev` are formulas retained by the implementation. They are not validated coverage statements such as “the true entropy rate is inside this interval with 95% probability.” `epsilon` is not a measured bias correction or an error bar. None of these fields quantifies uncertainty in the shuffle score.

If uncertainty matters, design a sampling or resampling analysis suited to the data and its temporal dependence. Treating adjacent symbols as independent observations can remove the very dependence being studied. A statistical analysis also needs to distinguish variation across recordings from variation within one recording.

## Historical figures in the manual

The supplied 0.9.0 manual illustrates length sensitivity using the even process and a symmetric two-state process. Its EMC-like curves were scaled to `[0, 1]` for a qualitative comparison. Those figures are historical examples under their original settings; they are not current-release numerical accuracy tests or a basis for a universal sample-size rule. Consult the current [method definition](emc.md) when reproducing a result.

**Related:** [alphabets and encoding](alphabets.md), [reading returned fields](../guide/reading-the-numbers.md), and [performance](../project/performance.md).
