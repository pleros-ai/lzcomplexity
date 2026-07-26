# The route to chaos

*One 1-D map, 121 parameter values: `h` tracks randomness, EMC peaks at the edge of chaos.*

The logistic map is about the cheapest system that produces every regime a complexity measure has
to tell apart — period-doubling, a Feigenbaum accumulation point, full chaos, and narrow periodic
windows buried inside the chaotic band. Binarise the orbit and both `h` and `emc` become one-line
calls. They then do visibly different things, which is the entire reason to compute both.

<div class="lz-stats" markdown>
  <div class="lz-stat">
    <div class="lz-stat__v">20 000</div>
    <div class="lz-stat__k">symbols per point</div>
  </div>
  <div class="lz-stat">
    <div class="lz-stat__v">121</div>
    <div class="lz-stat__k">values of r</div>
  </div>
  <div class="lz-stat">
    <div class="lz-stat__v">1.017</div>
    <div class="lz-stat__k">h at r = 4</div>
  </div>
  <div class="lz-stat">
    <div class="lz-stat__v">7.44</div>
    <div class="lz-stat__k">peak EMC, at r = 3.57</div>
  </div>
  <div class="lz-stat">
    <div class="lz-stat__v">1.5 s</div>
    <div class="lz-stat__k">whole sweep</div>
  </div>
</div>

## The map and the partition

<div class="lz-formula">
  <p class="lz-math"><i>x</i><sub><i>n</i>+1</sub> = <i>r</i> · <i>x</i><sub><i>n</i></sub> · (1 − <i>x</i><sub><i>n</i></sub>)&emsp;&emsp;<i>s</i><sub><i>n</i></sub> = 1 if <i>x</i><sub><i>n</i></sub> &gt; 1⁄2, else 0</p>
  <dl class="lz-formula__key">
    <dt><i>x</i><sub><i>n</i></sub></dt><dd>orbit point in [0, 1]; the sweep starts from <i>x</i><sub>0</sub> = 0.4 and discards 2 000 steps of transient</dd>
    <dt><i>r</i></dt><dd>growth rate, swept from 3.4 to 4.0 in steps of 0.005</dd>
    <dt><i>s</i><sub><i>n</i></sub></dt><dd>the symbol fed to the library — a binary string of length <i>n</i> = 20 000</dd>
  </dl>
  <p class="lz-formula__cite">The threshold <i>x</i> = 1⁄2 is the generating partition of the logistic map, so the symbol sequence has the same entropy rate as the map itself — Lesne, Blanc &amp; Pezard, <i>Phys. Rev. E</i> <b>79</b>, 046208 (2009).</p>
</div>

The choice of threshold is not cosmetic. Kolmogorov–Sinai entropy is a supremum over partitions,
so a partition that is *not* generating loses information: the entropy rate of the symbol sequence
it produces sits at or below the entropy rate of the map. For the logistic map the critical point
at one half is the generating partition Lesne et al. use; for a measured signal you almost never
have one.

Two biases then run in opposite directions and should not be conflated. A non-generating partition
biases the *target* downward. The finite-`n` LZ76 estimator biases the *measurement* upward — by
enough to return 1.0173 at r = 4, where the true rate is 1. See
[Alphabets and log bases](../concepts/alphabets.md) for what changes when you use more than two
symbols.

## The sweep

This is the exact script that produced every number on this page.

```python title="logistic_sweep.py"
import json
import lzcomplexity as lz


def logistic_symbols(r, n=20000, burn=2000, x0=0.4):
    x = x0
    for _ in range(burn):
        x = r * x * (1 - x)
    out = []
    for _ in range(n):
        x = r * x * (1 - x)
        out.append('1' if x > 0.5 else '0')
    return ''.join(out)


rows = []
r = 3.40
while r <= 4.0001:
    s = logistic_symbols(r)
    c, _ = lz.factorization(s)
    h = lz.h(s)
    e, _ = lz.emc(s)
    rows.append({'r': round(r, 4), 'c': c, 'h': round(h, 4), 'emc': round(e, 4)})
    r += 0.005
print(json.dumps(rows))
```

