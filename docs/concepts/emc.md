# Effective measure complexity

*Excess entropy, what `emc()` actually computes, and why the sum collapses to one term.*

Entropy rate answers "how surprising is the next symbol?" Effective measure complexity answers a
different question: **how much does the past tell you about the future?** A fair coin is maximally
surprising and tells you nothing. A constant string is unsurprising and also tells you nothing. Both
have zero excess entropy. Everything interesting lives in between.

This page defines the quantity, then describes the estimator in `lzcomplexity` in full — including
the fact that its scalar output is determined by a single block size. That is not visible from the
formula, and it changes how you should read the number.

---

## The definition

Grassberger (1986) defined effective measure complexity as the total of the entropy-rate
over-estimates you make when you only ever look at finite blocks.

<div class="lz-formula">
  <p class="lz-math"><i>E</i> = Σ<sub><i>L</i>≥1</sub> [ <i>h</i>(<i>L</i>) − <i>h</i> ] = lim<sub><i>L</i>→∞</sub> [ <i>H</i>(<i>L</i>) − <i>L</i>·<i>h</i> ]</p>
  <dl class="lz-formula__key">
    <dt><i>H</i>(<i>L</i>)</dt><dd>Shannon block entropy of length-<i>L</i> words, with <i>H</i>(0) ≡ 0</dd>
    <dt><i>h</i>(<i>L</i>)</dt><dd><i>H</i>(<i>L</i>) − <i>H</i>(<i>L</i>−1), the apparent entropy rate at scale <i>L</i></dd>
    <dt><i>h</i></dt><dd>lim <i>h</i>(<i>L</i>), the true entropy rate</dd>
  </dl>
  <p class="lz-formula__cite">Grassberger, Int. J. Theor. Phys. 25:907 (1986), Eq. 26. The equality of the two forms is Props. 6 and 7 of Crutchfield &amp; Feldman, Chaos 13:25 (2003).</p>
</div>

Read the first form as **apparent memory**. Observed through a length-`L` window, a process looks
more random than it is, by exactly `h(L) − h` bits per symbol. Sum those over-estimates across all
window sizes and you get `E`.

Read the second form as the **subextensive part of the block entropy**. `H(L)` grows like
`E + L·h`: the slope is the entropy rate, the intercept is the excess entropy.

Crutchfield & Feldman's Prop. 8 supplies the third and most quotable reading — `E` is the mutual
information between the infinite past and the infinite future.

<div class="lz-formula">
  <p class="lz-math"><i>E</i> = lim<sub><i>L</i>→∞</sub> <i>I</i>[ <i>S</i><sub>−<i>L</i></sub>…<i>S</i><sub>−1</sub> ; <i>S</i><sub>0</sub>…<i>S</i><sub><i>L</i>−1</sub> ]</p>
  <p class="lz-formula__cite">Crutchfield &amp; Feldman (2003), Eq. 53. The units of E are bits (their Eq. 48); E ≥ 0 always (their Eq. 52).</p>
</div>

!!! note "Four communities, four names, one quantity"

    You will meet this number under names that look unrelated. They are the same object, and the
    equivalence is a theorem chain (Crutchfield & Feldman 2003, Props. 6–8), not a convention.

    | Name | Community | Primary reference |
    |---|---|---|
    | **excess entropy** `E` | computational mechanics | Crutchfield & Packard, *Physica D* 7:201 (1983) — coinage |
    | **effective measure complexity** | statistical physics | Grassberger, *Int. J. Theor. Phys.* 25:907 (1986) |
    | **predictive information** | neuroscience, machine learning | Bialek, Nemenman & Tishby, *Neural Comp.* 13:2409 (2001) |
    | **stored information** | nonlinear dynamics | Shaw, *The Dripping Faucet as a Model Chaotic System* (1984) |

    Grassberger's paper also defines a *second* quantity, **true** measure complexity, which is the
    entropy of the minimal predictive model — the statistical complexity. It is not this. Full
    citations on [References](../project/references.md).

---

## The one hump

This is the point of the measure, and it is what distinguishes it from everything else the library
computes.

