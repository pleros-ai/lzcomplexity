# EEG and neural time-series

*A defensible spontaneous-EEG complexity pipeline: filter, Hilbert, binarise, serialise, factorise, normalise against surrogates.*

Lempel–Ziv complexity is the most-used single scalar in the empirical study of conscious level.
Three result families established it.

- **Perturbational Complexity Index (PCI).** Casali et al. (2013) perturbed cortex with TMS,
  binarised the significant source-level response, and compressed it with LZ76. PCI separated
  wakefulness from every unconscious condition tested, with no overlap: across that study the
  maximum complexity observed during unconsciousness was 0.31 and the minimum during alert
  wakefulness 0.44.[^casali] Casarotto et al. (2016) turned that gap into a cutoff,
  **PCI\* = 0.31**, which separated conscious from unconscious conditions with 100 % sensitivity
  and 100 % specificity in a 150-subject benchmark population — a population the cutoff was
  itself fitted to, by ROC analysis, so that pair of figures is in-sample. Carried over to
  brain-injured patients, it detected the minimally conscious state with 94.7 %
  sensitivity.[^casarotto]
- **Spontaneous-EEG signal diversity (LZc / LZs).** Schartner et al. (2015) showed LZc falls
  under propofol in every participant;[^schartner15] Schartner et al. (2017) showed it *rises*
  above baseline under ketamine, LSD and psilocybin.[^schartner17] That two-sided movement is
  what makes LZ complexity a level-of-consciousness axis rather than a sedation-depth meter.
- **Consciousness, not responsiveness.** Sarasso et al. (2015) found ketamine subjects
  behaviourally unresponsive but reporting vivid experience on waking — and PCI stayed at 0.44
  (range 0.35–0.55), against 0.24 for propofol and 0.17 for xenon in the same study.[^sarasso]
  Same behavioural score, different complexity.

<div class="lz-stats">
  <div class="lz-stat"><span class="lz-stat__v">0.31</span><span class="lz-stat__k">PCI* clinical cutoff</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0.55</span><span class="lz-stat__k">PCI, alert wakefulness (±0.05)</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0.23</span><span class="lz-stat__k">PCI, loss of consciousness (±0.04)</span></div>
  <div class="lz-stat"><span class="lz-stat__v">0.44</span><span class="lz-stat__k">PCI, ketamine unresponsiveness</span></div>
</div>

The full per-condition table from Casali et al. (2013), 208 TMS/EEG measurements in 52
subjects. `N` counts measurements, not patients.

<div class="lz-scroll" markdown>

| Condition | PCI range | mean ± SD | measurements |
|---|---|---|---|
| Alert wakefulness (healthy) | 0.44 – 0.67 | 0.55 ± 0.05 | N = 110, n = 32 |
| All loss of consciousness | 0.12 – 0.31 | 0.23 ± 0.04 | N = 42 |
| NREM sleep | 0.18 – 0.28 | 0.24 ± 0.02 | — |
| Midazolam, deep sedation | 0.23 – 0.31 | 0.28 ± 0.03 | — |
| Propofol, deep | 0.13 – 0.30 | 0.23 ± 0.04 | — |
| Xenon | 0.12 – 0.31 | 0.23 ± 0.06 | — |
| Propofol, intermediate sedation | 0.34 – 0.42 | 0.39 ± 0.03 | N = 6 |
| VS/UWS (vegetative) | 0.19 – 0.31 | 0.24 ± 0.04 | N = 15, 6 patients |
| MCS (minimally conscious) | 0.32 – 0.49 | 0.39 ± 0.05 | N = 15 |
| EMCS (emerged from MCS) | 0.37 – 0.52 | 0.43 ± 0.05 | N = 14 |
| LIS (locked-in) | 0.51 – 0.62 | 0.57 ± 0.05 | N = 4 |

</div>