The sequence is binary, so `lz.h()` defaults to `log_base=2` and every `h` on this page is in
**bits per symbol**. Passing `log_base=2` explicitly changes nothing here; it starts to matter the
moment you move to a larger alphabet.

<div class="lz-run" markdown>

```console
$ time python3 logistic_sweep.py > logistic.json

real    0m1.487s
user    0m6.349s
sys     0m0.173s
```

</div>

!!! tip "Wall clock is a fraction of CPU time, because the surrogates run in parallel"

    2.42 million symbols and 2 904 factorizations: 242 for the `c` and `h` columns, and 2 662
    inside `emc()`. The auto block size is `mm = 21` at n = 20 000, and each `emc()` call
    factorizes the original sequence plus one surrogate per block size — 22 apiece.

    The ratio is the number to take away, not the absolute time. On the 16-core desktop that
    produced the run above, repeat timings ranged from 1.5 s to 3.0 s of wall clock against a
    steady 6.3–7.5 s of CPU; both figures move with core count and machine load. Generating the
    orbits in pure Python accounts for about 0.3 s of that, and `emc()` for the rest.

## The result

![Entropy rate and effective measure complexity of the logistic map from r = 3.4 to r = 4.0. The entropy rate stays near zero through the period-doubling cascade, then rises slightly above 1 bit per symbol at r = 4, with sharp downward spikes at the periodic windows. EMC starts around 1.7, climbs through the cascade to a sharp maximum at r = 3.57, then decays through the chaotic band toward zero at r = 4, with upward spikes at the periodic windows.](../assets/logistic-chart.svg)

The left axis is `h` in bits per symbol, plotted directly. EMC shares that axis rescaled to its own
maximum — 7.4410 at r = 3.570. The dashed line marks the Feigenbaum accumulation point
r<sub>∞</sub> = 3.5699.

Two different shapes, from the same 20 000 symbols:

- **`h` rises**, from 0.0014 in the period-2 window at r = 3.4 to **1.0173** at r = 4.
- **EMC humps.** It climbs through the period-doubling cascade, peaks at **7.4410** at r = 3.570 —
  the first grid point above r<sub>∞</sub> — and decays to **0.2400** at r = 4.

That is precisely the one-hump behaviour excess entropy is supposed to show: small for a memoryless
source, small for a trivially predictable one, large where structure is long-ranged. The estimator
earns its keep here, remaining biases and all — see
[Effective measure complexity](../concepts/emc.md).

## The numbers

Selected rows from the 121-point sweep. `c` is the raw LZ76 factor count; `h` and `emc` are both in
bits per symbol.

| r | regime | `c` | `h` | `emc` |
|---|---|---:|---:|---:|
| 3.400 | period 2 | 2 | 0.0014 | 1.7102 |
| 3.500 | period 4 | 2 | 0.0014 | 3.4955 |
| 3.555 | period 8 | 3 | 0.0021 | 5.2207 |
| **3.570** | **first grid point past r<sub>∞</sub>** | **43** | **0.0307** | **7.4410** |
| 3.600 | chaotic | 433 | 0.3093 | 3.8855 |
| 3.630 | period-6 window | 2 | 0.0014 | 2.8783 |
| 3.700 | chaotic | 788 | 0.5629 | 1.5152 |
| 3.740 | period-5 window | 4 | 0.0029 | 4.0956 |
| 3.800 | chaotic | 938 | 0.6701 | 1.1552 |
| 3.830 | period-3 window | 2 | 0.0014 | 1.8000 |
| **3.850** | **period-3 window, chaotic band** | **35** | **0.0250** | **5.0962** |
| 3.900 | chaotic | 1076 | 0.7687 | 1.3802 |
| 3.950 | chaotic | 1238 | 0.8844 | 0.5772 |
| **4.000** | **fully chaotic** | **1424** | **1.0173** | **0.2400** |