**Entropy rate is monotone in randomness.** `h` climbs from 0 for a constant sequence to `log k` for
a uniform i.i.d. one, and every source sits somewhere on that ladder.

**Excess entropy is not.** It vanishes at *both* ends — the perfectly ordered source has nothing to
predict, the perfectly random one has nothing predictable — and it is large in between, where a
process carries structure the past can be used to forecast.

Measured, `n = 20 000`, binary alphabet, default settings, `seed=1` for the two random families:

<div class="lz-scroll lz-compare" markdown>

| source | `h` | `emc` value | `multi_information` |
|---|---:|---:|---:|
| constant `"0"×20000` | 0.0007 | **−0.0000** | 0.0000 |
| period-8, motif `00010111` | 0.0029 | 5.3557 | 0.9994 |
| Thue–Morse | 0.0186 | **9.7664** | 0.9937 |
| Markov, P(stay) = 0.99 | 0.0807 | 2.0103 | 0.8880 |
| Markov, P(stay) = 0.95 | 0.2900 | 1.9653 | 0.7130 |
| Markov, P(stay) = 0.90 | 0.4772 | 1.2602 | 0.5351 |
| Markov, P(stay) = 0.80 | 0.7401 | 0.6301 | 0.2743 |
| Markov, P(stay) = 0.70 | 0.8966 | 0.2850 | 0.1214 |
| i.i.d. p = 0.5 | 1.0294 | **−0.1950** | −0.0014 |

</div>

`h` rises monotonically down the table. `emc` does not: near zero at the top, near zero at the
bottom, peaking at Thue–Morse — deterministic, aperiodic, hierarchically self-similar, the canonical
"structured but not periodic" sequence.

The two extremes hold at larger `n` too. At `n = 200 000`, `"0"×200000` returns
`-2.168404344971009e-19`, and `iid(200_000, 0.5, seed=1)` returns `-0.04860260770944613`.
`"01"×100000` returns exactly `0.0` — but read that one with care: at `n = 200 000` the default
`mm` is 24, the period 2 divides it, and the estimator degenerates for the separate reason set out
in rule 2 below.

??? note "Generators used for this table"

    ```python
    import random
    import lzcomplexity as lz

    def markov(n, q, seed):
        rng, s, out = random.Random(seed), "0", []
        for _ in range(n):
            out.append(s)
            if rng.random() >= q:
                s = "1" if s == "0" else "0"
        return "".join(out)

    def iid(n, p, seed):
        rng = random.Random(seed)
        return "".join("1" if rng.random() < p else "0" for _ in range(n))

    def thue_morse(n):
        return "".join(str(bin(i).count("1") % 2) for i in range(n))

    r = lz.lz76(markov(20_000, 0.95, seed=1))
    print(r["h"], r["emc"]["value"], r["emc"]["multi_information"])
    # 0.2900405613048538 1.965274837807028 0.7129568477395176
    ```

!!! danger "The complexity–entropy plane is filled, not a curve"

    Do not read the hump as a function `E(h)` — two sources with identical entropy rate can have
    wildly different excess entropy. Every periodic process has `h = 0`, yet a period-`p` process has
    `E = log₂ p`, unbounded over the family (Crutchfield & Feldman, Prop. 10). The `h = 0` axis
    therefore carries every value of `E` from 0 to ∞. Feldman, McTague & Crutchfield (2008)
    explicitly decline to assume any relationship between the two. Report `(h, emc)` as a *pair* —
    that pair is the complexity–entropy diagram the literature actually plots — and never infer one
    coordinate from the other.

---

## What the library computes

The block entropies `H(L)` cannot be plugged in directly: there are `k^L` words of length `L`, and
past `L ≈ log_k n` you have no counts for them. `lzcomplexity` substitutes a surrogate-data
construction — a ladder of block shuffles.

For each block size `l = 1 … mm`:

1. Copy the sequence and shuffle it in blocks of length `l`: repeatedly transpose two aligned,
   non-overlapping length-`l` blocks, `⌊n/2⌋` times. This destroys correlations longer than `l` and
   preserves everything inside a block.
