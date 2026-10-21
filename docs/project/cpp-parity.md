# Versions, backends and migration

The production branch is **`rust-backend`**. It provides a Rust core, PyO3 Python bindings and the two standalone tools. The `main` branch retains the original C++ implementation, including interfaces described in the version-0.9 manual.

Use the backend and revision as part of an analysis record. Matching function names alone does not guarantee matching numerical conventions.

## What changed in the architecture

| Area | Historical C++ implementation | Current Rust implementation |
|---|---|---|
| Native build | CMake, nanobind and C++ dependencies | Cargo and maturin; C suffix-array dependency |
| Suffix-array construction | CaPS-SA | Comparison sort for small inputs; `cdivsufsort` for larger inputs |
| LCP and factorization | Native C++ implementation | Kasai LCP, LPF and the current factorization pass |
| Parallel execution | C++ parallel-library configuration | Rayon |
| Python surface | Functions and classes from older releases | `factorization`, `h`, `emc`, `nid`, `lz76` |
| Spectral functions | Present in historical interfaces | Outside the current package |

The current C++ source also includes a Rust-compatible ChaCha8 generator. It is inaccurate to describe every C++ revision as time-seeded or intrinsically nondeterministic. Conversely, shared RNG code alone does not establish whole-library parity across arbitrary versions.

## Migrate a Python script

| Older code in the manual | Current Python form |
|---|---|
| Integer result from `factorization(seq)` | `count, boundaries = lz.factorization(seq)` |
| `factors(seq)` | Use the same `lz.factorization(seq)` pair |
| `entropy_density(seq)` | `lz.h(seq)` |
| Three-value `emc(seq)` result | `score, increments = lz.emc(seq)` |
| Tuple unpacking from `lz76(seq)` | Read the dictionary returned by `lz.lz76(seq)` |
| `lz.sequence`, `LempelZiv76`, `LZ_Args` objects | Use ordinary accepted Python inputs and keyword arguments |
| `lz.metrics` or `lz.spectral` submodules | Not exported by the current package |

```python
import lzcomplexity as lz

sequence = "banana"
count, boundaries = lz.factorization(sequence)
result = lz.lz76(sequence, log_base=2)
print(count, boundaries)
print(result["h"])
print(result["emc"]["value"])
print(result["emc"]["multi_information"])
```

`emc` and `multi_information` retain historical names. The first is a shuffle-based magnitude qualitatively similar to EMC; the second is the raw first-scale shuffle contrast. Neither label establishes exact excess entropy or calibrated mutual information. See [the measure definition](../concepts/emc.md).

## Defaults that need attention

Pass `log_base` explicitly when reproducing an old analysis. In the current Python/core interfaces, its default follows the observed input-byte alphabet, floored at two. An explicit `alphabet` does not set that default base. The CLI retains a different legacy default when a declared alphabet is supplied. See [alphabets and units](../concepts/alphabets.md).

A Python integer sequence is converted through decimal text: `[0, 1, 10]` becomes `"0110"`. A `bytes` object preserves one input symbol per byte. A Unicode string is analyzed as UTF-8 bytes. These conventions can change the effective length and alphabet compared with another implementation.

The current API documents factor boundaries separately from the reported complete-component count, including its possible end sentinel. Do not assume `len(boundaries)-1` always equals the returned count. See [factorization](../concepts/lz76.md).

## Compare implementations deliberately

For a differential comparison, record both exact commits and match input bytes, coercion, logarithm base, cutoff, RNG dependencies, seeds, sampler boundaries and aggregation rule. Run the same supported entry points and compare factor counts, boundaries and diagnostics separately from shuffled scores.

No blanket cross-version numerical-equivalence promise is made by these docs. Current CI builds the Rust workspace and its front-ends; it is not an exhaustive C++ differential test suite.

For the complete historical class tables and C++ examples, use the [reference manual](manual.md). For current types and functions, use the [Rust](../api/rust.md) and [Python](../api/python.md) references.