The regime labels are not decoration. Each one was read off the orbit by testing
`max|x[k] − x[k+q]| < 1e-9` for q = 1…63 on the same trajectory that produced the symbols.

!!! note "`c` here is usually one less than the textbook LZ76 count"

    `complexity` counts only **complete** LZ76 components. A trailing component that runs past the
    end of the sequence is not counted, so on every row in the table above the exhaustive-history
    count of Lempel & Ziv is `c + 1` — 1425 rather than 1424 at r = 4. The conversion is
    mechanical, and it is conditional:

    ```python
    c, factors = lz.factorization(seq)
    c_textbook = c + (1 if factors[-1] > len(seq) else 0)
    ```

    The correction is not automatic across a sweep. `factors[-1] == 20001` at 120 of the 121
    points; at r = 3.980 the parse lands exactly on `factors[-1] == 20000`, the trailing component
    is complete, and `c = 1313` already *is* the textbook count. Apply the test, do not add 1 by
    reflex. [LZ76 factorization](../concepts/lz76.md) covers the convention in full.

<div class="lz-tickrule"></div>

## Reading `h`

`h` rises **in trend**, not step by step. Inside the chaotic band — the 78 points with h ≥ 0.05 —
20 of the 77 consecutive steps go *down*.

Most of those reversals are small: 14 of the 20 drop `c` by under 5%. The two largest are steps
that jump clean over a periodic window the h ≥ 0.05 filter removed — `c` falls 25.3% across
r = 3.735 → 3.745, over the period-5 window, and 20.3% across r = 3.625 → 3.635, over the period-6
window. The rest are the band's own fine structure, and not all of them have a window nearby: the
16.2% drop at r = 3.770 → 3.775 has no orbit of period ≤ 200 anywhere within ±0.004.

The windows themselves never enter the band at all. At those points `c` is 2 to 5 and `h` is about
0.002, well under the 0.05 cutoff. The reading: a single step of a chaotic sweep is not a trend.

!!! warning "An assertion that `h` ≤ 1 fails at r = 4"

    The sweep returns **1.0173** where the true entropy rate is exactly 1 bit per symbol. At r = 4
    the map is conjugate to the Bernoulli shift, so in exact arithmetic the binarised orbit is an
    i.i.d. fair coin — but the LZ76 estimator has no ceiling at 1. The bound `c(S) < n / log n` is
    asymptotic, and at finite n the ratio approaches it from above, the excess decaying like
    `log log n / log n`. Test with a tolerance and state the direction of the bias; see
    [Sequence length and convergence](../concepts/convergence.md).

    Do not read 1.0173 as a calibration of that bias, though. Thirty genuine fair-coin sequences at
    n = 20 000 give h = 1.0232 ± 0.0034, and the r = 4 row falls below all thirty. The orbit is
    computed in double precision, which sheds about one bit per iterate, so after 22 000 steps it
    is measurably more compressible than a real coin.

## Reading EMC

EMC does the thing `h` cannot: it separates *structured* from *random*, rather than *ordered* from
*random*. The peak at r = 3.570 sits where the period-doubling cascade accumulates — the orbit has
stopped being periodic, `h` is still only 0.0307, and correlations extend over arbitrarily long
ranges. That is the largest EMC anywhere in the sweep, 31× the value at r = 4.

