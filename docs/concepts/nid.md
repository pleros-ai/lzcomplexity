# Information distance

`nid(x, y)` compares two byte sequences using their LZ76 factor counts. It asks how much additional parsing complexity appears when one sequence follows the other.

This is a computable LZ-based dissimilarity inspired by normalized information distance. The theoretical metric is defined using uncomputable Kolmogorov complexity; substituting finite LZ counts does not automatically transfer its metric guarantees. [Li et al. (2004)](https://doi.org/10.1109/TIT.2004.838101) is the theoretical reference.

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

For that example, `C(X) = C(Y) = 4` and `C(XY) = C(YX) = 5`, so the result is `(5 - 4) / 4 = 0.25`. It is not an edit-distance percentage or a calibrated probability of common origin.

## Identical inputs need not give zero

The concatenation of a sequence with itself can introduce another completed factor. For example:

```python
assert lz.factorization("banana")[0] == 3
assert lz.factorization("bananabanana")[0] == 4
assert lz.nid("banana", "banana") == 1 / 3
```

This finite self-distance is why callers should not assume an exact metric. A distance matrix can have a nonzero diagonal, and the implementation does not replace the diagonal with zeros. Do not silently force metric properties in a downstream analysis without making that additional choice explicit.

## What the comparison is sensitive to

LZ parsing recognizes repeated literal substrings. Symbolization, byte encoding, sampling length, phase and the join between `X` and `Y` can change the result. Two independent realizations of the same stochastic source need not share long literal substrings; the score is not a direct test of whether their generating processes are identical.

Concatenation introduces a boundary without a special separator. A phrase can cross that join. Adding a separator changes the measurement, so specify and consistently apply any such preprocessing rather than treating it as an innocuous formatting operation.

Keep compared lengths and representations similar. The denominator uses factor counts, not sequence length, so comparing a short excerpt to a long recording has a different interpretation from comparing two matched epochs. Integer factor counts also make small inputs coarse: changing one factor is substantial when the denominator is only a few factors.

## Empty and constant input

The public factor-count path returns one for empty or constant input. Consequently:

| Pair | Result |
|---|---:|
| `("", "")` | `0.0` |
| `("", "abc")` | `2/3` |
| `("aaaa", "aaaa")` | `0.0` |
| `("aaaa", "bbbb")` | `1.0` |

These are consequences of the implementation's short-input conventions, not evidence about the relationship between two unknown sources. Exclude missing sequences before constructing a scientific comparison matrix.

## The separate shuffle matrix

The `lzdistance` CLI also writes `shuffle_information_distance`. That field is a different statistic, defined on the concatenated sequence:

```text
d_shuffle(X, Y) = C(XY) / mean_l C(shuffle_l(XY))
```

It compares the concatenation with its own block-swapped surrogates. It does not compute a Shannon mutual information between `X` and `Y`. It can be asymmetric, nonzero for identical inputs and greater than one. For two identical constant inputs it is one, because all original and shuffled counts equal one.

Keep these outputs separate. The direct `information_distance` matrix corresponds to `lz.nid`; the shuffle matrix has no Python top-level binding and should be interpreted as a concatenation structure diagnostic. The [CLI reference](../cli/lzdistance.md) gives its JSON schema and matrix strategies.

## A usable comparison workflow

1. Apply the same preprocessing and symbolization to all recordings.
2. Use comparable lengths and retain the factor counts as well as the ratio.
3. Inspect self-distances and appropriate unrelated controls.
4. Treat the matrix as a dissimilarity unless your downstream method's required properties have been checked.
5. Report backend/version, encoding, lengths and the exact matrix field used.

The `extras` dictionary returned by `lz76()` is different again: it compares two halves of a **single** sequence using factor-count formulas. [Reading the numbers](../guide/reading-the-numbers.md) explains those fields.
