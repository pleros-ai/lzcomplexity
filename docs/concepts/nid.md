# Information distance

`nid(x, y)` compares two byte sequences using their LZ76 factor counts. It asks how much additional parsing complexity appears when one sequence follows the other.

Lower values indicate more shared patterns under this comparison. The normalized-information-distance framework is developed in [Li et al. (2004)](https://doi.org/10.1109/TIT.2004.838101).

## The implemented formula

Let `C(X)` denote the library's factor count and `XY` raw byte concatenation, without a separator. The code computes both concatenation orders:

```text
d(X, Y) = max(C(XY) - C(X), C(YX) - C(Y))
          / max(C(X), C(Y))
```

The factor counts are integers; the returned ratio is floating point. The two orders make this formula symmetric under swapping `X` and `Y`. Logarithm-base and shuffle-cap options do not enter it.

```python
import lzcomplexity as lz

assert lz.nid("abcd", "abce") == 0.25
assert lz.nid("abcd", "abce") == lz.nid("abce", "abcd")
```

For that example, `C(X) = C(Y) = 4` and `C(XY) = C(YX) = 5`, so the result is `(5 - 4) / 4 = 0.25`. The result is a ratio of factor-count differences.

## Identical inputs need not give zero

The concatenation of a sequence with itself can introduce another completed factor. For example:

```python
assert lz.factorization("banana")[0] == 3
assert lz.factorization("bananabanana")[0] == 4
assert lz.nid("banana", "banana") == 1 / 3
```

A distance matrix therefore preserves these finite self-distances on its diagonal. For clustering or embedding, choose a method that accepts a dissimilarity matrix, or check the additional properties your method requires.

## What the comparison is sensitive to

LZ parsing recognizes repeated literal substrings. Symbolization, byte encoding, sampling length, phase and the join between `X` and `Y` can change the result. Independent realizations of the same source can contain different literal patterns, so use suitable within-source and between-source controls when interpreting distances.

Concatenation introduces a boundary without a special separator. A phrase can cross that join. Adding a separator changes the analyzed sequence; apply that choice consistently across comparisons.

Keep compared lengths and representations similar. The denominator uses factor counts, not sequence length, so comparing a short excerpt to a long recording has a different interpretation from comparing two matched epochs. Integer factor counts also make small inputs coarse: changing one factor is substantial when the denominator is only a few factors.

## Empty and constant input

The public factor-count path returns one for empty or constant input. Consequently:

| Pair | Result |
|---|---:|
| `("", "")` | `0.0` |
| `("", "abc")` | `2/3` |
| `("aaaa", "aaaa")` | `0.0` |
| `("aaaa", "bbbb")` | `1.0` |

Exclude missing sequences before constructing a comparison matrix.

## The separate shuffle matrix

The `lzdistance` CLI also writes `shuffle_information_distance`. That field is a different statistic, defined on the concatenated sequence:

```text
d_shuffle(X, Y) = C(XY) / mean_l C(shuffle_l(XY))
```

This statistic describes the structure of the concatenation. It can be asymmetric, nonzero for identical inputs and greater than one. For two identical constant inputs it is one, because all counts in the ratio equal one.

The direct `information_distance` matrix corresponds to `lz.nid`. The `shuffle_information_distance` matrix is a separate concatenation-structure diagnostic available through the CLI. The [CLI reference](../cli/lzdistance.md) gives its JSON schema and matrix strategies.

## A usable comparison workflow

1. Apply the same preprocessing and symbolization to all recordings.
2. Use comparable lengths and retain the factor counts as well as the ratio.
3. Inspect self-distances and appropriate unrelated controls.
4. Treat the matrix as a dissimilarity unless your downstream method's required properties have been checked.
5. Record the package version, encoding, lengths and matrix field used.

The `extras` dictionary returned by `lz76()` is different again: it compares two halves of a **single** sequence using factor-count formulas. [Reading the numbers](../guide/reading-the-numbers.md) explains those fields.
