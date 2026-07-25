# Rust vs C++

*What the Rust rewrite kept bit-identical, what it changed on purpose, and what it fixed.*

`lzcomplexity` began as a C++ library with nanobind bindings. That implementation still lives on the
`main` branch and is the origin of most of what is documented on this site — the LZ76 driver, the
block-shuffle surrogate machinery, the information distance, the error heuristics and the CLI JSON
schema were all designed there first.

This site documents the **Rust rewrite** on the `rust-backend` branch, which is the current
production backend: it is what `pip install lzcomplexity` gives you and what
`cargo add lzcomplexity-core` builds against. The rewrite is a port rather than a redesign, with one
exception: the EMC estimator was replaced outright in 0.13.0. Four measures are bit-identical to the
C++ backend, two differ on purpose, and one differs because the C++ had a bug.

<div class="lz-stats">
  <div class="lz-stat"><div class="lz-stat__v">289</div><div class="lz-stat__k">differential cases matched</div></div>
  <div class="lz-stat"><div class="lz-stat__v">4</div><div class="lz-stat__k">measures bit-identical</div></div>
  <div class="lz-stat"><div class="lz-stat__v">2</div><div class="lz-stat__k">intentional divergences</div></div>
  <div class="lz-stat"><div class="lz-stat__v">1</div><div class="lz-stat__k">C++ bug fixed</div></div>
</div>

<div class="lz-tickrule"></div>

## The engine room

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr><th>Aspect</th><th>C++ (<code>main</code>)</th><th>Rust (<code>rust-backend</code>)</th></tr>
</thead>
<tbody>
<tr><td>Build system</td><td>CMake + nanobind</td><td class="is-changed">Cargo + maturin (PyO3)</td></tr>
<tr><td>Python packaging</td><td>one wheel per CPython version</td><td class="is-changed">one <code>abi3</code> wheel per platform (<code>abi3-py39</code>), Python 3.9 and later</td></tr>
<tr><td>Suffix array</td><td>CaPS (custom parallel construction)</td><td class="is-changed">hybrid: comparison sort below 2048 bytes, <code>cdivsufsort</code> above</td></tr>
<tr><td>LCP array</td><td>bundled with CaPS</td><td>Kasai's algorithm, O(<i>n</i>)</td></tr>
<tr><td>Parallelism</td><td>OpenMP, with TBB / Cilk / <code>std::thread</code> fallbacks chosen at configure time</td><td class="is-changed">rayon, always</td></tr>
<tr><td>Shuffle RNG</td><td><code>std::mt19937</code>, one function-local <code>static</code>, seeded from <code>std::random_device</code></td><td class="is-changed">ChaCha8, one instance per block size, seeded from an FNV-1a hash of the input</td></tr>
<tr><td>Shuffle reproducibility</td><td class="is-no">not reproducible run to run</td><td class="is-yes">reproducible on any machine, any thread count</td></tr>
<tr><td>Spectral analysis</td><td><code>spectral.psd</code>, <code>spectral.entropy</code>, <code>spectral.semc</code></td><td class="is-changed">removed — moved to a separate package</td></tr>
<tr><td>Python surface</td><td><code>lz76</code>, <code>factorization</code>, <code>factors</code>, <code>entropy_density</code>, <code>emc</code>, plus <code>metrics</code> and <code>spectral</code> submodules</td><td class="is-changed">five top-level functions, no submodules</td></tr>
<tr><td>Python defaults</td><td><code>alphabet=2</code>, <code>log_base=2</code> (hard-coded literals)</td><td class="is-changed"><code>alphabet=None</code>, <code>log_base=None</code> — auto-detected from the data</td></tr>
<tr><td>Toolchain to build from source</td><td>C++20 compiler + CMake</td><td>Rust + a C compiler — <code>cdivsufsort</code> compiles bundled C</td></tr>
</tbody>
</table>
</div>

