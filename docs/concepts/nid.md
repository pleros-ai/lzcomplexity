# Information distance

Information distance compares two sequences through the patterns they share.
The library's measure derives from **Normalized Information Distance (NID)**,
a framework for comparing objects by how much information is needed to describe
one when the other is known. [Li et al. (2004)](https://doi.org/10.1109/TIT.2004.838101)
develops this framework.

Here, Lempel–Ziv analysis recognizes recurring patterns in the supplied symbolic
sequences. A lower distance indicates more shared patterns; a larger distance
indicates greater difference. This lets you compare texts, recordings or
symbolic trajectories without requiring a position-by-position match.

## Compare two sequences

```python
import lzcomplexity as lz

first = b"00010111" * 256
second = b"00110011" * 256

distance = lz.nid(first, second)
print(distance)
```

Use `nid()` when you already hold sequences in Python. For collections stored
in files, [`lzdistance`](../cli/lzdistance.md) produces a pairwise distance
matrix. See the [batch comparison recipe](../recipes/batch-distance.md) for
both workflows.

## Prepare comparable inputs

Apply the same encoding and preprocessing to every sequence, and use matched
lengths where possible. For continuous recordings, choose one symbolization
rule and apply it consistently. Distance then describes similarity in those
symbolic patterns.

Compare related recordings alongside suitable unrelated controls to understand
the scale of distances in your data. The logarithm base does not affect `nid()`.

## Reading a distance matrix

`nid(first, second)` is symmetric: reversing the two arguments gives the same
value. At finite lengths, an identical sequence can have a nonzero
self-distance. Keep this in mind when choosing a clustering or embedding
method that expects a zero diagonal.

The [`information_distance` field](../cli/lzdistance.md#output-schema) in the
CLI report corresponds to Python's `nid()`. The [Python reference](../api/python.md#nid)
lists its complete signature.
