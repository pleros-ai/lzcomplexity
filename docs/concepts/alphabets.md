# Alphabets and log bases

*How the library decides what a symbol is, and the two knobs that people get backwards.*

The alphabet is the set of distinct **byte values** present in the sequence; its size `k` is
floored at 2. It is detected automatically on every call. Nothing you pass in overrides that
detection — not `alphabet=` in Python, not `-a` on the command line. Both of those are
normalisation knobs, and each one reaches only half the outputs you would expect.

<div class="lz-formula">
  <p class="lz-math"><i>h</i> = <i>c</i>(<i>S</i>) · log<sub><i>b</i></sub> <i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>number of complete LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length, in bytes</dd>
    <dt><i>k</i></dt><dd>detected alphabet size = max(2, number of distinct bytes)</dd>
    <dt><i>b</i></dt><dd>log base — the explicit setting if given, otherwise <i>k</i></dd>
  </dl>
  <p class="lz-formula__cite">Core implementation: entropy_density_from in crates/lzcomplexity-core/src/lz76.rs.</p>
</div>

`k` enters `h` in exactly one place: it is the default value of `b`. It is also the default
alphabet size in the separate `epsilon` term, covered in
[Which knob affects which output](#which-knob-affects-which-output). The factorization itself —
the factor count and the factor boundaries — does not depend on the alphabet at all.

## Detection, precisely

1. A 256-entry presence table is filled by one pass over the bytes. A *symbol* is a `u8` — never a
   Unicode character, never an integer.
2. `k = max(2, number of distinct bytes)`. The floor keeps the log base usable: at `k = 1`,
   `ln(1) = 0` and every division by it blows up.
3. The alphabet vector is collected by scanning `255 → 0`, so anything that prints it prints it in
   descending byte order. The CLI additionally renders it as signed `i8`, so a byte ≥ 128 shows up
   negative — see the [`lzcomplexity` reference](../cli/lzcomplexity.md).

## What that means in practice

Every row below is a real call through the Python API at version 1.0.0 — `lz.factorization()` for
`c`, `lz.h()` for `h`. Feeding the same strings to the CLI does not always give the same `n`,
because the CLI picks a reader first; see [Input formats](../cli/formats.md).

<div class="lz-scroll" markdown>

| Input | Python `len()` | Symbols `n` | Distinct bytes | `k` | `c` | `h` |
|---|---|---|---|---|---|---|
| `"aaaa"` | 4 | 4 | 1 | **2** | 1 | 0.5 |
| `"01010101"` | 8 | 8 | 2 | 2 | 2 | 0.75 |
| `"ACGTACGT"` | 8 | 8 | 4 | 4 | 4 | 0.75 |
| `"AACGTacgtN"` | 10 | 10 | 9 | **9** | 9 | 0.9431564734302231 |
| `"héllo wörld"` | 11 | **13** | 10 | 10 | 10 | 0.8568795017744898 |
| `"🙂🙂🙂🙂"` | 4 | **16** | 4 | 4 | 4 | 0.5 |

</div>

`c` counts only *complete* LZ76 components; the trailing component that runs past the end of the
sequence is not counted. See [LZ76 factorization](lz76.md) for that convention.

### A `str` is its UTF-8 bytes

A Python `str` is encoded to UTF-8 and the bytes become the symbols. For ASCII input that is
invisible. For anything else, `len(s)` and the library's `n` disagree:

```python
import lzcomplexity as lz

lz.h("héllo wörld")            # 0.8568795017744898
lz.h("héllo wörld".encode())   # 0.8568795017744898 — identical, bit for bit
len("héllo wörld")             # 11 characters
len("héllo wörld".encode())    # 13 symbols: é and ö are two bytes each
```

`"🙂🙂🙂🙂"` is four characters and sixteen symbols; its alphabet is the four bytes
`F0 9F 99 82`, so `k = 4`, `c = 4`, and `h = 4·log_4(16)/16 = 0.5`. That number describes a 16-byte
sequence over a 4-symbol alphabet, not four emoji. For character-level analysis, map characters to
distinct single bytes yourself — see [Input types](../api/inputs.md).

### Case and ambiguity codes inflate `k`

Detection is byte-wise and case-sensitive, so soft-masked FASTA and IUPAC ambiguity codes each add
symbols. `"AACGTacgtN"` has nine distinct bytes, not four, and the larger log base pushes `h` down
for the very same sequence:

```python
lz.h("AACGTacgtN")              # 0.9431564734302231  — normalised by log_9
lz.h("AACGTacgtN", log_base=4)  # 1.4948676426993133  — normalised by log_4
```

Upper-case the sequence and strip ambiguity codes, or pass `log_base=4` explicitly, before
comparing a soft-masked genome against a clean one. Worked through in [Genomics](../recipes/genomics.md).

### A constant sequence still has `k = 2`

`"aaaa"` contains one distinct byte, but `k` is floored at 2 and the factorization short-circuits
to `c = 1`. So `h("aaaa") = 1 · log_2(4)/4 = 0.5` — **not** 0. Perfectly ordered input has a small
positive `h` that decays like `log_2(n)/n`; that is a finite-size effect, not a signal. See
[Convergence](convergence.md).

### The 256-symbol ceiling

A sequence is stored as raw bytes: at most **256 distinct symbols**, exactly 1 byte per symbol.
There is no wide-alphabet mode, and `alphabet=` does not raise the ceiling — it only changes the
`epsilon` term.

If your alphabet is bigger than 256, reduce it before you call: quantise a continuous signal into
≤ 256 bins, or collapse rare categories. What you must **not** do is hand the library large
integers.

!!! warning "Integers above 9 are silently re-encoded as decimal digits"

    `lz.h(list(range(1000)))` returns `0.9221077297309833`, computed over a **2890-byte,
    10-symbol** sequence — not over 1000 samples of a 1000-symbol alphabet. The integer branch
    renders each element as a decimal string and concatenates, so `[0, 1, 10]` becomes the four
    symbols `"0110"`: `n` inflates and the alphabet collapses onto the ten ASCII digits. Integer
    input is safe only when every value is a single digit, `0 ≤ v ≤ 9`. Pass `bytes(...)` instead.
    Full rules in [Input types](../api/inputs.md).

!!! example "Recoding labels to bytes"

    ```python
    import lzcomplexity as lz

    labels = ["cat", "dog", "emu", "cat", "dog", "cat", "emu", "emu", "dog", "cat"]

    codes = {v: i for i, v in enumerate(sorted(set(labels)))}   # {'cat': 0, 'dog': 1, 'emu': 2}
    seq = bytes(codes[v] for v in labels)                       # b'\x00\x01\x02\x00\x01\x00\x02\x02\x01\x00'

    lz.factorization(seq)      # (5, [0, 1, 2, 3, 6, 8, 11])
    lz.h(seq)                  # 1.0479516371446924
    ```

    Joining the labels instead of recoding them measures a different sequence — 30 symbols over a
    9-letter alphabet:

    ```python
    lz.factorization("".join(labels))   # (13, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 19, 22, 25, 31])
    lz.h("".join(labels))               # 0.6707790427627
    ```

<hr class="lz-tickrule">

## Trap 1 — in Python, `alphabet=` does not change `h`

!!! warning "Every value of `alphabet=` returns the same `h`"

    Changing `alphabet=` and re-reading `h` looks like the parameter is being ignored, because it
    is: the entropy formula reads `log_base` only, and `alphabet` never appears in it. On a 58-nt
    DNA string (`n = 58`, `k = 4`, `c = 18`):

    ```python
    dna = "ACGTACGTTGCAACGTGGATCCGTAAGCTTACGATCGATCGGATCAGCTAGCATCGAT"

    lz.h(dna)                # 0.9089970509680715
    lz.h(dna, alphabet=2)    # 0.9089970509680715   ← unchanged
    lz.h(dna, alphabet=64)   # 0.9089970509680715   ← unchanged
    lz.h(dna, log_base=2)    # 1.817994101936143    ← this is the knob
    ```

    `alphabet=` feeds exactly one output: the finite-size term `epsilon` in `lz76()`.

    ```python
    lz.lz76(dna)["epsilon"]                # 1.3568356339445253
    lz.lz76(dna, alphabet=2)["epsilon"]    # 1.289790427595217
    ```

    The docstring shipped with 1.0.0 calls `alphabet` the size "used in the entropy formula". That
    wording is wrong; the behaviour above is what the code does.

Converting between bases after the fact works, because `h` is linear in `1/ln(b)`:
`h_bits = h_norm · log₂(k)`. On the DNA string above, `0.9089970509680715 · 2 = 1.817994101936143`
bit for bit — but that is luck, not a guarantee. Over 299 random sequences with `k` between 3 and
26, rescaling by hand and passing `log_base=2` gave identical floats in 157 cases and differed by
1–3 ULP in the other 142. Prefer `log_base=` if you need the two routes to match exactly.

Any `log_base` below 2 clamps to 2 — `log_base=0`, `1` and `2` all return the same float.

## Trap 2 — on the CLI, `-l` does not change the entropy density

!!! warning "`-l` leaves `lz76EntropyDensity` untouched; `-a` is the flag that moves it"

    The CLI computes its entropy density with its own function, which reads the **alphabet** and
    never the log base — the exact mirror image of Trap 1. Same 58-nt sequence, in a file, format
    forced to text:

    ```console
    $ lzcomplexity -n -F TXT dna.txt -o dna.json
    {"filename":"dna.txt","format":"PNM_RAWTXT","sequences":[{"alphabet":[84,71,67,65],
    "alphabet_size":4,"lz76Complexity":18,"lz76EntropyDensity":0.9089970509680715,
    "lz76RandomShuffleComplexity":{"max_block_size":-1,"multi_information":0.0,"value":0.0},
    "size":58}],"size":1}
    ```

    (Real output is one line with no whitespace; wrapped here to fit.) Varying the two flags:

    | Flags | `lz76EntropyDensity` |
    |---|---|
    | *(none)*, `-a auto`, `-a xyz` | 0.9089970509680715 |
    | `-l 2` | 0.9089970509680715 |
    | `-l 10` | 0.9089970509680715 |
    | `-a 2`, `-a 1`, `-a 0` | 1.817994101936143 |
    | `-a 2 -l 4` | 1.817994101936143 |

    Values below 2 clamp to 2; `auto` and anything unparseable fall back to detection. The
    reported `alphabet_size` is always the **detected** value — it never echoes `-a`.

    The README's `lzcomplexity input.txt -a 4 -l 2` example is exactly this trap: the `-l 2` does
    nothing to the reported entropy density.

`-l` is not inert, though — it still drives the shuffle stage. And on the CLI the two flags are
coupled: when `-l` is absent, `-a X` sets the log base to `X` as well, so `-a` alone moves the
[EMC](emc.md) numbers too.

<div class="lz-run" markdown>

```console
$ lzcomplexity -F TXT dna.txt -o e.json             # h=0.9089970509680715  mi=0.1009996723297858
$ lzcomplexity -F TXT dna.txt -a 2 -o e.json        # h=1.817994101936143   mi=0.2019993446595716
$ lzcomplexity -F TXT dna.txt -l 2 -o e.json        # h=0.9089970509680715  mi=0.2019993446595716
$ lzcomplexity -F TXT dna.txt -a 2 -l 4 -o e.json   # h=1.817994101936143   mi=0.1009996723297858
```

</div>

`h` is `lz76EntropyDensity` and `mi` is `lz76RandomShuffleComplexity.multi_information`, both read
back out of `e.json`. Look at the last two lines: `-l` moves `mi` on its own, and an explicit
`-l 4` cancels the log base that `-a 2` would otherwise have set. In Python the two parameters are
independent; there is no such coupling.

## When Python and the CLI agree

They agree **iff the CLI's effective alphabet `α` equals Python's effective log base `b`**:

- CLI: `α = max(2, the -a value, or the detected k)`, and it computes `c · log_α(n) / n`.
- Python: `b = max(2, log_base, or the detected k)`, and it computes `c · log_b(n) / n`.

With the defaults both resolve to the detected `k`, so the common case agrees. It stops agreeing
the moment you set one knob and not the other.

<div class="lz-scroll" markdown>

| Situation | Agree? |
|---|---|
| Neither flag nor keyword given (the default) | Yes — both resolve to `k` |
| CLI `-a X`, Python `alphabet=X, log_base=X` | Yes |
| CLI `-a X`, Python default | Only if `X == k` |
| CLI default, Python `log_base=Y` | Only if `Y == k` |
| CLI `-a X -l Y` with `X ≠ Y`, Python `log_base=Y` | No — the CLI uses `X`, Python uses `Y` |
| Any value below 2 on either side | Both clamp to 2, then the clamped values decide |

</div>

!!! note "Agreement is up to floating-point rounding, not bit-for-bit"

    The two implementations evaluate the same expression in different orders — the core computes
    `c / (n / (ln n / ln b))`, the CLI computes `c · ln(n) / (n · ln α)`. On a 47-symbol binary
    sequence with `α = b = 2` they land 1 ULP apart:

    ```text
    CLI     1.0636446737255048    3c730347b004f13f
    Python  1.063644673725505     3d730347b004f13f
    ```

    A randomised sweep of 20 000 `(n, k, c)` triples found the two forms differing in roughly 46%
    of cases: 1 ULP apart in about 8 700 of them, 2 ULP in about 600, and 3 ULP in fewer than ten.
    Any test comparing CLI JSON against Python floats needs a tolerance.

    A second and larger source of CLI-versus-Python disagreement is format detection, not
    arithmetic. With `-F` absent, the CLI classifies a file from its **first three bytes** (up to
    the first newline): if any of them is not an ASCII alphanumeric, space or tab, the file is read
    as a *bitstream* — every byte expanded into eight `0`/`1` symbols, so `n` grows roughly
    eightfold and `k` collapses to 2. A file whose first bytes are ASCII is read as text even if
    high bytes appear later. Pass `-F` explicitly for non-ASCII input. See
    [Input formats](../cli/formats.md).

<hr class="lz-tickrule">

## Which knob affects which output

<div class="lz-scroll" markdown>

| Output | Python `alphabet=` | Python `log_base=` | CLI `-a` | CLI `-l` |
|---|---|---|---|---|
| `complexity` / `factors` | — | — | — | — |
| `h` / `lz76EntropyDensity` | — | ✓ | ✓ | — |
| `epsilon` | ✓ | ✓ | not emitted | not emitted |
| `normal_error`, `poison_error` | — | ✓ (via `h`) | not emitted | not emitted |
| `factors_stddev` | — | — | not emitted | not emitted |
| EMC value, `summands`, `multi_information` | — | ✓ | ✓ (via the coupling) | ✓ |
| `nid`, `InformationDistance` | — | — | — | — |

</div>

✓ = changes the value; — = no effect. `epsilon` exists only in the Python `lz76()` dict — the CLI
computes it internally and never writes it to the JSON. The EMC row is why `-a` is not a harmless
relabelling on the CLI even when all you want is the entropy density.

!!! note "`-a` means something else in `lzdistance`"

    The `-a` column above is `lzcomplexity`'s `--alphabet`. In [`lzdistance`](../cli/lzdistance.md)
    the short flag `-a` is `--adn`, a *format* switch, and there is no alphabet flag at all.

Two things to carry away:

- **In Python, change the units of `h` with `log_base=`.** `alphabet=` is an `epsilon` knob.
- **On the CLI, change the units of `lz76EntropyDensity` with `-a`.** `-l` reaches only the shuffle
  stage.

Related reading: [Entropy density](entropy-density.md) for what `h` means and how far it can be
trusted, [Reading the numbers](../guide/reading-the-numbers.md) for a walk through a full result
dict, and the [Python API reference](../api/python.md) for exact signatures.