!!! note
    The `suffix` crate is **not** a dependency of this workspace, despite what the repository
    README's comparison table still says. Suffix arrays are built by `build_suffix_array`
    (`crates/lzcomplexity-core/src/suffix_array.rs`), which branches on a `SORT_CUTOFF` of 2048
    bytes: below it, a plain `sort_unstable_by` over suffix slices, which has a much lower fixed
    cost at that size; at or above it, `cdivsufsort` — Rust bindings that compile Yuta Mori's
    bundled C `libdivsufsort` — which is `O(n log n)` in the worst case and so does not degrade on
    repetitive input the way the comparison sort does.

    The suffix array of a string is unique, so the two branches cannot disagree. That is asserted,
    not assumed: `suffix_array::tests::fuzz_sa_matches_reference` checks `build_suffix_array`
    against the comparison sort on 300 random inputs of length 1–5000 over five alphabets
    (including non-UTF-8 bytes and all-equal bytes), spanning both branches. It passes.

## What is bit-identical

Four outputs match the C++ backend exactly. This was established by differential testing: two
virtualenvs, one holding the Rust wheel and one the published C++ wheel `lzcomplexity==0.10.2`, run
over a 289-case sweep of random corpora across alphabets and lengths, including binary sequences up
to 700 000 symbols. **289 of 289 cases matched.**

| Measure | Rust name | Status |
|---|---|---|
| Factor count | `factorization(seq)[0]` | exact integer match |
| Factor boundaries | `factorization(seq)[1]` | exact, element for element |
| Entropy density | `h(seq)` | exact **at equal log base** — see the warning below |
| Information distance | `nid(a, b)` | exact |

Two details are worth knowing about that agreement.

**The factor-count convention was inherited, not re-derived.** `complexity` counts only *complete*
LZ76 components. When the greedy parse runs past the end of the sequence, the trailing partial
component is not counted, so the library's count is one **less** than the textbook exhaustive-history
count whenever the sequence ends mid-component. The exact conversion, verified over 500 random
strings, is `c_textbook = c + (1 if factors[-1] > len(seq) else 0)`. Both backends do this, and both
are short of the textbook count by the same one. See [LZ76 factorization](../concepts/lz76.md).

**The C++ information distance has a dead second formula.** Its core exposes two overloads: the
Python-facing `(T1, T2, args)` one uses `max(C(XY) − C(X), C(YX) − C(Y)) / max(C(X), C(Y))`, and a
four-argument helper uses `min` instead. Tracing the call graph showed the `min` helper is
unreachable from every application path — the Python binding, `lzcomplexity --dlz` and the
`lzdistance` matrices all reach the `max` form. The Rust port implements only the `max` form, which
is why every distance path agrees. See [Information distance](../concepts/nid.md).

**Version caveat.** The 289-case sweep ran Rust 0.11.0 against C++ 0.10.2. Exactly one commit has
touched those four code paths since (`8f858be`): the suffix-array swap described above, which the
fuzz test pins to the comparison-sort reference, plus a pass that reuses and parallelizes the
factor counts, which only reorders independent integer computations. The sweep has not been re-run
against 1.0.

!!! warning
    `h` on non-binary data halves after migrating unless you pass `log_base=2`. The C++ *Python
    binding* hard-coded `log_base=2`, so `entropy_density` returned bits per symbol. The Rust `h`
    defaults `log_base` to the auto-detected alphabet size, so it returns a normalised value
    instead. On a 58-nt DNA string:

    ```pycon
    >>> import lzcomplexity as lz
    >>> dna = "ACGTACGTTGCAACGTGGATCCGTAAGCTTACGATCGATCGGATCAGCTAGCATCGAT"
    >>> lz.h(dna)                # Rust default: base 4, the detected alphabet
    0.9089970509680715
    >>> lz.h(dna, log_base=2)    # what the C++ binding returned
    1.817994101936143
    ```

    The ratio is exactly `log2(k)`. On binary input the two conventions coincide, which is why the
    change is easy to miss. See [Alphabets and log bases](../concepts/alphabets.md).

## What differs on purpose

### `emc`: a new estimator, and a new RNG

