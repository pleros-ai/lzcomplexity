# Your first factorization

Start with a short sequence you can inspect by eye, then explore entropy density, effective measure complexity E and information distance.

Install the package using the [installation guide](install.md).

## 1. Inspect a short sequence

```python
import lzcomplexity as lz

sequence = "1000110011101"
count, boundaries = lz.factorization(sequence)
print(count)       # 5
print(boundaries)  # [0, 1, 2, 5, 9, 13]
```

![Binary LZ76 factors: 1·0·001·1001·1101. The matching prefix 100 crosses earlier factor boundaries.](../assets/sequence-workflow.svg)

In `1001`, the prefix `100` already occurs across earlier factor boundaries: `1·0·0`. Adding `1` gives `1001`, which is new. The parser matches substrings, including those crossing factor boundaries; [LZ76 factorization](../concepts/lz76.md) explains the full rule.

`count` is the LZ76 complexity. The boundaries are positions in the input bytes. To display the factors:

```python
pieces = [
    sequence[start:min(end, len(sequence))]
    for start, end in zip(boundaries, boundaries[1:])
]
assert pieces == ["1", "0", "001", "1001", "1101"]
```

A repeated suffix can be an unfinished factor. For example, `banana` has three complete factors followed by a copied tail, `b | a | n | ana`: the function returns count `3` and boundaries `[0, 1, 2, 3, 7]`. The boundary beyond the input length marks that unfinished tail; it is excluded from the count. See [LZ76 factorization](../concepts/lz76.md) for this convention. For non-ASCII strings, encode to bytes before slicing by the returned positions.

## 2. Choose the entropy units

```python
h_normalized = lz.h(sequence)
h_bits = lz.h(sequence, log_base=2)
print(round(h_normalized, 6))  # 1.423246
print(round(h_bits, 6))        # 1.423246
```

The default base is the observed alphabet size. This sequence is binary, so the default and base-two results are the same, in bits per input byte. Short-sample estimates can exceed one; see [reading the numbers](reading-the-numbers.md).

Short examples help explain the calculation; use longer samples to study a source's entropy rate.

## 3. Measure a periodic sequence

```python
sequence = "00010111" * 256
result = lz.lz76(sequence, log_base=2, max_block_size=8)

print(result["complexity"])       # 4
print(result["h"])                # 0.021484375
print(result["emc"]["value"])    # Chao–Shen estimate
print(result["emc"]["max_block_size"])  # 8
```

The repeated motif introduces little new information as the string grows, giving a small h. E describes organization across the ranges examined by the calculation. Its value is returned in `result["emc"]["value"]`.

You can also inspect E's profile increments:

```python
E = result["emc"]["value"]
terms = result["emc"]["summands"]
assert len(terms) == 8
assert all(term >= 0 for term in terms)
assert abs(sum(terms) - E) < 1e-12
```

The increments sum to E. See [effective measure complexity E](../concepts/emc.md) for their interpretation and the calculation.

## 4. Compare two sequences

```python
assert lz.nid("abcd", "abce") == 0.25
```

`nid` compares recurring patterns using the factor counts of each sequence and both concatenation orders. Lower values indicate greater similarity under this measure. For finite sequences, self-distance can be nonzero; see [information distance](../concepts/nid.md) when using the results in clustering or other downstream methods.

## 5. Keep the measurement settings

```python
record = {
    "version": lz.__version__,
    "length_bytes": len(sequence.encode("utf-8")),
    "log_base": 2,
    "max_block_size": 8,
    "complexity": result["complexity"],
    "h": result["h"],
    "E": result["emc"]["value"],
}
```

For real data, record how the sequence was produced: thresholds or bins, sampling, windowing, missing-data handling and record identifiers. Identical inputs and settings give reproducible results.

**Continue with:** [reading the numbers](reading-the-numbers.md), [input encoding](../api/inputs.md), or a [neural time-series example](../recipes/neuro.md).
