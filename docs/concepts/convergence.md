# Sequence length and convergence

Longer sequences can reveal dependencies that a short sample misses. How much data you need depends on the source, its alphabet and the patterns preserved by your symbolization.

## What changes with length?

Entropy rate describes a continuing source. For stationary ergodic sources with a finite alphabet, normalized LZ parsing approaches the entropy rate as the observed length increases. At finite lengths, the estimate also reflects how many repeated patterns the sample contains. The [entropy-density page](entropy-density.md) explains the formula and its interpretation.

E examines organization across a finite set of ranges. Its automatic maximum block size grows with input length. Increasing the sample size therefore changes both the observed sequence and the ranges included in the calculation. To study length alone, choose an explicit fixed `max_block_size`.

## Short samples

A short sample may contain only one or two occurrences of a longer pattern. Factor counts are integers, so one additional factor can produce a substantial relative change when the count is small.

The following boundary cases are useful when inspecting short inputs:

- Empty and constant inputs have factor count one.
- `h` is zero for lengths zero and one.
- E is zero for inputs of at most ten bytes, which the range sampler leaves unchanged.
- A final copied phrase can have an end boundary beyond the input and be excluded from the completed-factor count.

Use short strings to follow the calculation. Use longer samples and comparisons across lengths to study the source.

## A practical length study

1. **Choose a representation.** Keep the symbolization, encoding and logarithm base consistent.
2. **Examine several lengths.** Use defined windows or prefixes and record whether samples overlap.
3. **Include repeated observations.** Independent realizations, trials or recording epochs reveal variation beyond a single sequence.
4. **Choose a maximum range for E.** A fixed cap separates length effects from range selection. You can also examine the automatic cap as a second analysis.
5. **Choose relevant controls.** Examples include independent symbols with matched frequencies, rearranged observations or a domain-specific reference model.
6. **Inspect stability.** Compare the result over nearby lengths and ranges before choosing the settings for the main analysis.

Calls on identical bytes and settings are deterministic. To study variation, change the observations or samples being analyzed.

## Sources of variation

| Source | What changes | What to record |
|---|---|---|
| Sampling | The recording or window | Participants, trials, epochs or generator seeds |
| Representation | Which distinctions become symbols | Thresholds, bins, encoding and missing-data handling |
| Measurement settings | Normalization and included ranges | Logarithm base, length and maximum block size |
| Software | The implementation used | Package version |

These choices help distinguish a change in the source from a change in how it was observed or measured.

## Estimating uncertainty

The fields `epsilon`, `normal_error`, `poison_error` and `factors_stddev` describe particular algebraic diagnostics; their formulas are in [reading the numbers](../guide/reading-the-numbers.md).

Estimate sampling uncertainty from repeated observations or a resampling analysis that respects temporal dependence. For example, consecutive symbols in a recording can be correlated, so resampling individual symbols answers a different question from resampling whole trials or suitable recording windows.

**Related:** [alphabets and encoding](alphabets.md), [effective measure complexity E](emc.md), and [performance](../project/performance.md).
