# Performance

*Measured wall-clock, throughput, scaling exponents and the cost model — how they were produced, and where the time goes.*

Every number on this page came from a run on one machine, described below in enough detail to be
re-run and contradicted. Everything is measured except the n = 10⁷ naive row and the whole
[task table](#why-this-matters) at the end, which are projections and are marked as such.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">12 MB/s</span><span class="lz-stat__k">incompressible data</span></div>
<div class="lz-stat"><span class="lz-stat__v">38 MB/s</span><span class="lz-stat__k">repetitive data</span></div>
<div class="lz-stat"><span class="lz-stat__v">1.01</span><span class="lz-stat__k">scaling exponent α</span></div>
<div class="lz-stat"><span class="lz-stat__v">0.7 µs</span><span class="lz-stat__k">python call floor</span></div>
<div class="lz-stat"><span class="lz-stat__v">13n</span><span class="lz-stat__k">bytes peak RSS</span></div>
<div class="lz-stat"><span class="lz-stat__v">~1500×</span><span class="lz-stat__k">vs naive at n = 10⁶</span></div>
</div>

---

## Methodology

Read this before quoting any absolute number.

| | |
|---|---|
| **Machine** | Intel Core i7-10870H @ 2.20 GHz (turbo 5.00 GHz), 8 physical cores / 16 logical threads, L1d 256 KiB, L2 2 MiB, L3 16 MiB (single instance), 15 GiB RAM, Linux 7.1.4-zen1-1-zen x86_64 |
| **Thread count** | `rayon::current_num_threads()` = 16 (the default global pool). Single-thread runs set `RAYON_NUM_THREADS=1` |
| **Toolchain** | rustc 1.95.0 / cargo 1.95.0, `--release` with the shipped profile: `lto = "thin"`, `codegen-units = 1`, `opt-level = 3` |
| **What was timed** | the `lzcomplexity-core` entry points the Python API calls 1:1 — `factorization` → `lz76_factors`, `h` → `lz76_entropy_density`, `nid` → `lz76_information_distance`, `emc` → `lz76_random_shuffle_complexity` (with shuffle terms on, as the binding sets it), `lz76` → `metrics::lz76`. `LzArgs::new()` defaults throughout |
| **Warm-up** | one untimed call per cell, then the timed repetitions |
| **Repetitions** | *R* = 200 / 100 / 30 / 10 for n = 10³ / 10⁴ / 10⁵ / 10⁶ on `factorization`, `h` and `nid`; 50 / 25 / 10 / 5 on `emc` and `lz76`. Wall clock via `std::time::Instant`, per-cell **median** |
| **Runs** | the whole suite ran 3× at 16 threads and 2× at 1 thread. Tables report the **median of the per-run medians**, except where a multi-versus-single-thread comparison is being made, which uses the **minimum** |

### The corpora

Four, all deterministic. `k` is the auto-detected alphabet size.

| corpus | construction | `k` |
|---|---|---|
| `random_binary` | i.i.d. uniform over `{0,1}`, ChaCha8 seed `0x00C0FFEE` | 2 |
| `repetitive` | a 32-symbol random binary motif (ChaCha8 seed `0x0000BEEF`) tiled to length n | 2 |
| `dna` | i.i.d. uniform over `{A,C,G,T}`, ChaCha8 seed `0x000D11A` | 4 |
| `english` | the first n bytes of a 2 873 215-byte file of real technical prose | 54 / 76 / 92 / 143 |

For `nid` the second operand is an independent same-class, same-length sequence: a different seed,
or for `english` the bytes `[n, 2n)`.

!!! warning "Treat these as order-of-magnitude figures on a 2020 laptop, not as clean-room results."

    The machine was shared with other workloads during the sweep. Across the three 16-thread runs
    the max/min spread of the per-run medians had a **median of 1.16× and a worst cell of 1.80×**
    (`random_binary`, n = 10⁶, `lz76`). The two single-thread runs were dirtier still: median spread
    **1.25×**, with several cells 5–8× apart — the worst being `english`, n = 10⁶, `h` at
    **60.4 ms versus 504.6 ms**. That is why every single-thread figure below is a minimum rather
    than a median, and why single-thread absolutes are quoted to one significant figure.

    What *does* reproduce exactly: factor counts, iteration counts, and every returned float. See
    [Determinism](determinism.md). Peak RSS reproduces exactly only single-threaded; with a live
    rayon pool it varies by ~15 % between runs.

---

## Wall clock

Milliseconds, 16 threads. `c_LZ` is the LZ76 factor count of the input; `mm` is the EMC block-size
ceiling actually used.

<div class="lz-scroll lz-compare" markdown>

| corpus | n | k | `c_LZ` | `mm` | `factorization` | `h` | `nid` | `emc` | `lz76` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `random_binary` | 1 000 | 2 | 108 | 17 | 0.075 | 0.075 | 0.276 | 0.511 | 0.588 |
| `random_binary` | 10 000 | 2 | 777 | 20 | 0.722 | 0.718 | 1.72 | 3.58 | 4.23 |
| `random_binary` | 100 000 | 2 | 6 130 | 23 | 7.43 | 7.48 | 16.3 | 37.4 | 37.9 |
| `random_binary` | 1 000 000 | 2 | 50 746 | 26 | **81.5** | 89.7 | 278 | **1 269** | 1 463 |
| `repetitive` | 1 000 | 2 | 7 | 17 | 0.154 | 0.153 | 0.495 | 0.603 | 0.680 |
| `repetitive` | 10 000 | 2 | 7 | 20 | 0.345 | 0.337 | 0.880 | 2.77 | 3.15 |
| `repetitive` | 100 000 | 2 | 7 | 23 | 2.35 | 2.39 | 7.77 | 27.5 | 29.8 |
| `repetitive` | 1 000 000 | 2 | 7 | 26 | **26.3** | 27.8 | 112 | **1 199** | 1 229 |
| `dna` | 1 000 | 4 | 201 | 17 | 0.074 | 0.075 | 0.278 | 0.507 | 0.580 |
| `dna` | 10 000 | 4 | 1 488 | 20 | 0.733 | 0.733 | 1.69 | 3.76 | 4.30 |
| `dna` | 100 000 | 4 | 11 885 | 23 | 7.31 | 7.68 | 19.6 | 41.1 | 46.2 |
| `dna` | 1 000 000 | 4 | 99 030 | 26 | **88.6** | 88.0 | 285 | **1 399** | 1 419 |
| `english` | 1 000 | 54 | 269 | 17 | 0.075 | 0.077 | 0.278 | 0.504 | 0.582 |
| `english` | 10 000 | 76 | 1 803 | 20 | 0.684 | 0.643 | 1.71 | 3.59 | 4.08 |
| `english` | 100 000 | 92 | 7 977 | 23 | 5.16 | 5.22 | 13.8 | 34.5 | 35.6 |
| `english` | 1 000 000 | 143 | 51 883 | 26 | **63.7** | 61.2 | 206 | **1 328** | 1 387 |

</div>

The `c_LZ` column counts **complete LZ76 components only** — one fewer than the textbook exhaustive
history whenever the parse runs past the end of the sequence. That convention is explained on
[LZ76 factorization](../concepts/lz76.md); it matters here because it is exactly the ±1 you will see
in the comparison at the bottom of this page.

---

## Throughput at n = 10⁶

| corpus | `factorization` | throughput |
|---|---:|---:|
| `random_binary` | 81.5 ms/MB | 12.3 MB/s |
| `dna` | 88.6 ms/MB | 11.3 MB/s |
| `english` | 63.7 ms/MB | 15.7 MB/s |
| `repetitive` | 26.3 ms/MB | 38.1 MB/s |

One line to remember: **~12 MB/s on incompressible data, ~38 MB/s on highly repetitive data.**

!!! note "These are single-core figures, despite the 16-thread header."

    `factorize` contains no rayon calls at all — `grep -n rayon` over `lz76.rs`, `suffix_array.rs`,
    `lpf.rs` and `sequence.rs` returns nothing. rayon appears only in `shuffle.rs` (the EMC block
    sweep) and `metrics.rs` (the `rayon::join` inside `nid` and the extras). The whole suite ran with
    the default pool, so the header says 16 threads, but a lone `factorization` or `h` call uses one
    core start to finish.

---

## Scaling

Exponents for `t ∝ n^α`, computed per decade as `log₁₀(t(10n)/t(n))`, plus an overall
`log(t(10⁶)/t(10³)) / log(10³)`.

<div class="lz-scroll lz-compare" markdown>

| op | corpus | 10³→10⁴ | 10⁴→10⁵ | 10⁵→10⁶ | overall |
|---|---|---:|---:|---:|---:|
| `factorization` | `random_binary` | 0.98 | 1.01 | 1.04 | **1.01** |
| `factorization` | `dna` | 1.00 | 1.00 | 1.08 | **1.03** |
| `factorization` | `english` | 0.96 | 0.88 | 1.09 | **0.98** |
| `factorization` | `repetitive` | 0.35 | 0.83 | 1.05 | 0.74 [^cutoff] |
| `h` | `random_binary` / `dna` / `english` | 0.98 / 0.99 / 0.92 | 1.02 / 1.02 / 0.91 | 1.08 / 1.06 / 1.07 | **1.03 / 1.02 / 0.97** |
| `nid` | `random_binary` / `dna` / `english` | 0.80 / 0.78 / 0.79 | 0.98 / 1.06 / 0.91 | 1.23 / 1.16 / 1.17 | **1.00 / 1.00 / 0.96** |
| `emc` (16 t) | `random_binary` / `dna` / `english` | 0.84 / 0.87 / 0.85 | 1.02 / 1.04 / 0.98 | 1.53 / 1.53 / 1.59 | 1.13 / 1.15 / 1.14 [^par] |
| `emc` (1 t) | `random_binary` / `dna` / `english` / `repetitive` | 0.96 / 0.96 / 0.99 / 0.91 | 1.08 / 1.08 / 0.93 / 1.01 | 1.13 / 1.08 / 1.28 / 1.08 | **1.05 / 1.04 / 1.06 / 1.00** |
| suffix array | `random_binary` / `dna` / `english` | 0.98 / 0.96 / 0.90 | 0.89 / 0.98 / 0.87 | 1.10 / 1.07 / 1.07 | **0.99 / 1.00 / 0.95** |
| LCP (Kasai) | `random_binary` / `dna` / `english` | 1.44 / 1.43 / 1.37 | 1.13 / 1.18 / 1.03 | 1.22 / 1.12 / 1.14 | 1.26 / 1.25 / 1.18 [^kasai] |

</div>

**`factorization`, `h` and `nid` are empirically linear, α ≈ 1.00 ± 0.05.** The headline case is
`factorization` on `random_binary`: **t ∝ n^1.01**.

That is the expected result. `factorize` is a suffix array (`cdivsufsort`, O(n log n) worst case and
near-linear in practice), a Kasai LCP pass (O(n)), a Crochemore–Ilie LPF stack pass (O(n) amortised),
and an O(`c_LZ`) walk. The asymptotic ceiling for the whole pipeline is O(n log n), not O(n) — the
in-source comment calling divsufsort "linear-time" is wrong, and these docs do not repeat it.

[^cutoff]: Not sublinear scaling. `repetitive` at n = 10³ sits below the `SORT_CUTOFF = 2048`
    comparison-sort branch, where a period-32 input is close to that sort's worst case (0.154 ms),
    while n = 10⁴ takes the divsufsort branch and costs only 0.345 ms. See
    [the crossover](#the-sort_cutoff-2048-crossover).

[^par]: A parallel-efficiency collapse, not an algorithmic term. The single-threaded exponent for the
    same operation is 1.05, so `emc` really is O(`mm` · n) with `mm` = Θ(log n).

[^kasai]: Kasai's pass measures α ≈ 1.2 rather than 1.0 because at small n the arrays fit in cache
    while at n = 10⁶ the `rank`/`sa`/`lcp` arrays total 12 MB and the access into `sa[rank[i]-1]` is
    random. It is memory-latency bound, not super-linear.

### Where the time goes

Wall-clock split of one `factorization` on random binary, two independent runs:

| n | suffix array | LCP | LPF | LZ76 walk | total |
|---:|---|---|---|---|---|
| 10⁵ | 4.7 / 4.0 ms (60–62 %) | 1.8 / 1.4 ms (22–23 %) | 1.3 / 1.0 ms (16 %) | 0.04 / 0.03 ms (0.4–0.5 %) | 7.9 / 6.5 ms |
| 10⁶ | 54.0 / 44.7 ms (56–60 %) | 29.7 / 19.0 ms (26–31 %) | 12.7 / 10.3 ms (13–14 %) | 0.5 / 0.25 ms (0.3–0.5 %) | 96.9 / 74.4 ms |
| 10⁷ | 742.8 / 548.4 ms (44–46 %) | 698.1 / 558.8 ms (43–45 %) | 176.1 / 136.8 ms (11 %) | 4.8 / 5.2 ms (0.3–0.4 %) | 1 621.8 / 1 249.1 ms |

Index construction is essentially the whole cost — 83 % at n = 10⁵ rising to ~89 % at n = 10⁷ — and
**the LZ76 parse itself is free**, never more than 0.5 % of the call. Suffix sorting is the single
largest phase up to n = 10⁶, but by n = 10⁷ the LCP phase has caught up and the two are within a
couple of points of each other. Do not say "suffix sorting dominates" without the size qualifier.

Memory is ~13 bytes per symbol at peak: measured `VmHWM` was 13.3 bytes/symbol at n = 10⁷ and
15.9 at n = 10⁶ (the difference is a ~3 MB fixed overhead). Six length-n 4-byte arrays are allocated
over a call; at most three are live at once.

---

## Rules of thumb

<div class="lz-cards" markdown>
<div class="lz-card" markdown>

### `factorization` and `h` cost the same

They agree to within **11 %** in every one of the 16 cells, and within 5 % in 12 of them. `h` is
`entropy_density_from(lz76_factorization(...))` and `factorization` is `lz76_factors` — one
`factorize()` call each. Returning the boundary vector is free.
<p class="lz-card__api"><code>lz.factorization(s)</code> ≈ <code>lz.h(s)</code></p>
</div>
<div class="lz-card" markdown>

### `nid` is 2–4× one factorization

Measured range **2.2×–4.2×** across all 16 cells, even though it performs four factorizations — two
of length n and two of length 2n, so 6n bytes of suffix-array work. `rayon::join` recovers most of
the difference.
<p class="lz-card__api"><code>lz.nid(a, b)</code></p>
</div>
<div class="lz-card" markdown>

### `emc` is 5–21× one factorization

At 16 threads: ~5–7× up to n = 10⁵ for the i.i.d.-like corpora, rising to ~15–21× at n = 10⁶.
Single-threaded, the same corpora sit at **27–50×**. `repetitive` is an outlier and must not be
folded in — 3.9× / 8.0× / 11.7× / **45.6×** at 16 threads, 14× / 53× / 79× / **86×** at one.
<p class="lz-card__api"><code>lz.emc(s)</code></p>
</div>
<div class="lz-card" markdown>

### `lz76` is `emc` plus a little

The full bundle measured **+1 % to +18 %** over `emc` alone, typically +10–15 %. The only extra work
is the two half-sequence factorizations behind `extras`; the whole-sequence count is computed once
and reused for the entropy, the shuffle stage and the extras. If you want more than one number, ask
for all of them at once.
<p class="lz-card__api"><code>lz.lz76(s)</code></p>
</div>
</div>

!!! tip "The factor boundaries are free — ask for them rather than the count alone."

    `lz.factorization(s)` returns `(complexity, factors)` for the same 81.5 ms that `lz.h(s)` takes
    at n = 10⁶ (89.7 ms measured, a 10 % gap that sits inside the noise floor). There is no cheaper
    count-only path to reach for.

---

## Why `emc` costs what it costs

`emc(u)` does one factorization of `u`, then for **every** block size `l = 1…mm` it clones the
sequence, applies n/2 block-swap steps, and factorizes the result.

<div class="lz-formula">
  <p class="lz-math"><i>t</i><sub>emc</sub> ≈ <i>t</i><sub>fac</sub>(<i>u</i>) + Σ<sub><i>l</i> ≤ <i>mm</i></sub> [ <i>t</i><sub>shuffle</sub>(<i>n</i>, <i>l</i>) + <i>t</i><sub>fac</sub>(shuffle<sub><i>l</i></sub>(<i>u</i>)) ]</p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd>maximum block size, Θ(log <i>n</i>) — 17 to 26 at realistic lengths</dd>
    <dt><i>t</i><sub>shuffle</sub></dt><dd><i>n</i>/2 block swaps, each drawing at least two ChaCha8 values</dd>
    <dt><i>t</i><sub>fac</sub>(shuffle<sub><i>l</i></sub>(<i>u</i>))</dt><dd>the factorization cost of a <i>randomised</i> sequence of the same length and alphabet — not of <i>u</i></dd>
  </dl>
</div>

In words: **a single `emc` call performs on the order of twenty complete factorizations, on top of
thirteen million random block swaps at n = 10⁶.**

`mm` is `max_block_size(n) + 10` once n > 50, and `max_block_size` solves `n ≈ m·2^m` by fixed-point
iteration:

| n | `max_block_size(n)` | `mm` used |
|---:|---:|---:|
| < 10 | 1 | 1 |
| 10 | 2 | 2 |
| 50 | 4 | 4 |
| 51 | 4 | **14** ← the `n > 50` cliff |
| 100 | 4 | 14 |
| 10³ | 7 | 17 |
| 10⁴ | 10 | 20 |
| 10⁵ | 13 | 23 |
| 10⁶ | 16 | 26 |
| 10⁷ | 19 | 29 |
| 10⁸ | 22 | 32 |

### How well the model predicts

Milliseconds. The two right-hand columns are measured; everything left of "model sum" is a component
timing.

<div class="lz-scroll lz-compare" markdown>

| corpus | n | `mm` | 1 factorization | `mm` factorizations | `mm` shuffle-copies | model sum | measured 1 t | measured 16 t |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `random_binary` | 10³ | 17 | 0.075 | 1.32 | 0.342 | 1.73 | 2.19 | 0.467 |
| `random_binary` | 10⁴ | 20 | 0.722 | 14.0 | 4.17 | 18.9 | 19.8 | 3.51 |
| `random_binary` | 10⁵ | 23 | 7.43 | 145.5 | 47.1 | 200.0 | 237.6 | 33.9 |
| `random_binary` | 10⁶ | 26 | 81.5 | 2 233 | 565 | 2 879 | 3 184 | 1 251 |
| `dna` | 10⁶ | 26 | 88.6 | 2 252 | 528 | 2 868 | 2 826 | 1 372 |
| `repetitive` | 10⁶ | 26 | 26.3 | 806 | 552 | 1 385 | 2 257 | 1 175 |
| `english` | 10⁶ | 26 | 63.7 | 1 663 | 571 | 2 298 | 3 208 | 1 283 |

</div>

The model is excellent for `dna` only: 2 868 predicted against 2 826 measured at n = 10⁶, an
over-prediction of 1.5 %. For `random_binary` — also i.i.d. — it *under*-predicts by 5–26 % of the
predicted value. Do not quote "within 1 %" as a general result; it holds for one corpus.

At n = 10⁶ it under-predicts `repetitive` and `english` much more badly — by 63 % and 40 % of the
predicted value — and the reason is the third row of the formula key. Shuffling an i.i.d. sequence is
statistically a no-op, so the cost is unchanged. Shuffling a *structured* sequence destroys the
structure, and the factorization of the result is far more expensive.

At n = 10⁶, shuffled at the mid-range block size `l = 13` (= `mm`/2):

| corpus | `C_LZ(u)` | `C_LZ` shuffled | factorize `u` | factorize shuffled |
|---|---:|---:|---:|---:|
| `random_binary` | 50 746 | 50 770 | 80.2 ms | 80.7 ms |
| `dna` | 99 030 | 99 051 | 87.3 ms | 86.9 ms |
| `repetitive` | **7** | **28 206** | **28.7 ms** | **63.1 ms** |
| `english` | 51 883 | 115 991 | 60.4 ms | 72.2 ms |

The block size matters, and one row cannot stand for the sweep. Over the full `l = 1…26` the shuffled
count runs 50 745 → 50 819 for `random_binary` (flat, as expected), 4 272 → 49 713 for `repetitive`
and 89 916 → 205 993 for `english`. Both structured corpora peak at `l = 1`, where the shuffle is
most destructive; `english` then falls monotonically to its minimum at `l = mm`, while `repetitive`
oscillates, dipping at block sizes that divide its 32-symbol motif (its minimum, 4 272, is at
`l = 16`).

The shuffle stage itself is nearly linear in `mm`, not quadratic, even though each step swaps `l`
bytes: at n = 10⁶ the whole sweep of `l = 1…mm` costs **528–571 ms across all four corpora**, an
average of ~43 ns per step. The work is content-independent by construction — `shuffle_copy_seeded`
performs exactly n/2 swaps whatever the bytes are — so the 8 % spread across corpora is measurement
noise, not signal. At n = 10⁶ the factorization stage outweighs the shuffle
stage roughly 4:1 on random data and 1.5:1 on repetitive data.

!!! note "You pay for `mm` factorizations, and all `mm` of them reach the answer."

    Every block size enters the total. The estimator forms one rung `Ê(l) = l · g · (C_LZ(shuffled at
    l) − C_LZ(original))` per block size, projects the whole ladder onto the non-negative
    non-decreasing cone, and reads the value off the top of the projection — so a scale that violates
    monotonicity is pooled with its neighbours rather than discarded, and the per-scale `summands` are
    the increments of the fitted ladder. Full treatment on
    [Effective measure complexity](../concepts/emc.md).

    Versions up to 1.0.2 summed the first differences instead, which telescopes: the total then
    reduced exactly to `mm · g · (C_LZ(shuffled at mm) − C_LZ(original))` and the other `mm − 1`
    factorizations were computed and then algebraically cancelled. **That is the one respect in which
    the intermediate scales used to be wasted work; they are not any more.** The cost model below is
    unchanged either way — the projection is a single linear pass over `mm` floats and does not show
    up in any timing on this page.

    A **constant** input returns exactly `0.0` — both counts are 1, so every rung is zero. A long
    **i.i.d.** input also returns exactly `0.0`, because shuffling does not change its complexity and
    the whole ladder sits at or below zero: on 10⁵ random bits the value is `0.0`. A **periodic**
    input does not: `lz.emc("01" * 500)[0]` is `2.033`, because shuffling destroys the period and
    `C_LZ` jumps from 2 to 14.

### Parallel behaviour

`emc` fans out over `mm` rayon tasks; `nid` over 4. Minimum of the per-run medians, milliseconds:

<div class="lz-scroll lz-compare" markdown>

| corpus | n | `emc` 16 t | `emc` 1 t | speed-up | `nid` 16 t | `nid` 1 t | speed-up |
|---|---:|---:|---:|---:|---:|---:|---:|
| `random_binary` | 10³ | 0.467 | 2.19 | 4.7× | 0.230 | 0.610 | 2.7× |
| `random_binary` | 10⁴ | 3.51 | 19.8 | 5.6× | 1.60 | 4.67 | 2.9× |
| `random_binary` | 10⁵ | 33.9 | 237.6 | **7.0×** | 15.3 | 46.7 | 3.1× |
| `random_binary` | 10⁶ | 1 251 | 3 184 | **2.5×** | 254 | 614 | 2.4× |
| `repetitive` | 10⁵ | 27.0 | 186.3 | 6.9× | 7.43 | 19.0 | 2.6× |
| `repetitive` | 10⁶ | 1 175 | 2 257 | **1.9×** | 107 | 275 | 2.6× |
| `dna` | 10⁶ | 1 372 | 2 826 | 2.1× | 276 | 678 | 2.5× |
| `english` | 10⁶ | 1 283 | 3 208 | 2.5× | 175 | 382 | 2.2× |

</div>

`emc`'s parallel speed-up peaks at ~5–7× around n = 10⁵ and **collapses to ~2× at n = 10⁶**. The
bound is the thread count, not `mm`: 26 tasks are submitted but at most 16 are live, and each live
task holds its own shuffled copy plus divsufsort's working set and the `sa`/`lcp`/`rank` arrays. Peak
RSS (`VmHWM`) of a single `emc(n = 10⁶)` call, across four repeats:

| `RAYON_NUM_THREADS` | peak RSS |
|---:|---:|
| 1 | 32 MB (stable) |
| 4 | 82–96 MB |
| 16 | **247–285 MB** |

The multi-threaded figures vary run to run by ~15 %, because how many of the 26 tasks are live at
once is a scheduling accident. That is ~16–18 MB of live working set per worker, and a quarter of a
gigabyte in aggregate against a 16 MiB L3. At n = 10⁵ the aggregate is ~28 MB, close enough to L3
that scaling stays good. Attributing the slowdown specifically to memory bandwidth rather than
latency or allocator contention is an inference — no hardware counters were collected — but the RSS
numbers themselves are measured.

`nid`'s ceiling is 4× by construction (a `rayon::join` of four tasks) and it reaches 2.0–3.1× in
practice.

**Practical consequence: `emc` at n ≥ 10⁶ is memory-bound, and adding cores past about 4 buys
little.** If you need `emc` over many sequences, parallelise across sequences rather than relying on
the intra-call parallelism. [Comparing many sequences](../recipes/batch-distance.md) shows the
pattern.

---

## The `SORT_CUTOFF = 2048` crossover

`build_suffix_array` uses a plain comparison suffix sort below 2048 bytes and `cdivsufsort` at or
above it. Median of 200, milliseconds:

<div class="lz-scroll lz-compare" markdown>

| corpus | n | comparison sort | divsufsort | ratio |
|---|---:|---:|---:|---:|
| `random_binary` | 256 | 0.0093 | 0.146 | 0.06 |
| `random_binary` | 1 024 | 0.050 | 0.168 | 0.30 |
| `random_binary` | **2 047** | 0.138 | 0.199 | 0.69 |
| `random_binary` | **2 048** | 0.131 | 0.198 | 0.66 |
| `random_binary` | 4 096 | 0.300 | 0.274 | 1.09 |
| `random_binary` | 16 384 | 1.34 | 0.704 | 1.91 |
| `repetitive` | 1 024 | 0.144 | 0.150 | 0.96 |
| `repetitive` | **2 047** | **0.408** | 0.159 | **2.57** |
| `repetitive` | 4 096 | 1.17 | 0.176 | 6.63 |
| `repetitive` | 16 384 | **13.70** | 0.309 | **44.4** |
| `english` | 2 047 | 0.134 | 0.182 | 0.74 |
| `english` | 16 384 | 1.29 | 0.557 | 2.31 |

</div>

**What the cutoff buys: it avoids divsufsort's fixed ~0.12–0.15 ms setup cost on small inputs.** The
mechanism is checkable in the vendored C — `BUCKET_A_SIZE = 256` and `BUCKET_B_SIZE = 256 × 256` are
zeroed in plain loops on every call, 65 536 entries or 256 KB, before any work begins. At n = 256 the
comparison sort is ~16× faster as a result. If you call `factorization` on thousands of short
sequences, that constant is the whole difference.

**What the cutoff does not buy: a good answer for structured input.** It is tuned for random data. On
the `repetitive` corpus divsufsort is already 2.6× faster *at* the cutoff, and the comparison sort's
O(n² log n) worst case is visible immediately above it. An adversarial sweep at n = 2047 through
`build_suffix_array` gives 0.46 ms for `(ab)*`, 0.40 ms for a period-32 tiling and 0.39 ms for a
Fibonacci word, against 0.03 ms for `aⁿ` (nearly sorted, and therefore easy). Worst observed is
~0.5 ms, so the choice is safe — it is not optimal for structured input.

!!! note "The cutoff cannot change a result."

    The suffix array of a string is unique: the n suffixes are pairwise distinct, so their
    lexicographic order is a total order with no ties. Both branches compute that same order, so LCP,
    LPF, boundaries and counts are bit-identical either side of 2048. An in-tree fuzz test
    (`fuzz_sa_matches_reference`) asserts it on 300 random inputs of length 1–5000 over five
    alphabets, spanning both branches.

---

## Python binding overhead

Measured head-to-head on byte-identical inputs — the same file read by a Rust binary and by Python —
against a release wheel built with maturin 1.14.1 on CPython 3.14.6.

<div class="lz-scroll lz-compare" markdown>

| n | op | Rust core | Python API | overhead |
|---:|---|---:|---:|---:|
| 1 000 | `factorization` | 0.062 ms | 0.064 ms | +3 % |
| 1 000 | `h` | 0.061 ms | 0.058 ms | −5 % (noise) |
| 1 000 | `nid` | 0.203 ms | 0.238 ms | +17 % (≈35 µs, noise) |
| 1 000 | `emc` | 0.409 ms | 0.405 ms | −1 % |
| 10 000 | `factorization` | 0.651 ms | 0.692 ms | +6 % |
| 100 000 | `factorization` | 6.47 ms | 6.90 ms | +7 % |
| 100 000 | `emc` | 33.4 ms | 33.9 ms | +2 % |
| 1 000 000 | `factorization` | 80.0 ms | 76.9 ms | −4 % |
| 1 000 000 | `h` | 87.0 ms | 76.1 ms | −13 % (noise) |
| 1 000 000 | `nid` | 273.6 ms | 261.4 ms | −4 % |
| 1 000 000 | `emc` | 1 311.8 ms | 1 337.7 ms | +2 % |

</div>

Every gap is under ±15 %, and the sign is not systematic — Python comes out *faster* on four of the
eleven rows. The signs are not stable either: a repeat of the n = 10³ `nid` row measured 0.238 ms in
Rust against 0.216 ms in Python, reversing the +17 % into −9 %. **The binding overhead is not
measurable above the noise: the Python API is the Rust core.**

The fixed per-call floor is **~0.7 µs**. Independent medians-of-5000 of `lz.factorization("01")` on
an idle machine landed between 0.65 and 0.75 µs across two sessions. `str` and `bytes` inputs cost
the same to within noise.

!!! warning "Passing `jobs=` to any Python function changes nothing — the argument is accepted and discarded."

    Every Python function takes `jobs: int = 0` and immediately drops it (`let _ = jobs;`). The
    docstring says "Reserved for future use". To control threading from Python, set the
    `RAYON_NUM_THREADS` environment variable **before importing** the module. The CLI's `-j/--jobs`
    flag is real — both binaries call `ThreadPoolBuilder::build_global()`. See
    [Python API](../api/python.md).

!!! warning "Running `lzcomplexity` in several Python threads gives no speed-up at all."

    The binding does not release the GIL — `Python::with_gil` is used, but there is no
    `py.allow_threads`. Verified: four `lz.factorization()` calls on a 100 kB input take **26.05 ms**
    back-to-back and **26.47 ms** spread across four `threading.Thread`s, a speed-up of **0.98×**.
    Use `multiprocessing` or a process pool instead. The intra-call rayon parallelism inside `nid`
    and `emc` still works.

!!! example "Reproducing the n = 10⁵ ratios"

    Run against the 1.0.0 wheel, CPython 3.14.6, 16 threads, on the same laptop but under a different
    ambient load than the tables above.

    ```python
    import time, random, statistics
    import lzcomplexity as lz

    random.seed(11)
    s  = "".join(random.choice("01") for _ in range(100_000))
    s2 = "".join(random.choice("01") for _ in range(100_000))

    def med(fn, r=7):
        fn()                                     # warm-up, untimed
        ts = []
        for _ in range(r):
            t = time.perf_counter(); fn(); ts.append((time.perf_counter() - t) * 1000)
        return statistics.median(ts)

    f = med(lambda: lz.factorization(s))
    n = med(lambda: lz.nid(s, s2))
    e = med(lambda: lz.emc(s), 3)
    print("fac %.2f  nid %.2f (%.1fx)  emc %.2f (%.1fx)" % (f, n, n / f, e, e / f))
    ```

    <div class="lz-run" markdown>

    ```console
    $ python3 bench.py
    fac 7.91  nid 16.85 (2.1x)  emc 39.87 (5.0x)
    ```

    </div>

    Against the wall-clock table: `factorization` 7.43, `nid` 16.3, `emc` 37.4. Every value lands
    within 7 % of the tabulated median, and both ratios fall in the quoted bands.

---

<hr class="lz-tickrule">

## Against the naive quadratic implementations

This is the practical reason to reach for this library. Nearly every LZc implementation in the
EEG/MEG and consciousness literature is a transcription of the Kaspar & Schuster (1987) pointer
loop — an `i / j / k / k_max` scan that restarts from position 0 after every emitted factor. Two
concrete examples: NeuroKit2's `_complexity_lempelziv_count` runs it in pure Python, and antropy's
`_lz_complexity` runs the same loop numba-JIT compiled. antropy is therefore the strongest form of
the naive approach, and the fair thing to benchmark against.

### The cost of that loop

<div class="lz-formula">
  <p class="lz-math">naive work ≈ 0.95 · <i>c</i>(<i>S</i>) · <i>n</i> ≈ <i>n</i>² ⁄ log<sub>2</sub> <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>the LZ76 factor count, which itself grows like <i>n</i> ⁄ log<sub><i>k</i></sub> <i>n</i></dd>
    <dt>0.95</dt><dd>fitted constant, accurate to ~10 % across four decades</dd>
  </dl>
  <p class="lz-formula__cite">Measured inner-iteration counts: 93 290 at n = 10³, 7 150 999 at 10⁴, 580 769 053 at 10⁵, 48 726 095 291 at 10⁶. Ratios to c·n: 0.89 / 0.93 / 0.95 / 0.96.</p>
</div>

The true growth is Θ(n² / log n), not pure Θ(n²) — `iters / n²` drifts *down* with n
(0.0933 → 0.0487 over those four decades) precisely because `c(S) ~ n / log₂ n`.

### Head to head

Both implementations compiled with `rustc -O` and run on identical random binary sequences in the
same process, so this isolates the algorithm rather than Python versus Rust.

<div class="lz-scroll lz-compare" markdown>

| n | naive (compiled) | `lzcomplexity` | speed-up | `c` naive / `c` here |
|---:|---:|---:|---:|---|
| 10³ | 0.31 ms | 0.12 ms | **2.6×** | 105 / 104 |
| 10⁴ | 24.4 ms | 0.96 ms | **25.3×** | 768 / 767 |
| 10⁵ | 1 965 ms | 8.44 ms | **233×** | 6 110 / 6 109 |
| 10⁶ | **172.4 s** | 112.6 ms | **1 531×** | 50 759 / 50 759 |
| 10⁷ | ≈ 4.1 h *(extrapolated)* | 1.57 s | **≈ 9 400×** | — |

</div>

The `c` column and the iteration counts reproduce bit-identically on re-run. The *times* do not: a
second run measured 2.78–2.95 ns per naive iteration against 3.34–3.54 ns, giving speed-ups of ≈4×,
34×, 280× and 1 800×. Quote these **to one significant figure and as lower bounds: ~3× at 10³, ~25×
at 10⁴, ~230× at 10⁵, ~1 500× at 10⁶.** The single extrapolated row uses the measured factor count
c(10⁷) = 433 968 together with the run-1 per-iteration constant; that model predicted the n = 10⁶
runtime as 173 s against a measured 172.4 s, a 0.3 % error, so the extrapolation is trustworthy for
that run. With run 2's constant it would be ≈3.2 h.

The pure-Python version is a further ~60× slower per iteration — 156 ns at n = 10³ rising to 239 ns
at n = 2 × 10⁵, so there is no single constant. Measured end to end: 123.4 s at n = 10⁵ and 525.6 s
at n = 2 × 10⁵.

### Why this matters

These lengths are not hypothetical. One EEG channel at 1 kHz for ten minutes is 6 × 10⁵ samples; the
standard "concatenate across channels, then binarise" LZc protocol at 64 channels gives 3.8 × 10⁷
symbols per recording. See [EEG and neural time-series](../recipes/neuro.md).

| task | pure Python | compiled naive | `lzcomplexity` |
|---|---|---|---|
| 500-surrogate test at n = 10⁶ | ≈ 62 days | ≈ 24 hours | **≈ 56 seconds** |
| one n = 10⁷ recording | ≈ 10 days | ≈ 4 hours | **1.6 seconds** |

**The quadratic implementations are not slow because they are written in Python. They are slow
because they are quadratic.** Compiling them buys ~60×, which is a constant. Replacing
Θ(c·n) = Θ(n²/log n) with O(n log n) buys a factor that grows with n.

!!! note "The outputs agree, up to one documented convention."

    The `c` column above differs by at most 1, always in the same direction. antropy and NeuroKit2
    both end with `if len_substring != 1: complexity += 1`, which counts a trailing incomplete
    component; this library counts complete productions only. The conversion is
    `c_textbook = c + (1 if factors[-1] > len(seq) else 0)`.

    Checked exhaustively against a transcription of antropy's loop on all **524 284** binary strings
    of length 2–18: Δ is 1 on 425 554 of them and 0 on 98 730 (18.8 %). The conversion holds on every
    string **except the 34 constant ones** (`"0"*L` and `"1"*L`), where the library's single-symbol
    early exit returns `c = 1` with boundaries `[0, 1, L]` while the textbook loop returns 2. Do not
    use the conversion on constant input; special-case it. The factor *boundaries* always agree. Full
    treatment on [LZ76 factorization](../concepts/lz76.md).
