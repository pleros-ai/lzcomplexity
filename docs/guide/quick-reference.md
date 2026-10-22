---
title: Python usage cheatsheet
template: cheatsheet.html
sheet_kind: python
sheet_description: Install, prepare inputs and call the five functions.
description: A short usage cheatsheet for the lzcomplexity Python library.
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

For state codes 0–255, use `bytes(labels)`; for a NumPy `uint8` array,
use `labels.tobytes()`. [Input guide](../api/inputs.md).

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

Here nonpositive → 0, positive → 1.

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## Factorization

```python
count, boundaries = lz.factorization(
    "1000110011101"
)
# count: 5
# boundaries: [0, 1, 2, 5, 9, 13]
```

Use the returned count. Slice encoded bytes using `boundaries`.
[Worked example](first-factorization.md).

</section>

<section class="lz-sheet-section lz-sheet-teal" markdown="1">

## Entropy density h

```python
h = lz.h("01010101", log_base=2)
# 0.75
```

`log_base=2` selects bits per input byte. Returns `float`.

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

Unpack `(E, increments)`. `max_block_size` sets the maximum range.

</section>

<section class="lz-sheet-section lz-sheet-blue" markdown="1">

## NID · compare sequences

```python
distance = lz.nid("abcd", "abce")
# 0.25
```

Returns `float`. Use the same symbol encoding for both inputs.

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

[Full result reference](../api/python.md#lz76).

</section>

<section class="lz-sheet-section lz-sheet-violet" markdown="1">

## Two useful controls

| Setting | Effect |
|---|---|
| `log_base=2` | h in bits per input byte. |
| `max_block_size=16` | E includes ranges up to 16 bytes. |

Omit these controls for automatic settings. `max_block_size` belongs to
`emc` and `lz76`.

</section>

</div>
