# Python API

Five functions share the same Rust core. Import the package once, then choose
whether you need a factor count, entropy density, the shuffle-based score, a
pairwise distance, or the complete result.

```python
import lzcomplexity as lz

complexity, boundaries = lz.factorization("banana")
assert complexity == 3
assert boundaries == [0, 1, 2, 3, 7]
assert lz.h("01010101", log_base=2) == 0.75
assert lz.nid("abcd", "abce") == 0.25
```

This reference covers the current Rust-backed API. The supplied 0.9 manual
explains the library's background but uses older function names and return
shapes. The signatures below, rather than those historical examples, apply to
this version. See [input conversion](inputs.md) before analysing numeric arrays.

## Choose a function

| Function | Work performed | Return |
|---|---|---|
| [`factorization`](#factorization) | One full-sequence factorization | `(int, list[int])` |
| [`h`](#h) | Factor count and entropy normalization | `float` |
| [`emc`](#emc) | Original and block-shuffled factorizations | `(float, list[float])` |
| [`nid`](#nid) | Original and concatenated sequence factorizations | `float` |
| [`lz76`](#lz76) | Full analysis, reusing the original factorization | `dict` |

## Shared parameters

| Parameter | Default | Current behavior |
|---|---|---|
| `seq` | required | Converts to bytes using the [input rules](inputs.md). |
| `partitions` | `1` | Accepted for compatibility. The current Rust factorization does not use this value. |
| `alphabet` | `None` | Alphabet parameter for the `epsilon` diagnostic. `None` uses the observed byte alphabet, floored at two. It does not set the default logarithm base. |
| `log_base` | `None` | Uses the observed byte alphabet, floored at two. Set `2` for base-two normalization. Explicit zero or one is also floored to two. |
| `jobs` | `0` | Accepted but currently ignored by Python bindings. Rayon manages native parallelism. |
| `max_block_size` | `-1` | Only `emc` and `lz76`: positive values set the shuffle cap; nonpositive values use the length-based automatic cap. |

`alphabet`, `log_base` and `jobs` are unsigned 32-bit parameters. Negative or
out-of-range values raise conversion errors. `partitions` and `max_block_size`
use signed 32-bit parameters. For repeatable measurements, pass `log_base` and
`max_block_size` explicitly and record the package version and input encoding.

!!! example "Alphabet size and units are separate controls"

    ```python
    import lzcomplexity as lz

    assert lz.h("01010101", alphabet=4) == 0.75
    assert lz.h("01010101", log_base=4) == 0.375
    ```

    The first call still uses the observed two-symbol alphabet as its default
    base. The second changes the actual entropy units.

## `factorization`

```python
factorization(seq, partitions=1, alphabet=None, log_base=None, jobs=0)
```

Returns `(complexity, boundaries)`. `complexity` is the library's LZ76 count;
`boundaries` contains the parse positions, beginning at zero.

The final boundary can exceed the input length. The count excludes a terminal
component that extends beyond the input, so **do not replace the returned
count with `len(boundaries) - 1`**. For `"banana"`, the count is three even
though slicing the boundary intervals displays four fragments.

```python
import lzcomplexity as lz

seq = "banana"
complexity, boundaries = lz.factorization(seq)
fragments = [seq[a:b] for a, b in zip(boundaries, boundaries[1:])]
assert fragments == ["b", "a", "n", "ana"]
```

Boundaries index **bytes**, including for Unicode strings. Slice encoded bytes
when the input contains non-ASCII characters. See [LZ76 factorization](../concepts/lz76.md)
for the counting convention and [degenerate inputs](#empty-and-constant-inputs).

## `h`

```python
h(seq, partitions=1, alphabet=None, log_base=None, jobs=0)
```

Returns the finite-sequence LZ entropy-density estimate

```text
h = C * log_b(N) / N
```

Here `C` is the returned factor count and `N` is the number of input bytes.
For `N <= 1`, the function returns zero. `log_base=2` gives bits per input
byte; the default uses observed-alphabet units. Finite estimates are not
clipped to `[0, 1]`. A value above one in normalized units is possible and
must not be silently clipped.

Use this function when entropy density is all you need: it avoids the shuffle
stage performed by `emc` and `lz76`. See [entropy density](../concepts/entropy-density.md)
for assumptions and finite-length interpretation.

## `emc`

```python
emc(seq, partitions=1, alphabet=None, log_base=None,
    max_block_size=-1, jobs=0)
```

Returns `(value, summands)` for the **shuffle-based magnitude qualitatively
similar to EMC**. The historical function name does not mean that the number
is exact effective measure complexity or excess entropy.

Each scale starts from the original bytes, performs aligned block swaps, and
obtains another LZ76 count. The current calculation weights the contrast by
block size, fits a nonnegative nondecreasing ladder, and returns its last
level. `summands[l - 1]` is the increase between fitted levels `l-1` and `l`.
The increments sum to `value`; they are not established conditional-entropy
estimates. The [EMC-like measure](../concepts/emc.md) page gives the full
calculation and actual shuffle boundaries.

```python
import math
import lzcomplexity as lz

seq = b"00010111" * 256
value, summands = lz.emc(seq, log_base=2, max_block_size=8)
assert value >= 0
assert len(summands) == 8
assert math.isclose(sum(summands), value, abs_tol=1e-12)
```

Compare matched lengths, encodings, units and block caps. There is one
content-seeded surrogate per scale. The cap, fixed suffix and finite-sample
LZ behavior can influence the score. No calibrated confidence interval is
returned. Base-two normalization does not make the score a calibrated count
of shared-information bits.

## `nid`

```python
nid(seq1, seq2, partitions=1, alphabet=None, log_base=None, jobs=0)
```

Returns the LZ-based normalized information-distance estimate

```text
max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))
```

`XY` means concatenation, preserving the symbol codebook. The ratio uses
factor counts, so `alphabet` and `log_base` do not change it. The formula is
symmetric in the two arguments. Finite-sequence values are not clipped and
need not satisfy every metric axiom or give a zero diagonal. Use an
appropriate similarity workflow rather than assuming the output is a
probability. See [information distance](../concepts/nid.md).

## `lz76`

```python
lz76(seq, partitions=1, alphabet=None, log_base=None,
     max_block_size=-1, jobs=0)
```

Returns the complete analysis as a dictionary. The original factorization is
reused; shuffled surrogates and half-sequences still require additional work.

```python
import lzcomplexity as lz

result = lz.lz76(b"00010111" * 256, log_base=2, max_block_size=8)
print(result["complexity"])
print(result["h"])
print(result["emc"]["value"])
```

| Key | Type | Meaning |
|---|---|---|
| `complexity` | `int` | LZ76 count. |
| `h` | `float` | Entropy density in the requested units. |
| `factors` | `list[int]` | Byte boundary positions, including the terminal convention described above. |
| `emc` | `dict` | `value`, `summands`, `max_block_size`, `multi_information`. |
| `epsilon` | `float` | Legacy finite-size expression using the alphabet parameter. |
| `factors_stddev` | `float` | Legacy dispersion formula based on boundary lengths; not a standard error on `h` or the shuffle score. |
| `normal_error` | `float` | Legacy formula using `h`, boundary dispersion and length. |
| `poison_error` | `float` | Legacy field spelling, retained for compatibility; evaluates `h / N` when `N > 1`. |
| `extras` | `dict` | Half-sequence factor-count ratios listed below. |

`emc["multi_information"]` is the raw single-symbol shuffle contrast before
projection. It can be negative and is not calibrated Shannon mutual
information. The legacy error fields are not confidence intervals on the
EMC-like score.

The five `extras` keys are `rajski_distance`, `redundancy`, `fh_uncertainty`,
`lh_uncertainty` and `pearson_coefficient`. They are formulas involving the
whole- and half-sequence LZ counts, not Shannon entropies or a correlation
coefficient calculated from the original samples. Their definitions appear in
the [Rust reference](rust.md#extra-measures).

## Empty and constant inputs

The API preserves the following conventions instead of rejecting degenerate
inputs:

| Input | `factorization` | `h` | `emc` |
|---|---|---:|---|
| `""` | `(1, [0, 1, 0])` | `0.0` | `(0.0, [0.0])` with automatic cap |
| `"a"` | `(1, [0, 1, 1])` | `0.0` | `(0.0, [0.0])` with automatic cap |
| `"aaaa"` | `(1, [0, 1, 4])` | `0.5` | `(0.0, [0.0])` with automatic cap |

The empty boundary list is therefore non-monotonic. Validate nonempty input
in your own pipeline when emptiness indicates missing data. Constant input
scores zero under the shuffle contrast because swapping identical symbols
cannot change its factor count; that does not say it has no visible pattern.
Inputs of ten bytes or fewer are also unchanged by the current block sampler.

## Version and runtime help

```python
import lzcomplexity as lz

print(lz.__version__)
help(lz.emc)
```

Type stubs ship with the package. The public Python API does not expose the
old `sequence`, `LempelZiv76`, `factors` or `entropy_density` names, nor the
removed spectral-analysis API. Native functions run synchronously; Python's
`jobs=` argument does not resize Rayon's thread pool. Use the CLI `-j` when
explicit per-process worker control is required.

Source: [Python bindings](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-py/src/lib.rs)
and [type stubs](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/python/lzcomplexity/__init__.pyi).