2. LZ76-factorize the surrogate, giving `C(u^RS(l))`.
3. The surrogate is (idealised) an i.i.d. stream of `l`-blocks, whose entropy rate is `H(l)/l`.
   Multiply back to recover the block entropy: `H_l = l · C(u^RS(l)) · g`.
4. Take the discrete derivative and subtract the entropy density.

<div class="lz-formula">
  <p class="lz-math"><i>g</i> = log<sub><i>k</i></sub>(<i>n</i>) ⁄ <i>n</i> &nbsp;·&nbsp; <i>ĥ</i> = <i>C</i>(<i>u</i>)·<i>g</i> &nbsp;·&nbsp; <i>H</i><sub><i>l</i></sub> = <i>l</i>·<i>C</i>(<i>u</i><sup>RS(<i>l</i>)</sup>)·<i>g</i> &nbsp;·&nbsp; <i>H</i><sub>0</sub> = 0</p>
  <p class="lz-math">term<sub><i>l</i></sub> = ( <i>H</i><sub><i>l</i></sub> − <i>H</i><sub><i>l</i>−1</sub> ) − <i>ĥ</i> &nbsp;&nbsp;&nbsp;&nbsp; <i>Ê</i> = Σ<sub><i>l</i>=1</sub><sup><i>mm</i></sup> term<sub><i>l</i></sub></p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count — the library's <code>complexity</code>, which counts complete components only</dd>
    <dt><i>u</i><sup>RS(<i>l</i>)</sup></dt><dd>the block-shuffled surrogate at block size <i>l</i></dd>
    <dt><i>k</i></dt><dd>log base — the sequence's alphabet size unless you pass <code>log_base</code></dd>
    <dt><i>mm</i></dt><dd>the largest block size, returned as <code>max_block_size</code></dd>
  </dl>
  <p class="lz-formula__cite">ĥ is exactly the library's entropy density: the same number lz76(...)["h"] returns.</p>
</div>

So `ĥ` stands in for `h`, `H_l − H_{l−1}` for `h(L)`, and `term_l` for the summand `h(L) − h`.
Structurally the code is a faithful transcription of Grassberger's sum. Every approximation is in
how `H_l` is obtained; see [Convergence](convergence.md).

`C(·)` throughout is the library's own factor count. It counts **complete** LZ76 components only, so
it is one less than the textbook exhaustive-history count whenever the sequence ends mid-component —
see [LZ76 factorization](lz76.md). The same convention applies to the original and to every
surrogate. Since `Ê` is `mm·g` times a *difference* of two such counts, the convention shifts the
result by at most `mm·g` in either direction, which at `n = 20 000` is 0.015.

### Choosing `mm`

With `max_block_size=-1` (the default) the library solves `n = M·2^M` by fixed-point iteration, then
**adds 10** whenever `n > 50`. The `n = M·2^M` rule is the standard undersampling bound: seeing all
`2^M` binary words of length `M` takes on the order of `M·2^M` samples, so `M` is the largest block
length at which a length-`n` sample is not hopeless.

| `n` | `M` from `n = M·2^M` | default `mm` |
|---:|---:|---:|
| 50 | 4 | 4 |
| 51 | 4 | **14** |
| 100 | 4 | 14 |
| 1 000 | 7 | 17 |
| 10 000 | 10 | 20 |
| 20 000 | 11 | 21 |
| 100 000 | 13 | 23 |

The `M` rule is citable (Melchert & Hartmann, *Phys. Rev. E* 91:023306, 2015). The `+10` is not: it
carries no stated justification and it deliberately pushes `mm` past the undersampling bound. Note
also the 3.5× discontinuity between `n = 50` and `n = 51`.

The logarithm in the `M` rule is hardcoded to base 2 regardless of alphabet, so `mm` comes out the
same for a binary and a 27-letter sequence of equal length. For `k = 2` the rationale transfers; for
larger alphabets it does not, and the docs will not pretend otherwise.

### `multi_information`