!!! note "`summands` is a scale-resolved profile, and the whole of it feeds the total"

    Each block size `l` contributes a rung `Ê(l) = l · g · (C_LZ(shuffled at l) − C_LZ(original))`
    with `g = log_k(n)/n`. Excess entropy is a mutual information between past and future, so the
    true ladder is non-negative and rises with `l`; the raw rungs are neither, because each rests on
    its own surrogate draw. The library projects the ladder onto that shape and reports the
    increments, so **every scale reaches the total**, every summand is non-negative, and they sum to
    `value`. Two consequences for this page: the EMC column is comparable across the sweep only
    because every point uses the same n and therefore the same `mm = 21`; and each rung still rides
    on a single surrogate draw, so the column is one realisation. Full treatment in
    [Effective measure complexity](../concepts/emc.md).

    Up to 1.0.1 the total was the sum of the raw first differences, which telescopes down to the
    scale-`mm` rung alone. That is why the three period-3 rows at r = 3.830–3.840 used to report
    **−1.3 × 10⁻¹⁵** — floating-point zero. The block shuffle permutes `mm`-aligned blocks, `mm = 21`,
    and 3 divides 21, so the scale-21 shuffle is the identity on a period-3 sequence and the surrogate
    *was* the original. Those rows now report ≈1.80, from the scales that did see structure.

The repaired sweep says something the old one could not. Its **minimum is now at r = 4.000**, full
chaos, at `emc` 0.2400 — the physically correct place for excess entropy to bottom out. Under 1.0.1
the smallest value anywhere in the sweep was the floating-point zero in the period-3 window, an
artefact of the block grid rather than a statement about the map. No point in the sweep returns
exactly zero any more.

## The period-3 window

The window near r = 3.85 deserves its own paragraph, because it is where `h` and EMC disagree most
loudly. The sweep walks through it in seven steps:

| r | orbit | `c` | `h` | `emc` |
|---|---|---:|---:|---:|
| 3.830 | period 3 | 2 | 0.0014 | 1.8000 |
| 3.835 | period 3 | 3 | 0.0021 | 1.7962 |
| 3.840 | period 3 | 3 | 0.0021 | 1.7962 |
| 3.845 | period 6 | 4 | 0.0029 | 2.9904 |
| **3.850** | **aperiodic band** | **35** | **0.0250** | **5.0962** |
| 3.855 | period 30 | 5 | 0.0036 | 5.2403 |
| 3.860 | chaotic | 880 | 0.6287 | 2.6554 |

At r = 3.850 the orbit has period-doubled its way off the 3-cycle into a narrow chaotic band around
it. **Complexity collapses to 35** — 2.5% of the 1424 at r = 4 — while **EMC jumps to 5.10**,
21× the r = 4 value. To `h`, a window inside chaos looks almost like a fixed point; to EMC it looks
like the most structured thing in the neighbourhood. Both readings are correct, and you only get
the second one by computing EMC.

!!! example "Reproducing the r = 3.850 row"

    ```python
    import lzcomplexity as lz

    def logistic_symbols(r, n=20000, burn=2000, x0=0.4):
        x = x0
        for _ in range(burn):
            x = r * x * (1 - x)
        out = []
        for _ in range(n):
            x = r * x * (1 - x)
            out.append('1' if x > 0.5 else '0')
        return ''.join(out)

    s = logistic_symbols(3.8499999999999903)
    c, factors = lz.factorization(s)
    e, summands = lz.emc(s)

    print(s[:30])
    print(c, len(factors), factors[-1])
    print(lz.h(s))
    print(e, len(summands), summands[0])
    ```

    ```text
    001011001001001011001011001001
    35 37 20001
    0.025003496664211537
    5.09618887724563 21 0.9687068993334527
    ```

    The literal `3.8499999999999903` is the value the sweep loop actually reaches at that step —
    see the warning below.

## Individual points are not reproducible; the curve is

!!! warning "Writing `r = 3.40 + 0.005 * i` changes the numbers"

    The sweep advances `r` by repeated addition, so the value at step 90 is `3.8499999999999903`,
    not `3.85`. Recomputing the same grid as `3.40 + 0.005 * i` shifts each `r` by about 10⁻¹⁴ —
    and after 22 000 iterations of a chaotic map, a 10⁻¹⁴ perturbation is a completely different
    orbit. Measured effect on `c`:

    | r | `c`, accumulated | `c`, multiplied | change |
    |---|---:|---:|---:|
    | 3.570 | 43 | 46 | +6.98% |
    | 3.600 | 433 | 442 | +2.08% |
    | 3.900 | 1076 | 1085 | +0.84% |
    | 4.000 | 1424 | 1423 | −0.07% |

    EMC moves by 0.04 to 0.09 on the same four points. Across all 78 chaotic-band points the
    median change in `c` is 0.77% and the largest is 3.76% (at r = 3.625); 47 of the 78 come in
    under 1%. The sensitivity is largest near r<sub>∞</sub>, where the orbit is most delicately
    structured. **Treat individual `c`, `h` and `emc` values from a chaotic sweep as one draw from
    a distribution, not as constants.** The shape of both curves is stable; the third decimal
    place is not.