!!! danger "PCI is not what this page computes, and `lzcomplexity` does not implement it"

    A number from the pipeline below is **not** comparable to a published PCI value, and
    calling it one is a category error. PCI requires a TMS perturbation, a source-level inverse
    solution, a nonparametric bootstrap significance mask instead of an amplitude threshold,
    and a normalisation by source entropy rather than by a shuffled surrogate. This page covers
    *spontaneous*-EEG LZc — the Schartner family. The reference PCI implementation is
    [`iTCf/PCIcalc`](https://github.com/iTCf/PCIcalc).

<hr class="lz-tickrule">

## The pipeline

The canonical recipe is Schartner et al. (2015), step by step. It takes a channels × time array
of floats and returns one binary string.

### Step 1 — filter

Notch out mains (50 and 100 Hz in Europe, 60 and 120 Hz in North America), bandpass, downsample.
Schartner et al. (2015) used Butterworth notch filtering at 50 and 100 Hz, downsampled
1000 → 250 Hz, applied a surface Laplacian, then linearly de-trended and baseline-subtracted each
channel; they analysed non-overlapping 10 s segments over 25 selected channels. The 2017 MEG
study band-passed 1–150 Hz, downsampled to 600 Hz, and then applied a further 30 Hz low-pass
before computing complexity, to keep muscle artefact out of the envelope.

```python
import numpy as np
from scipy.signal import butter, filtfilt, hilbert
import lzcomplexity as lz

FS = 250.0  # Hz, after downsampling

def bandpass(x, fs=FS, lo=1.0, hi=45.0, order=4):
    """Zero-phase bandpass. x is (n_channels, n_samples)."""
    b, a = butter(order, [lo / (fs / 2), hi / (fs / 2)], btype="band")
    return filtfilt(b, a, x, axis=-1)
```

Use `filtfilt`, not `lfilter`. A causal filter imposes a phase gradient across frequency, and
the next step reads phase.

### Step 2 — Hilbert transform, keep the instantaneous amplitude

Take the analytic signal per channel and keep its modulus. This discards the carrier and keeps
the envelope, which is what makes the result a measure of amplitude *dynamics* rather than of
the oscillation itself.

### Step 3 — binarise about the channel's own mean envelope

The threshold is per channel: the mean of that channel's instantaneous amplitude. Below the
mean is `0`, above is `1`.

```python
def binarise(x):
    """Hilbert instantaneous amplitude, thresholded at each channel's own mean."""
    amp = np.abs(hilbert(x, axis=-1))
    return (amp > amp.mean(axis=-1, keepdims=True)).astype(np.uint8)
```

### Step 4 — serialise column by column

Read the binary matrix out **observation by observation**: all channels at *t* = 0, then all
channels at *t* = 1, and so on. This ordering is what makes LZc sensitive to spatial diversity
as well as temporal diversity. A channel-by-channel (row-major) read-out gives a different,
purely temporal quantity.

```python
def serialise(b):
    """Column-major: all channels at t=0, then all channels at t=1, ..."""
    return "".join(map(str, b.T.ravel()))
```

!!! warning "A list of ints is silently concatenated as decimal digits"

    `lz.factorization([0, 1, 10])` factorises the string `"0110"` — four symbols, not three.
    Integer sequences are converted by joining each element's decimal representation. For
    binary EEG data that is harmless, but the moment you move to a coarse-grained alphabet
    larger than 10 it corrupts the sequence with no error raised. Build a `str` of
    single characters, or pass `bytes`. See [Input types](../api/inputs.md).

### Step 5 — factorise

```python
def lzc(s):
    """Raw LZ76 factor count of a binary string."""
    return lz.factorization(s)[0]
```

`lz.factorization` returns `(complexity, factor_boundaries)`. The boundaries are indices, so
factor *i* spans `[factors[i], factors[i+1])`.

!!! danger "This library is LZ76. Schartner's published LZc is a dictionary variant"

    Schartner et al. (2017) state plainly that they apply "the encoding step of the Lempel-Ziv
    1978 (LZ78) compression algorithm"; the 2015 paper reports a "standard open source
    Lempel-Ziv compression algorithm" and notes that different implementations gave nearly
    identical outcomes. `lzcomplexity` implements **LZ76** — a different parse, a different
    count, a different denominator. Both quantities move the same way, but they are not
    interchangeable. On the wake epoch below:

    <div class="lz-scroll">

    | Parse | raw count | surrogate ⟨count⟩ | normalised | wake − anaesthesia |
    |---|---|---|---|---|
    | LZ76 — `lz.factorization` | 1 063 | 1 420.4 | 0.7484 | 0.0847 |
    | LZ78 dictionary — Schartner's `cpr` | 2 212 | 2 342.3 | 0.9444 | 0.1169 |

    </div>

    Same string, same surrogates, same ranking — and a normalised value 0.20 apart. Do not put
    a number from this page next to a published LZc and call the difference an effect. If you
    need bit-comparable LZc values, run Schartner's own code.

!!! note "`complexity` counts complete components only"

    The library counts **complete** LZ76 components. When the sequence ends part-way through a
    component, that trailing incomplete one is not counted, so the library's count is one less
    than the textbook exhaustive-history count whenever the sequence ends mid-component. The
    conversion:

    ```python
    c, factors = lz.factorization(s)
    c_textbook = c + (1 if factors[-1] > len(s) else 0)
    ```

    Checked against a reference exhaustive-history parse over all 131 070 binary strings of
    length 1–16, this is right for every string except a constant one (`"0000"` and friends),
    where the library reports `c = 1` with `factors[-1] == len(s)` and the textbook count is 2.
    Real EEG strings are not constant.

    On the 20 000-symbol epoch used below: `c = 1063` and `factors[-1] = 20001 > 20000`, so
    `c_textbook = 1064`. That is a 0.09 % difference here, and 20 % or more on a ten-symbol toy
    string. It matters when you reproduce a hand-worked example. It nearly cancels in a ratio —
    recomputing LZc<sub>N</sub> from textbook counts moves it from 0.7484 to 0.7486. See
    [LZ76 factorization](../concepts/lz76.md).

### Step 6 — normalise against shuffled surrogates

Divide the raw count by the mean count over randomly shuffled copies of the *same* string.

<div class="lz-formula">
  <p class="lz-math">LZc<sub>N</sub> = <i>c</i>(<i>s</i>) ⁄ ⟨ <i>c</i>(shuffle(<i>s</i>)) ⟩</p>
  <dl class="lz-formula__key">
    <dt><i>c</i>(·)</dt><dd>LZ76 factor count — <code>lz.factorization(s)[0]</code></dd>
    <dt><i>s</i></dt><dd>the serialised binary string</dd>
    <dt>shuffle</dt><dd>uniform random permutation of the symbols of <i>s</i>; preserves length and 1-fraction, destroys all structure</dd>
  </dl>
  <p class="lz-formula__cite">Schartner et al., PLOS ONE 10(8): e0133532 (2015): the raw value divided by the value obtained for the same binary sequence randomly shuffled.</p>
</div>

```python
def lzc_normalised(s, n_surrogates=20, seed=0):
    """Schartner-style normalisation: raw count / mean count over shuffled copies."""
    rng = np.random.default_rng(seed)
    buf = np.frombuffer(s.encode(), dtype=np.uint8).copy()
    surrogates = []
    for _ in range(n_surrogates):
        rng.shuffle(buf)
        surrogates.append(lzc(buf.tobytes().decode()))
    return lzc(s) / float(np.mean(surrogates))
```

The shuffle has to be yours. `lzcomplexity` does shuffle internally, but only inside
[`lz.emc()`](../concepts/emc.md), which block-shuffles at many scales for a different purpose
and returns entropy terms rather than the surrogate factor counts themselves.

!!! warning "In Schartner et al. (2017), `LZcN` means something else"

    That paper reserves the subscript N — `LZcN`, `LZsN`, `ACEN`, `SCEN` — for normalisation by
    **phase-randomised** surrogates, which preserve the power spectrum; the symbol shuffle is
    the separate, unsubscripted normalisation carried over from 2015. This page writes
    LZc<sub>N</sub> for the shuffle ratio throughout, matching the 2015 usage. The two
    denominators are nowhere near each other. On the wake epoch below, twenty symbol shuffles
    average 1 420.4 factors, twenty phase-randomised surrogates average 1 070.4 — a
    phase-randomised surrogate keeps the power spectrum, so it stays nearly as compressible as
    the data. The same raw count of 1 063 therefore reads as **0.7484** against a shuffle and
    **0.9931** against a phase randomisation. Say which surrogate you divided by, in words,
    rather than relying on the subscript.

<hr class="lz-tickrule">

## Why the surrogate normalisation is not optional

**The raw factor count moves when the signal's 1-fraction moves, with no change in temporal
structure at all.** A shuffled surrogate has the same length and the same symbol frequencies as
your data and none of its structure, so dividing by it cancels the part of the raw count that is
bookkeeping.

Measured on two synthetic 8-channel, 250 Hz recordings — one broadband ("wake"), one dominated
by a shared 1.2 Hz oscillation ("anaesthesia") — first 10 s of each, put through the pipeline
above. Both are built by the [listing at the end of this page](#complete-listing); synthetic
data is not EEG, and the point of the table is the direction and size of the gap between the
columns, not the values themselves.

<div class="lz-scroll" markdown>

| | 1-fraction | raw `c(s)` | surrogate ⟨`c`⟩ | LZc<sub>N</sub> | `lz.h`, bits |
|---|---|---|---|---|---|
| wake | 0.4532 | 1 063 | 1 420.4 | **0.7484** | 0.7594 |
| anaesthesia | 0.6418 | 891 | 1 342.5 | **0.6637** | 0.6365 |
| change | +0.19 | −16.2 % | −5.5 % | **−11.3 %** | −16.2 % |

</div>

The raw count fell 16.2 %. But the surrogate baseline fell 5.5 % as well, purely because the
1-fraction moved from 0.45 to 0.64 and an unbalanced string is cheaper to factorise. About a
third of the apparent raw effect is that bookkeeping. The normalised drop, 11.3 %, is the part
attributable to structure.

`lz.h()` fell by exactly the same 16.2 % as the raw count, because at fixed *n* and fixed
alphabet it *is* the raw count times a constant. It carries the whole artefact. Only a
surrogate denominator removes it.

!!! warning "`lz.h()` is a different normalisation and will not reproduce published LZc values"

    A paper reporting "normalised LZ complexity" may mean any of three incompatible quantities.
    Handing your string to `lz.h()` and comparing against a published LZc is a units error.

    <div class="lz-scroll">

    | Normalisation | Formula | Where it comes from |
    |---|---|---|
    | Analytic (Ziv asymptote) | `c(s) / (n / log_k n)` | **this is `lz.h()`** |
    | Source entropy | `c · log₂L / (L·H(L))`, `H` = binary entropy of the 1-fraction | PCI — Casali et al. Eq. 2 |
    | Shuffled surrogate | `c(s) / ⟨c(shuffle(s))⟩` | Schartner LZc / LZs — the recipe on this page |

    </div>

    Only the shuffled surrogate measures its denominator on a string of your string's own
    length and symbol frequencies, so it is the only one of the three that cancels the
    finite-*n* and 1-fraction bookkeeping. It is not therefore *unbiased* — it is a ratio of two
    random factor counts, and the expectation of a ratio is not the ratio of expectations.
    `lz.h()` converges from *above* and is still +0.012 too high for an i.i.d. fair coin at
    *n* = 10⁶; see
    [Sequence length and convergence](../concepts/convergence.md). The table above prints both
    for the same data, and they disagree: 0.7484 against 0.7594 for the wake epoch.

!!! tip "20 surrogates is already more than enough at EEG epoch lengths"

    Twenty shuffles of the 20 000-symbol wake epoch gave a surrogate count of 1420.40 ± 2.98
    (sd; range 1415–1427). One shuffle therefore pins the denominator to 0.21 % (that is the
    relative sd, not the standard error), and the mean of twenty to 0.05 %, against an 11 %
    effect. Taking the extreme single surrogate either way, wake lands in 0.7449–0.7512 and
    anaesthesia in 0.6590–0.6679: even one shuffle resolves the contrast. Surrogate count
    matters at *n* ≲ 10³, not at *n* ≳ 10⁴.

<hr class="lz-tickrule">

## Per-channel or concatenated?

Schartner et al. (2017) name both, and they measure different things.

<div class="lz-cards" markdown>
<div class="lz-card" markdown>
### LZs — per channel
Factorise each channel's binary row independently, then average across channels. Measures
**temporal** diversity: how varied one site's envelope is over time. Blind to whether channels
move together.
<p class="lz-card__api"><code>mean(lzc_normalised(row) for row in b)</code></p>
</div>
<div class="lz-card" markdown>
### LZc — concatenated
Serialise the whole channels × time matrix column by column and factorise once. Measures
**spatio-temporal** diversity: repeated spatial patterns compress, so channels moving in
lockstep lower the score.
<p class="lz-card__api"><code>lzc_normalised(serialise(b))</code></p>
</div>
</div>

The choice is not cosmetic. On the same two recordings:

<div class="lz-scroll" markdown>

| | mean LZs<sub>N</sub> (per channel) | LZc<sub>N</sub> (concatenated) |
|---|---|---|
| wake | 0.5737 | 0.7484 |
| anaesthesia | 0.5616 | 0.6637 |
| difference | **0.0122** | **0.0847** |

</div>

The manipulation here was a *shared* slow oscillation added to every channel. Single-channel
LZs barely notices — each channel's envelope is still varied over time. Concatenated LZc drops
about seven times as much, because the spatial pattern at each time point became repetitive.

Choose by hypothesis, and say which you used:

- **A drug or state that changes local signal richness** → LZs. Schartner et al. (2017) found
  the psychedelic increase was most pronounced for single-channel LZs.
- **A manipulation of integration or spatial differentiation** → LZc.
- **Both** → report both. They are cheap and they are not redundant.

!!! danger "LZc is not comparable across montages"

    Changing the channel count changes the sequence length, the set of possible spatial
    patterns and the surrogate baseline all at once. A 60-channel LZc and a 19-channel LZc from
    the same subject are different measurements, not two estimates of one thing. Absolute LZ
    values do not transfer between labs. **Compare within subject**, with the same montage,
    reference, epoch length and binarisation.

<hr class="lz-tickrule">

## Sampling rate and epoch length

**Do not compare LZ values computed from epochs of different length.** The raw count grows with
*n*, and the normalised value drifts downward. Measured on the wake recording, epochs cut from
the same continuous data:

<div class="lz-scroll" markdown>

| epoch | *n* symbols | raw `c(s)` | surrogate ⟨`c`⟩ | LZc<sub>N</sub> | `lz.h`, bits |
|---|---|---|---|---|---|
| 2 s | 4 000 | 265 | 343.7 | 0.7710 | 0.7927 |
| 5 s | 10 000 | 581 | 767.1 | 0.7573 | 0.7720 |
| 10 s | 20 000 | 1 063 | 1 420.4 | 0.7484 | 0.7594 |
| 20 s | 40 000 | 1 930 | 2 650.8 | 0.7281 | 0.7376 |
| 40 s | 80 000 | 3 554 | 4 967.9 | 0.7154 | 0.7236 |

</div>

The raw count rises 13-fold across that range. Normalisation removes most of the length
dependence but not all of it: LZc<sub>N</sub> still drifts 0.7710 → 0.7154, a change of 0.056
from epoch length alone — comparable to the 0.085 wake/anaesthesia effect this page is trying
to detect. Fix the epoch length across every condition and every subject in a study, and state
it in the methods.

**Practical sizing.** A 10 s epoch at 250 Hz over 60 channels is *n* = 150 000 symbols,
comfortably asymptotic. Schartner et al. (2017) used 2 s epochs at 600 Hz over 90 source
channels — 108 000 symbols. Two seconds at 250 Hz over 8 channels is 4 000 symbols, which the
table above shows is already in the biased regime. If your montage is small, buy the length
back with a longer epoch.

!!! note "How short is too short — the effective-length rule"

    Lesne, Blanc & Pezard (2009) found that LZ76 error bars behave as if the sequence had
    **effective length `N_eff = N·h / ln k`**. With the library's default `log_base` (the
    alphabet size) the `ln k` cancels exactly and this reduces to `N_eff ≈ N · lz.h(seq)`. For
    the 10 s wake epoch that is 20 000 × 0.7594 ≈ 15 200. A low-entropy epoch — deep
    anaesthesia, burst suppression — is statistically far shorter than its symbol count
    suggests. Details in
    [Sequence length and convergence](../concepts/convergence.md).

<hr class="lz-tickrule">

## Pitfalls

### Binarisation threshold changes the answer

**Papers routinely leave the threshold unstated, and it changes the number.** Mean of the
Hilbert amplitude (Schartner), median of the Hilbert amplitude, mean or median of the raw
signal, a statistical significance mask (Casali) and a generating partition (Lesne) all give
different answers on identical data. Measured on the same wake epoch:

<div class="lz-scroll" markdown>

| Binarisation | 1-fraction | raw `c(s)` | LZc<sub>N</sub> |
|---|---|---|---|
| Hilbert amplitude vs its mean *(Schartner)* | 0.453 | 1 063 | 0.7484 |
| Hilbert amplitude vs its median | 0.500 | 1 071 | 0.7483 |
| Raw filtered signal vs its mean | 0.500 | 1 186 | **0.8275** |

</div>

Mean versus median of the envelope is a rounding error: the two differ by 0.00003, which the
table's four decimals do not resolve. Skipping the Hilbert step and thresholding the raw signal
changes LZc<sub>N</sub> by 0.079, nearly as large as the whole 0.085 wake/anaesthesia effect.
Amplitude binarisation and raw-signal binarisation are different measurements; state which one
you used, every time.

### LZ complexity is closer to bandwidth than to randomness

Aboy, Hornero, Abásolo & Álvarez (2006) concluded that LZ is most useful as a scalar estimate
of the "bandwidth of random processes" and of harmonic variability in quasi-periodic
signals.[^aboy] If your LZ went up, the honest first hypothesis is that the
signal got broader-band, not that the system got more complex. Run a **phase-randomised
surrogate** control — randomise the Fourier phases, keep the amplitudes — as Schartner et al.
(2017) did, and show the effect survives it.

### Filtering artefacts

Filter-edge transients contaminate the envelope. Filter a generous margin of data and cut the
epoch out of the middle; do not filter exactly the epoch you intend to analyse. Ringing from a
narrow notch is periodic, and LZ76 parses periodic structure into few long factors, so this
artefact should push your complexity *down* rather than up — check the direction on your own
data rather than assuming it. A steep highpass corner sitting near the low edge of your band
behaves the same way.

### Maximum complexity is not proof of randomness

Estevez-Rams et al. (2013) showed that typical random sequences of finite length fall *short*
of the maximum attainable LZ complexity, and that maximum-LZ sequences of a given length are
produced by a deterministic generating algorithm of size O(log *n*) — which makes them of
negligible algorithmic complexity, the opposite of random.[^maxlz] A high LZc means "hard for
LZ76 to compress at this length", nothing stronger. That caveat comes from Ernesto Estévez Rams,
listed among this package's own authors.

### Epilepsy and anaesthesia-depth work need their own calibration

The sign of the effect depends on montage and reference. Intracranial recordings show LZ
complexity *increasing* at focal electrodes during seizures; some scalp feature-stack studies
report the opposite direction. Binarisation and normalisation differ across those studies and
are a plausible explanation for the disagreement, but nobody has isolated the cause. Calibrate
on labelled data from your own recording setup before trusting a direction.

For depth of anaesthesia, the two figures of merit this literature uses are the prediction
probability `Pk` and R² against effect-site concentration — produce those and your result is
directly comparable to published work. Use a sliding window of 2–10 s on a small number of
frontal channels, and establish each subject's own awake baseline. There is no population
cutoff for spontaneous-EEG LZc; the only validated cutoff in this field is PCI\* = 0.31, and
that is for PCI.

### Sleep staging separates N3, not everything

LZC is progressively lower with increasing NREM depth: NREM2 significantly lower than wake, REM
and NREM1, and NREM3 significantly lower again than NREM2. The finer orderings are not reliable:
NREM1 was estimated *higher* than wake — not significant after correction for multiple
comparisons — and REM was not significantly different from wake.[^sleep] The same work found no
significant difference in posterior LZC between dream and non-dream awakenings within NREM2,
which led the authors to question whether LZC tracks the richness of experience at all. On that
evidence LZC indexes *state*, not content.

<hr class="lz-tickrule">

## Complete listing

Everything above, in order, as one runnable module.

```python title="eeg_lzc.py"
import numpy as np
from scipy.signal import butter, filtfilt, hilbert
import lzcomplexity as lz

FS = 250.0


def bandpass(x, fs=FS, lo=1.0, hi=45.0, order=4):
    b, a = butter(order, [lo / (fs / 2), hi / (fs / 2)], btype="band")
    return filtfilt(b, a, x, axis=-1)


def binarise(x):
    amp = np.abs(hilbert(x, axis=-1))
    return (amp > amp.mean(axis=-1, keepdims=True)).astype(np.uint8)


def serialise(b):
    return "".join(map(str, b.T.ravel()))


def lzc(s):
    return lz.factorization(s)[0]


def lzc_normalised(s, n_surrogates=20, seed=0):
    rng = np.random.default_rng(seed)
    buf = np.frombuffer(s.encode(), dtype=np.uint8).copy()
    surrogates = []
    for _ in range(n_surrogates):
        rng.shuffle(buf)
        surrogates.append(lzc(buf.tobytes().decode()))
    return lzc(s) / float(np.mean(surrogates))


if __name__ == "__main__":
    # Two synthetic 8-channel, 40 s recordings at 250 Hz: broadband noise, and the
    # same noise attenuated under a shared 1.2 Hz oscillation. Every number on this
    # page comes from these two arrays. Replace both with your own
    # (n_channels, n_samples) data.
    n_t = int(FS * 40)
    t = np.arange(n_t) / FS
    wake = np.random.default_rng(1).standard_normal((8, n_t))
    anaesthesia = 0.35 * np.random.default_rng(2).standard_normal((8, n_t)) \
        + 4.0 * np.sin(2 * np.pi * 1.2 * t)[None, :]

    for name, x in [("wake", wake), ("anaesthesia", anaesthesia)]:
        b = binarise(bandpass(x[:, : int(FS * 10)]))  # first 10 s
        s = serialise(b)
        lzs = float(np.mean([lzc_normalised("".join(map(str, row))) for row in b]))
        print(f"{name:11s} n={len(s)} ones={b.mean():.4f} c={lzc(s):4d} "
              f"LZcN={lzc_normalised(s):.4f} h={lz.h(s, log_base=2):.4f} "
              f"mean_LZsN={lzs:.4f}")
```

<div class="lz-run" markdown>

```text
wake        n=20000 ones=0.4532 c=1063 LZcN=0.7484 h=0.7594 mean_LZsN=0.5737
anaesthesia n=20000 ones=0.6418 c= 891 LZcN=0.6637 h=0.6365 mean_LZsN=0.5616
```

</div>

!!! example "Reading that output"

    `LZcN` is the only Schartner-shaped number in the block: the wake epoch factorises into
    74.8 % as many pieces as a symbol-shuffled version of itself, the anaesthesia epoch into
    66.4 %. It is still an LZ76 count, not the LZ78 count Schartner published — see the parse
    warning in step 5. `n = 20000` is 8 channels × 2500 samples serialised column-major.
    `c` is the raw LZ76 factor count and is meaningless on its own — it depends on *n*, on the
    1-fraction and on the montage. `h` is the library's own analytic normalisation in bits per
    symbol, a different quantity from `LZcN`; do not report it as one. `mean_LZsN` is the
    per-channel, temporal-only counterpart, and it separates the two conditions far less than
    `LZcN` does. `log_base=2` is a no-op on binary input, because the auto-detected alphabet is
    already 2 — pass it anyway so the units are explicit in your code.

For what each returned field means in general, see
[Reading the numbers](../guide/reading-the-numbers.md). For the estimator's convergence
behaviour and how long a sequence has to be, see
[Sequence length and convergence](../concepts/convergence.md).

<hr class="lz-tickrule">

## Reporting checklist

Everything on this list changes the number. State all of it.

| Item | Why |
|---|---|
| Filter type, band, notch, filter direction | Ringing and phase distortion both change the envelope |
| Sampling rate after downsampling | Sets *n* |
| Epoch length, and whether epochs were averaged | LZc<sub>N</sub> drifted 0.056 over 2 s → 40 s above |
| Channel count, montage, reference | LZc is not portable across montages |
| Binarisation: which signal, which threshold | Worth up to 0.079 in LZc<sub>N</sub> |
| Serialisation order — per channel or concatenated | LZs and LZc are different measures |
| Which LZ parse: LZ76 factorisation or LZ78 dictionary | Worth 0.20 in the normalised value above |
| Which normalisation: analytic, source entropy, or shuffled surrogate | The three disagree |
| Which surrogate: symbol shuffle or phase randomisation | Worth 0.24 in the normalised value above |
| Number of surrogates and the RNG seed | Reproducibility |
| Library version and `log_base` | `lz.__version__`; `log_base` defaults to the alphabet size, not 2 |

Every number on this page was produced with `lzcomplexity` 1.0.0 (NumPy 2.5.1, SciPy 1.18.0) on
the two synthetic recordings the listing above generates; the epoch-length and binarisation
tables slice the same 40 s wake array. Synthetic Gaussian noise is not EEG — treat the values as
a demonstration of which knobs move the number, not as reference figures. Full citations are on
the [References](../project/references.md) page.

[^casali]: A. G. Casali, O. Gosseries, M. Rosanova, M. Boly, S. Sarasso, K. R. Casali,
    S. Casarotto, M.-A. Bruno, S. Laureys, G. Tononi, M. Massimini, "A Theoretically Based Index
    of Consciousness Independent of Sensory Processing and Behavior", *Science Translational
    Medicine* **5**(198), 198ra105 (2013).
    DOI [10.1126/scitranslmed.3006294](https://doi.org/10.1126/scitranslmed.3006294)

[^casarotto]: S. Casarotto, A. Comanducci, M. Rosanova, S. Sarasso, M. Fecchio, M. Napolitani,
    A. Pigorini, A. G. Casali, P. D. Trimarchi, M. Boly, et al., "Stratification of unresponsive
    patients by an independently validated index of brain complexity", *Annals of Neurology*
    **80**, 718–729 (2016). DOI [10.1002/ana.24779](https://doi.org/10.1002/ana.24779)

[^schartner15]: M. Schartner, A. Seth, Q. Noirhomme, M. Boly, M.-A. Bruno, S. Laureys,
    A. Barrett, "Complexity of Multi-Dimensional Spontaneous EEG Decreases during Propofol
    Induced General Anaesthesia", *PLOS ONE* **10**(8): e0133532 (2015).
    DOI [10.1371/journal.pone.0133532](https://doi.org/10.1371/journal.pone.0133532)

[^schartner17]: M. M. Schartner, R. L. Carhart-Harris, A. B. Barrett, A. K. Seth,
    S. D. Muthukumaraswamy, "Increased spontaneous MEG signal diversity for psychoactive doses
    of ketamine, LSD and psilocybin", *Scientific Reports* **7**, 46421 (2017).
    DOI [10.1038/srep46421](https://doi.org/10.1038/srep46421)

[^sarasso]: S. Sarasso, M. Boly, M. Napolitani, O. Gosseries, V. Charland-Verville,
    S. Casarotto, et al., "Consciousness and Complexity during Unresponsiveness Induced by
    Propofol, Xenon, and Ketamine", *Current Biology* **25**(23), 3099–3105 (2015).

[^aboy]: M. Aboy, R. Hornero, D. Abásolo, D. Álvarez, "Interpretation of the Lempel-Ziv
    Complexity Measure in the Context of Biomedical Signal Analysis", *IEEE Transactions on
    Biomedical Engineering* **53**(11), 2282–2288 (2006).
    DOI [10.1109/TBME.2006.883696](https://doi.org/10.1109/TBME.2006.883696)

[^maxlz]: E. Estevez-Rams, R. Lora Serrano, B. Aragón Fernández, I. Brito Reyes, "On the
    non-randomness of maximum Lempel Ziv complexity sequences of finite size", *Chaos* **23**,
    023118 (2013). DOI [10.1063/1.4808251](https://doi.org/10.1063/1.4808251)

[^sleep]: "EEG Lempel-Ziv complexity varies with sleep stage, but does not seem to track dream
    experience", *Frontiers in Human Neuroscience* **16** (2022), PMC9871639. See also
    C. Höhn, M. Hahn, J. D. Lendner, K. Hoedlmoser, "Spectral slope and Lempel–Ziv complexity as
    robust markers of brain states during sleep and wakefulness", *eNeuro* **11**(3),
    ENEURO.0259-23.2024.
    DOI [10.1523/ENEURO.0259-23.2024](https://doi.org/10.1523/ENEURO.0259-23.2024)