The `l = 1` term gets its own field. It is `H_1 − ĥ`, the entropy-rate increase caused by destroying
*all* correlations, which is the multi-information (total correlation) rate of the source. The name
is correct.

It is also the best-calibrated number the estimator produces — but only for short-memory processes.
For an order-1 Markov chain, Crutchfield & Feldman's Prop. 11 gives `E = H(1) − h` **exactly**, so
the `l = 1` term *is* the whole excess entropy:

| source | analytic `E` | `multi_information` | `emc` value |
|---|---:|---:|---:|
| Markov, P(stay) = 0.95, `n = 20 000` | 0.713603 | **0.712957** | 1.965275 |

For periodic, hierarchical or higher-order sources it saturates near `H(X₁) ≤ log k` and badly
under-states `E`. In the one-hump table it reads ≈0.99 for both the period-8 and the Thue–Morse
sequence, whose true excess entropies are 3 bits and divergent respectively.

---

<div class="lz-tickrule"></div>

## The sum telescopes

Everything above describes `mm` scales contributing to a total. They do not. The sum is telescoping
and collapses exactly:

<div class="lz-formula">
  <p class="lz-math">Σ<sub><i>l</i>=1</sub><sup><i>mm</i></sup> [ ( <i>H</i><sub><i>l</i></sub> − <i>H</i><sub><i>l</i>−1</sub> ) − <i>ĥ</i> ] = <i>H</i><sub><i>mm</i></sub> − <i>mm</i>·<i>ĥ</i></p>
  <p class="lz-math"><i>Ê</i> = <i>mm</i> · <i>g</i> · ( <i>C</i>(<i>u</i><sup>RS(<i>mm</i>)</sup>) − <i>C</i>(<i>u</i>) )</p>
  <p class="lz-formula__cite">Only the largest block size survives. Block sizes 1 … mm−1 contribute nothing to the returned total.</p>
</div>

This is not an implementation bug. It is the textbook identity `Σ[h(L) − h] = lim[H(L) − L·h]`:
*any* estimator written in the `Σ (ΔH_l − ĥ)` form collapses to `H_mm − mm·ĥ`. What matters is the
consequence, and the consequence is invisible from the API.

**`emc` is a two-point contrast.** It compares the LZ76 factor count of the original sequence against
the factor count of *one* surrogate, shuffled at *one* block size, and multiplies the difference by
`mm`. The other `mm − 1` shuffles and factorizations are computed in full, then algebraically
cancelled out of the returned scalar. They survive only in `summands`.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">1</span><span class="lz-stat__k">block size sets the value</span></div>
<div class="lz-stat"><span class="lz-stat__v">1</span><span class="lz-stat__k">surrogate draw behind it</span></div>
<div class="lz-stat"><span class="lz-stat__v">mm−1</span><span class="lz-stat__k">factorizations cancelled</span></div>
<div class="lz-stat"><span class="lz-stat__v">1e−13</span><span class="lz-stat__k">worst residue measured</span></div>
</div>

### Verification

The closed form can be inverted. Solve `C(u^RS(mm)) = Ê/(mm·g) + C(u)` and the answer must be an
integer, because it is a factor count. It is, to floating point, on every input tried — the random
rows use the generators above with `seed=1`:

<div class="lz-scroll lz-compare" markdown>

| sequence | `n` | `k` | `mm` | `C(u)` | recovered `C(u^RS(mm))` | residue |
|---|---:|---:|---:|---:|---:|---:|
| `"01"×512` | 1 024 | 2 | 17 | 2 | 15 | 0 |
| i.i.d. binary | 1 024 | 2 | 17 | 110 | 109 | 0 |
| Thue–Morse | 1 024 | 2 | 17 | 18 | 67 | 0 |
| Markov P(stay)=0.95 | 1 024 | 2 | 17 | 31 | 47 | 0 |
| period-8 `00010111` | 20 000 | 2 | 21 | 4 | 361 | 1.7e−13 |
| Markov P(stay)=0.95 | 20 000 | 2 | 21 | 406 | 537 | 1.1e−13 |
| i.i.d. binary | 20 000 | 2 | 21 | 1441 | 1428 | 0 |
| `"ABRACADABRA"×10` | 110 | 5 | 14 | 6 | 12 | 1.8e−15 |

