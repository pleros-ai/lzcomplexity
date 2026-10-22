# `nid()`

Compare two symbolic sequences using an information-distance estimate derived
from Normalized Information Distance (NID). Lower values indicate more shared
patterns under this measure. Use it for recordings, texts or symbolic
trajectories encoded in the same way.

```text
nid(seq1, seq2, partitions=1, alphabet=None, log_base=None, jobs=0) -> float
```

## Parameters

| Parameter | Default | Behavior |
|---|---|---|
| `seq1` | required | First supported [sequence input](../inputs.md). |
| `seq2` | required | Second supported sequence input, converted independently of `seq1`. |
| `partitions` | `1` | Accepted; has no effect. |
| `alphabet` | `None` | Does not change the returned distance. |
| `log_base` | `None` | Does not change the returned distance: the ratio uses factor counts. |
| `jobs` | `0` | Accepted; has no effect. Native workers use [Rayon configuration](../../project/determinism.md). |

Arguments may have different lengths or accepted Python types. They must
represent corresponding symbols with the same byte codes. If byte `0`
represents a resting state in one sequence, it must represent the same state
in the other. Do not create independent codebooks for a comparison.

See [parameter types and errors](../python.md#parameter-types-and-errors)
for integer ranges and conversion errors.

## Return value

Returns a `float`. The calculation uses the LZ76 counts of each input and
both concatenation orders, making the result symmetric:
`nid(a, b) == nid(b, a)`. It needs no alignment of individual positions.

The finite-sequence estimate is returned without clipping. Self-distance
can be nonzero, so evaluate that baseline before passing the values to a
method that requires a zero diagonal.

## Compare sequences

```python
import lzcomplexity as lz

first = "abcd"
second = "abce"
distance = lz.nid(first, second)

assert distance == 0.25
assert lz.nid(second, first) == distance
assert lz.nid("banana", "banana") == 1 / 3
```

This comparison concerns recurring patterns in the sequences, rather than
a position-by-position mismatch fraction.

## Build pairwise comparisons

```python
import lzcomplexity as lz

samples = [b"abcd", b"abce", b"wxyz"]
distances = [
    [lz.nid(first, second) for second in samples]
    for first in samples
]

assert distances[0][1] == 0.25
assert all(
    distances[i][j] == distances[j][i]
    for i in range(len(samples))
    for j in range(len(samples))
)
```

For many sequences, compute each unordered pair once, then fill the symmetric
matrix. The [batch-distance recipe](../../recipes/batch-distance.md) gives a
larger example; the [`lzdistance` binary](../../cli/lzdistance.md) handles
pairwise file collections from the command line.

## Input and length choices

Use matched preprocessing and, when possible, comparable sample lengths.
For text, decide whether symbols represent bytes, characters, words or other
tokens before coding them. A single shared codebook is required for tokenized
inputs. See [input encoding](../inputs.md) and
[sequence length](../../concepts/convergence.md).

Empty inputs are accepted. `nid("", "")` returns `0.0`; comparing an empty
input with a nonempty one need not give zero. Treat an absent observation
separately from an observed sequence when preparing a dataset.

**Related:** [information distance](../../concepts/nid.md),
[`factorization()`](factorization.md), [`lz76()`](lz76.md).