None of this is a defect in the library — the factorization is exact and deterministic, and the
same string always gives the same three numbers, on any machine and any thread count
(see [Determinism](../project/determinism.md)). The variability is entirely in the input.

<div class="lz-tickrule"></div>

## Adapting the recipe

The same nine-line helper covers any 1-D map, and with one substitution, any measured time series.

<div class="lz-cards" markdown>

<div class="lz-card" markdown>

### Another 1-D map

Replace the update rule. Smooth unimodal maps with a quadratic maximum — the sine map, for
instance — period-double through the same Feigenbaum route. Sweep the sine map over r ∈ (0, 1],
not 3.4 to 4: EMC peaks at 7.89 near r = 0.8655 and `h` reaches 1.0123 at r = 1, the same shape
one axis over. Keep the generating partition if the map has a known one.

<p class="lz-card__api"><code>x = r * math.sin(math.pi * x)</code></p>

</div>

<div class="lz-card" markdown>

### A measured signal

Binarise about the **median** of the window rather than a fixed level. It is invariant under any
monotone increasing rescaling of the recording, and it holds the symbol frequencies at 50/50 up to
ties, so differences in `h` reflect ordering rather than amplitude drift.

<p class="lz-card__api"><code>''.join('1' if v &gt; median else '0' for v in x)</code></p>

</div>

<div class="lz-card" markdown>

### An oscillatory signal

For band-limited data — EEG, LFP, MEG — the published choice is the mean of the instantaneous
Hilbert amplitude, per channel. That pipeline, and the surrogate normalisation that goes with it,
is on the [EEG recipe](neuro.md).

<p class="lz-card__api"><code>lz.h(binarised, log_base=2)</code></p>

</div>

<div class="lz-card" markdown>

### A control parameter that is not r

Anything swept monotonically works: coupling strength, temperature, noise amplitude, drug
concentration. The read is unchanged — `h` for how much new information the source produces, EMC
for how far back its memory reaches.

<p class="lz-card__api"><code>lz.emc(seq)</code></p>

</div>

</div>

Four rules carry over unchanged:

1. **Hold n fixed across the sweep.** The auto block-size cap `mm` is a function of length and EMC
   scales with `mm`, so EMC computed at different n is not comparable. `h` tolerates varying n
   better, but still carries a length-dependent bias.
2. **Hold the alphabet fixed.** `h` defaults to `log_base` = alphabet size, floored at 2, so a
   point that happens to emit a single symbol does not silently change units.
3. **Burn in.** 2 000 steps is enough for the logistic map at these parameters. Too short and you
   factorize the transient rather than the attractor, which distorts `c` most in the periodic
   windows: at r = 3.830, dropping the burn-in entirely takes `c` from 2 to 12.
4. **Use at least ~10³ symbols.** Below that, finite-size bias dominates `h` and the EMC noise
   floor swamps the signal. [Sequence length and convergence](../concepts/convergence.md) gives the
   numbers.

## What to read next

- [Entropy density (h)](../concepts/entropy-density.md) — what the normalisation does, and why `h`
  has no ceiling at 1.
- [Effective measure complexity](../concepts/emc.md) — the ladder and its projection, the `mm`
  heuristic, and when the number is noise.
- [Reading the numbers](../guide/reading-the-numbers.md) — a short tour of every field the library
  returns.
- [References](../project/references.md) — the source papers behind these measures.
