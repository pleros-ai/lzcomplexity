# Alphabets, encoding and log bases

Symbolization is part of the measurement. Before calling `lzcomplexity`, decide what one symbol represents and how the same rule will be applied to every sample.

## The core sees bytes

A Rust `Sequence` is a vector of bytes. Python strings are UTF-8 encoded before analysis; Python `bytes` are used directly. The observed alphabet is the set of distinct byte values, with a size floored at two for ordinary automatic normalization.

| Input | Core representation | Consequence |
|---|---|---|
| `"ACGT"` | Four ASCII bytes | One byte per DNA letter |
| `b"\x00\x01"` | Two raw bytes | Zero bytes are ordinary symbols |
| `"é"` | UTF-8 bytes `c3 a9` | One Unicode character becomes two analyzed bytes |
| `[0, 1, 10]` | ASCII bytes of `"0110"` | Integer elements are concatenated as decimal text |

Integer lists are concatenated as decimal text: `[1, 10]` and `[11, 0]` both become `"110"`. For categories numbered `0…255`, construct a `bytes` object to give each category exactly one byte. For a larger vocabulary, choose an encoding that preserves the distinctions required by your analysis.

## Observed alphabet versus declared alphabet

Python has two related options with different jobs:

- `alphabet` selects the alphabet parameter used in the finite-size `epsilon` formula.
- `log_base` selects the normalization of `h` and E.

When `log_base=None`, Python/core use the observed input alphabet size, floored at two, even if `alphabet` was supplied explicitly.

```python
import lzcomplexity as lz

sequence = "01010101"
assert lz.h(sequence) == 0.75
assert lz.h(sequence, alphabet=4) == 0.75
assert lz.h(sequence, alphabet=4, log_base=4) == 0.375
```

The CLI uses a declared `-a` value when `-l` is omitted. These commands therefore request different units:

```sh
lzcomplexity sample.txt -a 4 -o normalized.json
lzcomplexity sample.txt -a 4 -l 2 -o bits.json
```

For reproducible Python/CLI comparisons, always choose the same explicit base.

## What base conversion changes

With all other settings fixed, converting from base two to base `b` divides `h` and E by `log₂(b)`. The factor count, input sequence and maximum block range stay the same.

`nid` is built from factor-count ratios; logarithm-base options do not enter that formula.

## Preparing continuous data

LZ76 works on symbols rather than floating-point measurements. A reproducible conversion might be binary thresholding, fixed-width bins, ranks, or a domain-specific state vocabulary. Each choice preserves some distinctions and discards others.

For example, binary thresholding maps samples above a chosen threshold to `1` and other samples to `0`. The choice of threshold, its treatment of ties, and whether it was fitted separately to each recording can all change the sequence. Record these choices alongside sampling rate, filtering, missing-data handling and epoch boundaries.

Convert floating-point measurements to symbols before analysis. Passing their decimal strings directly measures patterns among digits, signs and punctuation. The [neural time-series recipe](../recipes/neuro.md) shows a concrete symbolic workflow.

## Transformations and normalization

A one-to-one relabeling of individual byte symbols preserves the LZ count because equality between substrings is preserved. E can change because its sampling seeds are derived from the input bytes; see [calculating E](../shuffle-measure.md).

A cyclic shift changes both the parse start and the sampling seed. A many-to-one mapping changes the alphabet and may create new repeated substrings. Adding whitespace, separators or metadata changes the analyzed data unless you remove them before calling the library.

In Rust, symbol-changing mutations invalidate observed alphabet metadata. Explicit size declarations from `set_alphabet_size` and `with_alphabet_size` remain until `determine_alphabet` or `clear`. Constructor hints are described separately in the [Rust reference](../api/rust.md).

## A measurement record

Record the encoded byte length, observed symbol set, preprocessing rule, logarithm base, maximum block range and package version with each analysis. These details make comparisons easier to reproduce.
