# Your first factorization

*From a six-letter word to four factors, three counted, and the numbers that follow.*

`lz.factorization` is the one call that shows you the whole machine. Everything else in the
library — the entropy density, the distance, the effective measure complexity — is derived
from the parse it returns. This page runs it on `"banana"` and takes the result apart
completely.

If `import lzcomplexity` fails, start at [Install](install.md).

---

## Run it

```python
>>> import lzcomplexity as lz
>>> lz.factorization("banana")
(3, [0, 1, 2, 3, 7])
```

Two things come back: an integer complexity, and a list of integers that is **not** a list of
substrings.

```python
>>> complexity, factors = lz.factorization("banana")
>>> complexity
3
>>> factors
[0, 1, 2, 3, 7]
```

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">6</span><span class="lz-stat__k">n symbols</span></div>
  <div class="lz-stat"><span class="lz-stat__v">3</span><span class="lz-stat__k">alphabet, auto-detected</span></div>
  <div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">pieces in the parse</span></div>
  <div class="lz-stat"><span class="lz-stat__v">3</span><span class="lz-stat__k">complexity reported</span></div>
</div>

The gap between the last two tiles is the whole story of this page. Take the boundary vector
first, then the count.

## The boundary vector

`factors` holds **half-open boundary indices**. `factors[0]` is always `0`; every later entry
is the exclusive end of one piece of the parse. So the pieces are the consecutive pairs, and
this is the exact expression that reconstructs them:

```python
>>> seq = "banana"
>>> complexity, factors = lz.factorization(seq)
>>> [seq[start:end] for start, end in zip(factors, factors[1:])]
['b', 'a', 'n', 'ana']
```

Four pieces, from five boundaries. In general `len(factors) - 1` is the number of pieces the
parser emitted — which, as the next section shows, is not the number it reports.

!!! warning
    The last boundary can point one symbol past the end of the sequence, and for `"banana"` it
    does: `factors[-1]` is `7` while `len(seq)` is `6`. Python slicing clamps silently, so
    `seq[3:7]` returns `'ana'` and the comprehension above still works. Rust's `&seq[3..7]`
    panics and C reads out of bounds. In any language that does not clamp, write
    `end = min(end, len(seq))`.

    The final boundary is always exactly `n` or `n + 1` — never anything else. That is a
    consequence of the parse rule, not a range check.

## The `b | a | n | ana` decomposition

LZ76 walks left to right. At each position it takes **the longest prefix of what remains that
has already started somewhere earlier in the sequence, plus one more symbol**. That extra
symbol is the *innovation*: it is what makes the new piece impossible to copy from the past,
and it is what forces the parse forward.

<div class="lz-formula">
  <p class="lz-math">|<i>f</i><sub><i>k</i></sub>| = LPF[<i>p</i><sub><i>k</i></sub>] + 1</p>
  <dl class="lz-formula__key">
    <dt><i>p</i><sub><i>k</i></sub></dt><dd>start position of the k-th component</dd>
    <dt>LPF[<i>p</i>]</dt><dd>longest prefix of <i>S</i>[<i>p</i>:] that also starts at some <i>j</i> &lt; <i>p</i></dd>
    <dt>+ 1</dt><dd>the innovation symbol</dd>
  </dl>
  <p class="lz-formula__cite">Lempel &amp; Ziv (1976), the exhaustive history. The copy may overlap the region it is being written into — it only has to <i>start</i> earlier.</p>
</div>

For `"banana"`:

| # | span | piece | where it comes from |
|---|---|---|---|
| 1 | `[0,1)` | `b` | first symbol, pure innovation |
| 2 | `[1,2)` | `a` | nothing earlier starts with `a`, so LPF = 0, plus 1 innovation |
| 3 | `[2,3)` | `n` | LPF = 0, plus 1 innovation |
| 4 | `[3,7)` | `ana` | LPF = 3: `ana` already starts at index 1. The innovation symbol would be index 6 — past the end. |

Piece 4 is the interesting one, and it is where the overlap matters. `S[3:6]` is `ana`, and
the earlier occurrence starts at index 1 — but that occurrence is `S[1:4]`, which runs *into*
the region being copied. LZ76 allows this; a non-overlapping matcher would find only `an`,
ending the component at index 6 instead of 7 — the same four visible pieces, but a *complete*
final component and therefore a count of 4. See [How LZ76 works](../concepts/lz76.md) for why
the overlap is part of the definition rather than an optimisation.

