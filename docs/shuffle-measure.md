# Shuffle-based EMC-like measure

The public names `emc`, `emc_value` and `LzShuffle` are retained for compatibility.
They denote a magnitude **qualitatively similar to effective measure complexity
(EMC)**. This note writes the score as `E*` to distinguish it from theoretical
EMC or excess entropy. No equality, consistency result, or general ordering by
theoretical EMC is asserted for this finite-sequence measure.

The definition below describes the existing shuffle method. It does not replace
shuffling with direct block histograms or a fitted state model.

## Published recipe and version history

The [2019 written-language paper, Eq. (4)](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0214863)
describes summing the differences between the LZ entropy-rate estimates of
block-shuffled and original sequences across scales. It explicitly distinguishes
that quantity from theoretical excess entropy and motivates a qualitative
similarity. With the same normalization and surrogate counts, that diagnostic
has the form

```text
paper-style sum = g * sum_l(C_l - C)
```

That is a different operational definition from the current released
`l * g * (C_l - C)` ladder followed by isotonic projection. The scale multiplier
and the projection cannot be justified merely by requiring this qualitative
measure to equal theoretical EMC.

This distinction predates PR #14. Commit `37ef34c` changed the older Rust sum of
absolute shuffle contrasts to a block-entropy-difference construction; commit
`fb0a16d` introduced the current isotonic projection. Those changes are already
in released 2.0.0. The documentation correction here preserves that released
calculation. Whether to restore a paper-style aggregation is a scientific
method decision requiring explicit approval, not a silent implementation fix.
Results from these aggregation rules must be labelled separately.

## Operational definition

For a byte sequence `u` of length `N`, let `C` be its LZ76 factor count. For
each block size `l = 1, ..., M`, generate a block-shuffled surrogate and let
`C_l` be its LZ76 factor count. With logarithm base `b`, compute

```text
g   = log_b(N) / N
r_l = l * g * (C_l - C)
```

The raw contrasts `r_l` can be negative or decrease with scale. The existing
implementation fits them by nonnegative, nondecreasing least-squares isotonic
regression (pool adjacent violators, followed by clipping below zero). Write
that fitted sequence as `f_1, ..., f_M`, with `f_0 = 0`. The returned values are

```text
E*                 = f_M
summands[l - 1]    = f_l - f_(l-1)
multi_information  = r_1
```

The fitted increments are nonnegative and sum to the score by construction.
They are not established estimates of conditional-entropy differences. A zero
increment can reflect pooled scales or clipping. A rising fitted profile alone
does not establish a correlation length, hierarchy, or amount of shared
information. The legacy `multi_information` name denotes the raw first-scale
contrast; it can be negative and is not calibrated Shannon mutual information.

The projection is part of this operational measure. Theorems about the shape
of theoretical excess entropy do not prove that this shuffle contrast is that
quantity or justify interpreting its fitted increments as information bits.

## What the shuffle actually does

For every scale, start from a fresh copy of the original sequence. Run
`floor(N/2)` iterations, each selecting two distinct non-overlapping blocks
aligned at multiples of `l` and swapping their contents. The symbols inside
each selected block retain their order. A scale-specific seed is derived from
an FNV-1a hash of the full input and the block size. The pinned `rand` and
`rand_chacha` versions are part of the reproducible shuffle behavior.

This is a finite series of block swaps, not a uniform permutation of all input
symbols or a certified independent sample of blocks. There is one surrogate
per scale, not an average over independent surrogate realizations.

The current legacy boundary rule permits block ends only through `N - 1`,
leaving the suffix starting at `floor((N - 1)/l) * l` fixed. At least the last
byte is therefore unchanged; when `N` is divisible by `l`, a whole final block
is fixed. A scale is unchanged if fewer than two eligible blocks exist.
Inputs of at most ten bytes are also unchanged by the shuffle. Their zero
shuffle contrast is an implementation consequence, not evidence about the
complexity of an unknown source.

Constant sequences are unchanged by every block permutation, so this contrast
returns zero for them. That describes the sensitivity of this measure to
block shuffling; it does not assert that constant sequences lack a pattern.

## Scale selection and normalization

A positive `max_block_size` sets `M` explicitly. Nonpositive values use the
existing helper approximating `N = M * 2**M`, then add ten for `N > 50`.
This is a length-based scale-selection convention. It is not a coverage or
convergence guarantee, and its base-two heuristic does not adapt to a larger
observed alphabet.

Python/core `log_base=None` uses the number of distinct input bytes, floored
at two. An explicit `alphabet` argument affects the finite-size `epsilon`
diagnostic but does not change that default base. Specify `log_base` explicitly
for comparisons. Changing the base rescales this score; choosing base two does
not turn `E*` into a calibrated shared-information bit count.

The CLI has a legacy default for its `h` report that uses an explicitly declared
alphabet when supplied. Pass `-l 2` for a base-two CLI report and `log_base=2` in
Python when comparing outputs. See runtime help for each interface.

## Interpreting measurements

The [reproducible diagnostics](shuffle-validation.md) assess implementation
invariants and sensitivity rather than equality to theoretical EMC.

The useful question is how the shuffle-based score responds to controlled
changes in structure under matched measurement settings. Exact theoretical EMC
values can be reference context, but equality to them is not this measure's
acceptance criterion. Nor is there a universal guarantee that a larger `E*`
means a larger theoretical EMC.

Comparisons should record and match:

- input length and byte encoding;
- shuffle block-size cap and its automatic/explicit setting;
- logarithm base and package/source revision;
- source generation seeds, phases, or repeated samples when applicable.

Check sensitivity to length, cap and representation. Content-derived seeds
make repeated identical calls reproducible, but a cyclic shift or a bijective
symbol relabeling also changes those seeds. Identical source information need
not imply bit-identical finite-surrogate scores.

Finite-sample LZ behavior, artificial seams between shuffled blocks, incomplete
block mixing, the fixed suffix, sparse scales and the projection can all affect
the result. Monotonicity and nonnegativity of the fitted ladder are arithmetic
properties, not scientific validation of the chosen score.

The API does not supply calibrated confidence intervals for `E*`.
`factors_stddev`, `normal_error` and `poison_error` are legacy diagnostics, not
uncertainty on this measure. The `extras` fields are factor-count ratios,
not Shannon information measures or calibrated statistical correlations.

## Versioned documentation

The separately maintained website and C++ branch may describe historical
versions. Match their method, parameter conventions and source revision before
using a table, timing or parity result. The documentation site has not been
redesigned or redeployed as part of this clarification.