Two independent changes stack here. Either one on its own would already make the numbers
incomparable.

**The estimator was replaced in 0.13.0.** The C++ summed *absolute* complexity differences over
every block size. The Rust version sums block-entropy first differences in excess of the sequence's
own entropy density. These are different statistics, not two spellings of one.

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr><th>Aspect</th><th>C++ (<code>0.10.2</code>)</th><th>Rust (<code>1.0</code>)</th></tr>
</thead>
<tbody>
<tr><td>Per-scale term</td><td><code>g · |C<sub>LZ</sub>(u<sup>RS(l)</sup>) − C<sub>LZ</sub>(u)|</code></td><td class="is-changed"><code>(H<sub>l</sub> − H<sub>l−1</sub>) − ĥ</code>, with <code>H<sub>l</sub> = l · C<sub>LZ</sub>(u<sup>RS(l)</sup>) · g</code></td></tr>
<tr><td>Sign</td><td>never negative — the term is an absolute value</td><td class="is-changed">signed; negative totals are routine on near-random input</td></tr>
<tr><td>Scales reaching the total</td><td>all <code>mm</code> of them</td><td class="is-changed">telescopes down to the largest one alone</td></tr>
<tr><td><code>g</code></td><td><code>log<sub>a</sub>(N) / N</code>, <code>a</code> = <code>alphabet</code></td><td class="is-changed"><code>log<sub>k</sub>(N) / N</code>, <code>k</code> = <code>log_base</code></td></tr>
</tbody>
</table>
</div>

That last row only bites if you pass `alphabet` and `log_base` separately; on the defaults both
resolve to the detected alphabet size.

**The RNG was replaced in the port.** The C++ shuffle drew from a single function-local
`static std::mt19937` seeded once from `std::random_device`. That state was shared by every call and
mutated from `parallel_for` workers, so the C++ `emc` is not reproducible run to run and not
reproducible under a different thread count.

The Rust shuffle hashes the sequence bytes with FNV-1a once, mixes the hash with the block index
using the 64-bit golden-ratio constant, and seeds a private `ChaCha8Rng` per block size. No shared
state, no scheduling dependence: the same input always returns the same `emc`, on any machine, with
any number of threads. Verified bit-identical across repeated runs and across the Rust and Python
entry points. See [Determinism](determinism.md).

The port also added two guards the C++ lacked in the swap kernel: a zero block size divided by zero
in C++, and a degenerate block count sent it into an infinite loop. Two further in-kernel
differences are not guards, and both widen the set of eligible second blocks — C++ rejected the
block immediately to the right of the first (`op2 == op1 + block_size`) where Rust accepts it, and
C++ required `op2 < n − block_size − 1` where Rust allows `op2 ≤ n − block_size − 1`.

!!! danger
    An `emc` value produced by the C++ backend cannot be reproduced by the Rust backend, and neither
    can be reproduced by a second run of the C++ backend. Do not mix `emc` numbers from the two
    implementations in one figure, table or statistical test, and do not read a small gap between
    them as a rounding artefact.

    The two backends would disagree even if they drew identical surrogates, because the formulas in
    the table above are different. On top of that, the Rust sum telescopes:
    `sum_l [(H_l − H_{l−1}) − h_hat]` reduces exactly to
    `mm * g * (C_LZ(shuffled at mm) − C_LZ(original))`, so the whole value rests on one surrogate at
    one block size. A different RNG stream is a different surrogate and therefore a different number.
    The per-scale `summands` differ for both reasons at once.

    The two agree only where the shuffle kernel is a no-op: `n ≤ 10` returns exactly `0.0` in both.
    Elsewhere, near-zero is not zero — `lz.emc("0" * 20000)` returns `-2.168404344971009e-19`, and
    `lz.emc("01" * 10000)`, a perfectly periodic input, returns `1.7102391718320673`. The resonance
    artefacts come from the block grid, which both implementations share.
    See [Effective measure complexity](../concepts/emc.md).