## Only complete components are counted

Four pieces are visible. The reported complexity is 3.

!!! warning
    Compare `lz.factorization("banana")[0]` against a textbook and you are off by one: this
    library says **3**, Kaspar–Schuster-style implementations (`antropy.lziv_complexity`, most
    MATLAB EEG toolboxes) say **4**. Neither is a bug. The library counts only components whose
    innovation symbol actually lands inside the sequence. `ana`'s would have been index 6, past
    the end, so that component is complete as a *copy* but not as a *production*, and is not
    counted.

The test is exactly the final boundary. Landing on `n` means the last component is complete;
landing on `n + 1` means it was truncated. So the conversion is one line:

```python
>>> seq = "banana"
>>> complexity, factors = lz.factorization(seq)
>>> c_textbook = complexity + (1 if factors[-1] > len(seq) else 0)
>>> c_textbook
4
```

Checked against a reference Kaspar–Schuster counter on 500 random non-constant strings
(lengths 2–120, alphabets `01`, `012`, `ACGT`, `ab`): **500 / 500 agreement, and no other
correction was ever needed.**

**The parse itself does not need correcting.** Clamp the final boundary to `n` and the pieces
are the LZ76 exhaustive history, symbol for symbol. Only the count differs, and only when the
sequence ends mid-component:

| sequence | parse | library `c` | textbook `c` |
|---|---|---|---|
| `010011101101100` | `0 · 1 · 00 · 11 · 101 · 101100` | 6 | 6 |
| `banana` | `b · a · n · ana` | 3 | 4 |
| `ABRACADABRA` | `A · B · R · AC · AD · ABRA` | 5 | 6 |
| `01010101010101010101` | `0 · 1 · 010101010101010101` | 2 | 3 |

The first row is the worked example in Estevez-Rams et al. (2013) §II, reproduced exactly —
boundaries *and* count, because that sequence happens to end on a component boundary. The other
three overshoot, and there the counts diverge by one.

!!! note
    Constant sequences skip the parser entirely, and the conversion above does not apply to
    them. `lz.factorization("aaaa")` returns `(1, [0, 1, 4])` from a special-case path — the
    boundary vector is synthetic and ends at `n`, so the formula says 1 while a textbook
    counter says 2. The same shortcut makes `lz.factorization("")` return `(1, [0, 1, 0])`:
    a complexity of 1 and a decreasing boundary list, for an empty input. Guard empty input
    yourself.

The difference is at most 1, always in the same direction. It shifts `h` by exactly
`log_k(n) / n` — about 0.0013 at `n = 10⁴`, `k = 2` — so it is negligible at research lengths
and glaring on tutorial-length strings, which is exactly where readers compare numbers against
a paper.

<hr class="lz-tickrule">

## Three more numbers from the same string

### `h` — entropy density

```python
>>> lz.h("banana")
0.8154648767857287
```

`h` is the complexity normalised by the LZ76 asymptote, `n / log_k n`:

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>c</i>(<i>S</i>) &middot; log<sub><i>k</i></sub> <i>n</i> &frasl; <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>complexity as reported above — 3 for <code>banana</code></dd>
    <dt><i>n</i></dt><dd>sequence length in bytes — 6</dd>
    <dt><i>k</i></dt><dd>log base; defaults to the auto-detected alphabet size, floored at 2</dd>
  </dl>
</div>

`"banana"` has three distinct bytes, so the default units are **base-3 symbols per symbol, not
bits**. Ask for bits explicitly:

```python
>>> lz.h("banana", log_base=2)
1.292481250360578
```

That value is above 1, which is expected: `h` is a finite-`n` estimator with a large upward
bias at short lengths and it is not bounded by 1. Six symbols is far too short to read as an
entropy rate at all. See [Entropy density](../concepts/entropy-density.md) for the estimator,
[Alphabets and log bases](../concepts/alphabets.md) for why auto-detection is a hazard in
comparative studies, and [Convergence](../concepts/convergence.md) for how long a sequence has
to be before the number means anything.

### `nid` — normalised information distance

