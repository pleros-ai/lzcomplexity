# Python cheatsheet

Measure randomness with **h**, correlations across ranges with **E**, and
shared patterns between sequences with **information distance**.

```sh
python -m pip install lzcomplexity
```

## Five functions

```python
import lzcomplexity as lz

count, boundaries = lz.factorization("banana")
# 3, [0, 1, 2, 3, 7]

h = lz.h("01010101", log_base=2)
# 0.75

sequence = b"00010111" * 256
E, increments = lz.emc(sequence, log_base=2, max_block_size=16)

distance = lz.nid("abcd", "abce")
# 0.25

result = lz.lz76(sequence, log_base=2, max_block_size=16)
print(result["complexity"], result["h"], result["emc"]["value"])
```

| Call | Use it for | Returns |
|---|---|---|
| `factorization(sequence)` | Count LZ76 factors. | `(int, list[int])`: count and byte boundaries. |
| `h(sequence)` | Measure randomness or variability. | `float`: entropy density. |
| `emc(sequence)` | Capture correlations across ranges. | `(float, list[float])`: E and its increments. |
| `nid(first, second)` | Compare shared patterns. | `float`: information distance. |
| `lz76(sequence)` | Get count, h, E and factor boundaries together. | `dict`: the complete analysis. |

Use the returned factor count directly. Boundaries index bytes, and the final
boundary can extend beyond the input. The [factorization guide](first-factorization.md)
shows how to display the phrases.

## Choose a representation

The library processes **symbol sequences**: text, discrete states, event codes,
DNA bases, or a signal that you have converted into symbols.

| Input | What gets analyzed |
|---|---|
| `"ACGTACGT"` | UTF-8 bytes; ASCII characters use one byte each. |
| `b"ACGTACGT"` | The supplied bytes, unchanged. |
| `bytes([0, 1, 10])` | Three symbols with codes 0, 1 and 10. |
| `["AC", "GT"]` | Joined text: `"ACGT"`. |
| `[0, 1, 10]` or an integer array | Joined decimal text: `"0110"`. |

For a Python list of integer state labels from 0 to 255, use `bytes(labels)`
to preserve one symbol per sample. With NumPy, a validated `uint8` array's `.tobytes()` does
the same. Use the same symbol encoding for sequences you compare.

### Continuous signals → symbols

Choose a threshold, bins or another encoding suited to your experiment:

```python
signal = [-0.2, 0.5, 0.1, -0.7, 0.8, 0.4, -0.1, -0.3]
symbols = bytes(value > 0 for value in signal)  # negative/zero: 0; positive: 1
h = lz.h(symbols, log_base=2)
```

The analysis describes the resulting symbolic sequence. See
[input conversion](../api/inputs.md) for arrays, Unicode and encoding choices.

## Two useful controls

| Parameter | When to set it |
|---|---|
| `log_base=2` | Report h in bits per input byte. The default uses the observed byte alphabet size, with a minimum of two. |
| `max_block_size=16` | Include ranges up to 16 bytes in E. Available in `emc` and `lz76`; omit it to select the range automatically. |

Compare h and E using the same encoding, input length and settings. A longer
sequence gives more room to investigate correlations across ranges.

Continue with the [Python API](../api/python.md),
[signal recipes](../recipes/dynamical-systems.md), or the
[standalone binaries cheatsheet](../cli/quick-reference.md).