</div>

The residues are float-summation rounding, up to 1.7e−13 on these inputs. They vanish exactly when
`g = log_k(n)/n` is a dyadic rational (`k = 2`, `n` a power of two), and often — not always — when
it is not: three of the four non-dyadic rows above still land on 0.

One corollary of the closed form is worth keeping: `C(u^RS(mm))` and `C(u)` are both integers, so
**`Ê` is always an integer multiple of `mm·g`**. At `n = 20 000` that step is 0.0150, and every
`n = 20 000` value on this page is a multiple of it. There is no resolution below one factor.

The `mm` dependence shows up directly in a sweep. `mm` is not a "cut off a convergent tail"
parameter — it is a multiplier on the answer:

```python
>>> s = markov(20_000, 0.95, seed=1)     # analytic E = 0.713603 bits
>>> for mm in (1, 2, 4, 8, 16, 21, 32, 64):
...     print(mm, round(lz.emc(s, max_block_size=mm)[0], 4))
1 0.713
2 0.8844
4 1.0344
8 1.263
16 1.6574
21 1.9653
32 1.5316
64 1.8745
```

`mm = 21` is the value auto-selected at `n = 20 000`.

At `mm = 1` the sum is the `l = 1` term alone, which for an order-1 source is the whole of `E` —
0.713 against an analytic 0.7136. That is a property of this source, not of `mm = 1`. From there the
value climbs steeply, is already non-monotone by `mm = 8` (1.263, below the 1.285 at `mm = 7`), and
above about 20 wanders inside a 1.4–2.1 band with no further trend. The mechanism is the closed form:
`Ê = mm·g·(C(u^RS(mm)) − C(u))` multiplies the difference of two finite-`n` factor counts — one
surrogate draw, and a residual LZ bias that does not cancel — by `mm`. Larger `mm` amplifies both.
`multi_information` is `0.7130` at every setting, because it depends on `l = 1` only.

!!! danger "Do not report `emc` as a bit count"

    A value of 1.97 does not mean the sequence stores 1.97 bits of predictive information. The
    estimator does not recover `E = log₂ p` for periodic sources, it overshoots order-1 Markov
    sources by roughly 2–3× (measured 2.2× to 2.8× for P(stay) from 0.99 down to 0.70 at
    `n = 20 000`), and it goes negative — which the true excess entropy, being a mutual
    information, never can. Treat `emc` as an **ordinal index at fixed `n`**. "A has more
    multi-scale structure than B" is supportable; "A stores 1.97 bits" is not.

---

## Reading the output

### The summands vector strictly dominates the scalar

`emc()` returns `(value, summands)`, where `summands[l-1]` is the term for block size `l`. The scalar
is the sum of the vector, to floating-point rounding — recomputing `sum(summands)` in Python can
differ in the last ulp or two (1.3e−15 on the Markov example below). The vector answers a question
the scalar cannot: **at what scale does the structure live?**

!!! example "Two summand profiles, `n = 20 000`, `mm = 21`"

    An order-1 Markov chain, P(stay) = 0.95. The `l = 1` term is 2.6× the largest of the others and
    is the only one near the analytic `E = 0.7136` — the scale-1 structure is visible. The
    other 20 terms are individually small but alternate in sign and still add to 1.25, more than
    `l = 1` itself; that residue is the estimator, not the process:

    ```python
    >>> r = lz.lz76(markov(20_000, 0.95, seed=1))
    >>> r["complexity"], r["h"]
    (406, 0.2900405613048538)
    >>> r["emc"]["value"], r["emc"]["multi_information"], r["emc"]["max_block_size"]
    (1.965274837807028, 0.7129568477395176, 21)
    >>> [round(x, 4) for x in r["emc"]["summands"]]
    [0.713, 0.1715, 0.0864, 0.0636, 0.1086, 0.0657, 0.0764, -0.0221, 0.1836,
     -0.025, 0.04, -0.0214, 0.1665, -0.0664, 0.1314, -0.0143, -0.0421, 0.275,
     -0.275, 0.1136, 0.2365]
    ```

    An i.i.d. binary sequence. Nothing dominant, everything inside ±0.1 — correctly identifying "no
    structure":

    ```python
    >>> r = lz.lz76(iid(20_000, 0.5, seed=1))
    >>> r["complexity"], r["h"], r["emc"]["value"]
    (1441, 1.0294296769465379, -0.19502727398084962)
    >>> [round(x, 4) for x in r["emc"]["summands"]]
    [-0.0014, -0.0014, -0.0143, -0.02, 0.03, -0.0014, -0.0214, 0.03, -0.0129,
     -0.03, -0.0357, -0.0071, 0.0021, 0.0436, -0.0029, -0.0371, 0.0193,
     -0.0936, -0.0086, 0.02, -0.0522]
    ```

