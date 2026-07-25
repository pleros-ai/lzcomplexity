---
hide:
  - navigation
  - toc
---

<section class="lz-hero" markdown>

<p class="lz-eyebrow">Lempel–Ziv 76 · Rust core · v1.0</p>

<h1 class="lz-hero__title">
  Measure the structure<br><span>in the noise.</span>
</h1>

<p class="lz-hero__sub">
  <code>lzcomplexity</code> factorises any symbolic sequence with Lempel–Ziv 76 and returns
  complexity, entropy rate, effective measure complexity and information distance. A Rust core, a
  Python API of five functions, and two command-line tools — linear-time and reproducible run to run.
</p>

<p class="lz-hero__cta">
  <a class="lz-btn lz-btn--primary" href="guide/install/">Get started <span aria-hidden="true">→</span></a>
  <a class="lz-btn lz-btn--ghost" href="api/python/">API reference</a>
  <a class="lz-btn lz-btn--bare" href="https://github.com/pleros-ai/lzcomplexity">GitHub</a>
</p>

<div class="lz-cmd"><span class="lz-cmd__dollar">$</span><code>pip install lzcomplexity</code></div>

<div class="lz-band-scroll">
  <svg class="lz-band" viewBox="0 0 1160 250" role="img"
       aria-labelledby="lzband-t lzband-d" preserveAspectRatio="xMidYMid meet">
    <title id="lzband-t">LZ76 factorization of a 72-symbol binary sequence</title>
    <desc id="lzband-d">A sequence that begins perfectly periodic and ends random. LZ76 cuts it
      into 10 factors: long factors of 20 and 18 symbols on the ordered left side, then
      progressively shorter factors of 8, 3, 5, 9 and 5 symbols as the sequence becomes random.
      Complexity c(S) = 10, entropy density h = 0.857.</desc>

    <defs>
      <linearGradient id="lz-ramp" gradientUnits="userSpaceOnUse" x1="40" y1="0" x2="1120" y2="0">
        <stop offset="0"    style="stop-color: var(--lz-viz-1)"/>
        <stop offset="0.30" style="stop-color: var(--lz-viz-2)"/>
        <stop offset="0.62" style="stop-color: var(--lz-viz-3)"/>
        <stop offset="1"    style="stop-color: var(--lz-viz-4)"/>
      </linearGradient>
    </defs>

    <g class="lz-band__grid" stroke="var(--lz-border)" stroke-width="1">
      <path d="M40 18V214M160 18V214M280 18V214M400 18V214"/>
      <path d="M517 18V214M604 18V214M651 18V214M728 18V214M769 18V214M851 18V214
               M887 18V214M962 18V214M1001 18V214M1043 18V214M1090 18V214"/>
    </g>

    <rect x="40" y="24" width="1080" height="8" rx="4" fill="url(#lz-ramp)" opacity="0.85"/>
    <text class="lz-band__cap" x="40"   y="16">ordered</text>
    <text class="lz-band__cap lz-band__cap--end" x="1120" y="16">random</text>

    <g class="lz-band__stream" fill="var(--lz-text)">
      <text class="lz-f lz-f1"  x="43"   y="86" textLength="15"  lengthAdjust="spacing">0</text>
      <text class="lz-f lz-f2"  x="58"   y="86" textLength="15"  lengthAdjust="spacing">1</text>
      <text class="lz-f lz-f3"  x="73"   y="86" textLength="300" lengthAdjust="spacing">01010101010101010100</text>
      <text class="lz-f lz-f4"  x="373"  y="86" textLength="30"  lengthAdjust="spacing">11</text>
      <text class="lz-f lz-f5"  x="403"  y="86" textLength="270" lengthAdjust="spacing">001100110011001101</text>
      <text class="lz-f lz-f6"  x="673"  y="86" textLength="120" lengthAdjust="spacing">10100111</text>
      <text class="lz-f lz-f7"  x="793"  y="86" textLength="45"  lengthAdjust="spacing">000</text>
      <text class="lz-f lz-f8"  x="838"  y="86" textLength="75"  lengthAdjust="spacing">10111</text>
      <text class="lz-f lz-f9"  x="913"  y="86" textLength="135" lengthAdjust="spacing">010011101</text>
      <text class="lz-f lz-f10" x="1048" y="86" textLength="75"  lengthAdjust="spacing">10010</text>
    </g>

    <g class="lz-band__brackets" fill="none" stroke="url(#lz-ramp)" stroke-width="2"
       stroke-linecap="square">
      <path class="lz-b lz-b1"  pathLength="1" d="M42 108V120H53V108"/>
      <path class="lz-b lz-b2"  pathLength="1" d="M57 108V120H68V108"/>
      <path class="lz-b lz-b3"  pathLength="1" d="M72 108V120H368V108"/>
      <path class="lz-b lz-b4"  pathLength="1" d="M372 108V120H398V108"/>
      <path class="lz-b lz-b5"  pathLength="1" d="M402 108V120H668V108"/>
      <path class="lz-b lz-b6"  pathLength="1" d="M672 108V120H788V108"/>
      <path class="lz-b lz-b7"  pathLength="1" d="M792 108V120H833V108"/>
      <path class="lz-b lz-b8"  pathLength="1" d="M837 108V120H908V108"/>
      <path class="lz-b lz-b9"  pathLength="1" d="M912 108V120H1043V108"/>
      <path class="lz-b lz-b10" pathLength="1" d="M1047 108V120H1118V108"/>
    </g>

    <g class="lz-band__idx" fill="url(#lz-ramp)">
      <text class="lz-l lz-l1"  x="47"   y="140">1</text>
      <text class="lz-l lz-l2"  x="62"   y="140">2</text>
      <text class="lz-l lz-l3"  x="220"  y="140">3 · 20 symbols</text>
      <text class="lz-l lz-l4"  x="385"  y="140">4</text>
      <text class="lz-l lz-l5"  x="535"  y="140">5 · 18 symbols</text>
      <text class="lz-l lz-l6"  x="730"  y="140">6 · 8</text>
      <text class="lz-l lz-l7"  x="812"  y="140">7</text>
      <text class="lz-l lz-l8"  x="872"  y="140">8</text>
      <text class="lz-l lz-l9"  x="977"  y="140">9</text>
      <text class="lz-l lz-l10" x="1082" y="140">10</text>
    </g>

    <g class="lz-band__axis" stroke="var(--lz-border-strong)" stroke-width="1">
      <path d="M40 176H1120"/>
      <path d="M40 176v6M400 176v6M760 176v6M1120 176v6"/>
    </g>
    <g class="lz-band__cap" fill="var(--lz-muted)">
      <text x="40"   y="198">n = 0</text>
      <text x="400"  y="198">24</text>
      <text x="760"  y="198">48</text>
      <text class="lz-band__cap--end" x="1120" y="198">72</text>
    </g>
  </svg>
