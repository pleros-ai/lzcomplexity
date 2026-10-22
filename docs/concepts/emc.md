# Structural complexity E

E describes the organization of a sequence through correlations across different ranges. It complements entropy density: `h` captures the production of new information, while E reflects the context connecting that information.

In written language, that context extends from relationships between letters to words and longer passages. In a symbolic trajectory, it can extend from local transitions to recurring patterns over many observations. Studying `h` and E together helps distinguish randomness from organized variation.

## Calculate E

```python
import lzcomplexity as lz

sequence = "00010111" * 256
E, summands = lz.emc(sequence, log_base=2, max_block_size=8)

print(E)
print(summands)
```

`emc()` returns E and the increments of its fitted scale profile. Their sum is E. Use `lz76()` to obtain E alongside entropy density, factor boundaries and the other analysis fields:

```python
result = lz.lz76(sequence, log_base=2, max_block_size=8)
print(result["h"])
print(result["emc"]["value"])
print(result["emc"]["summands"])
```

## Read the scale profile

The calculation examines block ranges from one byte up to `max_block_size`. Its fitted profile accumulates the response across those ranges, and E is the final value. The returned `summands` describe how much the fitted profile rises at each step.

For example, fitted values `[2.5, 2.5, 4]` have increments `[2.5, 0, 1.5]` and E equal to `4`. A flat part of the profile can result from fitting neighboring ranges together. The [calculation reference](implementation.md#6-structural-complexity-e) explains the formula and fitting procedure.

## Choose the range

A positive `max_block_size` sets the largest examined block range in bytes. Start with a value appropriate to the patterns you want to investigate, then check how the result changes over several choices.

When the argument is omitted or nonpositive, the library chooses a range from the input length. Longer inputs can therefore change both the available evidence and the examined ranges. Use a fixed range when you want to study the effect of length separately.

## Interpret E alongside h

Independent random symbols can have high `h` with little organized context. Recurrent patterns constrain local choices while introducing relationships over longer ranges. Their balance is often clearer in the joint behavior of `h` and E than in either value alone.

A constant sequence has an obvious pattern and returns E equal to zero: every range comparison leaves the sequence unchanged. E distinguishes forms of organization through their response across ranges, rather than counting every visible regularity.

For useful comparisons, keep the input length, encoding, logarithm base and maximum range consistent. Check several sample lengths and ranges, particularly when the sequence contains periodic patterns. Repeated calls on the same bytes and settings return the same result.

The [language study](https://doi.org/10.1371/journal.pone.0214863) explores entropy and complexity across levels of organization in text. The [dynamical-systems recipe](../recipes/dynamical-systems.md) applies the same perspective to symbolic trajectories.

**Continue with:** [implementation of E](implementation.md#6-structural-complexity-e), [sequence length](convergence.md), or the [Python reference](../api/python.md).
