# Effective measure complexity

*Excess entropy, what `emc()` actually computes, and why the per-scale ladder is projected before
the total is read off it.*

Entropy rate answers "how surprising is the next symbol?" Effective measure complexity answers a
different question: **how much does the past tell you about the future?** A fair coin is maximally
surprising and tells you nothing. A constant string is unsurprising and also tells you nothing. Both
have zero excess entropy. Everything interesting lives in between.

This page defines the quantity, then describes the estimator in `lzcomplexity` in full — including
the shape constraint it imposes on its own intermediate results, what that fixes, and the two biases
it does not fix.

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

### Two properties the estimator is held to

Everything the library does with its intermediate results follows from one lemma. Write the partial
sum of the definition as the **finite-scale excess entropy**:

<div class="lz-formula">
  <p class="lz-math"><i>E</i>(<i>L</i>) = Σ<sub><i>j</i>=1</sub><sup><i>L</i></sup> [ <i>h</i>(<i>j</i>) − <i>h</i> ] = <i>H</i>(<i>L</i>) − <i>L</i>·<i>h</i></p>
  <p class="lz-formula__cite">The two forms are the same identity as above, stopped at a finite L rather than taken to the limit.</p>
</div>

For a stationary source `h(j)` is non-increasing and bounded below by `h`, so **every summand
`h(j) − h` is non-negative** (Crutchfield & Feldman, Lemma 1). Two consequences follow immediately,
and they are the only two facts the estimator needs:

1. `E(L) ≥ 0` for every `L`.
2. `E(L)` is **non-decreasing** in `L`, climbing to `E`.

A ladder of estimates `Ê(1), Ê(2), …` that violates either one is telling you about its own noise,
not about the source.

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
| constant `"0"×20000` | 0.0007 | **0.0000** | 0.0000 |
| period-8, motif `00010111` | 0.0029 | 5.3557 | 0.9994 |
| Thue–Morse | 0.0186 | **9.7664** | 0.9937 |
| Markov, P(stay) = 0.99 | 0.0807 | 2.0103 | 0.8880 |
| Markov, P(stay) = 0.95 | 0.2900 | 1.9653 | 0.7130 |
| Markov, P(stay) = 0.90 | 0.4772 | 1.2602 | 0.5351 |
| Markov, P(stay) = 0.80 | 0.7401 | 0.6744 | 0.2743 |
| Markov, P(stay) = 0.70 | 0.8966 | 0.3178 | 0.1214 |
| i.i.d. p = 0.5 | 1.0294 | **0.0000** | −0.0014 |

</div>

`h` rises monotonically down the table. `emc` does not: zero at the top, zero at the bottom, peaking
at Thue–Morse — deterministic, aperiodic, hierarchically self-similar, the canonical "structured but
not periodic" sequence.

Both extremes are **exactly** `0.0`, and at `n = 200 000` they stay there: `"0"×200000` and
`iid(200_000, 0.5, seed=1)` both return `0.0`. `"01"×100000` returns `0.746252539205446` — a periodic
sequence is not a structureless one, and the estimator no longer confuses the two.

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
    # 0.2900405613048538 1.9652748378070268 0.7129568477395176
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
4. Subtract the extensive part to leave the **rung** `Ê(l)`, an estimate of `E(l)`.

<div class="lz-formula">
  <p class="lz-math"><i>g</i> = log<sub><i>k</i></sub>(<i>n</i>) ⁄ <i>n</i> &nbsp;·&nbsp; <i>ĥ</i> = <i>C</i>(<i>u</i>)·<i>g</i> &nbsp;·&nbsp; <i>H</i><sub><i>l</i></sub> = <i>l</i>·<i>C</i>(<i>u</i><sup>RS(<i>l</i>)</sup>)·<i>g</i></p>
  <p class="lz-math"><i>Ê</i>(<i>l</i>) = <i>H</i><sub><i>l</i></sub> − <i>l</i>·<i>ĥ</i> = <i>l</i> · <i>g</i> · ( <i>C</i>(<i>u</i><sup>RS(<i>l</i>)</sup>) − <i>C</i>(<i>u</i>) )</p>
  <dl class="lz-formula__key">
    <dt><i>C</i>(·)</dt><dd>LZ76 factor count — the library's <code>complexity</code>, which counts complete components only</dd>
    <dt><i>u</i><sup>RS(<i>l</i>)</sup></dt><dd>the block-shuffled surrogate at block size <i>l</i></dd>
    <dt><i>k</i></dt><dd>log base — the sequence's alphabet size unless you pass <code>log_base</code></dd>
    <dt><i>mm</i></dt><dd>the largest block size, returned as <code>max_block_size</code></dd>
  </dl>
  <p class="lz-formula__cite">ĥ is exactly the library's entropy density: the same number lz76(...)["h"] returns. Ê(l) is the finite-scale excess entropy E(l) = H(l) − L·h with both terms estimated from LZ76 counts.</p>
