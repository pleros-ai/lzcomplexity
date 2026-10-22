# Effective measure complexity E

Effective measure complexity E captures organization and correlations across
different ranges of a sequence. It complements entropy density h: h captures
the production of new information, while E reflects the context connecting
that information.

In written language, that context extends from relationships between letters
to words and longer passages. In a symbolic trajectory, it can extend from
local transitions to recurring patterns over many observations. Studying h
and E together helps distinguish randomness from organized variation.

## Calculate E

```python
import lzcomplexity as lz

sequence = b"00010111" * 256
E, increments = lz.emc(sequence, log_base=2, max_block_size=16)

print(E)
print(increments)
```

`emc()` returns E and its increments across the examined ranges. The increments
sum to E. Use `lz76()` to obtain E alongside entropy density and factorization:

```python
result = lz.lz76(sequence, log_base=2, max_block_size=16)
print(result["h"])
print(result["emc"]["value"])
```

## Choose the ranges

`max_block_size` sets the largest examined range in bytes. A value of `16`,
for example, includes ranges from one to sixteen bytes. When the argument is
omitted or nonpositive, the library chooses the maximum from the input length.

Start with a range appropriate to the patterns you want to investigate, then
compare several choices. Keep the input length, encoding, logarithm base and
maximum range consistent when comparing samples.

## Read E alongside h

Independent random symbols can have high h with little organized context.
Recurrent patterns connect observations across time and constrain the possible
continuations. Their balance is often clearer in the joint behavior of h and E
than in either value alone.

The increments show how the fitted profile increases as longer ranges are
included. For example, increments `[2.5, 0, 1.5]` give E equal to `4`. Examine
this profile when you want to see where the result changes across ranges.

The [language study](https://doi.org/10.1371/journal.pone.0214863) explores
entropy and complexity across levels of text organization. The
[dynamical-systems recipe](../recipes/dynamical-systems.md) applies the same
perspective to symbolic trajectories.

Continue with the [Python cheatsheet](../guide/quick-reference.md),
[sequence length](convergence.md), or the [Python reference](../api/python.md#emc).