```python
>>> lz.nid("banana", "bandana")
0.4
```

`nid` measures how much one sequence fails to explain the other, by counting how many *new*
components appear when you concatenate them. Here `c("banana") = 3`, `c("bandana") = 5`,
`c("bananabandana") = 5` and `c("bandanabanana") = 6`, so the value is
`max(5 - 3, 6 - 5) / max(3, 5) = 2 / 5`. Unrelated strings go to 1:

```python
>>> lz.nid("banana", "kumquat")
1.0
```

!!! warning
    `nid(x, x)` is not always 0, so `nid` is not a metric on finite sequences.
    `lz.nid("banana", "banana")` returns `0.3333333333333333`, because appending `banana` to
    itself still costs one new component. It reaches 0 only when a sequence is fully
    reproducible from its own copy — `lz.nid("abc", "abc")` is `0.0`. Details in
    [Information distance](../concepts/nid.md).

### `emc` — effective measure complexity

```python
>>> lz.emc("banana")
(0.0, [0.0])
```

That zero is not a result, it is a degenerate case: for `n < 10` the automatic selector picks a
single block scale, and the block-size-1 shuffle of `"banana"` factorises into 3 components —
the same as the original — so the single term is exactly zero. Give it something longer:

```python
>>> lz.emc("banana" * 4)
(1.0847959727678895,
 [0.36159865758929655, -0.36159865758929655, 1.0847959727678895])
```

`emc` returns `(value, summands)`. Each summand belongs to one block scale; the value is their
sum.

!!! danger
    That sum telescopes, so the total carries information from one scale only. Each term is
    `(H_l − H_{l−1}) − ĥ`, and the whole sum collapses exactly to
    `mm · g · (C_LZ(shuffled at mm) − C_LZ(original))`, where `mm` is the largest block size and
    `g = log_k(n) / n`. **Only the largest scale survives; the intermediate scales cancel.**
    Check it on the example above — `mm = 3`, `n = 24`, `k = 3`, `C_LZ = 4` — and inverting the
    identity recovers a shuffled complexity of exactly `7.0`, an integer, as it must be.

    Two consequences follow, and both bite in practice. The value has **no lower bound at
    zero**: if the shuffle at scale `mm` factorises into *fewer* components than the original,
    the result is negative.

    ```python
    >>> import random
    >>> r = random.Random(0)
    >>> noise = "".join(r.choice("01") for _ in range(2000))
    >>> lz.emc(noise)[0]
    -0.39476823424783447
    ```

    And `0.0` means only that the shuffle at scale `mm` happened to leave the complexity
    unchanged — it is not a signature of periodicity. `lz.emc("0101" * 100)[0]` is `0.0`, but
    `lz.emc("01" * 400)[0]` is `2.6641152724252684`, for the same period-2 structure at a
    different length.

    The per-scale `summands` stay informative about where structure sits. Read
    [Effective measure complexity](../concepts/emc.md) before reporting any of this.

## What to read next

<div class="lz-cards" markdown>
  <div class="lz-card" markdown>

### Reading the numbers

Which of the four numbers to report, what each one is sensitive to, and which comparisons are
invalid.

[Reading the numbers](reading-the-numbers.md)

<p class="lz-card__api"><code>guide/reading-the-numbers</code></p>

  </div>
  <div class="lz-card" markdown>

### How LZ76 works

The exhaustive history, why it is unique and minimal, and how it differs from LZ77, LZ78 and
the stringology "LZ-factorization".

[How LZ76 works](../concepts/lz76.md)

<p class="lz-card__api"><code>concepts/lz76</code></p>

  </div>
  <div class="lz-card" markdown>

### Python API

All five functions and every parameter, including the two that are accepted and ignored.

[Python API](../api/python.md)

<p class="lz-card__api"><code>lz.factorization · lz.h · lz.nid · lz.emc · lz.lz76</code></p>

  </div>
  <div class="lz-card" markdown>

### What counts as a sequence

`str` is analysed as UTF-8 bytes, `bytearray` is not treated as bytes, and `list[int]` is
concatenated as decimal text. Read this before passing anything but ASCII.

[Input types](../api/inputs.md)

<p class="lz-card__api"><code>api/inputs</code></p>

  </div>
</div>
