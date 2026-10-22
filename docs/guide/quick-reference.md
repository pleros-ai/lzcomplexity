---
title: Python cheatsheet
template: cheatsheet.html
sheet_kind: python
sheet_description: Python API · symbolic sequence analysis
description: A compact Python reference for inputs, LZ76 factors, entropy density, effective measure complexity E and information distance.
---

<div class="lz-sheet-grid" markdown="1">

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Install and run

```sh
python -m pip install lzcomplexity
```

```python
import lzcomplexity as lz
```

Python 3.9+.

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Choose your symbols

| Input | Analyzed as |
|---|---|
| `"ACGT"` | UTF-8 bytes. |
| `b"ACGT"` | Raw bytes, unchanged. |
| `bytes([0, 1, 10])` | Three symbols: 0, 1, 10. |
| `["AC", "GT"]` | Joined text: `"ACGT"`. |
| `[0, 1, 10]` | Decimal text: `"0110"`. |

Integer arrays also become decimal text. For a Python list of state codes
0–255, use `bytes(labels)`. For a validated NumPy `uint8` array, use
`labels.tobytes()`. Keep one shared encoding when comparing sequences.

ASCII characters occupy one byte; other UTF-8 characters may occupy several.
[Input details](../api/inputs.md).

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Continuous signals → symbols

Threshold, bin, or encode a signal before analysis:

```python
signal = [-0.2, 0.5, 0.1, -0.7,
          0.8, 0.4, -0.1, -0.3]
symbols = bytes(x > 0 for x in signal)
h = lz.h(symbols, log_base=2)
```

Here nonpositive → 0 and positive → 1. The result describes your chosen
symbolic representation. [Signal recipes](../recipes/dynamical-systems.md).

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Factorization

```python
count, boundaries = lz.factorization(
    "banana"
)
# count:      3
# boundaries: [0, 1, 2, 3, 7]
```

Returns **`(int, list[int])`**. Use the returned count directly. Boundaries
are byte offsets; the last can extend beyond the input.
[Display the phrases](first-factorization.md).

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## h · randomness and variability

```python
h = lz.h("01010101", log_base=2)
# 0.75
```

Returns **`float`**: entropy density. Use h to investigate randomness,
variability or degrees of freedom in a symbolic sequence.

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## E · effective measure complexity

```python
sequence = b"00010111" * 256
E, increments = lz.emc(
    sequence, log_base=2,
    max_block_size=16,
)
```

Returns **`(float, list[float])`**. E captures correlations across ranges.
The fitted increments sum to E; here the maximum range is 16 bytes.

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## NID · compare sequences

```python
distance = lz.nid("abcd", "abce")
# 0.25
```

Returns **`float`**. Derived from Normalized Information Distance, it compares
shared patterns between sequences. Lower values indicate greater similarity.

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Get the complete result

```python
result = lz.lz76(
    sequence, log_base=2,
    max_block_size=16,
)
count = result["complexity"]
h = result["h"]
E = result["emc"]["value"]
boundaries = result["factors"]
```

Returns **`dict`**. The `emc` entry also contains `summands` and
`max_block_size`. [All result fields](../api/python.md#lz76).

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Two useful controls

| Setting | Effect |
|---|---|
| `log_base=2` | h in bits per input byte. |
| `max_block_size=16` | E includes ranges up to 16 bytes. |

Without `log_base`, the observed byte alphabet sets the base, with a minimum
of two. Omit `max_block_size` to select E's range automatically; this parameter
belongs to `emc` and `lz76`.

For comparisons, match encoding, sequence length and settings. Longer
sequences support exploration of longer correlations.

</section>

</div>
