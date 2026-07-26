# Rust vs C++

*What the Rust rewrite kept bit-identical, what it changed on purpose, and what it fixed.*

`lzcomplexity` began as a C++ library with nanobind bindings. That implementation still lives on the
`main` branch and is the origin of most of what is documented on this site — the LZ76 driver, the
block-shuffle surrogate machinery, the information distance, the error heuristics and the CLI JSON
schema were all designed there first.

This site documents the **Rust rewrite** on the `rust-backend` branch, which is the current
production backend: it is what `pip install lzcomplexity` gives you and what
`cargo add lzcomplexity-core` builds against. The rewrite is a port rather than a redesign: the EMC
estimator was replaced outright in 0.13.0, but that replacement has since been applied to the C++
backend too, so **all five measures are now bit-identical across the two backends**. One field still
differs on purpose (the Python default log base), and one differs because the C++ had a bug.

<div class="lz-stats">
  <div class="lz-stat"><div class="lz-stat__v">289</div><div class="lz-stat__k">differential cases matched</div></div>
  <div class="lz-stat"><div class="lz-stat__v">188</div><div class="lz-stat__k">EMC cases bit-identical</div></div>
  <div class="lz-stat"><div class="lz-stat__v">1</div><div class="lz-stat__k">intentional divergence</div></div>
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
<tr><td>Shuffle RNG</td><td>ChaCha8, one instance per block size, seeded from an FNV-1a hash of the input — <code>modules/core/inc/lz/rng.h</code></td><td>the same, bit-for-bit</td></tr>
<tr><td>Shuffle reproducibility</td><td class="is-yes">reproducible on any machine, any thread count</td><td class="is-yes">reproducible on any machine, any thread count</td></tr>
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

### `emc`: the two backends now agree bit-for-bit

**This section used to say the two `emc` values were incomparable. That is no longer true.** Both
backends now run the same estimator over the same surrogates and return identical values — the same
`value`, the same `multi_information` and the same `summands`, to the last bit.

Getting there took two changes, because an `emc` value is a function of both the formula and the
surrogates it is computed from.

**The estimator.** The C++ originally summed *absolute* complexity differences across block sizes.
It moved to the block-entropy ladder in 0.13.0 (Rust) and the matching C++ release, and both now
project that ladder onto the non-negative non-decreasing cone excess entropy provably occupies,
reading the total and the per-scale terms off the projection.

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr><th>Aspect</th><th>C++ (<code>0.10.2</code>, the last release before the port)</th><th>Both backends, current</th></tr>
</thead>
<tbody>
<tr><td>Per-scale term</td><td><code>g · |C<sub>LZ</sub>(u<sup>RS(l)</sup>) − C<sub>LZ</sub>(u)|</code></td><td class="is-changed">the increments of the projected ladder <code>Ê(l) = H<sub>l</sub> − l·ĥ</code>, with <code>H<sub>l</sub> = l · C<sub>LZ</sub>(u<sup>RS(l)</sup>) · g</code></td></tr>
<tr><td>Sign</td><td>never negative — the term is an absolute value</td><td>never negative — the projection enforces it</td></tr>
<tr><td>Scales reaching the total</td><td>all <code>mm</code> of them</td><td>all <code>mm</code> of them</td></tr>
<tr><td><code>g</code></td><td><code>log<sub>a</sub>(N) / N</code>, <code>a</code> = <code>alphabet</code></td><td class="is-changed"><code>log<sub>k</sub>(N) / N</code>, <code>k</code> = <code>log_base</code></td></tr>
<tr><td>Shuffle RNG</td><td>one shared <code>static std::mt19937</code> seeded from <code>std::random_device</code></td><td class="is-changed">ChaCha8 per block size, seeded from an FNV-1a hash of the input</td></tr>
<tr><td>Reproducible run to run</td><td class="is-no">no</td><td class="is-yes">yes, on any machine at any thread count</td></tr>
</tbody>
</table>
</div>

The `g` row only bites if you pass `alphabet` and `log_base` separately; on the defaults both
resolve to the detected alphabet size.

**The surrogates.** The old C++ shuffle drew from a single function-local
`static std::mt19937` seeded once from `std::random_device`, shared across calls and mutated from
`parallel_for` workers — so its `emc` was reproducible neither run to run nor across thread counts,
and could not agree with anything. Both backends now hash the sequence bytes with FNV-1a once, mix
the hash with the block index using the 64-bit golden-ratio constant, and seed a private ChaCha8 per
block size. The C++ implementation lives in `modules/core/inc/lz/rng.h` and reproduces Rust's
generator exactly, down to the rejection zone `(range << range.leading_zeros()) − 1` that `rand`
uses for its bounded draws.

Three divergences in the swap kernel itself also had to go, all of which changed which blocks were
eligible: the C++ consumed one extra draw before its second-block loop, it rejected the block
immediately to the right of the first (`op2 == op1 + block_size`) where Rust accepts it, and it
required `op2 < n − block_size − 1` where Rust allows equality. The C++ also gained the two guards
it lacked — a zero block size divided by zero, and a degenerate block count looped forever.

<div class="lz-formula">
  <p class="lz-math"><i>Ê</i>(<i>l</i>) = <i>l</i> · <i>g</i> · ( C<sub>LZ</sub>(<i>u</i><sup>RS(<i>l</i>)</sup>) − C<sub>LZ</sub>(<i>u</i>) ),&nbsp;&nbsp; <i>g</i> = log<sub><i>k</i></sub>(<i>N</i>) ⁄ <i>N</i>,&nbsp;&nbsp; <i>Ê</i> = non_negative_isotonic( <i>Ê</i>(1), …, <i>Ê</i>(<i>mm</i>) )(<i>mm</i>)</p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd>largest block size; every scale from 1 to <i>mm</i> enters the projection</dd>
    <dt><i>u</i><sup>RS(<i>l</i>)</sup></dt><dd>the block-shuffled surrogate at scale <i>l</i> — one draw per scale, now from the same stream in both backends</dd>
    <dt><i>N</i>, <i>k</i></dt><dd>sequence length and log base</dd>
  </dl>
  <p class="lz-formula__cite">Now shared by both backends. Earlier versions summed the first differences of this ladder, which telescopes to Ê(mm) alone.</p>
</div>

!!! success "Measured parity"

    188 of 188 differential comparisons of `lz76RandomShuffleComplexity` between the two backends came
    back **bit-identical** in `value`, `multi_information` and every `summand` — across lengths 1 to
    20 000, alphabets 2, 4, 5 and 26, and periodic, Thue–Morse, Markov, i.i.d. and constant inputs, at
    both the auto block size and pinned ones. Worst absolute difference: exactly zero.

!!! warning "Two caveats on that parity"

    **It does not extend backwards.** An `emc` from any earlier release of either backend is not
    comparable with a current one. Do not mix values across versions in one figure, table or test.

    **It is not guarded by a test.** The C++ repository has no test suite, and the parity rests on the
    C++ reproducing the *internal* algorithms of Rust's `rand` 0.8.6 and `rand_chacha` 0.3.1. A bump
    of those dependencies could desynchronise the backends silently. If you rely on cross-backend
    agreement, verify it on your own data rather than assuming it.

    `entropy_density` is a separate story and still differs between the two **command-line tools** —
    that divergence is about which flag feeds the log base, is unrelated to `emc`, and is described
    under [`lzcomplexity`](../cli/lzcomplexity.md).

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
(0.17727411228758438, [0.05909137076252813, 0.0, 0.11818274152505626, 0.0])
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
