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

An integer list is **not** a direct categorical-symbol array. In particular, `[1, 10]` and `[11, 0]` both become `"110"`. For categories numbered `0…255`, construct a `bytes` object if each category should occupy exactly one byte. Map a larger vocabulary to a carefully specified representation; the core does not supply arbitrary-width atomic symbols.

## Observed alphabet versus declared alphabet

Python has two related options with different jobs:

- `alphabet` selects the alphabet parameter used in the legacy finite-size `epsilon` formula.
- `log_base` selects the information units used by `h` and the shuffle contrast.

When `log_base=None`, Python/core use the observed input alphabet size, floored at two, even if `alphabet` was supplied explicitly.

```python
import lzcomplexity as lz

sequence = "01010101"
assert lz.h(sequence) == 0.75
assert lz.h(sequence, alphabet=4) == 0.75
assert lz.h(sequence, alphabet=4, log_base=4) == 0.375
```

The CLI retains a legacy default that uses a declared `-a` value when `-l` is omitted. These commands therefore request different units:

```sh
lzcomplexity sample.txt -a 4 -o normalized.json
lzcomplexity sample.txt -a 4 -l 2 -o bits.json
```

For reproducible Python/CLI comparisons, always choose the same explicit base.

## What base conversion changes

If everything else is fixed, converting from base two to base `b` divides entropy density and the EMC-like score by `log₂(b)`. It does not change the LZ factor count, the input, the block-size cap or the shuffled copies. Choosing base two does not establish that the EMC-like score is calibrated past/future information in bits.

`nid` is built from factor-count ratios; logarithm-base options do not enter that formula.

## Preparing continuous data

LZ76 works on symbols rather than floating-point measurements. A reproducible conversion might be binary thresholding, fixed-width bins, ranks, or a domain-specific state vocabulary. Each choice preserves some distinctions and discards others.

For example, binary thresholding maps samples above a chosen threshold to `1` and other samples to `0`. The choice of threshold, its treatment of ties, and whether it was fitted separately to each recording can all change the sequence. Record these choices alongside sampling rate, filtering, missing-data handling and epoch boundaries.

Do not pass unprocessed decimal strings of floating-point measurements and interpret the result as dynamics of the original signal: the parser would measure repeated digits, signs and punctuation. The [neural time-series recipe](../recipes/neuro.md) shows a concrete symbolic workflow.

## Transformations and normalization

A one-to-one relabeling of individual byte symbols preserves the LZ count because equality between substrings is preserved. It can still change the EMC-like score: shuffle seeds are derived from the complete bytes, so relabeling changes the chosen surrogate.

A cyclic shift also changes the parse start and the shuffle seed. A many-to-one mapping changes the alphabet and may create new repeated substrings. Adding whitespace, separators or metadata changes the analyzed data unless you remove them before calling the library.

In Rust, symbol-changing mutations invalidate observed alphabet metadata. Explicit size declarations from `set_alphabet_size` and `with_alphabet_size` remain until `determine_alphabet` or `clear`. Constructor hints have their legacy behavior; see the [Rust reference](../api/rust.md).

## A measurement record

Keep the encoded byte length, observed symbol set, preprocessing rule, explicit logarithm base, shuffle cap and package/source version with each analysis. A description such as “binary entropy” is not enough to reconstruct the calculation.
