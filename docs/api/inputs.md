# Input types and symbol encoding

The core analyses **bytes**. Your encoding decides what one symbol means,
which changes the factorization, sequence length and normalization. Prepare
that encoding before asking the library to measure a signal.

## What the Python bridge accepts

Conversion follows this order:

| Input | Bytes actually analysed |
|---|---|
| `str` | Its UTF-8 encoding. |
| `bytes` | The supplied bytes, unchanged. |
| `list[str]` | Strings joined with no separator, then UTF-8 encoded. |
| Integer sequences, including compatible one-dimensional integer arrays | Decimal representation of each integer, concatenated with no separator. |
| Empty `list` | Empty byte sequence. |

Integer inputs use the Python sequence protocol and must fit signed 64-bit
integers. Tuples and compatible arrays work. Materialize iterators and
generators with `list(...)`, or convert them to bytes. Bin continuous values
before passing them to the library.

!!! tip "Use `bytes` for one symbol per sample"

    `bytes([0, 1, 10])` represents **three** symbols. Passing `[0, 1, 10]`
    directly represents the four ASCII characters `"0110"`.

```python
import lzcomplexity as lz

assert lz.factorization([0, 1, 10]) == lz.factorization("0110")
assert lz.factorization(["ban", "ana"]) == lz.factorization("banana")
assert lz.factorization((0, 1)) == lz.factorization("01")
```

## Text and Unicode

ASCII text has one byte per character. Other Unicode characters often occupy
multiple bytes. Factor boundaries index the encoded bytes, not Python
character positions.

```python
import lzcomplexity as lz

text = "café"
encoded = text.encode("utf-8")
assert len(text) == 4
assert len(encoded) == 5
assert lz.factorization(text) == lz.factorization(encoded)
```

If the scientific symbols are words, event names or characters, create an
explicit codebook instead of assuming UTF-8 preserves one symbol per item.
A lone surrogate that cannot be UTF-8 encoded is rejected.

`bytes` can contain every value from zero to 255, including NUL. It does not
perform UTF-8 decoding or integer-to-decimal conversion. Convert `bytearray`
and `memoryview` objects explicitly with `bytes(...)`; passing them directly
can take the integer-sequence path and change the data.

## Integer symbols

For integers already in `[0, 255]`, use the standard constructor:

```python
import lzcomplexity as lz

symbols = bytes([0, 1, 10, 0, 1, 10])
assert len(symbols) == 6
result = lz.lz76(symbols, log_base=2, max_block_size=4)
```

Negative values and values above 255 need a codebook. Do not serialize each
integer as text: the minus sign and each decimal digit would become separate
symbols. Python booleans passed as an integer list become decimal `0`/`1`;
NumPy boolean masks should be converted explicitly to bytes.

For NumPy integer arrays whose range is known and validated:

```python
import numpy as np
import lzcomplexity as lz

labels = np.array([0, 1, 10, 0, 1, 10], dtype=np.uint8)
symbols = labels.tobytes()
assert len(symbols) == labels.size
value = lz.h(symbols, log_base=2)
```

Do not call `.tobytes()` on an arbitrary `int64` array and expect one byte per
sample: it writes the full binary representation, usually eight bytes per
element. Likewise, casting unknown values to `uint8` can wrap them. Validate
or map the labels before conversion.

## A codebook for categories

This example uses one shared mapping for two sequences of string labels.
Sorting makes the mapping reproducible for the same label vocabulary.

```python
import lzcomplexity as lz

first = ["rest", "move", "rest", "move"]
second = ["rest", "rest", "move", "move"]
vocabulary = sorted(set(first) | set(second))
if len(vocabulary) > 256:
    raise ValueError("The byte representation supports at most 256 symbols")
codebook = {label: code for code, label in enumerate(vocabulary)}
a = bytes(codebook[label] for label in first)
b = bytes(codebook[label] for label in second)
distance = lz.nid(a, b)
```

Keep the codebook with the results. Independent codebooks can assign the
same byte to different events, making a distance comparison misleading.
Use one encoding across a comparison of E values as well.

There is no general wide-alphabet mode. Encoding a single category as a
multi-byte token changes the measured sequence and introduces internal token
structure. When more than 256 states matter, make the reduction or alternate
method an explicit analysis decision.

## Continuous signals

Continuous observations must be symbolized deliberately. For a fixed binary
threshold:

```python
import numpy as np
import lzcomplexity as lz

signal = np.array([-0.2, 0.5, 0.1, -0.7, 0.8, 0.4, -0.1, -0.3])
symbols = (signal > 0.0).astype(np.uint8).tobytes()
assert len(symbols) == signal.size
entropy_density = lz.h(symbols, log_base=2)
```

The threshold is part of the model of the data. A median split, fixed
threshold and quantile bins answer different questions. Record the rule,
handle missing values first, and use matched preprocessing across groups.
See the [neural time-series recipe](../recipes/neuro.md) for a complete
workflow. The core does not filter, interpolate or infer sampling intervals.

## Rejection and degenerate-input behavior

| Input | Result |
|---|---|
| `[1.5, 2.5]`, a nonempty float array | `TypeError`; no implicit binning. |
| `None`, a scalar integer, a dictionary | `TypeError`. |
| `iter([0, 1])`, a generator | `TypeError`; use `list(...)` or prepared bytes. |
| `("a", "b")` | `TypeError`; only a `list[str]` uses string joining. |
| `["a", 1]` | Conversion `TypeError` in the string-list branch. |
| Integer values outside signed 64-bit range | Rejected during conversion. |
| `""`, `b""`, `[]`, `()` | Accepted as empty input. |

An empty integer-compatible sequence can be accepted even if its declared
array dtype would reject a nonempty value. Validate emptiness and dtype in
your own pipeline rather than relying on type rejection to detect missing
data.

The library returns a factor count of one for empty and constant sequences.
For empty input, `factorization` returns `(1, [0, 1, 0])`; `h` returns zero.
See [the Python reference](python.md#empty-and-constant-inputs) for the complete
empty-input results.

## Memory and length

The byte representation itself uses one byte per symbol, but factorization
also needs suffix-array, LCP and longest-previous-factor storage and temporary
buffers. Examining several ranges in parallel increases the working set. Input
conversion can allocate additional copies; preparing bytes avoids the
intermediate integer-to-decimal representation.

Index arrays use 32-bit values, and the suffix-array dependency uses signed
indices. Check memory use on representative lengths and range limits before
scaling up an analysis.

Source: [input coercion](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-py/src/lib.rs).