</div>

That gives `mm` independent estimates of one increasing curve. The last step turns them into a
number.

`C(·)` throughout is the library's own factor count. It counts **complete** LZ76 components only, so
it is one less than the textbook exhaustive-history count whenever the sequence ends mid-component —
see [LZ76 factorization](lz76.md). The same convention applies to the original and to every
surrogate, and since each rung is `l·g` times a *difference* of two such counts, the convention
shifts each rung by at most `l·g` in either direction — 0.015 at `n = 20 000` and `l = mm`.

---

<div class="lz-tickrule"></div>

## The ladder is projected before it is read

Each rung `Ê(l)` is a legitimate estimate of `E(l)` on its own. As a *sequence* they are badly
behaved, because each one rests on its own surrogate draw rather than on slices of one consistent
block-entropy curve: the ladder wanders up and down, and individual rungs fall below zero. The
[two properties](#two-properties-the-estimator-is-held-to) above say the true curve does neither.

So the library projects the ladder onto the set of sequences that are non-negative and
non-decreasing — the smallest correction, in least squares, that restores both properties — and
reads the answer off the projection.

<div class="lz-formula">
  <p class="lz-math"><i>Ê</i><sub>fit</sub> = argmin<sub><i>v</i></sub> Σ<sub><i>l</i></sub> ( <i>v</i><sub><i>l</i></sub> − <i>Ê</i>(<i>l</i>) )²&emsp;subject to&emsp;0 ≤ <i>v</i><sub>1</sub> ≤ <i>v</i><sub>2</sub> ≤ … ≤ <i>v</i><sub><i>mm</i></sub></p>
  <p class="lz-math">summands[<i>l</i>−1] = <i>Ê</i><sub>fit</sub>(<i>l</i>) − <i>Ê</i><sub>fit</sub>(<i>l</i>−1) &emsp;·&emsp; <i>Ê</i> = <i>Ê</i><sub>fit</sub>(<i>mm</i>) = Σ summands</p>
  <p class="lz-formula__cite">Isotonic regression, computed by pool-adjacent-violators (Ayer et al., Ann. Math. Statist. 26:641, 1955); the non-negativity projection is the pointwise positive part, which preserves monotonicity. Implemented as non_negative_isotonic in crates/lzcomplexity-core/src/shuffle.rs.</p>
</div>

Pool-adjacent-violators is one left-to-right pass: carry a stack of blocks, and whenever the newest
block averages below its predecessor, merge the two and re-check. Every rung enters the fit, and the
result has three properties the raw ladder lacks.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">≥ 0</span><span class="lz-stat__k">value, and every summand</span></div>
<div class="lz-stat"><span class="lz-stat__v">= Σ</span><span class="lz-stat__k">summands sum to the value</span></div>
<div class="lz-stat"><span class="lz-stat__v">mm</span><span class="lz-stat__k">block sizes reaching the total</span></div>
<div class="lz-stat"><span class="lz-stat__v">↑</span><span class="lz-stat__k">partial sums climb, as theory requires</span></div>
</div>

### Why the raw sum was not usable

Versions up to and including 1.0.2 summed the first differences of the ladder directly,
`Σ_l [(H_l − H_{l−1}) − ĥ]`. That is the definition written as a sum, and it is *correct
mathematics* — but it is also the identity `Σ_l [h(l) − h] = H(L) − L·h`, so the `H_l` telescope and
the whole sum collapses to its own last rung:

<div class="lz-formula">
  <p class="lz-math">Σ<sub><i>l</i>=1</sub><sup><i>mm</i></sup> [ ( <i>H</i><sub><i>l</i></sub> − <i>H</i><sub><i>l</i>−1</sub> ) − <i>ĥ</i> ] = <i>H</i><sub><i>mm</i></sub> − <i>mm</i>·<i>ĥ</i> = <i>Ê</i>(<i>mm</i>) = <i>mm</i> · <i>g</i> · ( <i>C</i>(<i>u</i><sup>RS(<i>mm</i>)</sup>) − <i>C</i>(<i>u</i>) )</p>
  <p class="lz-formula__cite">The telescoping is not a bug in the old code — it is the identity. The problem is what the identity leaves you holding: one rung.</p>
</div>

The estimator computed `mm` surrogates and `mm` factorizations, then let all but the largest cancel
algebraically. The scalar was a two-point contrast between one surrogate at one block size and the
original, multiplied by `mm`. Three things went wrong, and all three are consequences of that:

<div class="lz-scroll lz-compare" markdown>

| symptom | ≤ 1.0.2 | now |
|---|---|---|
| **Negative values.** Excess entropy is a mutual information; it cannot be negative. One unlucky surrogate could make `C(u^RS(mm)) < C(u)`. | i.i.d. binary at `n = 20 000` returns **−0.1950**; 9 of 20 draws negative | **0.0**; 0 of 20 negative, by construction |
| **Periodic collapse.** The shuffle only permutes phase-aligned blocks, so if the period `p` divides `mm` the shuffle is the identity and the top rung is exactly zero — dragging the whole total to zero. | period 3, 7 and 21 at `mm = 21` all return floating-point **zero** | **1.8193**, **4.1974**, **6.0262** — the scales that *did* see structure now carry it |
| **Single-draw variance.** The total inherited the Monte-Carlo noise of one surrogate, amplified by `mm`. | s.d. **0.1230** over the i.i.d. null at `n = 20 000` | s.d. **0.0702**, a 43 % reduction, at no extra cost |

</div>

The projection changes nothing when the ladder is already well behaved: a monotone non-negative
ladder is its own projection, so the value is still exactly `Ê(mm)`. That is why
`"01"×64` (`2.4609375`), `"0011"×256` (`4.150390625`), Thue–Morse (`9.7664`), the period-8 motif
(`5.3557`) and `markov(8192, 0.9, seed=6)` (`1.30126953125`) are unchanged to the last bit. The
values that moved are the ones that were reporting an artefact.

!!! warning "This is a breaking change, and it is not a bug fix in the surrogate"

    `emc` values from 1.0.2 and earlier are not comparable with current ones for any input whose
    ladder was not monotone. Nothing about the shuffle, the seeding or the factorization changed —
    only how the `mm` rungs are turned into one number. See [Releases](../project/releases.md).

### What the projection does not fix

Two biases live in the rungs themselves, not in how they are combined, and the projection cannot
touch either. Both were present before and remain.

**`emc` overshoots.** Against order-1 Markov sources, where Crutchfield & Feldman's Prop. 11 gives
the answer exactly as `E = H(1) − h`, the estimator lands 2.2–2.8× high. That is the finite-`n` LZ76
bias entering each rung twice, without cancelling, and then being multiplied by `l`.

<div class="lz-scroll lz-compare" markdown>

| P(stay) | analytic `E` | `emc` value | ratio | `multi_information` |
|---:|---:|---:|---:|---:|
| 0.99 | 0.919207 | 2.0103 | 2.19× | 0.8880 |
| 0.95 | 0.713603 | 1.9653 | 2.75× | **0.7130** |
| 0.90 | 0.531004 | 1.2602 | 2.37× | 0.5351 |
| 0.80 | 0.278072 | 0.6744 | 2.43× | 0.2743 |
| 0.70 | 0.118709 | 0.3178 | 2.68× | 0.1214 |

</div>

**`emc` scales with `mm`, and `mm` is a function of `n`.** The rungs keep climbing past the block
length at which a length-`n` sample can support a block-entropy estimate at all, so the top of the
ladder is set by undersampling rather than by the source. Sweeping `mm` by hand on the P(stay) = 0.95
sequence, whose analytic `E` is 0.7136:

```python
>>> s = markov(20_000, 0.95, seed=1)
>>> for mm in (1, 2, 4, 8, 16, 21, 32, 64):
...     print(mm, round(lz.emc(s, max_block_size=mm)[0], 4))
1 0.713
2 0.8844
4 1.0344
8 1.2741
16 1.6645
21 1.9653
32 1.791
64 1.8869
```

`mm = 21` is the value auto-selected at `n = 20 000`. At `mm = 1` the ladder is one rung and the
answer is the `l = 1` term alone, which for an order-1 source *is* the whole of `E` — 0.713 against
an analytic 0.7136. That is a property of this source, not of `mm = 1`.

!!! danger "Do not report `emc` as a bit count"

    A value of 1.97 does not mean the sequence stores 1.97 bits of predictive information. The
    estimator does not recover `E = log₂ p` for periodic sources and it overshoots order-1 Markov
    sources by 2–3×. Treat `emc` as an **ordinal index at fixed `n`**. "A has more multi-scale
    structure than B" is supportable; "A stores 1.97 bits" is not.

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
carries no stated justification and it deliberately pushes `mm` past the undersampling bound, which
is exactly where the rungs stop tracking `E(l)` and start tracking the sample size. Note also the
3.5× discontinuity between `n = 50` and `n = 51`.

The logarithm in the `M` rule is hardcoded to base 2 regardless of alphabet, so `mm` comes out the
same for a binary and a 27-letter sequence of equal length. For `k = 2` the rationale transfers; for
larger alphabets it does not, and the docs will not pretend otherwise.

### `multi_information`

The `l = 1` rung gets its own field. It is `H_1 − ĥ`, the entropy-rate increase caused by destroying
*all* correlations, which is the multi-information (total correlation) rate of the source. The name
is correct, and the field is reported **before** the projection — it is a separate diagnostic, not a
term of the sum, and its values are unchanged from 1.0.2 and earlier.

It is also the best-calibrated number the estimator produces — but only for short-memory processes.
For an order-1 Markov chain, Prop. 11 makes the `l = 1` term the whole excess entropy, and the
measured 0.712957 against an analytic 0.713603 is the closest this library gets to `E`. For periodic,
hierarchical or higher-order sources it saturates near `H(X₁) ≤ log k` and badly under-states `E`: it
reads ≈0.99 for both the period-8 and the Thue–Morse sequence, whose true excess entropies are 3 bits
and divergent respectively.

!!! note "`multi_information` is usually `summands[0]`, but not always"

    Both are the `l = 1` term; the field is raw and `summands[0]` is fitted. They agree whenever the
    first rung survives the projection unpooled, which is the common case — 0.7129568477395176 for
    the Markov example, in both places. They differ when rung 1 was clamped or pooled: for
    `SEQ = "01001010101101010101110101010101010000100101011"`, `multi_information` is
    `0.11818274152505626` while `summands[0]` is `0.05909137076252813`, because rungs 1 and 2 pooled.
    Quote `multi_information` when you want the scale-1 contrast, `summands[0]` when you want the
    first term of the sum.

---

## Reading the output

### The summands vector strictly dominates the scalar

`emc()` returns `(value, summands)`, where `summands[l-1]` is the fitted estimate of `h(l) − h` at
block size `l`. Every entry is non-negative and they sum to the value. The vector answers a question
the scalar cannot: **at what scale does the structure live?**

Read it two ways. The **increments** are the per-scale terms. Their **running sum** is `Ê_fit(l)`,
the excess entropy accumulated up to scale `l`, which is the more directly interpretable curve — it
climbs and flattens, and where it flattens is where the source runs out of structure.

!!! example "Three summand profiles, `n = 20 000`, `mm = 21`"

    An order-1 Markov chain, P(stay) = 0.95. Weight concentrated at `l = 1`, exactly as an order-1
    source should give, and the running sum has covered 65 % of its final value by `l = 7`:

    ```python
    >>> r = lz.lz76(markov(20_000, 0.95, seed=1))
    >>> r["complexity"], r["h"]
    (406, 0.2900405613048538)
    >>> r["emc"]["value"], r["emc"]["multi_information"], r["emc"]["max_block_size"]
    (1.9652748378070268, 0.7129568477395176, 21)
    >>> [round(x, 4) for x in r["emc"]["summands"]]
    [0.713, 0.1715, 0.0864, 0.0636, 0.1086, 0.0657, 0.0654, 0.0, 0.16, 0.0,
     0.0168, 0.0, 0.1225, 0.0, 0.0747, 0.0, 0.0, 0.0967, 0.0, 0.0, 0.2205]
    >>> import itertools
    >>> [round(x, 3) for x in itertools.accumulate(r["emc"]["summands"])]
    [0.713, 0.884, 0.971, 1.034, 1.143, 1.209, 1.274, 1.274, 1.434, 1.434,
     1.451, 1.451, 1.573, 1.573, 1.648, 1.648, 1.648, 1.745, 1.745, 1.745, 1.965]
    ```

    Thue–Morse — hierarchical, self-similar. Weight keeps arriving at every scale, and the running
    sum never flattens. That is the signature of long-range structure:

    ```python
    >>> r = lz.lz76(thue_morse(20_000))
    >>> r["emc"]["value"]
    9.766365797041026
    >>> [round(x, 4) for x in r["emc"]["summands"]]
    [0.9937, 0.1593, 0.8455, 0.0, 1.5749, 0.0, 0.0, 0.0, 1.8549, 0.0, 0.0, 0.0,
     0.4879, 0.0, 0.0, 0.0, 1.7013, 0.0, 0.0, 0.0, 2.1489]
    ```

    An i.i.d. binary sequence. The entire ladder sat below zero, so the projection flattened it to
    nothing — correctly identifying "no structure":

    ```python
    >>> r = lz.lz76(iid(20_000, 0.5, seed=1))
    >>> r["complexity"], r["h"], r["emc"]["value"]
    (1441, 1.0294296769465379, 0.0)
    >>> r["emc"]["summands"]
    [0.0, 0.0, 0.0, ...]      # all 21 entries
    ```

How to read a profile — rules of thumb, not tests:

| Shape of `summands` | Interpretation |
|---|---|
| weight concentrated at `l = 1` | short memory, order-1-like |
| weight decaying over the first few `l` | finite-order Markov, order ≈ where it dies |
| weight still arriving at large `l` | long-range or hierarchical correlation |
| all entries zero, or a single small entry | no detectable structure — check it against a null |

!!! note "A zero summand means two neighbouring scales were pooled, not that the scale was skipped"

    The projection replaces a run of violating rungs with their common mean, so the fitted curve is
    flat across that run and the increments inside it are exactly `0.0`. A zero therefore reads as
    "scale `l` added nothing beyond scale `l−1`", which is a statement about the estimate, not a gap
    in the computation. All `mm` surrogates were factorized either way. The length of a trailing run
    of zeros tells you how many top rungs were pooled — useful in the verification below.

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
  "value": 1.9652748378070268,
  "max_block_size": 21,
  "multi_information": 0.7129568477395176
}
```

</div>

`-e a:f` means "auto block size, and emit the summand terms". Full option grammar on
[`lzcomplexity`](../cli/lzcomplexity.md).

### Verification

Three invariants hold on every input, and all three are cheap to assert in a test:

```python
value, summands = lz.emc(seq)
assert all(s >= 0.0 for s in summands)                    # Lemma 1
assert abs(sum(summands) - value) < 1e-12                 # the sum is the value
partials = list(itertools.accumulate(summands))
assert all(a <= b + 1e-12 for a, b in zip(partials, partials[1:]))   # monotone
```

A fourth is more interesting, because it pins the arithmetic. Every rung is `l·g` times an integer
difference of factor counts, and every fitted level is a mean of rungs, so `value / g` is always a
**rational whose denominator is the size of the top pooled block** — which you can read straight off
the trailing run of zeros in `summands`. Measured:

<div class="lz-scroll lz-compare" markdown>

| sequence | `n` | `mm` | `value / g` | trailing zeros | top block |
|---|---:|---:|---:|---:|---:|
| `"01"×64` | 128 | 15 | 45 | 0 | 1 |
| `"01"×500` | 1 000 | 17 | 204 | 0 | 1 |
| `markov(8192, 0.9, 6)` | 8 192 | 20 | 820 | 0 | 1 |
| Markov P(stay)=0.95 | 20 000 | 21 | 2751 | 0 | 1 |
| period-8 `00010111` | 20 000 | 21 | 7497 | 0 | 1 |
| Thue–Morse | 20 000 | 21 | 13671 | 0 | 1 |
| the same `markov` with one symbol flipped | 8 192 | 20 | **5512 / 7** | 6 | **7** |
| i.i.d. binary | 20 000 | 21 | 0 | 20 | 21 |

</div>

When the top rung survives unpooled — the common case — `value / g` is a plain integer and the value
still equals the old closed form `mm · g · (C(u^RS(mm)) − C(u))`. When it is pooled, the denominator
appears, and it always equals the block size. There is no resolution below `g / (top block)`; at
`n = 20 000` and an unpooled top, that step is `mm·g = 0.0150`.

---

## Four rules

### 1. Never compare `emc` across different sequence lengths

!!! warning "The same process reports 16 % lower `emc` at 100× the length"

    `mm` grows with `n`, and the top of the ladder scales with `mm`, so changing the length alone
    moves the answer. A period-8 sequence has a constant true `E = log₂ 8 = 3` bits at every length.
    Measured:

    | `n` | 1 000 | 5 000 | 20 000 | 100 000 |
    |---|---:|---:|---:|---:|
    | `mm` | 17 | 19 | 21 | 23 |
    | `emc` | 5.9296 | 5.8367 | 5.3557 | 4.9701 |
    | `multi_information` | 1.0265 | 0.9928 | 0.9994 | 0.9931 |

    Truncate or resample every sequence in a study to a common length. If you cannot, pin
    `max_block_size` to one fixed value across the whole study — that removes the `mm` dependence but
    not the `O(1/log n)` LZ bias, which enters every rung twice with imperfect cancellation.

    Short sequences are worse than merely noisy. `emc` is identically `0.0` for `n ≤ 10`, because
    the block shuffle returns before swapping anything, and the `mm` jump from 4 to 14 across
    `n = 50 → 51` means sequences either side of that boundary are not on the same scale at all.

    The same caution applies to the log base: every rung is `∝ 1/ln(log_base)`, so binary and 4-ary
    results are not on a common scale unless you pass `log_base=2` to both. See
    [Alphabets](alphabets.md).

### 2. `0.0` now means one thing, and it is a real reading

!!! note "An exact zero means the whole ladder sat at or below zero"

    Under 1.0.2 an exact `0.0` was ambiguous: it could mean "structureless" *or* "the source period
    divides `mm`, so the top surrogate was the identity". The second case is gone — a period-`p`
    sequence with `p | mm` now reports the structure the other `mm − 1` scales found:

    | period `p` | motif | `p` divides 21 | `emc` (≤ 1.0.2) | `emc` (now) |
    |---:|---|---|---:|---:|
    | 2 | `01` | no | 1.7102 | 1.7102 |
    | 3 | `001` | **yes** | **−1.3e−15** | **1.8193** |
    | 4 | `0011` | no | 3.5255 | 3.5255 |
    | 5 | `00101` | no | 4.1106 | 4.1106 |
    | 7 | `0010111` | **yes** | **−2.7e−15** | **4.1974** |
    | 8 | `00010111` | no | 5.3557 | 5.3557 |
    | 11 | `00010110011` | no | 6.1509 | 6.1980 |
    | 16 | `0001001101011110` | no | 7.2610 | 7.2610 |
    | 21 | `000100110101111000101` | **yes** | **−8.9e−16** | **6.0262** |
    | 32 | `00010011010111100010101110110011` | no | 9.4213 | 9.4213 |

    `emc == 0.0` now means the projection found no monotone rise anywhere in the ladder, which is
    what a structureless input looks like. It is a **hard** zero, not a near-zero: test with
    `value == 0.0` if you like, though `abs(value) < 1e-12` remains the safer habit.

    It is still not proof of absence. A short structured sequence can produce it by noise — 8 of 20
    i.i.d. draws at `n = 2 048` return exactly `0.0`, and so would a weak signal at that length.
    Rule 3 is how you tell them apart.

### 3. Establish a null — the floor is one-sided now, and it is not at zero

`emc` ships no error bar. The `normal_error` and `poison_error` fields in the `lz76()` output are
uncertainties on the *entropy density*, not on `emc`.

Clamping at zero buys non-negativity at a price: on structureless input the estimator can no longer
err downward, so its noise piles up above zero and the null distribution has a **positive mean**.
Measured over 20 independent i.i.d. binary sequences per length, `iid(n, 0.5, seed)` for
`seed` in `range(20)`:

<div class="lz-scroll lz-compare" markdown>

| `n` | `mm` | mean | s.d. | max | exactly `0.0` | mean + 2 s.d. |
|---:|---:|---:|---:|---:|---:|---:|
| 1 000 | 17 | 0.1091 | 0.1762 | 0.5680 | 12/20 | 0.4615 |
| 2 048 | 18 | 0.1732 | 0.1721 | 0.4834 | 8/20 | 0.5173 |
| 5 000 | 19 | 0.1074 | 0.1056 | 0.3735 | 5/20 | 0.3186 |
| 20 000 | 21 | 0.0613 | 0.0702 | 0.2115 | 7/20 | 0.2018 |
| 100 000 | 23 | 0.0180 | 0.0260 | 0.0993 | 9/20 | 0.0700 |

</div>

Read the last column as the level a value has to clear before it is worth interpreting. At
`n = 20 000` that is ≈0.20, against ≈0.25 for the old two-sided floor — a modest improvement, and the
floor still falls with `n`. **A small positive `emc` is the null, not a weak signal.** Establish the
floor for your own data: shuffle, re-run 20–30 times, take mean + 2 s.d.

The comparison with 1.0.2 at `n = 20 000` is the whole trade in one line — the old floor was
`mean 0.0075, s.d. 0.1230, negative in 9/20`; the new one is `mean 0.0613, s.d. 0.0702, negative in
0/20`. Half the spread, no sign violations, a positive offset you have to subtract yourself.

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
the control parameter, one fixed `n`, `h` and `emc` side by side.

That sweep is also the clearest evidence for the projection. Its peak is unchanged — **7.4410 at
r = 3.570**, the first grid point past the Feigenbaum point — but its *minimum* moved. Under 1.0.2
the smallest value in the sweep was a floating-point zero at the period-3 window near r = 3.83, an
artefact of `3 | mm`. It is now **0.2400 at r = 4.000**, full chaos, which is where excess-entropy
theory says the minimum belongs. The shape the estimator was supposed to show is the shape it now
shows.

For what it does with recorded rather than generated data, see [Neuroscience](../recipes/neuro.md)
and [Genomics](../recipes/genomics.md).

## Field reference

`emc()` and the `"emc"` block of `lz76()` carry their full signatures on the
[Python API](../api/python.md) page. The fields:

| Field | Type | Meaning |
|---|---|---|
| `emc()[0]`, `lz76()["emc"]["value"]` | `float` | `Ê` — the top of the projected ladder. Always `>= 0.0` |
| `emc()[1]`, `lz76()["emc"]["summands"]` | `list[float]`, length `mm` | `summands[l-1]` is the fitted `h(l) − h` at block size `l`. Every entry `>= 0.0`; they sum to the value |
| `lz76()["emc"]["max_block_size"]` | `int` | the resolved `mm` |
| `lz76()["emc"]["multi_information"]` | `float` | the **raw**, unprojected `l = 1` contrast `H_1 − ĥ`. May be slightly negative; usually equals `summands[0]` |

The shuffle is seeded from an FNV-1a hash of the sequence bytes, so the same input yields the same
`emc` on any machine at any thread count, and the projection is a deterministic single pass over the
rungs. That is reproducibility of one surrogate realization, not an ensemble estimate — see
[Determinism](../project/determinism.md).