How to read a profile — rules of thumb, not tests:

| Shape of `summands` | Interpretation |
|---|---|
| weight concentrated at `l = 1` | short memory, order-1-like |
| weight decaying over the first few `l` | finite-order Markov, order ≈ where it dies |
| weight persisting to large `l` | long-range or hierarchical correlation |
| flat, everything near zero | no detectable structure |
| violent sign alternation, terms of order 1 or more | strong periodicity commensurate with the block grid; the estimator is confounded, treat the whole result as unreliable |

Theory says every summand should be non-negative (Crutchfield & Feldman, Lemma 1) and the partial
sums should climb monotonically to `E`. Neither holds here. Negative terms are normal, and the
alternating pairs are the telescoping made visible.

!!! tip "The summands cost nothing extra"

    All `mm` surrogate factorizations run whether or not you read the vector, and the Python API
    always requests it — `len(summands) == max_block_size` on every call. Reading only `emc()[0]`
    discards `mm − 1` factorizations' worth of information for zero saving in time or memory.

### The CLI reports the same numbers

<div class="lz-run" markdown>

```console
$ lzcomplexity mk95.txt -e a:f -o mk95.json
$ jq '.sequences[0].lz76RandomShuffleComplexity | {value, max_block_size, multi_information}' mk95.json
{
  "value": 1.965274837807028,
  "max_block_size": 21,
  "multi_information": 0.7129568477395176
}
```

</div>

`-e a:f` means "auto block size, and emit the summand terms". Full option grammar on
[`lzcomplexity`](../cli/lzcomplexity.md).

---

## Four rules

### 1. Never compare `emc` across different sequence lengths

!!! warning "The same process reports 16% lower `emc` at 100× the length"

    `mm` grows with `n`, and `mm` multiplies the closed form directly, so changing the length alone
    moves the answer. A period-8 sequence has a constant true `E = log₂ 8 = 3` bits at every length.
    Measured:

    | `n` | 1 000 | 5 000 | 20 000 | 100 000 |
    |---|---:|---:|---:|---:|
    | `mm` | 17 | 19 | 21 | 23 |
    | `emc` | 5.9296 | 5.8367 | 5.3557 | 4.9701 |
    | `multi_information` | 1.0265 | 0.9928 | 0.9994 | 0.9931 |

    Truncate or resample every sequence in a study to a common length. If you cannot, pin
    `max_block_size` to one fixed value across the whole study — that removes the `mm` dependence but
    not the `O(1/log n)` LZ bias, which enters `Ê` twice with imperfect cancellation.

    Short sequences are worse than merely noisy. `emc` is identically `0.0` for `n ≤ 10`, because
    the block shuffle returns before swapping anything, and the `mm` jump from 4 to 14 across
    `n = 50 → 51` means sequences either side of that boundary are not on the same scale at all.

    The same caution applies to the log base: `Ê ∝ 1/ln(log_base)`, so binary and 4-ary results are
    not on a common scale unless you pass `log_base=2` to both. See [Alphabets](alphabets.md).

### 2. A printed `0.0000` deserves suspicion, not celebration