<div class="lz-formula">
  <p class="lz-math">Ê = <i>mm</i> · <i>g</i> · ( C<sub>LZ</sub>(<i>u</i><sup>RS(<i>mm</i>)</sup>) − C<sub>LZ</sub>(<i>u</i>) ),&nbsp;&nbsp; <i>g</i> = log<sub><i>k</i></sub>(<i>N</i>) ⁄ <i>N</i></p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd>largest block size — the only scale that survives the telescoping</dd>
    <dt><i>u</i><sup>RS(<i>mm</i>)</sup></dt><dd>the one block-shuffled surrogate the value depends on; a different RNG stream gives a different one</dd>
    <dt><i>N</i>, <i>k</i></dt><dd>sequence length and log base</dd>
  </dl>
  <p class="lz-formula__cite">The Rust estimator only. Every scale below <i>mm</i> cancels, so the total rests on a single surrogate.</p>
</div>

### Spectral analysis, removed

`spectral.psd`, `spectral.entropy` and `spectral.semc` were FFT-based measures bundled into the C++
Python module. They are not LZ76 measures and they pulled a bundled FFT into every build, so the
Rust package does not ship them; they moved to a separate package. `lzcomplexity` has no `spectral`
attribute and no `metrics` attribute, and the CI wheel smoke test asserts that both stay absent.

## What differs as a bug fix

`factors_stddev`, and therefore `normal_error`, do **not** match the C++ backend. This is not a
design decision. The C++ `FoundStddev` built its length vector like this:

```cpp
std::vector<lz_uint> factors_length(lzf.size());   // lzf.size() ZEROS
for (auto i = 1ul; i < lzf.size(); i++) {
  auto size = lzf[i] - lzf[i - 1];
  factors_length.push_back(size);                  // appended AFTER the zeros
  ...
}
```

`std::vector<T> v(n)` value-initialises `n` zeros; `push_back` then appends the real factor lengths
after them. The sum of squares therefore ran over `2·lzf.size() − 1` elements, of which `lzf.size()`
were spurious zeros each contributing `mean²`. The Rust port iterates only the real lengths.

The relationship between the two values is exact:

<div class="lz-formula">
  <p class="lz-math">σ<sub>C++</sub> = √( σ<sub>Rust</sub><sup>2</sup> + <i>L</i> · μ<sup>2</sup> ⁄ ℓ<sub>max</sub> )</p>
  <dl class="lz-formula__key">
    <dt>σ<sub>Rust</sub></dt><dd>the shipped <code>factors_stddev</code></dd>
    <dt><i>L</i></dt><dd>number of factor boundaries, <code>len(factors)</code></dd>
    <dt>μ</dt><dd><code>(factors[-1] − 1) / L</code>, the mean the code uses</dd>
    <dt>ℓ<sub>max</sub></dt><dd>length of the longest factor — the divisor, which is why this quantity is not a standard deviation in either backend</dd>
  </dl>
</div>

`normal_error` is exactly proportional to `factors_stddev`, so it differs by the same ratio.

!!! example
    Take `"ABRACADABRA"`. The Rust library reports:

    ```pycon
    >>> import lzcomplexity as lz
    >>> lz.factorization("ABRACADABRA")
    (5, [0, 1, 2, 3, 5, 7, 12])
    >>> d = lz.lz76("ABRACADABRA")
    >>> d["factors_stddev"], d["normal_error"]
    (1.6187674827674623, 0.3320213997117467)
    ```

    Recomputing by hand from those boundaries: `L = 7`, `μ = (12 − 1)/7 = 1.571428…`, factor lengths
    `[1, 1, 1, 2, 2, 5]`, `ℓ_max = 5`, `Σ(ℓ − μ)² = 13.102040…`, so
    `σ_Rust = √(13.102040/5) = 1.6187674827674623` ✓. The C++ formula adds
    `L·μ² = 7 × 2.469387… = 17.285714…` to that sum, giving `σ_C++ = √(30.387755/5) = 2.4652689549840527`.
    Ratio **1.52**.