</div>

<div class="lz-readout">
  <div class="lz-readout__item">
    <span class="lz-readout__k">c(S)</span>
    <span class="lz-readout__v">10</span>
    <span class="lz-pips" aria-hidden="true"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></span>
    <span class="lz-readout__note">LZ76 factors</span>
  </div>
  <div class="lz-readout__item">
    <span class="lz-readout__k">h</span>
    <span class="lz-readout__v">0.857</span>
    <span class="lz-meter" aria-hidden="true"><i style="--v:85.7%"></i></span>
    <span class="lz-readout__note">entropy density</span>
  </div>
  <div class="lz-readout__item">
    <span class="lz-readout__k">n</span>
    <span class="lz-readout__v">72</span>
    <span class="lz-readout__note">symbols, alphabet 2</span>
  </div>
</div>

</section>

<div class="lz-tickrule"></div>

## Four measures, one factorization

<div class="lz-cards" markdown>

<div class="lz-card" markdown>
### Complexity

The number of LZ76 factors — how many distinct "new" pieces it takes to build the sequence from its
own past. Computed from a suffix array and a longest-previous-factor table, so the answer is an exact
integer, not a sample.

<p class="lz-card__api"><code>lz.factorization(seq)</code></p>
</div>

<div class="lz-card" markdown>
### Entropy rate

A non-parametric estimator of the source's entropy rate. No model, no bin count, no window — the
factor count and the length are enough. Converges for stationary ergodic sources.

