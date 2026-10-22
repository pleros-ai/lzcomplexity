# `h()`

Estimate entropy density: how much new information the sequence produces per
symbol. This captures unpredictability and the freedom to vary. Use `h()`
when that estimate is the only result you need.

```text
h(seq, partitions=1, alphabet=None, log_base=None, jobs=0) -> float
```

## Parameters

| Parameter | Default | Behavior |
|---|---|---|
| `seq` | required | A supported [sequence input](../inputs.md), converted to bytes. |
| `partitions` | `1` | Accepted; has no effect. |
| `alphabet` | `None` | Does not change h or select its logarithm base. |
| `log_base` | `None` | Normalization base. Omitted: observed byte alphabet size, with a minimum of two. Pass `2` for bits per input byte. Values zero and one are treated as two. |
| `jobs` | `0` | Accepted; has no effect. Native workers use [Rayon configuration](../../project/determinism.md). |

See [parameter types and errors](../python.md#parameter-types-and-errors)
for integer ranges and conversion errors. `alphabet` is retained in the
signature for consistency with the complete analysis; it affects its
`epsilon` diagnostic, not h.

## Return value and units

Returns a `float`. For byte length $N>1$, factor count $C$ and logarithm base
$b$, the estimate is

$$
\hat{h} = \frac{C\log_b N}{N}.
$$

For $N\leq1$, h is zero. The factor count follows the same boundary convention
as [`factorization()`](factorization.md).

With `log_base=2`, the result is in bits per input byte. The default uses
observed-alphabet units. A byte may represent one encoded state or part of a
UTF-8 character, depending on the input.

## Select the units

```python
import lzcomplexity as lz

sequence = "banana"
h_normalized = lz.h(sequence)
h_bits = lz.h(sequence, log_base=2)

assert round(h_normalized, 6) == 0.815465
assert round(h_bits, 6) == 1.292481
```

This sequence has three distinct byte symbols, so the default base is three.
Changing the base changes the units, while keeping the same factorization.

```python
import lzcomplexity as lz

assert lz.h("01010101", alphabet=4) == 0.75
assert lz.h("01010101", log_base=4) == 0.375
```

Only `log_base` selects the normalization in this example.

## Compare samples consistently

Use the same encoding, sample length and logarithm base across conditions.
For a longer recording, calculate h over windows of a fixed size:

```python
import lzcomplexity as lz

sequence = b"00010111" * 512
window_size = 1024
values = [
    lz.h(sequence[start:start + window_size], log_base=2)
    for start in range(0, len(sequence) - window_size + 1, window_size)
]
assert len(values) == 4
assert values == [lz.h(sequence[:window_size], log_base=2)] * 4
```

This is a finite-sequence estimate, so short samples can yield a normalized
value above one. Results are returned without clipping. Longer observations
and comparison across window sizes help assess convergence; see
[sequence length](../../concepts/convergence.md).

## Empty and constant sequences

Empty and one-byte inputs return `0.0`. A constant sequence longer than one
byte has factor count one and returns `log_b(N) / N`; for example,
`lz.h("aaaa", log_base=2)` is `0.5`. This finite-size value tends to zero as
the constant sequence becomes longer.

**Related:** [entropy density](../../concepts/entropy-density.md),
[`emc()`](emc.md), [`lz76()`](lz76.md).