The ratio is not a constant — it grows with sequence length, because the mean factor length grows
like `log n` while the dispersion around it stays near 2.5. Measured on i.i.d. random binary strings
(CPython `random.seed(0)`, re-seeded per row):

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr><th><i>n</i></th><th><i>c</i></th><th>Rust <code>factors_stddev</code></th><th>C++ formula</th><th>ratio</th></tr>
</thead>
<tbody>
<tr><td>100</td><td>18</td><td>3.0659</td><td>7.7071</td><td>2.51</td></tr>
<tr><td>1 000</td><td>107</td><td>6.9881</td><td>24.2590</td><td>3.47</td></tr>
<tr><td>10 000</td><td>777</td><td>15.7232</td><td>77.9885</td><td>4.96</td></tr>
<tr><td>100 000</td><td>6 138</td><td>35.1241</td><td>231.8863</td><td>6.60</td></tr>
<tr><td>1 000 000</td><td>50 789</td><td>91.6784</td><td>735.2056</td><td>8.02</td></tr>
</tbody>
</table>
</div>

Short, structured inputs sit much lower: `"aaaa"` 1.32×, `"01010101"` 1.26×, `"ABRACADABRA"` 1.52×,
and the 47-symbol binary sequence used elsewhere in these docs 1.78×. As a rule of thumb, expect a
factor of 1.3–1.8 on toy strings and 3–8 on realistic data. The Rust value is always the smaller of
the two.

!!! note
    The C++ figures in that table were recomputed by transcribing the C++ expression and evaluating
    it on the same factor boundaries — they were not produced by executing a C++ build. The Rust
    column is measured. Read the C++ column as "what the published formula evaluates to", which is
    the claim being made.

    Two further caveats apply to *both* backends. `factors_stddev` divides the sum of squares by the
    longest factor length rather than by the sample size, so it is not a standard deviation in
    either implementation. And `poison_error` is a long-standing misspelling of *Poisson* that the
    Rust port preserved because the dict key is now API-frozen. Neither error estimate is derived
    from a published statistical model; do not report them as confidence intervals. See
    [Reading the numbers](../guide/reading-the-numbers.md).

## API migration

The Python module keeps the import name `lzcomplexity`, but little else about the surface survived
unchanged. The Rust package exports exactly five functions and no submodules.

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr><th>C++ (0.10.2)</th><th>Rust (1.0)</th><th>What changed</th></tr>
</thead>
<tbody>
<tr><td><code>lz.factorization(seq)</code> → <code>int</code></td><td><code>lz.factorization(seq)</code> → <code>(int, list[int])</code></td><td class="is-changed">returns the count <em>and</em> the boundaries; take <code>[0]</code> for the old value</td></tr>
<tr><td><code>lz.factors(seq)</code> → <code>(int, list)</code></td><td><code>lz.factorization(seq)</code></td><td class="is-changed">merged into <code>factorization</code>; <code>factors</code> is gone</td></tr>
<tr><td><code>lz.entropy_density(seq)</code></td><td><code>lz.h(seq)</code></td><td class="is-changed">renamed, and the default log base changed</td></tr>
<tr><td><code>lz.emc(seq)</code> → <code>(max_block_size, emc_value, multi_information)</code></td><td><code>lz.emc(seq)</code> → <code>(value, summands)</code></td><td class="is-changed">different tuple <em>and</em> a different estimator; <code>max_block_size</code> and <code>multi_information</code> moved into <code>lz76(seq)["emc"]</code></td></tr>
<tr><td><code>lz.lz76(seq)</code> → 4-tuple</td><td><code>lz.lz76(seq)</code> → <code>dict</code></td><td class="is-changed">keys: <code>complexity</code>, <code>h</code>, <code>factors</code>, <code>emc</code>, <code>epsilon</code>, <code>factors_stddev</code>, <code>normal_error</code>, <code>poison_error</code>, <code>extras</code></td></tr>
<tr><td><code>lz.metrics.information_distance(a, b)</code></td><td><code>lz.nid(a, b)</code></td><td class="is-changed">the <code>metrics</code> submodule is gone</td></tr>
<tr><td><code>lz.metrics.rid(a, b)</code></td><td class="is-no">no equivalent</td><td>the binding was already commented out in the C++ source</td></tr>
<tr><td><code>lz.spectral.psd</code> / <code>.entropy</code> / <code>.semc</code></td><td class="is-no">no equivalent</td><td>moved to a separate package</td></tr>
<tr><td><code>alphabet=2</code>, <code>log_base=2</code></td><td><code>alphabet=None</code>, <code>log_base=None</code></td><td class="is-changed">auto-detected; <code>alphabet</code> feeds only <code>epsilon</code>, never <code>h</code></td></tr>
<tr><td><code>jobs=hardware_concurrency()</code></td><td><code>jobs=0</code></td><td class="is-changed">accepted and <strong>ignored</strong>; set <code>RAYON_NUM_THREADS</code> instead</td></tr>
<tr><td><code>partitions=1</code></td><td><code>partitions=1</code></td><td class="is-changed">signature unchanged, but inert — the Rust core stores the value and never reads it</td></tr>
</tbody>
</table>
</div>