<p class="lz-card__api"><code>lz.h(seq)</code></p>
</div>

<div class="lz-card" markdown>
### Effective measure complexity

Structure that is neither periodic nor random. Compares the sequence against block-shuffled versions
of itself at a ladder of scales, and reports the excess over the entropy rate.

<p class="lz-card__api"><code>lz.emc(seq)</code></p>
</div>

<div class="lz-card" markdown>
### Information distance

How much of one sequence you still need after you already have the other. A compression-based
distance in the normalized-information-distance family — no alignment, no feature engineering.

<p class="lz-card__api"><code>lz.nid(a, b)</code></p>
</div>

</div>

<div class="lz-formula">
  <p class="lz-math"><i>h</i> ≈ <i>c</i>(<i>S</i>) · log<sub><i>k</i></sub>&thinsp;<i>n</i> ⁄ <i>n</i></p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(<i>S</i>)</dt><dd>number of LZ76 factors</dd>
    <dt><i>n</i></dt><dd>sequence length, in bytes</dd>
    <dt><i>k</i></dt><dd>alphabet size, auto-detected, minimum 2</dd>
  </dl>
  <p class="lz-formula__cite">Converges to the entropy rate of a stationary ergodic source — Ziv &amp; Lempel (1978), Kontoyiannis et al. (1998).</p>
</div>

## Thirty seconds

```python
import lzcomplexity as lz

lz.factorization("banana")
# (3, [0, 1, 2, 3, 7])          complexity, then factor boundaries

lz.h("01010101")
# 0.75                           normalised entropy density

lz.nid("ABRACADABRA", "ABRACADABRZ")
# 0.3333333333333333             one symbol apart, so most information is shared

lz.lz76("ABRACADABRA")["emc"]["value"]
# 0.5417804008745377             effective measure complexity
```

Every function takes `str`, `bytes`, `list[str]`, `list[int]`, or any iterable of ints. Read
[Input types](api/inputs.md) before you feed it numeric data — the conversion rule surprises people.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">5</span><span class="lz-stat__k">Python functions</span></div>
<div class="lz-stat"><span class="lz-stat__v">O(n)</span><span class="lz-stat__k">factorization</span></div>
<div class="lz-stat"><span class="lz-stat__v">12 MB/s</span><span class="lz-stat__k">incompressible data</span></div>
<div class="lz-stat"><span class="lz-stat__v">2</span><span class="lz-stat__k">CLI tools</span></div>
<div class="lz-stat"><span class="lz-stat__v">7</span><span class="lz-stat__k">input formats</span></div>
<div class="lz-stat"><span class="lz-stat__v">MIT</span><span class="lz-stat__k">licence</span></div>
</div>

Measured scaling is `t ∝ n^1.01` on random binary input, from 10³ to 10⁶ symbols. See
[Performance](project/performance.md) for the full table and the methodology.

<div class="lz-tickrule"></div>

## Where to go next

<div class="lz-cards" markdown>

<div class="lz-card" markdown>
### I have data and want a number

Start at [Install](guide/install.md), then
[Your first factorization](guide/first-factorization.md). Read
[Reading the numbers](guide/reading-the-numbers.md) before you interpret anything.
</div>

<div class="lz-card" markdown>
### I want to know what it computes

[LZ76 factorization](concepts/lz76.md) defines the measure,
[Entropy density](concepts/entropy-density.md) and [EMC](concepts/emc.md) build on it, and
[Sequence length and convergence](concepts/convergence.md) tells you when to trust the result.
</div>

<div class="lz-card" markdown>
### I work with EEG, DNA, or time-series

The recipes are written for you: [EEG and neural time-series](recipes/neuro.md),
[DNA and FASTA](recipes/genomics.md), [the route to chaos](recipes/dynamical-systems.md),
and [comparing many sequences](recipes/batch-distance.md).
</div>

<div class="lz-card" markdown>
### I want the reference

[Python API](api/python.md), the [Rust crate](api/rust.md), and the two command-line tools
[`lzcomplexity`](cli/lzcomplexity.md) and [`lzdistance`](cli/lzdistance.md).
</div>

</div>
