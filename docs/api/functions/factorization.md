# `factorization()`

Find the LZ76 factor count and the positions separating its factors. Use this
function to inspect recurring patterns, display the parse, or obtain a count
without calculating h and E together.

```text
factorization(seq, partitions=1, alphabet=None, log_base=None, jobs=0)
    -> tuple[int, list[int]]
```

## Parameters

| Parameter | Default | Behavior |
|---|---|---|
| `seq` | required | String, bytes, list of strings, or compatible integer sequence, converted using the [input rules](../inputs.md). |
| `partitions` | `1` | Accepted; has no effect. |
| `alphabet` | `None` | Does not change the returned count or boundaries. |
| `log_base` | `None` | Does not change the returned count or boundaries. |
| `jobs` | `0` | Accepted; has no effect. Native workers use [Rayon configuration](../../project/determinism.md). |

Strings use UTF-8 bytes; integer lists become joined decimal text. Use `bytes`
for one byte per scientific symbol. For parameter types, ranges and exceptions,
see the [shared parameter reference](../python.md#parameter-types-and-errors).

## Return value

| Tuple item | Type | Meaning |
|---|---|---|
| `complexity` | `int` | The LZ76 count of completed factors. |
| `boundaries` | `list[int]` | Byte positions delimiting the parse. An interval is `[boundaries[i], boundaries[i + 1])`. |

The last boundary may extend beyond the input length. Such an unfinished
terminal factor appears in the boundary list but is excluded from the count.
Always use the returned `complexity`; do not substitute `len(boundaries) - 1`.

## Display a factorization

```python
import lzcomplexity as lz

sequence = "1000110011101"
count, boundaries = lz.factorization(sequence)
factors = [sequence[a:b] for a, b in zip(boundaries, boundaries[1:])]

assert count == 5
assert boundaries == [0, 1, 2, 5, 9, 13]
assert factors == ["1", "0", "001", "1001", "1101"]
print("·".join(factors))  # 1·0·001·1001·1101
```

Matches can cross earlier factor boundaries. The
[worked example](../../guide/first-factorization.md) illustrates the prefix
`100` in the factor `1001`.

### An unfinished terminal factor

```python
import lzcomplexity as lz

count, boundaries = lz.factorization("banana")
assert count == 3
assert boundaries == [0, 1, 2, 3, 7]
```

The input has six bytes. The final `ana` is a copied tail whose attempted
extension would end at position seven. Slicing the input displays
`b | a | n | ana`, while the completed-factor count remains three.
See [LZ76 factorization](../../concepts/lz76.md) for the parsing rule.

### Unicode and byte positions

Slice encoded bytes when displaying a non-ASCII input:

```python
import lzcomplexity as lz

data = "café".encode("utf-8")
count, boundaries = lz.factorization(data)
pieces = [data[a:min(b, len(data))] for a, b in zip(boundaries, boundaries[1:])]

assert len(data) == 5
assert b"".join(pieces) == data
```

A boundary need not coincide with a Unicode character boundary. Keep each
piece as bytes when it contains part of a multibyte character.

## Empty and constant sequences

| Input | Returned count | Returned boundaries |
|---|---:|---|
| `""` | `1` | `[0, 1, 0]` |
| `"a"` | `1` | `[0, 1, 1]` |
| `"aaaa"` | `1` | `[0, 1, 4]` |

These inputs use a special path. Empty input has a non-monotonic boundary
vector, and constant-input boundaries do not imply two completed factors.
Handle empty data separately when displaying factors.

**Related:** [`h()`](h.md), [`lz76()`](lz76.md), [input encoding](../inputs.md).