A minimal migration:

```python
# C++ 0.10.2
import lzcomplexity as lz

c         = lz.factorization(seq)
c, fac    = lz.factors(seq)
hbits     = lz.entropy_density(seq)
mm, e, mi = lz.emc(seq)
d         = lz.metrics.information_distance(a, b)
```

```python
# Rust 1.0
import lzcomplexity as lz

c, fac      = lz.factorization(seq)
hbits       = lz.h(seq, log_base=2)      # keep the old units explicitly
e, summands = lz.emc(seq)
full        = lz.lz76(seq)
mm, mi      = full["emc"]["max_block_size"], full["emc"]["multi_information"]
d           = lz.nid(a, b)
```

The right-hand column, run on `seq = "01001010101101010101110101010101010000100101011"` and the pair
`a, b = "ABRACADABRA", "ABRACADABRZ"`:

```pycon
>>> lz.factorization(seq)
(9, [0, 1, 2, 4, 7, 12, 22, 31, 36, 41, 48])
>>> lz.h(seq, log_base=2)
1.063644673725505
>>> lz.emc(seq)
(0.0, [0.11818274152505626, -0.11818274152505626, 0.35454822457516877, -0.35454822457516877])
>>> full = lz.lz76(seq)
>>> full["emc"]["max_block_size"], full["emc"]["multi_information"]
(4, 0.11818274152505626)
>>> lz.nid(a, b)
0.3333333333333333
```

!!! note
    The C++ module docstring showed attribute access — `result.complexity` — on the value returned
    by `lz76()`. That example was already stale in the C++ source: the binding returned a plain
    tuple, not an object. If you wrote code against that docstring it never worked; if you wrote it
    against the binding, the Rust `dict` is a one-line change.

The full Rust surface, with every parameter, is on the [Python API](../api/python.md) page.

## Behaviour the port deliberately did not change

Two C++ quirks were reproduced rather than silently corrected:

- **The CLI's `-l` flag does not affect `lz76EntropyDensity`.** The C++ batch path computed
  `c · log_alphabet(n) / n`, in which the log base cancels; the entropy density there is controlled
  by `-a`. The Rust CLI copies that formula, and the Rust `h()` copies the *other* C++ function —
  the single-sequence one that reads `log_base`. Both backends contain the same disagreement between
  their CLI and their library. See [`lzcomplexity`](../cli/lzcomplexity.md).
- **The `+10` bump on the auto EMC block size, and the `n ≤ 10` shuffle floor.** Both are inherited
  verbatim: the bump is applied only when `n > 50`, and the swap kernel returns without doing
  anything when `n ≤ 10`. Because the surrogate is then identical to the input, `emc` is `0.0` for
  sequences of ten symbols or fewer in both backends.

Everything else that differs is listed above.
