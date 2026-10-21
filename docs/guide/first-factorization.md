# Your first factorization

Start with a sequence you can inspect by eye, then move to a longer example. You will obtain a factor count, an entropy-rate estimate and a shuffle-based view of organization from the same core.

Install the package using the [installation guide](install.md). These examples use the current Python API.

## 1. Inspect a short sequence

```python
import lzcomplexity as lz

sequence = "banana"
count, boundaries = lz.factorization(sequence)
print(count)       # 3
print(boundaries)  # [0, 1, 2, 3, 7]
```

`count` is the LZ76 complexity reported by the implementation. `boundaries` contains parser positions in the input bytes. The final value can exceed the input length: the parser attempted to extend a repeated suffix into a new phrase. That unfinished phrase stays visible in the boundary vector but is excluded from the count.

For a display of the pieces actually present:

```python
pieces = [
    sequence[start:min(end, len(sequence))]
    for start, end in zip(boundaries, boundaries[1:])
]
assert pieces == ["b", "a", "n", "ana"]
```

The count of completed phrases is three, even though four pieces are visible. Read [LZ76 factorization](../concepts/lz76.md) before comparing counts with another implementation. For non-ASCII strings, encode to bytes before slicing by the returned positions.

## 2. Choose the entropy units

```python
h_normalized = lz.h(sequence)
h_bits = lz.h(sequence, log_base=2)
print(round(h_normalized, 6))  # 0.815465
print(round(h_bits, 6))        # 1.292481
```

The default base is the observed alphabet size, three for `banana`. Base two expresses the estimate in bits per input byte symbol. These are two units for the same normalized factor-count calculation, not two different source models.

A six-byte word is useful for understanding the API. It is far too short to establish the entropy rate of a language.

## 3. Analyze a longer periodic example

```python
sequence = "00010111" * 256
result = lz.lz76(sequence, log_base=2, max_block_size=8)

print(result["complexity"])       # 4
print(result["h"])                # 0.021484375
print(result["emc"]["value"])    # 3.2105712890625
print(result["emc"]["max_block_size"])  # 8
```

The periodic string introduces little new material as it grows, giving a small entropy-density estimate. Block swaps at some scales disturb its organization, giving a positive shuffle-based score. The score is **qualitatively similar to EMC**; it is not a calibrated number of past/future information bits.

Inspect the fitted increments if you need the scale profile:

```python
score = result["emc"]["value"]
terms = result["emc"]["summands"]
assert len(terms) == 8
assert all(term >= 0 for term in terms)
assert abs(sum(terms) - score) < 1e-12
```

These increments come from the fitted shuffle-contrast curve. They are not independent estimates of conditional entropies. [The shuffle explanation](../concepts/emc.md) follows the calculation step by step.

## 4. Compare two sequences

```python
assert lz.nid("abcd", "abce") == 0.25
```

`nid` uses the factor counts of each input and both concatenation orders. It is a finite LZ-based dissimilarity. Even identical inputs can have nonzero self-distance, so inspect the [distance conventions](../concepts/nid.md) before passing a matrix into a method requiring an exact metric.

## 5. Keep the measurement settings

```python
record = {
    "version": lz.__version__,
    "length_bytes": len(sequence.encode("utf-8")),
    "log_base": 2,
    "max_block_size": 8,
    "complexity": result["complexity"],
    "h": result["h"],
    "shuffle_score": result["emc"]["value"],
}
```

For real data, also record how the sequence was produced: thresholds or bins, sampling and windowing, missing-data rules and source identifiers. Repeated identical calls are reproducible; they do not generate independent shuffle samples.

**Continue with:** [reading the numbers](reading-the-numbers.md), [input encoding](../api/inputs.md), or a [neural time-series example](../recipes/neuro.md).
