# Input types

*Every accepted form, its exact byte conversion, and the integer list that silently rewrites your sequence.*

All five Python functions push their sequence argument through one coercion step that flattens
it into a flat array of bytes. A symbol is a byte in `0..=255` — not a Unicode character, not an
integer, not a category label. `n` is the number of bytes **after** that flattening, and the
alphabet is the set of distinct byte values in it.

That flattening is where data is lost without an error.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">1</span><span class="lz-stat__k">byte per symbol</span></div>
<div class="lz-stat"><span class="lz-stat__v">256</span><span class="lz-stat__k">distinct symbols, hard ceiling</span></div>
<div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">coercion branches</span></div>
<div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">warnings when conversion loses data</span></div>
</div>

!!! danger "`[0, 1, 10]` is measured as the four-symbol string `0110`"

    A list of integers is converted by concatenating each element's **decimal string**. Any value
    of 10 or more changes both the length of the sequence and its alphabet, and no exception or
    warning is raised. `[10, 2]` and `[1, 0, 2]` collapse onto the same three symbols and become
    indistinguishable to every function in the library.

    ```pycon
    >>> import lzcomplexity as lz
    >>> lz.factorization([0, 1, 10]) == lz.factorization("0110")
    True
    >>> lz.factorization([10, 2]) == lz.factorization([1, 0, 2]) == lz.factorization("102")
    True
    >>> lz.nid([10, 2], [1, 0, 2])          # distance zero between different data
    0.0
    >>> lz.nid([1, 23], [12, 3])
    0.0
    >>> lz.h([0, 1, 10]), lz.h([0, 1, 1, 0])
    (1.5, 1.5)
    ```

    Integer symbols `0`–`9` are exactly safe, because decimal conversion is the identity on a
    single digit. That is why the trap goes unnoticed: binary data works, ternary data works, and
    then someone raises the quantiser to 16 levels and `h` moves by 11 % with nothing on screen
    to show for it. Everything outside `0`–`9` needs one of the
    [recipes below](#recipes-for-numeric-data).

<div class="lz-tickrule"></div>

## The coercion order

Four branches are tried in strict order. The first that matches wins; nothing later gets a look.

<div class="lz-scroll lz-compare" markdown>

| # | Accepts | Conversion | Result |
|---|---|---|---|
| 1 | `str` and `str` subclasses | **UTF-8 encode** — symbols are the encoded bytes | one byte per ASCII character, 2–4 for the rest |
| 2 | `bytes` and `bytes` subclasses | verbatim | 1 byte = 1 symbol |
| 3 | a `list` whose first element is a `str` | `"".join(seq)`, then UTF-8 encode | no separator inserted |
| 3 | a `list` that is empty | empty sequence | `n = 0` |
| 4 | any object with the **sequence protocol** whose items are ints fitting in `int64` | **decimal string of each element, concatenated** | `n` = total digit count |
| — | anything else | — | `TypeError` |

</div>

Branch 4 is what catches NumPy arrays, `tuple`, `range`, `array.array`, `bytearray` and
`memoryview`. It is the single most consequential fact on this page.

Every transcript on this page runs against `lzcomplexity` 1.0.0 with these imports:

```python
import array

import numpy as np

import lzcomplexity as lz
```

```pycon
>>> lz.factorization("banana")
(3, [0, 1, 2, 3, 7])
>>> lz.factorization(b"banana")
(3, [0, 1, 2, 3, 7])
>>> lz.factorization(list("banana"))
(3, [0, 1, 2, 3, 7])
>>> lz.factorization([0, 1, 1, 0])
(3, [0, 1, 2, 4])
>>> lz.factorization((0, 1, 1, 0))
(3, [0, 1, 2, 4])
>>> lz.factorization(range(4))
(4, [0, 1, 2, 3, 4])
>>> lz.factorization(array.array("i", [0, 1, 1, 0]))
(3, [0, 1, 2, 4])
>>> lz.factorization(np.array([0, 1, 1, 0], dtype=np.uint8))
(3, [0, 1, 2, 4])
```

!!! warning "`bytearray` and `memoryview` give different answers from `bytes`"

    Passing a `bytearray` returns a plausible-looking number computed from the wrong sequence.
    Branch 2 tests for `bytes` specifically, so a `bytearray` or `memoryview` falls through to
    branch 4 and each **byte value** is spelled out in decimal.

    ```pycon
    >>> lz.h(b"abcabc")
    0.8154648767857287
    >>> lz.h(bytearray(b"abcabc"))
    0.7539531690476382
    >>> lz.h("979899979899")          # what the bytearray actually measured
    0.7539531690476382
    ```

    `b"banana"` is 6 symbols; `bytearray(b"banana")` is the 14 symbols `98971109711097`. Call
    `bytes(...)` on the buffer first.

!!! note "A tuple of ints is accepted, a tuple of strings is not"

    Branch 3 tests for `list` specifically, so `("b", "a", "n")` raises `TypeError` while
    `(0, 1, 1, 0)` works through branch 4. The asymmetry is an artefact of the branch order, not
    a design decision. Wrap string sequences in a `list`, or join them yourself.

<div class="lz-tickrule"></div>

## `str` — UTF-8 bytes, not characters

A `str` is encoded to UTF-8 and the **bytes** become the symbols. For ASCII input — DNA, binary
strings, symbolised time series — `str` and `bytes` are identical. For anything else, `n` is the
UTF-8 length, not `len(seq)`.

```pycon
>>> len("héllo"), len("héllo".encode())
(5, 6)
>>> lz.factorization("héllo")
(5, [0, 1, 2, 3, 4, 6])
>>> lz.factorization("héllo") == lz.factorization("héllo".encode())
True
>>> lz.factorization("日本")           # 2 characters, 6 symbols
(5, [0, 1, 2, 3, 5, 6])
>>> lz.factorization("😀")             # 1 character, 4 symbols
(4, [0, 1, 2, 3, 4])
```

!!! warning "Non-ASCII text inflates `n` and the alphabet against what you can see"

    `h` is divided by `n`, so every multi-byte character shifts the result. `é` contributes two
    symbols and two distinct byte values; an emoji contributes four of each. If your symbols are
    words, categories or non-Latin characters, map them to single bytes with a codebook — see
    [the label recipe](#recipes-for-numeric-data).

A `str` that cannot be UTF-8 encoded is rejected outright: `lz.factorization("\ud800")` raises
`TypeError`, because branch 1 fails on the lone surrogate and no later branch accepts a `str`.

## `bytes` — verbatim, and the form to prefer

One byte in, one symbol out, no transformation of any kind. Every byte value `0`–`255` is legal,
control bytes and NUL included.

!!! tip "`bytes` is the fastest input as well as the only lossless one"

    Passing `bytes` instead of a NumPy integer array cut `factorization` on a 1 000 000-symbol
    sequence from 0.152 s to 0.089 s on this machine — 1.7×, with `n` identical in both runs
    because every value was a single digit. The bridge makes one copy for `bytes`, two for `str`,
    and three for an integer sequence: an `int64` vector at 8 bytes per element, then the decimal
    string, then the byte vector.

    | input form | best of 5, 1 000 000 symbols |
    |---|---|
    | `bytes` | 0.089 s |
    | `str` | 0.090 s |
    | `list[int]` | 0.110 s |
    | NumPy `int64` | 0.152 s |
    | NumPy `uint8` | 0.155 s |

    Ratios are the durable part; absolute times are machine-dependent.

## `list[str]` — concatenated with no separator

Elements are joined with an empty string and the result is UTF-8 encoded. Multi-character
elements therefore inflate `n` exactly as multi-digit integers do.

```pycon
>>> lz.factorization(["ban", "ana"]) == lz.factorization("banana")
True
>>> lz.factorization(["s1", "s2", "s10"]) == lz.factorization("s1s2s10")
True
>>> lz.factorization(["", "", "a"])          # empty elements vanish
(1, [0, 1, 1])
>>> lz.factorization(["A", "C", "G", "T"]) == lz.factorization("ACGT")
True
```

`list[str]` is safe only when every element is a single character whose UTF-8 encoding is one
byte, i.e. ASCII. `["A", "C", "G", "T"]` is fine. `["cat", "dog"]` is not.

Only the **first** element is inspected to pick the branch. A list that starts with a string and
continues with something else fails with a different, lower-level message:

```pycon
>>> lz.factorization(["a", 1])
TypeError: 'int' object cannot be converted to 'PyString'
>>> lz.factorization([1, "a"])
TypeError: expected str, bytes, list[str], list[int], or iterable of ints
```

<div class="lz-tickrule"></div>

## `list[int]` and other integer sequences

Every element's decimal representation is written out and the pieces are concatenated. The
sequence length is the total number of characters produced, and a negative value contributes a
`-` symbol of its own.

<div class="lz-formula">
  <p class="lz-math"><i>n</i> = Σ<sub><i>i</i></sub> len(str(<i>v</i><sub><i>i</i></sub>))</p>
  <dl class="lz-formula__key">
    <dt><i>v</i><sub><i>i</i></sub></dt><dd>the <i>i</i>-th integer of the input</dd>
    <dt>len(str(·))</dt><dd>decimal digit count, plus one for a minus sign</dd>
    <dt>alphabet</dt><dd>a subset of the ten ASCII digits <code>0</code>–<code>9</code>, plus <code>-</code>; never more than 11 symbols however large the true alphabet</dd>
  </dl>
  <p class="lz-formula__cite">Measured: 2 000 draws from [0, 256) coerce to 5 163 symbols over 10 distinct values — 2.58× the intended length, 3.9 % of the intended alphabet.</p>
</div>

```pycon
>>> lz.factorization([-1, 0, 1]) == lz.factorization("-101")
True
>>> lz.factorization([0, 1, 2, 3]) == lz.factorization("0123")     # digits are safe
True
>>> lz.factorization([True, False, True]) == lz.factorization("101")
True
```

### How much it moves `h`

Each row draws 2 000 i.i.d. integers from the stated range with a fresh
`np.random.default_rng(12345)`, so every row is independently reproducible. `h(ints)` passes the
NumPy array directly; `h(bytes)` passes `(a - lo).astype(np.uint8).tobytes()` — the same data,
one byte per symbol.

<div class="lz-scroll lz-compare" markdown>

| value range | true `k` | coerced `n` | coerced `k` | `h(ints)` | `h(bytes)` | error |
|---|---|---|---|---|---|---|
| `[0, 2)` | 2 | 2 000 | 2 | 1.0308 | 1.0308 | +0.0 % |
| `[0, 4)` | 4 | 2 000 | 4 | 0.9897 | 0.9897 | +0.0 % |
| `[0, 10)` | 10 | 2 000 | 10 | 0.9391 | 0.9391 | +0.0 % |
| `[0, 12)` | 12 | 2 321 | 10 | 0.8527 | 0.9222 | **−7.5 %** |
| `[0, 16)` | 16 | 2 728 | 10 | 0.7935 | 0.8910 | **−10.9 %** |
| `[0, 26)` | 26 | 3 236 | 10 | 0.7831 | 0.8678 | **−9.8 %** |
| `[0, 64)` | 64 | 3 695 | 10 | 0.8873 | 0.8517 | **+4.2 %** |
| `[0, 256)` | 256 | 5 163 | 10 | 0.8795 | 0.7422 | **+18.5 %** |
| `[−1, 2)` | 3 | 2 651 | 3 | 0.7984 | 1.0343 | **−22.8 %** |

</div>

Three things to read off that table.

- **Values `0`–`9` are exact.** All three digit-only rows agree to every printed decimal, and
  that is structural rather than luck.
- **The alphabet saturates at 10.** A 256-symbol alphabet is reported as a 10-symbol one, so the
  automatic log base is wrong as well as the length. The damage arrives through two independent
  channels at once.
- **The error is not monotone.** It runs negative up to about 26 levels, then turns positive. You
  cannot correct a published number after the fact; you have to recompute it.

The `[0, 16)` row in full precision, showing the log base moving with the alphabet:

```pycon
>>> rng = np.random.default_rng(12345)
>>> a = rng.integers(0, 16, 2000)
>>> b = a.astype(np.uint8).tobytes()
>>> lz.h(a), lz.h(b)
(0.7934684569538849, 0.8909699731287946)
>>> lz.h(a) == lz.h(a, log_base=10)        # detected alphabet collapsed to the digits
True
>>> lz.h(b) == lz.h(b, log_base=16)        # the base you meant
True
>>> lz.factorization(a)[0], lz.factorization(b)[0]
(630, 650)
```

### The worst common case: a sign signal

`{−1, 0, +1}` is the most common symbolisation in time-series work, and it is the row with the
largest error above. The minus sign becomes its own symbol, `-1` becomes a two-symbol digram, and
the entropy rate is understated by roughly a quarter.

!!! danger "A `{−1, 0, +1}` array reports an entropy rate 24 % too low"

    600 i.i.d. symbols from `np.random.default_rng(11).integers(-1, 2, 600)`:

    ```pycon
    >>> sig = np.random.default_rng(11).integers(-1, 2, 600)
    >>> lz.h(sig)                                          # 806 symbols over {'-', '0', '1'}
    0.8011018558460719
    >>> lz.h((sig + 1).astype(np.uint8).tobytes())         # 600 symbols over {0, 1, 2}
    1.0577970948906243
    >>> lz.factorization(sig)[0], lz.factorization((sig + 1).astype(np.uint8).tobytes())[0]
    (106, 109)
    ```

    The intended sequence has `n = 600` and three symbols. What was measured has `n = 806` and
    the three symbols `-`, `0`, `1` — a different sequence, silently substituted. Shift the
    values non-negative and map them to bytes.

<div class="lz-tickrule"></div>

## Recipes for numeric data

One rule covers all of them: **one symbol must be one byte, so build the `bytes` object
yourself**. Each tab below is a complete conversion.

=== "Integer symbols"

    Shift to zero and cast to `uint8`. Handles negatives, and fails loudly above 256 distinct
    values instead of quietly reshaping the data.

    ```python
    import numpy as np


    def from_ints(x):
        """Integer symbols -> bytes, one byte per symbol."""
        a = np.asarray(x)
        lo, hi = int(a.min()), int(a.max())
        if hi - lo > 255:
            raise ValueError(f"alphabet too large: {hi - lo + 1} distinct values > 256")
        return (a - lo).astype(np.uint8).tobytes()
    ```

    ```pycon
    >>> x = np.random.default_rng(12345).integers(-5, 6, 1000)
    >>> b = from_ints(x)
    >>> len(b), len(set(b)), lz.h(b)
    (1000, 11, 0.9160809496708275)
    >>> lz.h(x)                       # the same data passed raw: n = 1457 over 11 digit symbols
    0.8247088823630667
    >>> from_ints(np.arange(1000))
    ValueError: alphabet too large: 1000 distinct values > 256
    ```

=== "Threshold or median"

    Binarisation is the safest symbolisation there is: exact length, alphabet 2, nothing to get
    wrong. Use it when the question is about temporal structure rather than amplitude.

    ```python
    import numpy as np

    rng = np.random.default_rng(12345)
    sig = rng.standard_normal(2000)

    median_split = (sig > np.median(sig)).astype(np.uint8).tobytes()
    fixed_thresh = (sig > 0.0).astype(np.uint8).tobytes()
    ```

    ```pycon
    >>> len(median_split), len(set(median_split)), lz.h(median_split)
    (2000, 2, 1.0307837227582362)
    >>> lz.h(fixed_thresh)
    1.0417495070428981
    ```

    The median split gives a balanced alphabet regardless of the signal's offset or drift; a fixed
    threshold does not, but is comparable across recordings. Both give `n == len(sig)`.

=== "Quantile bins"

    Equiprobable bins for a continuous signal. `k` bins, `k <= 256`.

    ```python
    import numpy as np


    def quantise(x, k):
        """Continuous signal -> k equiprobable bins, one byte per sample."""
        a = np.asarray(x, dtype=float)
        edges = np.quantile(a, np.linspace(0, 1, k + 1)[1:-1])
        return np.digitize(a, edges).astype(np.uint8).tobytes()
    ```

    ```pycon
    >>> sig = np.random.default_rng(12345).standard_normal(2000)
    >>> q = quantise(sig, 16)
    >>> len(q), len(set(q)), lz.h(q)
    (2000, 16, 0.9087893725913704)
    ```

    Dropping the `.tobytes()` and passing the bin array straight in gives `n = 2750` over 10 digit
    symbols and `h = 0.7879198534956601` — a 13 % error on identical bins.

=== "Arbitrary labels"

    Words, gene names, cluster IDs, state names: build a codebook.

    ```python
    def from_labels(x):
        """Hashable labels -> bytes via a sorted codebook (<= 256 labels)."""
        codes = {v: i for i, v in enumerate(sorted(set(x)))}
        if len(codes) > 256:
            raise ValueError(f"alphabet too large: {len(codes)} > 256")
        return bytes(codes[v] for v in x)
    ```

    ```pycon
    >>> rng = np.random.default_rng(12345)
    >>> labels = [["cat", "dog", "emu"][i] for i in rng.integers(0, 3, 1000)]
    >>> b = from_labels(labels)
    >>> len(b), len(set(b)), lz.h(b)
    (1000, 3, 1.0437598305961133)
    ```

    Sorting the label set makes the codebook deterministic across runs and processes, which
    matters because [`nid`](../concepts/nid.md) compares byte values directly.

=== "Sign of the difference"

    For a differenced series, cast through `int8` so `−1` lands on byte `255` instead of growing a
    minus sign.

    ```python
    d = np.sign(np.diff(sig)).astype(np.int8).astype(np.uint8).tobytes()
    ```

    ```pycon
    >>> sig = np.random.default_rng(12345).standard_normal(2000)
    >>> d = np.sign(np.diff(sig)).astype(np.int8).astype(np.uint8).tobytes()
    >>> len(d), sorted(set(d)), lz.h(d)
    (1999, [1, 255], 0.9763787747376407)
    ```

    On a continuous signal exact ties never occur, so the realised alphabet is `{1, 255}` and
    `k = 2`. The sequence is one symbol shorter than the input, as `np.diff` implies.

!!! warning "`chr()` is only correct below code point 128"

    `"".join(chr(v) for v in vals)` looks like a byte mapping and is one only for ASCII. Above
    127 the string is UTF-8 encoded on the way in, so each value becomes two symbols.

    ```pycon
    >>> vals = list(range(0, 200, 17))          # 12 values, 4 of them >= 128
    >>> lz.h(bytes(vals))
    1.0
    >>> lz.h("".join(chr(v) for v in vals))     # 12 characters, 16 symbols
    0.8782740018887891
    >>> len("".join(chr(v) for v in vals).encode()), len(bytes(vals))
    (16, 12)
    ```

    `bytes(vals)` is correct across the whole range and is faster. There is no reason to reach for
    `chr`.

<div class="lz-tickrule"></div>

## What raises, and what quietly does not

Every exception below is a builtin; this package defines no exception classes of its own. The
message is identical for every rejected type except a list that begins with a string.

<div class="lz-scroll lz-compare" markdown>

| input | outcome |
|---|---|
| `np.array([1.0, 2.0, 3.0])` | `TypeError: expected str, bytes, list[str], list[int], or iterable of ints` |
| `[1.5, 2.5]` | same `TypeError` |
| `np.array([True, False])` | same `TypeError` — NumPy `bool_` has no `__index__` |
| `np.array([[0, 1], [1, 0]])`, `np.array(5)`, `np.float64(1.0)` | same `TypeError` — only 1-D integer arrays are accepted |
| `iter([0, 1])`, a generator, `{0, 1}` | same `TypeError` — the **sequence** protocol is required, not iteration |
| `None`, `42`, `3.14`, `{"a": 1}` | same `TypeError` |
| `("b", "a", "n")` | same `TypeError` — a tuple of strings is not a `list[str]` |
| `[2**63]` | same `TypeError` — the value does not fit in `int64` |
| `["a", 1]` | `TypeError: 'int' object cannot be converted to 'PyString'` |
| `np.array([])` — empty, `float64` | **accepted** — `(1, [0, 1, 0])`, `h` = `0.0` |
| `[]`, `()`, `b""`, `""` | **accepted** — `(1, [0, 1, 0])`, `h` = `0.0` |
| `"a"` | **accepted** — `(1, [0, 1, 1])`, `h` = `0.0` |
| `[True, False, True]` | **accepted** — Python `bool` is an `int`, so this is `"101"` |

</div>

A float array raising is the best behaviour on this page: a raw continuous signal fails fast
instead of producing a plausible number. The exception is an **empty** float array, which has no
element to reject and slips through.

!!! warning "Degenerate input returns a number instead of raising"

    An empty or single-symbol sequence produces a full result with no complaint, and its
    complexity is `1` rather than `0`.

    ```pycon
    >>> lz.factorization(""), lz.factorization([]), lz.factorization(b"")
    ((1, [0, 1, 0]), (1, [0, 1, 0]), (1, [0, 1, 0]))
    >>> lz.h(""), lz.h("a"), lz.h("aa"), lz.h("ab")
    (0.0, 0.0, 0.5, 1.0)
    >>> lz.emc(""), lz.nid("", "")
    ((0.0, [0.0]), 0.0)
    >>> lz.lz76("") == lz.lz76([])
    True
    ```

    `lz76("")` still returns all nine keys, some with numerically meaningless values —
    `factors_stddev` is `1.3743685418725535`. Nothing in the Python layer validates length, so
    check `len(seq)` in your own pipeline if empty input should be an error.

    The boundary list `[0, 1, 0]` is non-monotonic because empty and constant sequences take a
    short-circuit path that never builds a suffix array.

!!! note "`numpy.bool_` is rejected while Python `bool` is accepted"

    `operator.index()` works on Python `bool` — a subclass of `int` — and raises on
    `numpy.bool`. A boolean mask therefore needs an explicit cast:
    `mask.astype(np.uint8).tobytes()`. That is the right conversion anyway; it is one byte per
    sample and alphabet 2.

<div class="lz-tickrule"></div>

## Alphabet consequences

The alphabet is detected on every call by a single pass over the bytes, filling a 256-entry
presence table. `k` is the number of distinct byte values, floored at 2. Nothing you pass
overrides that detection — the `alphabet=` parameter feeds only the ε correction term. Full
treatment in [Alphabets and log bases](../concepts/alphabets.md).

Whatever the coercion does to `k` therefore propagates straight into `h`, because `k` is the
default log base:

| you pass | `n` | detected `k` | log base used |
|---|---|---|---|
| `bytes` of 16 quantised levels | 2 000 | 16 | 16 |
| the same levels as a NumPy array | 2 728 | 10 | 10 |
| `"aaaa"` | 4 | 1, floored to **2** | 2 |
| `bytes(range(256))` | 256 | 256 | 256 |

The floor at 2 exists because `log 1 = 0` would divide by zero. A constant sequence reports
`k = 2`, and `h("aaaa")` is `0.5` rather than `0.0`.

## The 256-symbol ceiling

A sequence can contain at most 256 distinct symbols, and each symbol costs exactly one byte.
There is no wide-alphabet mode, and no parameter raises the limit.

```pycon
>>> lz.factorization(bytes(range(256)))[0]
256
>>> lz.h(bytes(range(256)))
1.0
>>> lz.h(bytes(range(256)), log_base=2)
8.0
```

Passing integers at or above 256 does not extend the alphabet. It reroutes you through decimal
concatenation and *shrinks* the alphabet to 10:

```pycon
>>> lz.h(np.arange(1000))
0.9221077297309833
>>> lz.h("".join(str(i) for i in range(1000))) == lz.h(np.arange(1000))
True
>>> len("".join(str(i) for i in range(1000)))       # what was actually measured
2890
>>> len(set("".join(str(i) for i in range(1000))))
10
```

That call measured a 2 890-symbol sequence over a 10-symbol alphabet, not 1 000 samples over
1 000 symbols.

If your alphabet genuinely exceeds 256 states, reduce it before calling — merge rare states,
cluster, or coarse-grain. Re-encoding one large symbol as a fixed-width block of bytes is *not*
the same measurement: it changes `n` and it changes the factorization.

## Memory

<div class="lz-scroll lz-compare" markdown>

| what | cost |
|---|---|
| the sequence itself | **exactly 1 byte per symbol** |
| working memory during factorization | **12 bytes per symbol**, measured |
| the alphabet vector | ≤ 256 bytes |
| transient copies, `bytes` input | 1 × `n` |
| transient copies, `str` input | about 2 × `n` |
| transient copies, integer sequence | 3 stages, including an `int64` vector at **8 bytes per element** |

</div>

The 12 bytes per symbol is the suffix array, the LCP table and the longest-previous-factor table,
all `Vec<u32>`. Measured directly: factorizing a 20 000 000-symbol binary sequence moved peak RSS
by 234 472 kB — 12.0 bytes per symbol — and took 3.27 s.

Practical consequence: a 100 M-symbol sequence needs about 100 MB for the data and about 1.2 GB
of working memory. A 10 M-sample `int64` array passed raw additionally burns 80 MB on the
intermediate integer vector before any of that starts, which is a second reason to hand over
`bytes`.

Those `u32` index arrays mean `n` must fit in 32 bits. Treat `n < 2**31` as the practical length
limit.

<div class="lz-tickrule"></div>

## Where to go next

<div class="lz-cards" markdown>
<div class="lz-card" markdown>
### [Alphabets and log bases](../concepts/alphabets.md)
How `k` is detected, why `alphabet=` does not do what its name suggests, and which knob reaches
which output.
<p class="lz-card__api"><code>lz.h(seq, log_base=2)</code></p>
</div>
<div class="lz-card" markdown>
### [The Python API](python.md)
Every signature, default and return shape, plus the two parameters that are accepted and
ignored.
<p class="lz-card__api"><code>lz.lz76(seq)</code></p>
</div>
<div class="lz-card" markdown>
### [EEG and neural time-series](../recipes/neuro.md)
Symbolisation choices for continuous recordings, worked end to end.
<p class="lz-card__api"><code>quantise(signal, 8)</code></p>
</div>
</div>
