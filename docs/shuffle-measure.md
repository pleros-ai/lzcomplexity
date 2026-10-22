# Calculating E

E combines LZ76 factor-count differences across a series of block ranges. This page gives the formula and sampling procedure used by `emc()` and the E fields returned by `lz76()`.

## Formula

Let `N` be the input byte length, `C` its LZ76 factor count, `b` the logarithm base and `M` the maximum block size. At each range `l = 1, …, M`, construct a rearranged copy as described below and compute its factor count `C_l`.

```text
g   = log_b(N) / N
r_l = l · (C_l · g - C · g)
```

For `N ≤ 1`, the normalization `g` is zero. The raw values `r_l` can be negative or decrease as the range increases.

The calculation fits a nonnegative, nondecreasing sequence `f_1, …, f_M` to the raw values by least-squares isotonic regression. It uses the pool-adjacent-violators algorithm, then clips fitted values below zero. With `f_0 = 0`, the result is:

```text
E                    = f_M
summands[l - 1]      = f_l - f_(l-1)
multi_information    = r_1
```

The increments are nonnegative and sum to E. The `multi_information` output stores the raw first-range difference; it can be negative and is separate from the fitted increments.

For a raw profile `[3, 2, 4]`, the first two values are pooled, giving `[2.5, 2.5, 4]`. E is `4` and the increments are `[2.5, 0, 1.5]`.

## Sampling at each range

Each range starts from a fresh copy of the original sequence and performs `floor(N/2)` block swaps. Each swap selects two distinct blocks aligned at multiples of `l`. Their contents are exchanged, preserving the order within each block. There is one sampled copy per range.

![One aligned block swap at width three; the final block stays fixed](assets/block-swap.svg)

*One swap at a block width of three bytes. A 24-byte sequence undergoes twelve swaps at each range.*

The suffix beginning at `floor((N - 1)/l) · l` remains fixed. This includes the final byte and, when the length is divisible by the block width, the entire last block. Inputs of at most ten bytes and ranges with fewer than two eligible blocks remain unchanged.

The sampler makes a finite series of swaps, rather than drawing uniformly from every possible block permutation. Its `ChaCha8` seed is derived from the input's FNV-1a hash and the block width. The pinned `rand` and `rand_chacha` dependencies fix the random stream, so identical inputs and parameters reproduce the same sampled copies.

## Maximum range and normalization

A positive `max_block_size` sets `M`. For a nonpositive value, the length-based helper approximates `N = M · 2^M` and adds ten when `N > 50`.

Python and Rust default to the observed alphabet size, floored at two, for the logarithm base. In Python, an explicit `alphabet` argument affects `epsilon`; use `log_base` to select the normalization of E. The CLI uses a declared alphabet as its default base when one is supplied. Set `log_base=2` in Python and `-l 2` in the CLI for matched base-two results.

## Comparing results

Use the same length, byte encoding, logarithm base and maximum range for direct comparisons. Examine several lengths and ranges to understand sensitivity. Periodic sequences can respond differently depending on the alignment between their period and the block ranges.

Cyclic shifts and symbol relabeling change the content-derived seeds and may change E, even when some factor counts are preserved. For variation across observations, analyze separate samples or recording windows; repeating an identical call reproduces the same result.

See [structural complexity E](concepts/emc.md) for interpretation and [inside the implementation](concepts/implementation.md) for the surrounding analysis pipeline.