!!! warning "A perfectly periodic sequence can report exactly zero EMC"

    The shuffle only permutes **phase-0 aligned** blocks. When the sequence period `p` divides the
    block size, every aligned block is identical, swapping them is the identity, and
    `C(u^RS(mm)) = C(u)`. Combined with the telescoping: `p | mm` ⇒ `Ê = 0`.

    Measured at `n = 20 000`, where `mm = 21`, one motif per period:

    | period `p` | motif | `p` divides 21 | `emc` |
    |---:|---|---|---:|
    | 2 | `01` | no | 1.7102 |
    | 3 | `001` | **yes** | **−1.3e−15** |
    | 4 | `0011` | no | 3.5255 |
    | 5 | `00101` | no | 4.1106 |
    | 7 | `0010111` | **yes** | **−2.7e−15** |
    | 8 | `00010111` | no | 5.3557 |
    | 11 | `00010110011` | no | 6.1509 |
    | 16 | `0001001101011110` | no | 7.2610 |
    | 21 | `000100110101111000101` | **yes** | **−8.9e−16** |
    | 32 | `00010011010111100010101110110011` | no | 9.4213 |

    Exactly the divisors of `mm` collapse. The value for a non-divisor period depends on the motif,
    not only on `p` — these are one motif each, not a law. Before believing a near-zero result, check
    your sequence's period against `max_block_size`, and check `n > 10`. Test with a tolerance
    (`abs(v) < 1e-12`), never `== 0.0` — the residue is a float, and only sometimes a hard zero.

### 3. Establish a null before believing a small value

`emc` ships no error bar. The `normal_error` and `poison_error` fields in the `lz76()` output are
uncertainties on the *entropy density*, not on `emc`. Because the total rests on a single surrogate
draw at a single block size, its Monte-Carlo noise is the noise of that one draw.

Measured over 20 independent i.i.d. binary sequences at `n = 20 000` — `iid(20_000, 0.5, seed)` for
`seed` in `range(20)`:

```text
mean = 0.0075   sd = 0.1230   min = -0.2700   max = 0.2100   negative in 9/20 runs
```

At that length, `|emc| ≲ 0.25` (two standard deviations) is indistinguishable from "no structure".
The floor scales with `n` and `mm`, so establish it for your own data: shuffle, re-run 20–30 times,
take the standard deviation.

A negative value means "no detectable structure", not "negative information".

### 4. Always report `h` alongside `emc`

Neither coordinate is interpretable alone — that is the entire content of the one-hump section. The
pair `(h, emc)` is the complexity–entropy diagram, and it is the object the literature plots. See
[Entropy density](entropy-density.md) and
[Reading the numbers](../guide/reading-the-numbers.md).

---

## Where it works anyway

None of the above makes the measure useless. Across a family of equal-length sequences generated by
one parametric process, `emc` traces the behaviour theory predicts: low in the ordered regime, low in
the fully chaotic regime, elevated in between. The logistic-map sweep in
[Dynamical systems](../recipes/dynamical-systems.md) works this through end to end — 121 values of
the control parameter, one fixed `n`, `h` and `emc` side by side. That is the shape of analysis this
estimator supports well.

For what it does with recorded rather than generated data, see [Neuroscience](../recipes/neuro.md)
and [Genomics](../recipes/genomics.md).

## Field reference

`emc()` and the `"emc"` block of `lz76()` carry their full signatures on the
[Python API](../api/python.md) page. The fields:

| Field | Type | Meaning |
|---|---|---|
| `emc()[0]`, `lz76()["emc"]["value"]` | `float` | `Ê` — the telescoped total |
| `emc()[1]`, `lz76()["emc"]["summands"]` | `list[float]`, length `mm` | `summands[l-1]` is the term for block size `l` |
| `lz76()["emc"]["max_block_size"]` | `int` | the resolved `mm` |
| `lz76()["emc"]["multi_information"]` | `float` | `summands[0]`, the `l = 1` term |

The shuffle is seeded from an FNV-1a hash of the sequence bytes, so the same input yields the same
`emc` on any machine at any thread count. That is reproducibility of one surrogate realization, not
an ensemble estimate — see [Determinism](../project/determinism.md).
