# `emc()`

Calculate effective measure complexity E and its range profile. E captures
organization and correlations across different ranges of a sequence. Use it
alongside h to study the balance between new information and the context
connecting it.

```text
emc(seq, partitions=1, alphabet=None, log_base=None,
    max_block_size=-1, jobs=0, *, method="chao-shen") -> tuple[float, list[float]]
```

## Parameters

| Parameter | Default | Behavior |
|---|---|---|
| `seq` | required | A supported [sequence input](../inputs.md), converted to bytes. |
| `partitions` | `1` | Accepted; has no effect. |
| `alphabet` | `None` | Does not change E or select its logarithm base. |
| `log_base` | `None` | Normalization base. Omitted: observed byte alphabet size, with a minimum of two. Pass `2` for base-two units. Values zero and one are treated as two. |
| `max_block_size` | `-1` | Largest examined range in bytes. A positive `M` includes ranges `1` through `M`; zero or a negative value selects the limit automatically from input length and alphabet size, stopping at the earliest raw ladder peak for Chao–Shen. |
| `method` | `"chao-shen"` | Keyword-only. `"chao-shen"` uses coverage-adjusted block entropies; `"shuffle"` reproduces the earlier random-shuffle estimate. |
| `jobs` | `0` | Accepted; has no effect. Native workers use [Rayon configuration](../../project/determinism.md). |

See [parameter types and errors](../python.md#parameter-types-and-errors)
for integer ranges and errors. `max_block_size=None` is not accepted: omit
the argument or use a nonpositive integer for automatic selection.

## Return value

Returns `(value, summands)`:

| Item | Type | Meaning |
|---|---|---|
| `value` | `float` | E: final level of the fitted range profile. |
| `summands` | `list[float]` | Nonnegative increases between successive fitted levels, in range order. Their sum is E. |

For an explicit positive limit `M`, `len(summands)` is `M`.
`summands[l - 1]` is the increase from fitted level `l - 1` to level `l`,
starting from level zero. A cumulative sum reconstructs the nondecreasing
profile.

## Calculate E and inspect its profile

```python
import math
from itertools import accumulate
import lzcomplexity as lz

sequence = b"00010111" * 256
E, increments = lz.emc(sequence, log_base=2, max_block_size=8)
profile = list(accumulate(increments))

assert math.isclose(E, 2.8818337213449023, abs_tol=1e-12)
assert len(increments) == 8
assert all(term >= 0 for term in increments)
assert math.isclose(sum(increments), E, abs_tol=1e-12)
assert profile[-1] == E
```

The profile shows where the fitted result increases as longer ranges are
included. A zero increment means that the fitted profile is flat at that step.

## Choose a maximum range

Choose ranges according to the patterns represented by the bytes. For one
byte per sampled state, sixteen bytes correspond to sixteen consecutive
observations. For UTF-8 text, bytes and characters can have different lengths.

Keep sequence length, encoding, logarithm base and maximum range the same
when comparing samples. An explicit cap also makes the amount of work
predictable. A longer maximum range does not guarantee a larger final E:
changing the included ranges can change the fitted profile.

Inspect an automatically selected limit through the complete result:

```python
import lzcomplexity as lz

result = lz.lz76(b"00010111" * 256, log_base=2)
selected_range = result["emc"]["max_block_size"]
assert selected_range == len(result["emc"]["summands"])
```

`lz76()` also exposes `multi_information`, the first-range contrast before
the fitted profile. Use [`lz76()`](lz76.md#effective-measure-complexity-result)
when those fields are needed together with h and the factorization.

## Reproducibility and short inputs

The same input bytes and parameters give the same E and increments. Worker
scheduling does not change the result. E can depend on the symbol encoding
and alignment, so keep preprocessing consistent and retain the settings.

Constant inputs return E zero. The shuffle estimator also returns zero
for inputs of ten bytes or fewer; Chao–Shen can give a nonzero estimate for
short inputs. Empty and one-byte inputs produce zero increments;
an explicit positive cap still determines their list length. Use longer
sequences when exploring E and its profile.

**Related:** [effective measure complexity](../../concepts/emc.md),
[reproducibility](../../project/determinism.md), [`h()`](h.md).
