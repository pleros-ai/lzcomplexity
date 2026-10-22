# Python API

Five functions share the same Rust core. Import the package once, then choose
whether you need a factor count, entropy density h, effective measure complexity E,
a pairwise distance, or the complete result.

```python
import lzcomplexity as lz

complexity, boundaries = lz.factorization("banana")
assert complexity == 3
assert boundaries == [0, 1, 2, 3, 7]
assert lz.h("01010101", log_base=2) == 0.75
assert lz.nid("abcd", "abce") == 0.25
```

See [input conversion](inputs.md) before analyzing numeric arrays.

## Choose a function

| Function | Work performed | Return |
|---|---|---|
| [`factorization`](#factorization) | One full-sequence factorization | `(int, list[int])` |
| [`h`](#h) | Factor count and entropy normalization | `float` |
| [`emc`](#emc) | Effective measure complexity across ranges | `(float, list[float])` |
| [`nid`](#nid) | Original and concatenated sequence factorizations | `float` |
| [`lz76`](#lz76) | Full analysis, reusing the original factorization | `dict` |

## Shared parameters

| Parameter | Default | Behavior |
|---|---|---|
| `seq` | required | Converts to bytes using the [input rules](inputs.md). |
| `partitions` | `1` | Unused. |
| `alphabet` | `None` | Alphabet parameter for the `epsilon` diagnostic. `None` uses the observed byte alphabet, floored at two. It does not set the default logarithm base. |
| `log_base` | `None` | Uses the observed byte alphabet, floored at two. Set `2` for base-two normalization. Explicit zero or one is also floored to two. |
| `jobs` | `0` | Unused. Configure Rayon for native worker control. |
| `max_block_size` | `-1` | Only `emc` and `lz76`: positive values set the maximum block range; nonpositive values select it automatically. |

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

$$
\hat{h} = \frac{C\log_b N}{N}
$$

Here `C` is the returned factor count and `N` is the number of input bytes.
For `N <= 1`, the function returns zero. `log_base=2` gives bits per input
byte; the default uses observed-alphabet units. Finite estimates can exceed
one in normalized units; the returned value is kept without clipping.

Use this function when entropy density is all you need; it avoids the additional
calculations performed by `emc` and `lz76`. See [entropy density](../concepts/entropy-density.md)
for assumptions and finite-length interpretation.

## `emc`

```python
emc(seq, partitions=1, alphabet=None, log_base=None,
    max_block_size=-1, jobs=0)
```

Returns `(value, summands)` for **effective measure complexity E**, which is sensitive
to correlations across different ranges of a sequence.

`value` is the final level of the fitted scale profile. `summands[l - 1]` is
the increase between fitted levels `l-1` and `l`, and the increments sum to
`value`. See [E](../concepts/emc.md) for interpretation and range selection.

```python
import math
import lzcomplexity as lz

seq = b"00010111" * 256
value, summands = lz.emc(seq, log_base=2, max_block_size=8)
assert value >= 0
assert len(summands) == 8
assert math.isclose(sum(summands), value, abs_tol=1e-12)
```

Compare E with matched lengths, encodings, units and range limits. Use
`max_block_size` to examine how the result changes as longer ranges are included.

## `nid`

```python
nid(seq1, seq2, partitions=1, alphabet=None, log_base=None, jobs=0)
```

Returns the LZ-based normalized information-distance estimate

$$
d(X,Y) = \frac{\max\left(C(XY)-C(X),\,C(YX)-C(Y)\right)}{\max\left(C(X),C(Y)\right)}
$$

`XY` means concatenation, preserving the symbol codebook. The ratio uses
factor counts, so `alphabet` and `log_base` do not change it. The formula is
symmetric in the two arguments. At finite lengths, self-distance can be
nonzero. Values are returned without clipping. See [information distance](../concepts/nid.md).

## `lz76`

```python
lz76(seq, partitions=1, alphabet=None, log_base=None,
     max_block_size=-1, jobs=0)
```

Returns the complete analysis as a dictionary. The original factorization is
reused for h, E and the other reported quantities.

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
| `epsilon` | `float` | Finite-size diagnostic using the alphabet parameter. |
| `factors_stddev` | `float` | Dispersion diagnostic derived from boundary lengths. |
| `normal_error` | `float` | Diagnostic using `h`, boundary dispersion and length. |
| `poison_error` | `float` | `h / N` when `N > 1`, otherwise zero. |
| `extras` | `dict` | Half-sequence factor-count ratios listed below. |

`emc["multi_information"]` is the raw first-range contrast, before fitting;
it can be negative.

The five `extras` keys are `rajski_distance`, `redundancy`, `fh_uncertainty`,
`lh_uncertainty` and `pearson_coefficient`. They summarize the original and
half-sequence factor counts. Their formulas appear in the
[Rust reference](rust.md#extra-measures). For uncertainty estimates, repeat the
analysis across independent samples or windows appropriate to the experiment;
the diagnostic fields do not provide confidence intervals.

## Empty and constant inputs

Empty and constant inputs return:

| Input | `factorization` | `h` | `emc` |
|---|---|---:|---|
| `""` | `(1, [0, 1, 0])` | `0.0` | `(0.0, [0.0])` with automatic cap |
| `"a"` | `(1, [0, 1, 1])` | `0.0` | `(0.0, [0.0])` with automatic cap |
| `"aaaa"` | `(1, [0, 1, 4])` | `0.5` | `(0.0, [0.0])` with automatic cap |

The empty boundary vector is non-monotonic, so handle empty inputs separately
when slicing. E is zero for constant sequences and for inputs of ten bytes or
fewer; use longer examples to explore its scale profile.

## Version and runtime help

```python
import lzcomplexity as lz

print(lz.__version__)
help(lz.emc)
```

Type stubs ship with the package. Native functions run synchronously. See
[parallel execution](../project/determinism.md) for worker configuration.

Source: [Python bindings](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-py/src/lib.rs)
and [type stubs](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/python/lzcomplexity/__init__.pyi).
