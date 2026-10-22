# Migrating older Python scripts

Use these replacements when adapting examples from the reference manual or an
earlier release to the five-function Python API.

## Function names and return values

| Earlier interface | Python API |
|---|---|
| Integer result from `factorization(seq)` | `count, boundaries = lz.factorization(seq)` |
| `factors(seq)` | Use the same `lz.factorization(seq)` pair |
| `entropy_density(seq)` | `lz.h(seq)` |
| Three-value `emc(seq)` result | `score, increments = lz.emc(seq)` |
| Tuple unpacking from `lz76(seq)` | Read the dictionary returned by `lz.lz76(seq)` |
| `lz.sequence`, `LempelZiv76`, `LZ_Args` objects | Pass ordinary Python inputs and keyword arguments |

```python
import lzcomplexity as lz

sequence = "banana"
count, boundaries = lz.factorization(sequence)
result = lz.lz76(sequence, log_base=2)
print(count, boundaries)
print(result["h"])
print(result["emc"]["value"])
```

The [Python reference](../api/python.md) lists complete signatures and return
values. Spectral analysis is provided separately from this package.

## Inputs and units

Strings are analyzed as UTF-8 bytes. Integer sequences are joined as decimal
text, so `[0, 1, 10]` becomes `"0110"`. Use `bytes([0, 1, 10])` to preserve
one byte per symbol. See [input types](../api/inputs.md) for codebook examples.

Set `log_base` explicitly when adapting a script that assumes particular units.
The Python default is the observed byte alphabet size, with a minimum of two.
The `alphabet` parameter controls the finite-size diagnostic separately.

## Factor boundaries

The returned count excludes a final copied tail. Consequently,
`len(boundaries) - 1` can exceed the reported count, and the final boundary
can extend beyond the input. The [factorization guide](../concepts/lz76.md)
walks through this convention with `"banana"`.

## Build and execution

Source builds use Cargo and maturin. The [installation guide](../guide/install.md)
covers the required tools; [parallel execution](determinism.md) explains worker
configuration for Python, Rust and the command line.
