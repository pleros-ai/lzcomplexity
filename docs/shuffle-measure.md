# EMC-like shuffle measure

The public names `emc`, `emc_value` and `LzShuffle` denote a magnitude
**qualitatively similar to effective measure complexity (EMC)**. They remain
for compatibility. The score is not an exact EMC or excess-entropy estimate.
This note describes the existing released calculation.

## Calculation

For an input byte sequence of length `N`, let `C` be its LZ76 factor count.
For each block size `l = 1, ..., M`, create a block-shuffled copy and count its
LZ76 factors, `C_l`. With logarithm base `b`, calculate

```text
g   = log_b(N) / N
r_l = l * g * (C_l - C)
```

The raw contrasts `r_l` can be negative or decrease with block size. The
implementation fits a nonnegative, nondecreasing sequence to them by
least-squares isotonic regression (pool adjacent violators, then clip below
zero). Calling the fitted values `f_1, ..., f_M`, with `f_0 = 0`, it returns

```text
score              = f_M
summands[l - 1]    = f_l - f_(l-1)
multi_information  = r_1
```

The summands are nonnegative fitted increments and sum to the score. They do
not represent established conditional-entropy differences. The historical
`multi_information` field contains the raw first-scale contrast, which can
be negative; it is not calibrated Shannon mutual information.

## Block shuffling

Each scale starts from a fresh copy of the original sequence and runs
`floor(N/2)` block-swap iterations. Each iteration selects two distinct,
non-overlapping blocks aligned at multiples of `l` and swaps their contents,
preserving symbol order inside each block. There is one surrogate per scale.

A scale-specific seed comes from an FNV-1a hash of the full input and the block
size. Calls with identical bytes and parameters are reproducible with the
pinned RNG dependencies. The finite series of swaps is not a uniform draw
from all possible block permutations.

For `N > 0`, the legacy boundary rule leaves the suffix starting at
`floor((N - 1)/l) * l` fixed. At least the final byte remains unchanged; when
`N` is divisible by `l`, a whole final block stays fixed. Inputs of at most ten
bytes, or scales with fewer than two eligible blocks, are unchanged.

Constant sequences also remain unchanged under every shuffle and therefore
score zero. This describes the measure's response to shuffling; it does not
mean that a constant sequence lacks a pattern.

## Parameters and comparisons

A positive `max_block_size` sets `M`. Nonpositive values use the existing
length-based helper approximating `N = M * 2**M`, then add ten when `N > 50`.
This convention selects scales; it does not guarantee convergence.

Python/core `log_base=None` follows the observed input alphabet size, floored
at two. An explicit `alphabet` argument affects the finite-size `epsilon`
diagnostic without changing that default base. The CLI's legacy default uses
an explicitly declared alphabet when supplied. Pass `-l 2` in the CLI and
`log_base=2` in Python to compare base-two results.

For comparisons, match input length, byte encoding, logarithm base, block-size
cap and package/source version. Examine sensitivity to length and cap. Cyclic
shifts and symbol relabeling also change the content-derived seeds and can
change the finite-surrogate score.

Finite-sample LZ behavior, seams between shuffled blocks, incomplete mixing,
the fixed suffix and the projection all affect the result. A larger score is
not a universal ranking by theoretical EMC. Base-two normalization does not
make it a calibrated shared-information bit count, and the API supplies no
calibrated confidence interval for this score. Its legacy error fields and
`extras` are separate diagnostics.
