# Python API

*Five functions, no classes, no custom exceptions — every signature, default and return shape.*

`lzcomplexity` exports five callables and one version string. There is nothing else on the
public surface: no classes, no submodules, no exception hierarchy.

```pycon
>>> import lzcomplexity as lz
>>> lz.__version__
'1.0.0'
>>> lz.__all__
['lz76', 'factorization', 'h', 'emc', 'nid']
```

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">5</span><span class="lz-stat__k">public functions</span></div>
<div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">classes</span></div>
<div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">custom exceptions</span></div>
<div class="lz-stat"><span class="lz-stat__v">3.9+</span><span class="lz-stat__k">requires-python</span></div>
</div>

## At a glance

| Function | Returns | What it gives you |
|---|---|---|
| [`factorization(seq, …)`](#lz-factorization) | `tuple[int, list[int]]` | complexity and the factor boundary list |
| [`h(seq, …)`](#lz-h) | `float` | normalized entropy density (entropy-rate estimator) |
| [`emc(seq, …)`](#lz-emc) | `tuple[float, list[float]]` | effective measure complexity and its per-scale summands |
| [`nid(seq1, seq2, …)`](#lz-nid) | `float` | normalized information distance between two sequences |
| [`lz76(seq, …)`](#lz-lz76) | `dict` (9 keys) | everything above plus ε, error estimates and `extras` |

All five are compiled `builtin_function_or_method` objects. Every parameter is
positional-or-keyword — there are no keyword-only parameters and no var-args — so
`lz.h(seq="abab")` and `lz.nid(seq1="ab", seq2="ba")` both work.

Throughout this page, `SEQ` is the 47-symbol binary sequence
`"01001010101101010101110101010101010000100101011"`.

<div class="lz-tickrule"></div>

## Shared parameters

Besides the sequence itself, four parameters appear on every function; `max_block_size`
appears on `emc` and `lz76` only.

| Parameter | Type | Default | Effect |
|---|---|---|---|
| `seq` | `str \| bytes \| list[int] \| list[str] \| sequence of ints` | — | the input. See [Inputs](inputs.md) for the coercion rules |
| `partitions` | `int` | `1` | **none — accepted and ignored** |
| `alphabet` | `int \| None` | `None` | affects `lz76(...)["epsilon"]` and nothing else |
| `log_base` | `int \| None` | `None` | entropy units. `None` means "use the detected alphabet size" |
| `max_block_size` | `int` | `-1` | largest shuffle block size for EMC. Any value `<= 0` selects automatically |
| `jobs` | `int` | `0` | **none — accepted and ignored** |

### `alphabet` affects only `epsilon`

The core reads the alphabet argument at exactly one place: the finite-size correction ε
computed inside the factorizer. `factorization`, `h`, `emc` and `nid` never surface ε, so
passing `alphabet` to those four is a no-op — verified bit-identical for `alphabet=2` against
`alphabet=99` on a 500-symbol ternary sequence.

<div class="lz-formula">
  <p class="lz-math">ε = 2 · (1 + log<sub><i>b</i></sub> log<sub><i>b</i></sub>(<i>k</i>·<i>n</i>)) ⁄ log<sub><i>b</i></sub> <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>k</i></dt><dd>max(<code>alphabet</code>, 2), or max(count of distinct bytes, 2) when <code>alphabet=None</code></dd>
    <dt><i>b</i></dt><dd>max(<code>log_base</code>, 2), or the <i>detected</i> alphabet size (minimum 2) when <code>log_base=None</code> — the detected size, not <i>k</i>, so passing <code>alphabet</code> does not move <i>b</i></dd>
    <dt><i>n</i></dt><dd>sequence length in bytes</dd>
  </dl>
</div>

```pycon
>>> SEQ = "01001010101101010101110101010101010000100101011"
>>> lz.lz76(SEQ)["epsilon"]
1.336734504313188
>>> lz.lz76(SEQ, alphabet=4)["epsilon"]
1.4104926018454618
>>> lz.h(SEQ) == lz.h(SEQ, alphabet=4)      # h ignores alphabet entirely
True
```

!!! warning "The `h` docstring is wrong about `alphabet`"

    `help(lz.h)` calls `alphabet` the "effective alphabet size used in the entropy formula".
    It is not used in the entropy formula — only `log_base` enters `h`. The docstring is a
    known defect; the behaviour documented on this page is the measured one.

### `log_base` sets the entropy units

`log_base=None` means "use the detected alphabet size", so `h` is normalized against the
alphabet by default rather than reported in bits. Pass `log_base=2` for bits.

```pycon
>>> lz.h("ABRACADABRA")                 # b = 5 distinct symbols
0.6772255010931718
>>> lz.h("ABRACADABRA", log_base=2)     # bits
1.572468917562408
```

`log_base` is inert for `factorization` (a raw count) and for `nid` (a ratio of raw counts).
Values `0` and `1` are silently clamped to `2`. Only integers are accepted:
`log_base=2.718` raises `TypeError`, so **there is no way to request natural log**.

!!! warning "`4294967295` is silently read as `None`"

    Passing `alphabet=4294967295` or `log_base=4294967295` returns the auto-detected result,
    not a base-4294967295 result. That value is the internal "no alphabet" sentinel
    (`u32::MAX`) and the API cannot distinguish it from `None`. `4294967294` is honoured
    normally.

    ```pycon
    >>> lz.h(SEQ, log_base=4294967295)
    1.063644673725505
    >>> lz.h(SEQ)                        # identical — the sentinel won
    1.063644673725505
    ```

### The two parameters that do nothing

<div class="lz-api lz-api--ignored" id="lz-partitions">
  <p class="lz-api__sig"><b>partitions</b>: <i>int</i> = 1 <span class="lz-api__badge">ignored</span></p>
  <p class="lz-api__lede">Accepted, stored, and never read by any algorithm.</p>
  <dl class="lz-api__params">
    <dt>what happens</dt><dd>The value is written to the internal <code>LzArgs.chunks</code> field. The only read of that field anywhere in the Rust workspace is inside a <code>PartialEq</code> implementation. No factorization, entropy, shuffle or distance code path consults it.</dd>
    <dt>not a speed knob</dt><dd>It cannot be one. There is no suffix-array partitioning in the core — one suffix array is built per sequence. Any timing difference between <code>partitions</code> values is measurement noise.</dd>
    <dt>range</dt><dd><code>i32</code>. Negative values are accepted without complaint; <code>partitions=2**31</code> raises <code>OverflowError</code>.</dd>
  </dl>
</div>

<div class="lz-api lz-api--ignored" id="lz-jobs">
  <p class="lz-api__sig"><b>jobs</b>: <i>int</i> = 0 <span class="lz-api__badge">ignored</span></p>
  <p class="lz-api__lede">Accepted and discarded on the first line of every function.</p>
  <dl class="lz-api__params">
    <dt>what happens</dt><dd>Each of the five bindings begins with <code>let _ = jobs;</code>. The value never reaches the argument struct and never reaches the core.</dd>
    <dt>the real knob</dt><dd>The <code>RAYON_NUM_THREADS</code> environment variable. Results are bit-identical across thread counts, so it is a pure performance dial.</dd>
    <dt>range</dt><dd><code>u32</code>. <code>jobs=-1</code> raises <code>OverflowError</code>. The parameter manages to be both inert and capable of failing.</dd>
  </dl>
</div>

Verified: `lz76(SEQ, partitions=1) == lz76(SEQ, partitions=99)` and
`lz76(SEQ, jobs=0) == lz76(SEQ, jobs=32)`, both `True`.

Both parameters exist because the C++ predecessor had them, and they are kept so that old
call sites keep working. They are documented here as inert rather than quietly removed.

<div class="lz-tickrule"></div>

## `factorization`

<div class="lz-api" id="lz-factorization">
  <p class="lz-api__sig"><b>lz.factorization</b>(<i>seq</i>, partitions=1, alphabet=None, log_base=None, jobs=0) <span class="lz-api__ret">→ tuple[int, list[int]]</span> <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">The LZ76 complexity — a count of complete components — together with the factor boundary indices.</p>
  <dl class="lz-api__params">
    <dt>seq</dt><dd><code>str | bytes | list[int] | list[str]</code>, or any object supporting the sequence protocol whose elements are Python ints. Iterators, generators and sets raise <code>TypeError</code>.</dd>
    <dt>partitions</dt><dd><code>int</code> — ignored. See <a href="#lz-partitions">above</a>.</dd>
    <dt>alphabet</dt><dd><code>int | None</code> — no effect on this function.</dd>
    <dt>log_base</dt><dd><code>int | None</code> — no effect on this function; the return is a raw count.</dd>
    <dt>jobs</dt><dd><code>int</code> — ignored. See <a href="#lz-jobs">above</a>.</dd>
    <dt>returns</dt><dd><code>(complexity, factors)</code>. <code>complexity</code> is a Python <code>int</code>; <code>factors</code> is a <code>list[int]</code> of boundary indices, always beginning <code>[0, 1, …]</code>. Factor <i>i</i> spans <code>seq[factors[i]:factors[i+1]]</code>.</dd>
  </dl>
</div>

```pycon
>>> lz.factorization("banana")
(3, [0, 1, 2, 3, 7])
>>> lz.factorization("ABRACADABRA")
(5, [0, 1, 2, 3, 5, 7, 12])
>>> lz.factorization("01010101")
(2, [0, 1, 2, 9])
>>> lz.factorization(SEQ)
(9, [0, 1, 2, 4, 7, 12, 22, 31, 36, 41, 48])
```

!!! warning "`len(factors) - 1` is not the complexity"

    Taking `len(factors) - 1` overcounts by one on most inputs, because the final boundary
    usually runs past the end of the sequence. `"banana"` has `n = 6` but a final boundary of
    `7`; `"01010101"` has `n = 8` and a final boundary of `9`. On 4 990 random non-constant
    strings (lengths 2–300, alphabets of size 2/3/4/26) the boundary list overshot in
    **3 674 of them — 74 %**. Use the returned integer, or:

    ```python
    complexity = len(factors) - 1 if factors[-1] <= len(seq) else len(factors) - 2
    ```

`complexity` counts **complete** components only. The trailing component that runs past the
end of the sequence is not counted, so this library's number is one less than the textbook
exhaustive-history count whenever the sequence ends mid-component. The conversion is exact:

<div class="lz-formula">
  <p class="lz-math"><i>c</i><sub>textbook</sub> = <i>c</i> + 1 &nbsp;if&nbsp; factors[−1] &gt; <i>n</i>, &nbsp;otherwise&nbsp; <i>c</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i></dt><dd>the integer returned by <code>factorization</code></dd>
    <dt>factors[−1]</dt><dd>the last element of the returned boundary list</dd>
    <dt><i>n</i></dt><dd><code>len(seq)</code> in bytes</dd>
  </dl>
  <p class="lz-formula__cite">Checked against an independent exhaustive-history implementation on 497 random non-constant strings — zero disagreements, on both the boundary list and the converted count.</p>
</div>

Background: [LZ76 factorization](../concepts/lz76.md).

Empty and constant sequences take a shortcut path that returns a synthetic boundary list and
bypasses the rule above:

| input | result | note |
|---|---|---|
| `""` | `(1, [0, 1, 0])` | non-monotonic; **an empty sequence reports complexity 1, not 0** |
| `"a"` | `(1, [0, 1, 1])` | |
| `"aaaa"` | `(1, [0, 1, 4])` | |
| `[]`, `()`, `b""` | `(1, [0, 1, 0])` | same as `""` |

<div class="lz-tickrule"></div>

## `h`

<div class="lz-api" id="lz-h">
  <p class="lz-api__sig"><b>lz.h</b>(<i>seq</i>, partitions=1, alphabet=None, log_base=None, jobs=0) <span class="lz-api__ret">→ float</span> <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">Normalized entropy density — the LZ76 entropy-rate estimator.</p>
  <dl class="lz-api__params">
    <dt>seq</dt><dd>as for <code>factorization</code>.</dd>
    <dt>partitions</dt><dd><code>int</code> — ignored.</dd>
    <dt>alphabet</dt><dd><code>int | None</code> — no effect, despite what the docstring says.</dd>
    <dt>log_base</dt><dd><code>int | None</code> — the logarithm base. <code>None</code> uses the detected alphabet size (minimum 2); pass <code>2</code> for bits. <code>0</code> and <code>1</code> clamp to <code>2</code>.</dd>
    <dt>jobs</dt><dd><code>int</code> — ignored.</dd>
    <dt>returns</dt><dd>a Python <code>float</code>. Sequences of length ≤ 1 short-circuit to <code>0.0</code>.</dd>
  </dl>
</div>

<div class="lz-formula">
  <p class="lz-math"><i>h</i> ≈ <i>c</i>(<i>S</i>) · log<sub><i>b</i></sub> <i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>the complexity returned by <code>factorization</code></dd>
    <dt><i>n</i></dt><dd>sequence length in bytes</dd>
    <dt><i>b</i></dt><dd>max(<code>log_base</code>, 2), or the detected alphabet size when <code>log_base=None</code></dd>
  </dl>
  <p class="lz-formula__cite">The core evaluates it as <code>c / (n / log_b(n))</code>. Recomputing the algebraically equal <code>c * log_b(n) / n</code> in Python reproduced the returned float on 1 452 of 2 000 random sequences and differed by 1–2 ulp on the rest — reproduce the evaluation order, not only the algebra.</p>
</div>

```pycon
>>> lz.h("01010101")            # c = 2, n = 8, b = 2  ->  2 * 3 / 8
0.75
>>> lz.h("banana")
0.8154648767857287
>>> lz.h("ABRACADABRA")
0.6772255010931718
>>> lz.h(SEQ)
1.063644673725505
>>> lz.h(""), lz.h("a"), lz.h("aa"), lz.h("ab")
(0.0, 0.0, 0.5, 1.0)
```

!!! warning "`h` is not bounded by 1"

    Values above 1 are normal on short random sequences and do not indicate a bug. `h` is a
    finite-size estimator that approaches its limit **from above**: averaged over random
    binary strings it reads about 1.18 at n = 10, 1.13 at n = 100 and 1.03 at n = 10 000
    (20 000 / 5 000 / 20 draws). `h(SEQ)` above is
    `1.0636…` for exactly this reason. See
    [Finite-size convergence](../concepts/convergence.md).

<div class="lz-tickrule"></div>

## `emc`

<div class="lz-api" id="lz-emc">
  <p class="lz-api__sig"><b>lz.emc</b>(<i>seq</i>, partitions=1, alphabet=None, log_base=None, max_block_size=-1, jobs=0) <span class="lz-api__ret">→ tuple[float, list[float]]</span> <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">Effective measure complexity via block shuffling, with the per-block-size terms that sum to it.</p>
  <dl class="lz-api__params">
    <dt>seq</dt><dd>as for <code>factorization</code>.</dd>
    <dt>partitions</dt><dd><code>int</code> — ignored.</dd>
    <dt>alphabet</dt><dd><code>int | None</code> — no effect.</dd>
    <dt>log_base</dt><dd><code>int | None</code> — sets the scaling factor <i>g</i> = log<sub><i>b</i></sub>(<i>N</i>) ⁄ <i>N</i>.</dd>
    <dt>max_block_size</dt><dd><code>int</code> — the largest shuffle block size. Any value <code>&lt;= 0</code> (including <code>-1</code>, <code>0</code> and <code>-100</code>) selects automatically from the sequence length. <b>It is never clamped to <code>len(seq)</code>.</b></dd>
    <dt>jobs</dt><dd><code>int</code> — ignored.</dd>
    <dt>returns</dt><dd><code>(emc_value, summands)</code>. <code>summands</code> always has exactly as many entries as the <i>effective</i> block size — never empty, never <code>None</code>, even for degenerate input. <code>emc_value</code> is the running total of those terms in order, so a plain <code>for</code> loop reproduces it exactly; CPython's builtin <code>sum()</code> compensates and can land 1 ulp away. <code>summands[0]</code> is the multi-information term.</dd>
  </dl>
</div>

```pycon
>>> lz.emc("ABRACADABRA")
(0.5417804008745377, [0.27089020043726886, 0.27089020043726886])
>>> lz.emc(SEQ)
(0.0, [0.11818274152505626, -0.11818274152505626,
       0.35454822457516877, -0.35454822457516877])
>>> lz.emc(""), lz.emc("a"), lz.emc("abab")
((0.0, [0.0]), (0.0, [0.0]), (0.0, [0.0]))
>>> lz.emc("a", max_block_size=5)
(0.0, [0.0, 0.0, 0.0, 0.0, 0.0])
```

!!! example "A sequence with genuine structure"

    A half-periodic, half-random binary string gives a clearly nonzero EMC and 17 summands.

    ```python
    import random
    import lzcomplexity as lz

    r = random.Random(42)
    s = "01" * 300 + "".join(r.choice("01") for _ in range(400))   # n = 1000
    value, summands = lz.emc(s)
    ```

    ```pycon
    >>> value
    2.2024383269103205
    >>> len(summands)
    17
    >>> summands
    [0.5082549985177665, 0.049828921423310524, 0.578015488510401, -0.17938411712391766,
     0.43849450852513194, 0.03986313713864864, 0.3786998028171593, -0.6178786256490494,
     0.6876391156416835, -0.1893499014085791, 0.38866558710182075, -0.40859715567114496,
     0.787296958488304, -0.6477759785030355, 0.8769890170502641, -1.5745939169766097,
     1.0862704870281668]
    >>> total = 0.0
    >>> for term in summands:
    ...     total += term
    ...
    >>> total == value
    True
    >>> sum(summands) == value       # builtin sum() compensates on CPython 3.12+
    False
    >>> lz.factorization(s)[0]
    51
    ```

### The sum telescopes — only the largest block size contributes

Each summand is `(H_l - H_{l-1}) - h_hat`. Summed over the scales, the block-entropy terms
cancel in pairs and everything except the largest scale drops out:

<div class="lz-formula">
  <p class="lz-math">Ê = Σ<sub><i>l</i> = 1…<i>mm</i></sub> [(<i>H</i><sub><i>l</i></sub> − <i>H</i><sub><i>l</i>−1</sub>) − <i>ĥ</i>] = <i>mm</i> · <i>g</i> · (<i>C</i><sub>LZ</sub>(<i>u</i><sup>RS(<i>mm</i>)</sup>) − <i>C</i><sub>LZ</sub>(<i>u</i>))</p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd>the effective <code>max_block_size</code>, which also equals <code>len(summands)</code></dd>
    <dt><i>g</i></dt><dd>log<sub><i>b</i></sub>(<i>N</i>) ⁄ <i>N</i></dd>
    <dt><i>C</i><sub>LZ</sub>(<i>u</i>)</dt><dd>complexity of the original sequence</dd>
    <dt><i>C</i><sub>LZ</sub>(<i>u</i><sup>RS(<i>mm</i>)</sup>)</dt><dd>complexity after block-shuffling at scale <i>mm</i></dd>
  </dl>
  <p class="lz-formula__cite">Confirmed by inverting the identity, which recovers integral factor counts: the worked example above gives exactly 64.000, and 300 random binary strings (n = 200…2 000) all came back integral to within 3 × 10<sup>−14</sup>.</p>
</div>

!!! danger "The total does not depend on the intermediate scales"

    Do not read the EMC total as an accumulation across scales. Algebraically it reduces to
    `mm × (the rise in entropy rate when the sequence is scrambled in blocks of size mm)` and
    nothing else — the intermediate summands cancel exactly. The per-scale `summands` remain
    informative and are worth plotting, but the returned scalar is a two-point statistic.

    A direct consequence: `emc(SEQ)` is exactly `0.0` because block-shuffling `SEQ` at
    `mm = 4` happens to leave its complexity at 9. An exact zero is a resonance, not an
    absence of structure — `lz.emc("01" * 1024)` is also exactly `0.0` (mm = 18, a multiple
    of the period), while `lz.emc("01" * 500)` returns `2.0330199940710654` (mm = 17).
    Constant sequences return `0.0` with all-zero summands.

### Automatic `max_block_size`

With `max_block_size <= 0` the block size is derived from the length. Measured values of
`len(summands)`:

| `len(seq)` | 5 | 9 | 10 | 20 | 47 | 50 | 51 | 100 | 1 000 | 10 000 |
|---|---|---|---|---|---|---|---|---|---|---|
| auto `mm` | 1 | 1 | 2 | 3 | 4 | 4 | 14 | 14 | 17 | 20 |

The jump at n = 51 is a `+10` step applied above 50 symbols.

!!! warning "A large `max_block_size` costs a full re-factorization per unit"

    Runtime grows linearly in `max_block_size`, and each unit re-factorizes the **whole**
    sequence, so a large value is only expensive when `len(seq)` is also large. Measured on a
    20 000-symbol binary string (one machine, ratios are the durable part): auto — mm = 21 —
    0.007 s, `max_block_size=50` 0.013 s, `max_block_size=200` 0.045 s. There is no upper
    clamp —
    `emc("abababab", max_block_size=100000)` really does return a 100 000-element list and
    perform 100 000 shuffle-and-refactorize rounds.

`emc` is reproducible run to run with no seeding, because the shuffle seed is derived from
the sequence content. See [Determinism](../project/determinism.md) and
[Effective measure complexity](../concepts/emc.md).

<div class="lz-tickrule"></div>

## `nid`

<div class="lz-api" id="lz-nid">
  <p class="lz-api__sig"><b>lz.nid</b>(<i>seq1</i>, <i>seq2</i>, partitions=1, alphabet=None, log_base=None, jobs=0) <span class="lz-api__ret">→ float</span> <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">Normalized information distance between two sequences, from four LZ76 factorizations.</p>
  <dl class="lz-api__params">
    <dt>seq1, seq2</dt><dd>two sequences, each accepted in any of the input forms. They need not be the same length or over the same alphabet.</dd>
    <dt>partitions</dt><dd><code>int</code> — ignored.</dd>
    <dt>alphabet</dt><dd><code>int | None</code> — no effect.</dd>
    <dt>log_base</dt><dd><code>int | None</code> — no effect; the result is a ratio of raw counts.</dd>
    <dt>jobs</dt><dd><code>int</code> — ignored.</dd>
    <dt>returns</dt><dd>a Python <code>float</code>. A zero-denominator guard exists but never fires, because every factorization returns at least <code>1</code>; <code>nid("", "")</code> is <code>0.0</code> because the numerator is zero, not because of the guard.</dd>
  </dl>
</div>

<div class="lz-formula">
  <p class="lz-math">NID(<i>X</i>, <i>Y</i>) = max[<i>C</i>(<i>XY</i>) − <i>C</i>(<i>X</i>), <i>C</i>(<i>YX</i>) − <i>C</i>(<i>Y</i>)] ⁄ max[<i>C</i>(<i>X</i>), <i>C</i>(<i>Y</i>)]</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 complexity of a sequence</dd>
    <dt><i>XY</i></dt><dd>concatenation of <i>X</i> followed by <i>Y</i></dd>
  </dl>
  <p class="lz-formula__cite">The four factorizations run concurrently inside one call, on the rayon pool.</p>
</div>

Background: [Information distance](../concepts/nid.md).

```pycon
>>> lz.nid("abcd", "abce")
0.25
>>> lz.nid("ABRACADABRA", "ABRACADABRZ")
0.3333333333333333
>>> lz.nid("ABRACADABRA", "ZYXWVUTSRQP")
0.9090909090909091
>>> lz.nid("aaaa", "bbbb")
1.0
>>> lz.nid("", ""), lz.nid("a", "")
(0.0, 0.0)
```

!!! warning "`nid(x, x)` is not always 0"

    A sequence compared with itself can return a nonzero distance, so a zero test is not a
    valid identity check. The result is 0 only when appending a copy of `x` to itself adds no
    new factors.

    ```pycon
    >>> lz.nid("abc", "abc")
    0.0
    >>> lz.nid(SEQ, SEQ)
    0.1111111111111111
    ```

    NID is symmetric by construction, and `nid(A, B) == nid(B, A)` held on every pair tested.
    Identity of indiscernibles fails on finite sequences, so it is not a metric in the strict
    sense.

!!! note "The `[0, 1]` range is empirical, not guaranteed"

    Every input tried returned a value in `[0, 1]`, but the numerator has no formal bound
    below the denominator for finite sequences. Treat the range as an observation.

<div class="lz-tickrule"></div>

## `lz76`

<div class="lz-api" id="lz-lz76">
  <p class="lz-api__sig"><b>lz.lz76</b>(<i>seq</i>, partitions=1, alphabet=None, log_base=None, max_block_size=-1, jobs=0) <span class="lz-api__ret">→ dict</span> <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">Everything the library computes about one sequence, in a single pass.</p>
  <dl class="lz-api__params">
    <dt>seq</dt><dd>as for <code>factorization</code>.</dd>
    <dt>partitions, jobs</dt><dd><code>int</code> — ignored.</dd>
    <dt>alphabet</dt><dd><code>int | None</code> — this is the one function where it matters; it changes <code>["epsilon"]</code>.</dd>
    <dt>log_base</dt><dd><code>int | None</code> — affects <code>h</code>, <code>epsilon</code>, the EMC scaling and both error estimates.</dd>
    <dt>max_block_size</dt><dd><code>int</code> — as for <code>emc</code>.</dd>
    <dt>returns</dt><dd>a <code>dict</code> with exactly 9 top-level keys, insertion-ordered as listed below. Two of them are nested dicts. A fresh object is built on every call; nothing is cached.</dd>
  </dl>
</div>

### The complete return value

Real output from `lzcomplexity` 1.0.0, unedited:

```pycon
>>> lz.lz76("ABRACADABRA")
{'complexity': 5,
 'h': 0.6772255010931718,
 'factors': [0, 1, 2, 3, 5, 7, 12],
 'emc': {'value': 0.5417804008745377,
         'summands': [0.27089020043726886, 0.27089020043726886],
         'max_block_size': 2,
         'multi_information': 0.27089020043726886},
 'epsilon': 2.10324356088793,
 'factors_stddev': 1.6187674827674623,
 'normal_error': 0.3320213997117467,
 'poison_error': 0.061565954644833804,
 'extras': {'rajski_distance': 0.3999999999999999,
            'redundancy': 0.375,
            'fh_uncertainty': 0.75,
            'lh_uncertainty': 0.75,
            'pearson_coefficient': 0.75}}
```

**Top level — 9 keys**

| Key | Type | Meaning |
|---|---|---|
| `complexity` | `int` | complete LZ76 components; identical to `factorization(seq)[0]` |
| `h` | `float` | normalized entropy density; identical to `h(seq)` |
| `factors` | `list[int]` | boundary indices; identical to `factorization(seq)[1]` |
| `emc` | `dict` | 4 keys, below |
| `epsilon` | `float` | Lempel–Ziv finite-size correction ε. Computed and reported, but **never applied** to `h` or inside the EMC path |
| `factors_stddev` | `float` | spread of factor lengths — **not a textbook standard deviation**, see below |
| `normal_error` | `float` | `sqrt(h**3) * factors_stddev / sqrt(n / log_b(n))` |
| `poison_error` | `float` | `h / n`. The key name is a frozen misspelling of *Poisson*; it is not corrected because that would break callers |
| `extras` | `dict` | 5 keys, below |

**`["emc"]` — 4 keys**

| Key | Type | Meaning |
|---|---|---|
| `value` | `float` | the same float as `emc(seq)[0]` |
| `summands` | `list[float]` | the same list as `emc(seq)[1]`; its length equals `max_block_size` |
| `max_block_size` | `int` | the block size actually used, after auto-selection |
| `multi_information` | `float` | the first summand, `summands[0]` |

**`["extras"]` — 5 keys.** All five compare the first half of the sequence (`fh`, split at
`n // 2`) with the second half (`lh`), using `mi = C(fh) + C(lh) − C(seq)`.

The core guards against a zero complexity by returning all-zero extras, but that guard never
fires: the factorizer returns at least `1` for every input, the empty sequence included. So
the extras are always populated, meaningful or not — see the empty-input example below.

| Key | Type | Definition |
|---|---|---|
| `rajski_distance` | `float` | `2 − (C(fh) + C(lh)) / C(seq)` |
| `redundancy` | `float` | `mi / (C(fh) + C(lh))` |
| `fh_uncertainty` | `float` | `mi / C(fh)` |
| `lh_uncertainty` | `float` | `mi / C(lh)` |
| `pearson_coefficient` | `float` | `mi / sqrt(C(fh) * C(lh))` |

!!! warning "`factors_stddev` divides by the longest factor, not the factor count"

    The value is not comparable with a standard deviation you compute yourself. The core
    evaluates `sqrt( Σ (len_i − mean)**2 / max_factor_len )`, where `mean` is
    `(factors[-1] − 1) / len(factors)` — divided by the number of *boundaries*, one more than
    the number of gaps. It is a heuristic dispersion measure inherited from the C++
    implementation, and `normal_error` is built on top of it, so it inherits the same caveat.

!!! note "The `lz76` docstring names a key that does not exist"

    `help(lz.lz76)` lists `entropy_density`. The key is `h`. The rename landed in 0.12.0 and
    the Rust docstring was missed; the type stub and the README are both correct.

### Cross-consistency with the standalone functions

Exact float equality, verified — the implementation factorizes once and reuses the count:

```pycon
>>> s = "ABRACADABRA"
>>> d = lz.lz76(s)
>>> d["complexity"] == lz.factorization(s)[0]
True
>>> d["factors"] == lz.factorization(s)[1]
True
>>> d["h"] == lz.h(s)
True
>>> (d["emc"]["value"], d["emc"]["summands"]) == lz.emc(s)
True
```

`lz76` costs several times more than `factorization` or `h` on the same input, because it
pays for `mm` extra shuffled re-factorizations — about 6× on a 100 000-symbol binary string,
where `mm` is 23. The multiplier tracks `mm`, so it grows slowly with length. If you only
need the complexity, call `factorization`.

### Degenerate input still returns the full dict

No exception is raised for an empty sequence. Every key is present, and some values are
numerically meaningless.

```pycon
>>> lz.lz76("")
{'complexity': 1,
 'h': 0.0,
 'factors': [0, 1, 0],
 'emc': {'value': 0.0, 'summands': [0.0], 'max_block_size': 1, 'multi_information': 0.0},
 'epsilon': 0.0,
 'factors_stddev': 1.3743685418725535,
 'normal_error': 0.0,
 'poison_error': 0.0,
 'extras': {'rajski_distance': 0.0,
            'redundancy': 0.5,
            'fh_uncertainty': 1.0,
            'lh_uncertainty': 1.0,
            'pearson_coefficient': 1.0}}
```

Note `factors_stddev = 1.374…` and a full set of nonzero `extras` for a sequence with no
data. If "no data" should be an error in your pipeline, check `len(seq)` yourself.

<div class="lz-tickrule"></div>

## Errors

There are no custom exception classes in this package. Everything raised is a builtin.

### Wrong sequence type → `TypeError`

| Call | Exception |
|---|---|
| `lz.factorization(None)` | `TypeError: expected str, bytes, list[str], list[int], or iterable of ints` |
| `lz.factorization(42)`, `lz.factorization(3.14)` | same message |
| `lz.factorization({0, 1})`, `lz.factorization({0: 1})` | same message — sets and dicts are rejected |
| `lz.factorization(iter([0, 1]))`, `lz.factorization(x for x in [0, 1])` | same message — **iterators and generators are rejected** |
| `lz.factorization([1.5, 2.5])`, `lz.factorization(np.array([0.0, 1.0]))` | same message — floats are rejected |
| `lz.factorization(np.array([[0, 1], [1, 0]]))` | same message — 2-D arrays are rejected |
| `lz.factorization([1, "a"])` | same message |
| `lz.factorization(["a", 1])` | `TypeError: 'int' object cannot be converted to 'PyString'` — **a different message**, because a list whose first element is a string commits to the string path |
| `lz.nid(None, "a")` | same message as the first row |

The module docstring's "any iterable of ints" is inaccurate. What is accepted is anything
that can be extracted into a vector of integers, which requires the **sequence protocol**.
Materialise lazy iterables into a list first. See [Inputs](inputs.md) for the full coercion
table, including the `bytearray` trap.

### Wrong argument type or out-of-range integer

| Call | Exception |
|---|---|
| `lz.factorization("ab", "x")` | `TypeError: argument 'partitions': 'str' object cannot be interpreted as an integer` |
| `lz.h("abab", partitions=None)` | `TypeError: argument 'partitions': 'NoneType' object cannot be interpreted as an integer` |
| `lz.h("abab", alphabet=2.5)` | `TypeError: argument 'alphabet': 'float' object cannot be interpreted as an integer` |
| `lz.h("abab", log_base=2.718)` | `TypeError: argument 'log_base': 'float' object cannot be interpreted as an integer` |
| `lz.emc("abab", max_block_size="x")` | `TypeError: argument 'max_block_size': 'str' object cannot be interpreted as an integer` |
| `lz.factorization("ab", alphabet=-1)` | `OverflowError: out of range integral type conversion attempted` |
| `lz.h("abab", log_base=-2)` | `OverflowError: out of range integral type conversion attempted` |
| `lz.factorization("ab", jobs=-1)` | `OverflowError: out of range integral type conversion attempted` |
| `lz.h(SEQ, alphabet=2**32)` | `OverflowError: out of range integral type conversion attempted` |
| `lz.factorization(SEQ, partitions=2**31)` | `OverflowError: out of range integral type conversion attempted` |
| `lz.h(SEQ, log_base=2**63)` | `OverflowError: Python int too large to convert to C long` |

`alphabet`, `log_base` and `jobs` are unsigned 32-bit: negatives and values ≥ 2³² raise
`OverflowError`, never `ValueError`. `partitions` and `max_block_size` are signed 32-bit, so
negatives are accepted there.

### Arity and keyword errors

| Call | Exception |
|---|---|
| `lz.factorization()` | `TypeError: factorization() missing 1 required positional argument: 'seq'` |
| `lz.nid("ab")` | `TypeError: nid() missing 1 required positional argument: 'seq2'` |
| `lz.factorization("ab", nonsense=1)` | `TypeError: factorization() got an unexpected keyword argument 'nonsense'` |
| `lz.h("abab", 1, None, None, 0, 9)` | `TypeError: h() takes from 1 to 5 positional arguments but 6 were given` |

### What does **not** raise

| Call | Result |
|---|---|
| `lz.factorization("ab", alphabet=0)` | `(2, [0, 1, 2])` — clamped to 2 |
| `lz.factorization("ab", log_base=0)` / `log_base=1` | clamped to 2 |
| `lz.factorization("ab", partitions=-5)` | ignored |
| `lz.emc("abab", max_block_size=0)` / `-100` | `(0.0, [0.0])` — auto-selected |
| `lz.h("")`, `lz.lz76("")` | `0.0`, and the full dict |

Nothing in the Python layer validates sequence length; every degenerate input returns a
value. LZ76 entropy estimates are not meaningful below a few hundred symbols, and the
shuffle-based EMC needs `n > 50` before the block size leaves single digits.

<div class="lz-tickrule"></div>

## Module attributes and packaging

| Name | Value | Notes |
|---|---|---|
| `lzcomplexity.__version__` | `'1.0.0'` | comes from the Rust crate version, not from `pyproject.toml`. **Not in `__all__`**, so `from lzcomplexity import *` does not bind it |
| `lzcomplexity.__all__` | `['lz76', 'factorization', 'h', 'emc', 'nid']` | a star-import binds exactly these five |
| `lzcomplexity.__doc__` | the package docstring | what `help(lzcomplexity)` shows |

The package is typed. `python/lzcomplexity/py.typed` is a git-tracked, zero-byte PEP 561
marker, and `__init__.pyi` carries the full signatures, so mypy and pyright resolve the API
without loading the compiled extension.

!!! note "`lz.lzcomplexity` raises `AttributeError` — that is deliberate"

    The compiled extension is a submodule named `lzcomplexity.lzcomplexity`. The package
    `__init__.py` deletes the attribute after importing from it, so `dir(lzcomplexity)` shows
    only the five public names. The submodule stays in `sys.modules` and
    `import lzcomplexity.lzcomplexity` still works — but the attribute is never re-bound.

    ```pycon
    >>> hasattr(lz, "lzcomplexity")
    False
    >>> "lzcomplexity.lzcomplexity" in sys.modules
    True
    ```

    It is namespace hygiene, not encapsulation. Do not depend on the submodule; it is not
    part of the public API.

!!! warning "`inspect.signature` reports `max_block_size=Ellipsis`"

    Documentation generators render the default as `...` rather than `-1`, because the
    negative default cannot be expressed as a literal in the generated text signature. The
    runtime default really is `-1` — `lz.emc(SEQ) == lz.emc(SEQ, max_block_size=-1)` is
    `True`. Prefer the `.pyi` stub or a hand-written signature when generating docs.

    ```pycon
    >>> lz.emc.__text_signature__
    '(seq, partitions=1, alphabet=None, log_base=None, max_block_size=..., jobs=0)'
    ```

Wheels are tagged `cp39-abi3-<platform>`: one wheel per platform covers every Python from 3.9
up. The module built for this page loaded unmodified under CPython 3.14. See
[Releases](../project/releases.md).

## Threading and the GIL

!!! danger "A call blocks every other Python thread for its whole duration"

    The extension never releases the GIL — there is no `allow_threads` call anywhere in the
    workspace. All heavy work (suffix array, longest-previous-factor table, factorization,
    the shuffle loop) runs with the GIL held. Measured here: a background Python thread
    ticking at roughly 17 700 iterations per second managed **one single tick** during the
    3.2-second `lz76` call on a 1 500 000-symbol sequence — total starvation. `Ctrl-C` is not
    serviced until the call returns, and a GUI or asyncio loop sharing the interpreter will
    freeze.

Practical consequences:

- `concurrent.futures.ThreadPoolExecutor` gives **no** speedup when batching sequences, and
  sometimes costs. Measured here: four `lz76` calls on 500 000-symbol sequences took 2.14 s
  across four threads against 2.18 s run back to back — four threads bought nothing. Under
  load the threaded version can run slower still, because the rayon pools of the concurrent
  calls oversubscribe the cores while the GIL serialises the Python-visible work.
- For Python-level parallelism use `multiprocessing` or `ProcessPoolExecutor`, and set
  `RAYON_NUM_THREADS=1` in the workers so the two levels of parallelism do not fight.
- Within a single call, parallelism already happens: the EMC shuffles fan out across the
  rayon pool, `nid` runs its four factorizations concurrently, and `extras` runs its two
  half-sequence factorizations concurrently. Control it with `RAYON_NUM_THREADS`; results
  are bit-identical across thread counts.
- Because the GIL is held throughout and no mutable state is shared, the functions are
  trivially thread-*safe*. They are not thread-*parallel*.

See [Performance](../project/performance.md) and
[Batch and distance workflows](../recipes/batch-distance.md).

<div class="lz-tickrule"></div>

## Migrating from the pre-1.0 API

| Pre-1.0 | 1.0.0 | What changed |
|---|---|---|
| `lz.entropy_density(seq, …)` | `lz.h(seq, …)` | renamed — the function *and* the `lz76(...)` dict key |
| `lz.metrics.nid(a, b, …)` (v0.10.1)<br>`lz.metrics.information_distance(a, b, …)` (v0.10.2) | `lz.nid(a, b, …)` | hoisted to the top level. Which old name you used depends on which release you came from |
| `lz.factors(seq, …)` | `lz.factorization(seq, …)` | folded in — `factorization` now returns the boundary list too |
| `lz.factorization(seq, …) -> int` | `-> tuple[int, list[int]]` | take `[0]` for the old value |
| `lz.lz76(seq, …) -> 4-tuple` | `-> dict` with 9 keys | EMC promoted to a nested dict; ε, `factors_stddev`, the two error estimates and `extras` are new |
| `lz.emc(seq, …) -> (max_block_size, value, multi_information)` | `-> (value, summands)` | different arity *and* different element meaning; the other two are now under `lz76(...)["emc"]` |
| `lz.metrics.rid(a, b, …)` | **removed** | reachable only through v0.10.1; already disabled in the last C++ release |
| `lz.metrics` submodule | **removed** | one metric did not justify a submodule |
| `lz.spectral.psd / entropy / semc` | **removed** | moved to a separate package |

### Defaults changed too

| Parameter | Pre-1.0 default | 1.0.0 default | Effect |
|---|---|---|---|
| `alphabet` | `2` | `None` (auto-detect, minimum 2) | non-binary data auto-detects instead of silently assuming binary |
| `log_base` | `2` | `None` (= detected alphabet size) | **`h` is normalized by default, not reported in bits** |
| `jobs` | `hardware_concurrency()` | `0` | now ignored |
| `partitions` | `1` | `1` | now ignored |
| `max_block_size` | `-1` | `-1` | unchanged |

!!! danger "The `log_base` default change silently rescales published numbers"

    An `h` value computed with 1.0.0 defaults is not comparable with one from the C++ era.
    Old `entropy_density(seq)` was always base 2; new `h(seq)` uses the detected alphabet
    size, so for DNA (k = 4) the value is `log(2)/log(4) = 0.5×` the old one. Pass
    `log_base=2` to reproduce the old numbers.

    Separately, **any EMC number published before 0.13.0 is not comparable with a 1.0.0
    number** — the estimator was replaced with a block-entropy formulation.

A drop-in shim for old call sites:

```python
import lzcomplexity as lz


def entropy_density(seq, **kw):        # was base-2 by default
    kw.setdefault("log_base", 2)
    return lz.h(seq, **kw)


def factorization(seq, **kw):          # was -> int
    return lz.factorization(seq, **kw)[0]


factors = lz.factorization             # was -> (int, list[int])


def emc(seq, **kw):                    # was -> (max_block_size, value, multi_information)
    d = lz.lz76(seq, **kw)["emc"]
    return (d["max_block_size"], d["value"], d["multi_information"])


nid = lz.nid
information_distance = lz.nid          # for code written against 0.10.2 / 0.11.0
```

`lz.metrics.rid` and `lz.spectral.*` have no replacement in this package.

## Where to go next

<div class="lz-cards" markdown>
<div class="lz-card" markdown>
### [What the inputs accept](inputs.md)
The coercion order, the `bytearray` trap, UTF-8 bytes versus code points, and why
`[0, 1, 10]` is not injective.
<p class="lz-card__api"><code>lz.factorization(seq)</code></p>
</div>
<div class="lz-card" markdown>
### [Reading the numbers](../guide/reading-the-numbers.md)
Which of these values to report, which to ignore, and what a given magnitude means.
<p class="lz-card__api"><code>lz.lz76(seq)</code></p>
</div>
<div class="lz-card" markdown>
### [The Rust API](rust.md)
The same five measures from the core crate, where `get_shuffle_terms` is a real option
instead of a hard-coded `true`.
<p class="lz-card__api"><code>lzcomplexity_core::lz76</code></p>
</div>
</div>
