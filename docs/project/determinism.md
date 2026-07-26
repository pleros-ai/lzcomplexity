# Determinism

*Same bytes in, same numbers out — what that guarantees, what it does not, and how to get an error bar anyway.*

Every number `lzcomplexity` returns is a function of the input bytes alone. Nothing reads the clock,
the process ID, the OS entropy pool, or the thread count. Run the same input twice, in two processes,
under two thread pools, and you get the same answer. Across two *machines* the integer part is still
identical; the last bit of a float can move, for the reason set out under
[Floating point](#floating-point).

That is a real property and worth having. It is also easy to over-read. Reproducibility means the
estimator will not change its mind; it does not mean the estimate is precise. The shuffle-based
measures still rest on a single surrogate realisation, and that realisation carries sampling error
which repetition will never reveal — because repetition returns the identical number.

<div class="lz-stats">
  <div class="lz-stat"><div class="lz-stat__v">3 / 5</div><div class="lz-stat__k">Python functions with no RNG at all</div></div>
  <div class="lz-stat"><div class="lz-stat__v">ChaCha8</div><div class="lz-stat__k">PRNG on the shuffle path</div></div>
  <div class="lz-stat"><div class="lz-stat__v">1</div><div class="lz-stat__k">surrogate drawn per block size</div></div>
  <div class="lz-stat"><div class="lz-stat__v">0</div><div class="lz-stat__k">reads of clock or OS entropy</div></div>
</div>

---

## Which outputs touch the RNG

Three of the five Python functions contain no randomness whatsoever. Their outputs are integer factor
counts and simple arithmetic over them.

<div class="lz-scroll lz-compare" markdown>

| Output | Randomness | Where the value comes from |
|---|---|---|
| `factorization` — `(complexity, factors)` | none | suffix array + LPF table, one pass |
| `h` — entropy density | none | `complexity · log_k(n) / n` |
| `nid` — normalised information distance | none | four factor counts |
| `lz76()["epsilon"]`, `["factors_stddev"]`, `["normal_error"]`, `["poison_error"]`, `["extras"]` | none | derived from factor counts |
| `emc()` and `lz76()["emc"]` | **block shuffle** | ChaCha8, seeded from the sequence bytes |
| CLI `lzcomplexity` → `lz76RandomShuffleComplexity` | **block shuffle** | same seed chain |
| CLI `lzdistance` → `shuffle_information_distance` | **block shuffle** | same seed chain |

</div>

The `lzdistance` matrix `information_distance` is RNG-free; only `shuffle_information_distance` goes
through the surrogate path. See [`lzdistance`](../cli/lzdistance.md).

!!! note

    `complexity` counts only **complete** LZ76 components, and that count is exact and
    platform-independent. The trailing component that runs past the end of the sequence is not
    counted, so the library's number is one less than the textbook exhaustive-history count whenever
    a sequence ends mid-component. The conversion is
    `c_textbook = c + (1 if factors[-1] > len(seq) else 0)`. For `"banana"`, `lz.factorization`
    returns `(3, [0, 1, 2, 3, 7])`, the last boundary `7` exceeds `len("banana") == 6`, and the
    textbook count is 4. It does **not** hold for sequences with fewer than two distinct symbols,
    which bypass the parser. [LZ76 factorization](../concepts/lz76.md) walks the boundary list and
    records how the rule was checked.

---

## The seed chain

The shuffle is seeded from the sequence content. `shuffle_factorization` hashes the sequence bytes
**once** with FNV-1a, then derives one seed per block size by mixing in the block index with the
64-bit golden-ratio constant.

<div class="lz-formula">
  <p class="lz-math">seed<sub><i>l</i></sub> = FNV1a(bytes) ⊕ ( <i>l</i> · 0x9E3779B97F4A7C15 )</p>
  <dl class="lz-formula__key">
    <dt>FNV1a</dt><dd>64-bit FNV-1a over the raw symbol bytes; offset basis 14695981039346656037, prime 1099511628211</dd>
    <dt><i>l</i></dt><dd>block size, 1 … <i>mm</i> — one seed, one surrogate, one factorization per value</dd>
    <dt>0x9E37…</dt><dd>the odd golden-ratio constant (the SplitMix64 increment); it decorrelates adjacent block sizes so <i>l</i> and <i>l</i>+1 do not shuffle alike</dd>
    <dt>⊕</dt><dd>bitwise XOR; the multiply wraps</dd>
  </dl>
  <p class="lz-formula__cite">crates/lzcomplexity-core/src/shuffle.rs:46–51 and 169–181. The seed drives <code>ChaCha8Rng::seed_from_u64</code> at sequence.rs:392.</p>
</div>

```rust
let base = fnv1a(seq.as_bytes());          // hashed once, for the whole call
let computed: Vec<(usize, i32)> = (1..=mm)
    .into_par_iter()
    .map(|idx| {
        let seed = seed_from_base(base, idx);
        let rand_seq = shuffle_copy_seeded(seq, idx as u32, (seq.len() / 2) as u32, seed);
        let c = lz76_factorization(&rand_seq, args);
        (idx, c as i32)
    })
    .collect();
for (idx, c) in computed { res[idx] = c; }
```

The chain is integer-only: a hash, a wrapping multiply, an XOR, and a stream cipher. No clock, no
`getrandom`, no address-dependent hashing, no floats. The permutation it produces and the LZ76 factor
counts of the surrogates are therefore bit-identical on every platform and at every optimisation
level. The float that comes out of them carries one caveat — see [Floating point](#floating-point).

### Verified

An 8 192-symbol first-order Markov chain, `p(flip) = 0.1`. The script prints the IEEE-754 bit
pattern so a difference has nowhere to hide.

??? note "Generating `markov.txt` — every number on this page comes from this one file"

    ```python
    import random

    def markov(n, q, seed):
        rng, s, out = random.Random(seed), "0", []
        for _ in range(n):
            out.append(s)
            if rng.random() >= q:
                s = "1" if s == "0" else "0"
        return "".join(out)

    open("markov.txt", "w").write(markov(8192, 0.9, seed=6) + "\n")
    ```

    The file has `complexity = 301`, `h = 0.4776611328125` and `mm = 20`.

```python
import struct
import lzcomplexity as lz

seq = open("markov.txt").read().strip()
value, _ = lz.emc(seq)
print("n        =", len(seq))
print("emc      =", value)
print("raw bits =", struct.pack(">d", value).hex())
```

<div class="lz-run" markdown>

```console
$ python3 determinism_check.py
n        = 8192
emc      = 1.30126953125
raw bits = 3ff4d20000000000
$ python3 determinism_check.py
n        = 8192
emc      = 1.30126953125
raw bits = 3ff4d20000000000
$ RAYON_NUM_THREADS=1 python3 determinism_check.py
n        = 8192
emc      = 1.30126953125
raw bits = 3ff4d20000000000
$ RAYON_NUM_THREADS=5 python3 determinism_check.py
n        = 8192
emc      = 1.30126953125
raw bits = 3ff4d20000000000
$ RAYON_NUM_THREADS=16 python3 determinism_check.py
n        = 8192
emc      = 1.30126953125
raw bits = 3ff4d20000000000
```

</div>

No parameter perturbs it either. `emc` returns the identical `1.30126953125` for every combination of
`partitions ∈ {1, 2, 8}` and `jobs ∈ {0, 1, 4}` — `partitions` is inert in the current core and
`jobs` is ignored by the Python bindings. Only `max_block_size` and `log_base` move an `emc` value,
and both do so deterministically. See [Python API](../api/python.md).

---

## Contrast with the C++ ancestor

The C++ implementation this library descends from seeded a single function-local `static` Mersenne
Twister from `std::random_device` at first use. Every call in the process drew from that one shared,
clock-seeded stream.

<div class="lz-scroll lz-compare" markdown>

| | C++ (`main`) | Rust (this library) |
|---|---|---|
| Engine | `std::mt19937` | `ChaCha8Rng` |
| Seed source | `std::random_device` at first call | FNV-1a of the sequence bytes ⊕ `l · 0x9E3779B97F4A7C15` |
| RNG state | one `static`, shared by every call | one RNG per block size, per call |
| Same input twice in one process | different answers | identical |
| Same input in two processes | different answers | identical |
| Under a thread pool | different answers, **and a data race** on the shared engine | identical |

</div>

Practical consequence for anyone migrating results: a C++ `emc` figure in an old notebook cannot be
reproduced, not even by the C++ binary that produced it. A Rust `emc` figure can be reproduced from
the input file indefinitely. The remaining behavioural differences between the two implementations
are catalogued in [Rust vs C++](cpp-parity.md).

---

## What reproducibility does not buy you

### A different sequence gets a different surrogate

The seed is a hash of the whole sequence. Change one symbol and the FNV-1a base changes completely,
so every surrogate is redrawn from an unrelated stream. `emc` is not a continuous function of its
input in any useful sense — a small perturbation resamples the Monte Carlo.

!!! example "One flipped symbol moves `emc` while `complexity` and `h` do not move at all"

    ```python
    import lzcomplexity as lz

    seq = open("markov.txt").read().strip()
    flipped = seq[:4000] + ("1" if seq[4000] == "0" else "0") + seq[4001:]

    print("hamming distance :", sum(a != b for a, b in zip(seq, flipped)))
    print("complexity       :", lz.lz76(seq)["complexity"], "->", lz.lz76(flipped)["complexity"])
    print("h                :", lz.h(seq), "->", lz.h(flipped))
    print("emc              :", lz.emc(seq)[0], "->", lz.emc(flipped)[0])
    ```

    <div class="lz-run" markdown>

    ```console
    $ python3 perturb.py
    hamming distance : 1
    complexity       : 301 -> 301
    h                : 0.4776611328125 -> 0.4776611328125
    emc              : 1.30126953125 -> 1.2495814732142858
    ```

    </div>

    The factorization of the input is untouched: same 301 components, same entropy density to the
    last bit. The entire change in `emc` comes from the surrogates — all twenty of them are redrawn
    from an unrelated stream, so the whole ladder shifts and the projection lands somewhere else. The
    direction and size of the jump are arbitrary.

    The two values are not even the same *kind* of rational. The original ladder is monotone at the
    top, so its value is `mm · g · (c_mm − c) = 820 · g` with `g = log₂(n)/n = 13/8192`. The flipped
    one has its top seven rungs pooled, so its value is their mean, `5512/7 · g` — the trailing run
    of six zeros in `summands` is what tells you the top block has size seven.

### The estimate still carries sampling error

The value is read off a ladder of `mm` rungs, one per block size, and each rung rests on exactly one
surrogate draw:

<div class="lz-formula">
  <p class="lz-math"><i>Ê</i>(<i>l</i>) = <i>l</i> · <i>g</i> · ( <i>C</i><sub>LZ</sub>(<i>u</i><sup>RS(<i>l</i>)</sup>) − <i>C</i><sub>LZ</sub>(<i>u</i>) ),&emsp;<i>Ê</i> = non_negative_isotonic( <i>Ê</i>(1) … <i>Ê</i>(<i>mm</i>) )(<i>mm</i>)</p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd>the largest block size — <code>lz76(seq)["emc"]["max_block_size"]</code></dd>
    <dt><i>u</i><sup>RS(<i>l</i>)</sup></dt><dd>the block-shuffled surrogate at block size <i>l</i>; <b>one</b> realisation per scale</dd>
    <dt><i>g</i></dt><dd>log<sub><i>k</i></sub>(<i>n</i>) ⁄ <i>n</i></dd>
  </dl>
  <p class="lz-formula__cite">Projecting rather than summing the first differences is what keeps all <i>mm</i> scales in the total; it also halves the spread over an i.i.d. null. It does not turn one draw per scale into an ensemble. Derived in <a href="../../concepts/emc/">Effective measure complexity</a>.</p>
</div>

So the returned scalar is a deterministic function of `mm` integers, each from a single shuffle.
Repeating the call re-derives the same shuffles from the same seeds and returns the same value. The
variance of the statistic is invisible from inside the library.

!!! danger "A repeated `emc` call is not a replication, and reporting it as one overstates the evidence"

    Determinism makes the number auditable, not accurate. Two separate properties get conflated when
    `emc` is reported without a null:

    - **Reproducible** — the same input always yields the same output. True here, by construction.
    - **Precise** — the output is close to the quantity it estimates. Not established. Measured over
      200 independent i.i.d. binary sequences at `n = 8 192` (`iid(8192, 0.5, seed=s)` for
      `s = 0 … 199`, generator as in [Effective measure complexity](../concepts/emc.md)), where the
      true excess entropy is exactly 0:

    ```text
    mean 0.0771   sd 0.0882   min 0.0000   max 0.3809
    95% interval [0.0000, 0.2928]   negative in 0/200   exactly zero in 67/200
    ```

    Each of those 200 numbers is perfectly reproducible, and none of them is negative — but the
    correct answer is 0 and the mean is 0.0771, so **the null is offset upward, not centred**. A
    reproducible 0.15 on this input would be pure noise. That offset is the price of the
    non-negativity constraint: on structureless input the estimator can only err in one direction.

    Only the *zero* is now unambiguous. `lz.emc("01" * 4096)`, a perfectly periodic input, returned
    `0.0` under 1.0.1 for a completely different reason — the aligned block shuffle at `mm = 18` was
    the identity — and now returns `0.904541015625`.
    [Effective measure complexity](../concepts/emc.md) tabulates the noise floor by length.

---

## Getting an uncertainty estimate anyway

Resample the surrogates yourself, with your own seeds. The recipe mirrors the library's estimator —
rebuild the whole ladder, project it, take the top — but draws a fresh uniform permutation of the
blocks at every scale on every repetition instead of reusing one.

```python
import random
import statistics
import lzcomplexity as lz


def pava_nonneg(y):
    """Project onto {0 <= v1 <= v2 <= ...}: pool adjacent violators, then clamp."""
    sums, counts = [], []
    for v in y:
        sums.append(float(v))
        counts.append(1)
        while len(sums) > 1 and sums[-2] / counts[-2] > sums[-1] / counts[-1]:
            s, c = sums.pop(), counts.pop()
            sums[-1] += s
            counts[-1] += c
    out = []
    for s, c in zip(sums, counts):
        out.extend([max(0.0, s / c)] * c)
    return out


def emc_ensemble(seq, reps=40, seed=0):
    """Resample every rung of the block-shuffle ladder with your own seeds."""
    mm = lz.lz76(seq)["emc"]["max_block_size"]
    h0 = lz.h(seq)
    rng = random.Random(seed)
    out = []
    for _ in range(reps):
        ladder = []
        for l in range(1, mm + 1):
            nblocks = len(seq) // l
            head, tail = seq[: nblocks * l], seq[nblocks * l :]
            blocks = [head[i * l : (i + 1) * l] for i in range(nblocks)]
            rng.shuffle(blocks)
            ladder.append(l * (lz.h("".join(blocks) + tail) - h0))
        out.append(pava_nonneg(ladder)[-1])
    return out


seq = open("markov.txt").read().strip()
draws = sorted(emc_ensemble(seq, reps=40, seed=0))

print(f"lz.emc      {lz.emc(seq)[0]:.4f}   (one realisation, fixed seed)")
print(f"mean        {statistics.mean(draws):.4f}")
print(f"sd          {statistics.stdev(draws):.4f}")
print(f"range       [{draws[0]:.4f}, {draws[-1]:.4f}]")
```

<div class="lz-run" markdown>

```console
$ python3 emc_ci.py
lz.emc      1.3013   (one realisation, fixed seed)
mean        1.2404
sd          0.0992
range       [1.1085, 1.5552]
```

</div>

The library's single draw, 1.3013, sits inside the range, 0.61 sd above the mean. Nothing should be
read into that offset: the library's surrogate applies ⌊n/2⌋ pairwise block swaps rather than a
uniform permutation, but at these sizes that is ~10 transpositions per block and the two mix
comparably. Read `sd = 0.0992` as the answer to "how much of my `emc` is the draw?"

!!! note "This is `mm` times as expensive as the pre-1.0.1 version of the recipe"

    The old recipe resampled only the scale-`mm` surrogate, because that was the only one the total
    depended on. Now every rung matters, so each repetition costs `mm` entropy calls rather than one —
    hence 40 reps here rather than 200. If that is too slow, resample at a reduced `max_block_size`
    and compare like with like; do not resample one scale and pretend it is the whole ladder.

That is a narrower question than "could this value have come from an unstructured sequence?" The
i.i.d. null measured above has a 95 % interval of `[−0.3491, 0.2856]` at the same length, so a gap of
less than about 0.35 between two `emc` values at `n = 8 192` is not evidence of anything. Compute
both: the ensemble sd tells you the Monte-Carlo error on one sequence, the null tells you the
detection threshold.

Two ways to spend the ensemble:

| Goal | What to report |
|---|---|
| An error bar on one sequence | mean and sd of `emc_ensemble(seq)` |
| A null for "is there structure at all?" | the same statistic computed on a fully symbol-shuffled copy of `seq`, repeated; then compare `lz.emc(seq)` against that distribution |

Because you supply the seed, this is reproducible too: `emc_ensemble(seq, seed=0)` returns the same
200 numbers every time. Record the seed in your analysis script alongside the library version.

---

## Parallelism does not affect results

The `mm` surrogate factorizations run on a `rayon` parallel iterator, and the `nid` path uses
`rayon::join` for its four factorizations. Neither can perturb a result:

- The parallel work is over **independent block sizes**. Each task derives its own seed from the
  once-computed content hash, so there is no shared RNG state and no task can observe another's
  draws.
- The reduction is **by index** — `res[idx] = c`, not a push onto a shared vector. Whatever order the
  tasks finish in, `res[l]` holds the surrogate complexity for block size `l`.
- `rayon::join` returns both results as a tuple; the arithmetic that consumes them is fixed.

!!! tip "Thread count is a pure performance knob — set it freely"

    `RAYON_NUM_THREADS` at 1, 5 and 16 all produce the identical 64-bit pattern
    `3ff4d20000000000` in the transcript above. Pin threads to fit a cluster scheduler, or let rayon
    take every core, without recording the choice as a methodological parameter.
    [Performance](performance.md) has the timings that tell you what the choice buys.

---

## Floating point

Two numerically identical formulas can round differently, and two of them ship in this project.

### The CLI and Python entropy paths can differ by a few ULP

The core — and therefore Python — computes `c / (n / (ln n / ln b))`. The `lzcomplexity` CLI computes
`c · ln n / (n · ln α)`. Same value in exact arithmetic, different order of operations in IEEE-754.

<div class="lz-run" markdown>

```console
$ ./target/release/lzcomplexity n47.txt -F TXT -o n47.json
$ python3 compare.py
CLI     1.0636446737255048 3ff104b04703733c
Python  1.063644673725505  3ff104b04703733d
diff    -2.220446049250313e-16
equal   False
```

</div>

That input is the 47-symbol string `01001010101101010101110101010101010000100101011`. A randomised
sweep of 20 000 `(n, k, c)` triples (`n` up to 10⁶, `k` up to 64) found the two forms disagreeing in
46 % of cases — by 1 ULP in 94 % of those disagreements, by 2 in the rest, and never by more than 3.

!!! warning "A golden-file test comparing CLI JSON to Python floats with `==` will fail on about half your fixtures"

    Compare with a tolerance. `abs(a - b) < 1e-12` is generous and still far tighter than any
    statistical claim you would make from these numbers. The same applies to exact assertions on
    `h`, `epsilon`, `normal_error` and every `emc` field.

    This is a rounding difference only when both paths resolve the same base, and they often do not:
    the CLI's entropy density follows `-a`, Python's follows `log_base`, and passing `-l 2` to the
    CLI does not change its `lz76EntropyDensity` at all. That is a much larger discrepancy than
    rounding, and it is covered in [Entropy density](../concepts/entropy-density.md).

### Across machines

The discrete part of every computation — the seed chain, the shuffle permutation, the LZ76 factor
counts — is exact integer arithmetic and is identical on every platform. The floats are then derived
from those integers with `f64::ln` and a handful of arithmetic operations. `ln` resolves to the
platform's libm and is not contractually correctly-rounded, so the final bit of `h` or `emc` is not
guaranteed byte-identical between, say, glibc and macOS. Everything upstream of it is.

In practice the common cases come out exact: whenever `k = 2` and `n` is a power of two,
`g = log₂(n)/n` is a dyadic rational, and once `ln(n)/ln(2)` rounds to the exact integer the rest of
the computation is exact in binary floating point. That is why the worked values on this page
terminate in clean binary fractions — `h = 301 · 13 / 8192` exactly. That first rounding is the step
libm does not guarantee, so treat the clean fractions as an observation on this platform, not a
contract.

!!! note

    The crate ships a second, unused content hash. The private `deterministic_seed` (djb2,
    `sequence.rs:399`) feeds `shuffle_copy`, which has no caller anywhere in the workspace — the EMC
    path calls `shuffle_copy_seeded` with the FNV-1a seed described above. `shuffle_copy` is `pub` in
    a `pub` module, so it is part of the published 1.0 surface and cannot be removed without a
    breaking change. A Rust caller who reaches for it gets a different, but equally deterministic,
    permutation. See [Rust crate](../api/rust.md).

---

## Checklist for a reproducible study

<div class="lz-cards" markdown>

<div class="lz-card" markdown>

### Record the version

The estimator changed shape at 0.13.0 — `emc` moved to the block-entropy form, and the formula it
replaced was non-negative by construction. A number without a version is not reproducible.
<p class="lz-card__api"><code>lz.__version__</code></p>

</div>

<div class="lz-card" markdown>

### Pin the length

`mm` is a function of `n`, and it enters `Ê` as an explicit factor, so two lengths are two
estimators. Truncate or resample every sequence in a study to one common length, or pin
`max_block_size` explicitly.
<p class="lz-card__api"><code>lz.emc(seq, max_block_size=20)</code></p>

</div>

<div class="lz-card" markdown>

### Pin the log base

`Ê ∝ 1/ln(log_base)`, and `h` scales the same way. Auto-detection reads the alphabet actually
present, so a sequence that happens to contain three symbols instead of two changes base silently.
<p class="lz-card__api"><code>lz.h(seq, log_base=2)</code></p>

</div>

<div class="lz-card" markdown>

### Keep the raw bytes

The seed is a hash of the input. Any preprocessing — a trailing newline, a different line ending, a
`bytearray` where you meant `bytes` — is a different sequence and therefore a different surrogate.
<p class="lz-card__api"><code>open(path, "rb").read()</code></p>

</div>

<div class="lz-card" markdown>

### Ship your surrogate seed

If you report an ensemble error bar, the seed you passed to `random.Random` is part of the method.
The library's own seed needs no recording — it is derived from the data.
<p class="lz-card__api"><code>emc_ensemble(seq, seed=0)</code></p>

</div>

<div class="lz-card" markdown>

### Compare floats with a tolerance

Never `==` across the CLI/Python boundary, across platforms, or against a stored fixture. `1e-12` is
tight enough to catch a real regression and loose enough to survive rounding.
<p class="lz-card__api"><code>abs(a - b) &lt; 1e-12</code></p>

</div>

</div>

<div class="lz-tickrule"></div>

Next: [Performance](performance.md) for what the shuffle path costs,
[Rust vs C++](cpp-parity.md) for the rest of the differences from the original implementation, and
[Reading the numbers](../guide/reading-the-numbers.md) for how to interpret these values once you
trust them.
