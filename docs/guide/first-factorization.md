# Your first factorization

Start with a short sequence you can inspect by eye, then explore entropy density, effective measure complexity E and information distance.

Install the package using the [installation guide](install.md).

## 1. Inspect a short sequence

```python
import lzcomplexity as lz

sequence = "ABRACADABRA!"
count, boundaries = lz.factorization(sequence)
print(count)       # 6
print(boundaries)  # [0, 1, 2, 3, 5, 7, 12]
```

The factors are `A | B | R | AC | AD | ABRA!`. For this example, each factor extends the longest match in the preceding text by one symbol:

| Position (from 1) | Earlier text | Matched prefix | Next symbol | Factor |
| --- | --- | --- | --- | --- |
| 1 | empty | none | `A` | `A` |
| 2 | `A` | none | `B` | `B` |
| 3 | `AB` | none | `R` | `R` |
| 4 | `ABR` | `A` | `C` | `AC` |
| 6 | `ABRAC` | `A` | `D` | `AD` |
| 8 | `ABRACAD` | `ABRA` | `!` | `ABRA!` |

The last match, `ABRA`, crosses earlier factor boundaries: it consists of the first `A`, `B`, `R` and the `A` in `AC`. Matches are substrings of earlier text; they do not have to be whole factors. “Next symbol” means the symbol that extends the match, rather than necessarily a symbol never seen before.

`count` is the LZ76 complexity. The boundaries are positions in the input bytes. To display the factors:

```python
pieces = [
    sequence[start:min(end, len(sequence))]
    for start, end in zip(boundaries, boundaries[1:])
]
assert pieces == ["A", "B", "R", "AC", "AD", "ABRA!"]
```

A repeated suffix can be an unfinished factor. For example, `banana` has three complete factors followed by a copied tail, `b | a | n | ana`: the function returns count `3` and boundaries `[0, 1, 2, 3, 7]`. The boundary beyond the input length marks that unfinished tail; it is excluded from the count. See [LZ76 factorization](../concepts/lz76.md) for this convention. For non-ASCII strings, encode to bytes before slicing by the returned positions.

## 2. Choose the entropy units

```python
h_normalized = lz.h(sequence)
h_bits = lz.h(sequence, log_base=2)
print(round(h_normalized, 6))  # 0.693426
print(round(h_bits, 6))        # 1.792481
```

The default base is the observed alphabet size, six for `ABRACADABRA!`. Base two expresses the estimate in bits per input byte. Both values describe the same sequence in different units.

Short examples help explain the calculation; use longer samples to study a source's entropy rate.

## 3. Measure a periodic sequence

```python
sequence = "00010111" * 256
result = lz.lz76(sequence, log_base=2, max_block_size=8)

print(result["complexity"])       # 4
print(result["h"])                # 0.021484375
print(result["emc"]["value"])    # 3.2105712890625
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
