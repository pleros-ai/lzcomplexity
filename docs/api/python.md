---
title: Python API
description: Complete Python function reference for factorization, entropy density, effective measure complexity and information distance.
---

# Python API

The Python package provides five functions for analyzing symbolic sequences.
This reference covers their parameters, returned values and behavior. For a
compact list of calls, use the separate [usage cheatsheet](../guide/quick-reference.md).

```python
import lzcomplexity as lz

count, boundaries = lz.factorization("1000110011101")
assert count == 5
assert boundaries == [0, 1, 2, 5, 9, 13]
assert lz.h("01010101", log_base=2) == 0.75
assert lz.nid("abcd", "abce") == 0.25
```

## Choose a function

| Function | Use it to obtain | Return type |
|---|---|---|
| [`factorization`](functions/factorization.md) | The LZ76 count and factor boundaries | `tuple[int, list[int]]` |
| [`h`](functions/h.md) | Entropy density: the production of new information | `float` |
| [`emc`](functions/emc.md) | Effective measure complexity E and its range profile | `tuple[float, list[float]]` |
| [`nid`](functions/nid.md) | Information distance between two sequences | `float` |
| [`lz76`](functions/lz76.md) | Factorization, h, E and diagnostic fields together | `dict` |

Choose an individual function when you need one quantity. Use `lz76()` when
you need several results for the same sequence; it shares the original
factorization between the calculations.

## Sequence inputs

All five functions accept the same input forms. `nid()` accepts two sequences,
converting each independently.

| Input | Sequence analyzed |
|---|---|
| `str` | UTF-8 bytes |
| `bytes` | The supplied bytes, unchanged |
| `list[str]` | Strings joined without a separator, then UTF-8 encoded |
| Integer sequence | Decimal representations joined without a separator |
| Empty `list` | Empty byte sequence |

Compatible integer tuples and one-dimensional integer arrays are accepted.
Materialize generators before passing them in. Integer elements must fit a
signed 64-bit integer.

Encoding is part of the measurement. For example, `[0, 1, 10]` becomes the
four characters `"0110"`, while `bytes([0, 1, 10])` contains three symbols.
The [input reference](inputs.md) explains Unicode, arrays, codebooks and
continuous observations in detail.

The 256-byte limit on the website playground is specific to that demonstration;
it is not a Python API input limit.

## Shared parameters

All functions accept positional or keyword arguments. Parameters after the
sequence usually read most clearly as keywords.

| Parameter | Default | Meaning |
|---|---|---|
| `partitions` | `1` | Accepted but has no effect on the calculation. |
| `alphabet` | `None` | Alphabet size used by the `epsilon` diagnostic in `lz76()`. Automatic selection uses the observed byte alphabet, with a minimum of two. It does not select entropy units. |
| `log_base` | `None` | Logarithm base for h, E and related diagnostics. Automatic selection uses the observed byte alphabet, with a minimum of two. Pass `2` for bits per input byte. |
| `max_block_size` | `-1` | Only `emc()` and `lz76()`: largest examined range for E, in bytes. Positive values set it explicitly; zero or a negative value selects it automatically. |
| `jobs` | `0` | Accepted but has no effect. Use Rayon worker configuration. |

Setting `alphabet` does not change the default `log_base`. Explicit logarithm
bases zero and one are treated as two.

```python
import lzcomplexity as lz

assert lz.h("01010101", alphabet=4) == 0.75
assert lz.h("01010101", log_base=4) == 0.375
```

### Parameter types and errors

`alphabet`, `log_base` and `jobs` accept unsigned 32-bit integers;
`partitions` and `max_block_size` accept signed 32-bit integers. `None` is
accepted only for `alphabet` and `log_base`. For those two parameters,
`2**32 - 1` is reserved for automatic selection.

An unsupported sequence representation or incompatible argument type raises
`TypeError`. An integer parameter outside its supported range raises
`OverflowError`. A list of integers outside signed 64-bit range is rejected
as an unsupported sequence. Each function page identifies the parameters
that affect its result.

## `factorization`

Returns the LZ76 count and byte boundaries. The count follows the completed-factor
convention, so it cannot always be inferred from the number of boundary intervals.

[Full `factorization()` reference →](functions/factorization.md)

## `h`

Returns the finite-sequence entropy-density estimate in the selected units.
Use it to compare the production of new information in consistently encoded samples.

[Full `h()` reference →](functions/h.md)

## `emc`

Returns effective measure complexity E and the nonnegative increments of its
range profile. Set `max_block_size` to use the same examined ranges across samples.

[Full `emc()` reference →](functions/emc.md)

## `nid`

Returns an information-distance estimate derived from Normalized Information
Distance (NID). Both arguments must use a common symbol encoding.

[Full `nid()` reference →](functions/nid.md)

## `lz76`

Returns a dictionary containing factorization, h, E, the E profile, finite-size
diagnostics and the five additional count-based ratios.

[Full `lz76()` reference and output schema →](functions/lz76.md)

## Empty and constant inputs

These inputs have defined results rather than raising an exception:

| Input | `factorization()` | `h()` | `emc()` with automatic range |
|---|---|---:|---|
| `""` | `(1, [0, 1, 0])` | `0.0` | `(0.0, [0.0])` |
| `"a"` | `(1, [0, 1, 1])` | `0.0` | `(0.0, [0.0])` |
| `"aaaa"` | `(1, [0, 1, 4])` | `0.5` | `(0.0, [0.0])` |

Handle an empty sequence before slicing its boundary vector. Detailed boundary
and finite-length behavior appears on the individual function pages.

## Parallel execution and runtime help

Calls run synchronously in the native Rust core. Set `RAYON_NUM_THREADS`
before the first calculation to select its worker count; the Python `jobs`
keyword does not control it. See [parallel execution](../project/determinism.md)
for process-based workloads and worker configuration.

```python
import lzcomplexity as lz

print(lz.__version__)
help(lz.emc)
```

Type stubs are included for editor completion and type checking.
[Binding source](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-py/src/lib.rs).
