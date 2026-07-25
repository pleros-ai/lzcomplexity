# Rust crate

*`lzcomplexity-core` is the algorithm crate. The Python package and both CLI tools are skins over it.*

Everything the Python API returns is computed by `lzcomplexity-core`. Depending on it directly
removes the PyO3 layer and hands you the result structs — including fields the Python dict never
surfaces.

```toml
[dependencies]
lzcomplexity-core = "1.0"
```

Or `cargo add lzcomplexity-core`. Only this crate is published to crates.io; `lzcomplexity-py`
ships inside the wheel and `lzcomplexity-cli` ships as GitHub release binaries.

<div class="lz-stats" markdown>
<div class="lz-stat"><span class="lz-stat__v">1.75</span><span class="lz-stat__k">MSRV</span></div>
<div class="lz-stat"><span class="lz-stat__v">2021</span><span class="lz-stat__k">edition</span></div>
<div class="lz-stat"><span class="lz-stat__v">0</span><span class="lz-stat__k">cargo features</span></div>
<div class="lz-stat"><span class="lz-stat__v">4</span><span class="lz-stat__k">direct dependencies</span></div>
<div class="lz-stat"><span class="lz-stat__v">MIT</span><span class="lz-stat__k">licence</span></div>
</div>

| | |
|---|---|
| Crate name | `lzcomplexity-core` |
| Library name (what you `use`) | `lzcomplexity_core` |
| Crate type | rlib |
| Direct dependencies | `rayon 1.10`, `rand 0.8`, `rand_chacha 0.3`, `cdivsufsort 2.0` |
| Generated API docs | [docs.rs/lzcomplexity-core](https://docs.rs/lzcomplexity-core) |
| Registry | [crates.io/crates/lzcomplexity-core](https://crates.io/crates/lzcomplexity-core) |

## Cargo features

There are none. The manifest has no `[features]` table, so there is no way to opt out of any
dependency — in particular `rayon` is unconditional and there is no serial-only build. To run
single-threaded, size the thread pool instead ([Threads](#threads-and-the-rayon-pool)).

!!! warning "A source build fails without a C compiler, not only a Rust toolchain"

    `cargo build` stops with a `cc` error on a machine that has rustc but no C compiler.
    `cdivsufsort` is a Rust binding that compiles Yuta Mori's vendored C `libdivsufsort` sources
    through a `cc` build dependency. On Windows that means the MSVC build tools. The repository
    README's "no C++ toolchain" line is true as written — but a **C** toolchain is required.

!!! note "The MSRV is declared, not tested"

    `rust-version = "1.75"` is recorded in the workspace manifest and on crates.io for every
    published version. CI builds with `dtolnay/rust-toolchain@stable` only — there is no 1.75
    job, so an accidental MSRV bump would not be caught. Treat 1.75 as a claim, not as a
    guarantee backed by a build.

<div class="lz-tickrule"></div>

## Module map

Six public modules. The pipeline: `sequence` holds the bytes, `suffix_array` and `lpf` build the
index, `lz76` walks it, and `shuffle` and `metrics` build the derived measures on top. The table
is ordered by how often you will reach for each one, not by pipeline order.

| Module | Contains | Reach for it when you want |
|---|---|---|
| `sequence` | `Sequence`, `shuffle_in_place`, `shuffle_copy`, `shuffle_copy_seeded` | to construct input, or to slice / shift / concatenate bytes |
| `lz76` | `factorize`, `lz76_factorization`, `lz76_factors`, `lz76_entropy_density`, `entropy_density_from`, and every result struct | complexity, factor boundaries, entropy density |
| `lpf` | `lpf` — the Crochemore–Ilie longest-previous-factor pass | nothing, unless you are reimplementing the parse |
| `suffix_array` | `build`, `build_suffix_array`, `build_lcp`, `SuffixArray` | a suffix array or LCP array for your own purposes |
| `shuffle` | `lz76_random_shuffle_complexity`, `shuffle_factorization`, `max_block_size`, `merge_sequences` | EMC and the block-shuffle ladder |
| `metrics` | `lz76_information_distance`, `lz76`, `lz76_extras`, `mutual_information`, and the `_z` variants | NID, the full `LempelZiv` bundle, the extras |

The crate root re-exports the types you need most, but **not the functions**:

```rust
pub use lz76::{LempelZiv, Lz76Result, LzArgs, LzExtra, LzShuffle};
pub use sequence::Sequence;
pub use suffix_array::{build_suffix_array, SuffixArray};

pub const NO_ALPHABET: u32 = u32::MAX;
pub const ALPHABET_SIZE: u32 = 2;
```

So `LzArgs` and `Sequence` come from the crate root, while `lz76_entropy_density` and
`lz76_information_distance` have to be reached through `lz76::` and `metrics::`.

<div class="lz-tickrule"></div>

## Building a `Sequence`

`Sequence` is a byte vector plus a cached alphabet. Three constructors matter:

```rust
use lzcomplexity_core::Sequence;

let a = Sequence::from_str("banana");                          // &str, bytes copied
let b = Sequence::from_bytes(vec![0u8, 1, 0, 1, 1, 0, 0, 1]);  // raw bytes, any values
let c = Sequence::from_bytes_with_alphabet(vec![0, 1, 2], 3);  // bytes + a proposed alphabet size
```

All three run `determine_alphabet()`, which scans the bytes, counts distinct values and floors
the result at 2. A constant string therefore reports `alphabet_size() == 2`, not 1.

```rust
assert_eq!(Sequence::from_str("aaaa").alphabet_size(), 2);
assert_eq!(Sequence::from_bytes(vec![0, 1, 0, 1, 1, 0, 0, 1]).alphabet_size(), 2);
```

!!! warning "`from_bytes_with_alphabet` proposes an alphabet size; the scan overrules it"

    `determine_alphabet()` keeps the value you passed only when it already equals the distinct-byte
    count, and otherwise replaces it with `max(2, distinct)`. So
    `from_bytes_with_alphabet(vec![0, 1], 10).alphabet_size()` is `2`, not `10`. To override the
    alphabet for a measurement, set `LzArgs::alphabet` — that value is not re-derived — or call
    `set_alphabet_size` after construction.

`from_str` is an inherent method, not the `FromStr` trait — it is infallible, so there is no
`Result` to unwrap and no `.parse()` form.

!!! warning "A non-ASCII `&str` becomes more symbols than it has characters"

    `Sequence::from_str("héllo")` has `len() == 6` and `alphabet_size() == 5`, because `é` is two
    UTF-8 bytes and each byte is a separate symbol. Every measure in this crate operates on
    bytes. If your alphabet has more than 256 members, or you want one symbol per Unicode scalar,
    map it to bytes yourself before constructing the `Sequence`. See
    [Alphabets and log bases](../concepts/alphabets.md).

Beyond the constructors: `as_bytes`, `len`, `take(l)`, `drop(l)`, `split_at(l)`, `reverse_copy`,
`right_shift` / `left_shift`, `granularity(gr)`, `char_density`, `map(f)`, `xor_with`, and
`&a + &b` for concatenation.

<div class="lz-tickrule"></div>

## `LzArgs` — every field

Every measure entry point takes `&LzArgs` — the `lz76*` functions in `lz76`, `shuffle` and
`metrics`. `LzArgs::new()` and `LzArgs::default()` are the same thing and are what you want unless
you are deliberately overriding the alphabet, the log base or the EMC block-size ceiling.

<div class="lz-api" id="lz-lzargs">
  <p class="lz-api__sig"><b>LzArgs</b> { chunks, max_context, block_size, get_shuffle_terms, alphabet, log_base } <span class="lz-api__badge lz-api__badge--stable">stable</span></p>
  <p class="lz-api__lede">Configuration for every <code>lz76*</code> entry point. Derives <code>Clone</code> and <code>Debug</code>; the hand-written <code>PartialEq</code> compares five of the six fields — <code>log_base</code> is not part of the equality.</p>
  <dl class="lz-api__params">
    <dt>chunks</dt><dd><code>i32</code>, default <code>0</code> — <b>inert</b>. Never read by any algorithm in the workspace.</dd>
    <dt>max_context</dt><dd><code>i32</code>, default <code>0</code> — <b>inert</b>. Never read anywhere.</dd>
    <dt>block_size</dt><dd><code>i32</code>, default <code>-1</code> — the EMC block-size ceiling <i>mm</i>. Any value <code>&lt;= 0</code> means auto-select. Read by <code>shuffle::shuffle_factorization</code> only.</dd>
    <dt>get_shuffle_terms</dt><dd><code>bool</code>, default <code>false</code> — populate <code>LzShuffle::summands</code> with the per-scale terms. Off by default in Rust; the Python <code>emc()</code> and <code>lz76()</code> bindings hard-code it to <code>true</code>.</dd>
    <dt>alphabet</dt><dd><code>u32</code>, default <code>NO_ALPHABET</code> — the effective alphabet size. Feeds <code>epsilon</code> and nothing else. Clamped to a floor of 2.</dd>
    <dt>log_base</dt><dd><code>u32</code>, default <code>NO_ALPHABET</code> — the logarithm base for entropy density and <code>epsilon</code>. Clamped to a floor of 2; pass <code>2</code> for bits.</dd>
  </dl>
</div>

### The `NO_ALPHABET` sentinel

`alphabet` and `log_base` are `u32`, so there is no `Option` to hold "unset". The crate uses a
sentinel instead:

```rust
pub const NO_ALPHABET: u32 = u32::MAX;   // 4294967295
```

A field equal to `NO_ALPHABET` means "take it from the sequence" — `factorize` substitutes
`seq.alphabet_size()`. Any other value overrides the auto-detection, and is then clamped upward
to 2, so `0` and `1` are silently equivalent to `2`.

```rust
use lzcomplexity_core::{lz76, LzArgs, Sequence};

let seq = Sequence::from_str("banana");

let auto = LzArgs::new();                     // alphabet = log_base = NO_ALPHABET -> 3
assert_eq!(lz76::lz76_entropy_density(&seq, &auto), 0.8154648767857287);

let mut bits = LzArgs::new();
bits.log_base = 2;                            // report h in bits instead
assert_eq!(lz76::lz76_entropy_density(&seq, &bits), 1.292481250360578);
```

!!! warning "Two of the six fields do nothing, and do it silently"

    Setting `chunks` or `max_context` changes no returned value. Neither field is read by any
    code path in the workspace. `chunks` is what the CLI's `--partitions` flag and the Python
    `partitions=` argument write to, so a caller who "tunes" it gets identical numbers *and*
    identical timings. They exist because the struct mirrors the C++ `LZ_Args`; treat them as
    reserved.

!!! note "`alphabet` moves `epsilon` only"

    Inside `factorize`, `alphabet` and `log_base` feed only the `epsilon` expression. The factor
    count, the boundary vector and `factors_stddev` depend on neither. `log_base` re-enters
    downstream in `entropy_density_from`, which applies its own independent floor-of-2 clamp;
    `alphabet` does not re-enter at all.

<div class="lz-tickrule"></div>

## Worked examples

One program, four entry points. These are the same values the Python API returns for the same
inputs — the binding does not round, rescale or reorder anything.

```rust title="src/main.rs"
use lzcomplexity_core::{lz76, metrics, shuffle, LzArgs, Sequence};

fn main() {
    let args = LzArgs::new();

    // 1 — complexity and factor boundaries
    let r = lz76::lz76_factors(&Sequence::from_str("banana"), &args);
    println!("c = {}  factors = {:?}", r.factorization, r.lzf);

    // 2 — normalised entropy density
    let h = lz76::lz76_entropy_density(&Sequence::from_str("01010101"), &args);
    println!("h = {h}");

    // 3 — normalised information distance
    let d = metrics::lz76_information_distance(
        &Sequence::from_str("abcd"),
        &Sequence::from_str("abce"),
        &args,
    );
    println!("nid = {d}");

    // 4 — effective measure complexity (ask for the per-scale terms)
    let mut emc_args = LzArgs::new();
    emc_args.get_shuffle_terms = true;
    let s = shuffle::lz76_random_shuffle_complexity(
        &Sequence::from_str(&"01".repeat(64)),
        &emc_args,
    );
    println!("emc = {}  mm = {}", s.emc_value, s.max_block_size);
    println!("summands = {:?}", s.summands);
}
```

<div class="lz-run" markdown>

```text
$ cargo run --release
c = 3  factors = [0, 1, 2, 3, 7]
h = 0.75
nid = 0.25
emc = 2.4609375  mm = 15
summands = [1.0390625, -1.0390625, 1.3125, -1.3125, 1.640625, -1.640625, 1.9140625,
            -1.9140625, 1.96875, -1.96875, 3.0078125, -3.0078125, 2.84375, -2.84375, 2.4609375]
```

</div>

The same four calls from Python:

<div class="lz-scroll lz-compare" markdown>

| Rust | Python | Value |
|---|---|---|
| `lz76::lz76_factors(&s, &args)` → `.factorization`, `.lzf` | `lz.factorization("banana")` | `(3, [0, 1, 2, 3, 7])` |
| `lz76::lz76_entropy_density(&s, &args)` | `lz.h("01010101")` | `0.75` |
| `metrics::lz76_information_distance(&a, &b, &args)` | `lz.nid("abcd", "abce")` | `0.25` |
| `shuffle::lz76_random_shuffle_complexity(&s, &args)` | `lz.emc("01" * 64)` | `2.4609375` and 15 summands |
| `metrics::lz76(&s, &args)` | `lz.lz76(seq)` | the full bundle |

</div>

!!! tip "Set `get_shuffle_terms` if you want the summands — Rust leaves it off"

    `LzArgs::new()` leaves `get_shuffle_terms = false`, so `LzShuffle::summands` comes back empty
    and `summands.len() == 0`. The Python `emc()` and `lz76()` bindings force it to `true`, which
    is why `len(summands) == max_block_size` always holds from Python and not from Rust.
    `emc_value` is identical either way — `2.4609375` for `"01" * 64` with the flag on and with it
    off — so the flag costs nothing but the allocation.

### Cheaper variants when you already have the count

Three functions take a pre-computed complexity so you do not pay for a second factorization:

| Function | Signature | Saves |
|---|---|---|
| `lz76::entropy_density_from` | `(count: u32, &Sequence, &LzArgs) -> f64` | one `factorize` |
| `shuffle::lz76_random_shuffle_complexity_with` | `(&Sequence, &LzArgs, complexity: i32) -> LzShuffle` | one `factorize` |
| `metrics::lz76_extras_with` | `(&Sequence, &LzArgs, c_lz: f64) -> LzExtra` | one `factorize` |

`metrics::lz76` uses all three internally: it factorizes the sequence once and threads the count
through the entropy, EMC and extras stages.

<div class="lz-tickrule"></div>

## Result structs

### `Lz76Result` — from `lz76_factors`

| Field | Type | Meaning |
|---|---|---|
| `factorization` | `u32` | the complexity `c(S)` — complete LZ76 components only |
| `epsilon` | `f64` | the Lempel–Ziv finite-size correction term |
| `lzf` | `Vec<u32>` | half-open factor boundaries; factor *k* is `S[lzf[k-1] .. min(lzf[k], n)]` |

### `LzShuffle` — from `lz76_random_shuffle_complexity`

| Field | Type | Meaning |
|---|---|---|
| `max_block_size` | `i32` | the block-size ceiling *mm* actually used |
| `emc_value` | `f64` | the effective measure complexity |
| `multi_information` | `f64` | the `l = 1` term of the sum, kept separately |
| `summands` | `Vec<f64>` | the *mm* per-scale terms — **empty unless `get_shuffle_terms` is set** |

### `LzExtra` — from `lz76_extras`

Five ratios computed by splitting **one** sequence at `len() / 2` and comparing the two halves.
They are not a comparison of two inputs.

| Field | Type | Formula |
|---|---|---|
| `lz_rajski_distance` | `f64` | `2 − (fh + lh) / c` |
| `redundancy` | `f64` | `mi / (fh + lh)` |
| `fh_uncertainty` | `f64` | `mi / fh` |
| `lh_uncertainty` | `f64` | `mi / lh` |
| `lz_pearson_coefficient` | `f64` | `mi / sqrt(fh · lh)` |

`fh` and `lh` are the complexities of the first and second half, and `mi = fh + lh − c`. For odd
lengths `mid = len() / 2` rounds down, so the second half is one symbol longer.

### `LempelZiv` — from `metrics::lz76`

| Field | Type | Notes |
|---|---|---|
| `complexity` | `u32` | |
| `entropy_density` | `f64` | `0.0` for `len() <= 1` |
| `random_shuffle_complexity` | `LzShuffle` | the EMC bundle |
| `paired_shuffle_complexity` | `LzShuffle` | **always `Default::default()`** — hard-wired, never populated |
| `lz_normal_errors` | `f64` | `sqrt(h³) · factors_stddev / sqrt(n / log_b n)` |
| `lz_poison_errors` | `f64` | `h / n` |
| `epsilon` | `f64` | |
| `factors_stddev` | `f64` | see below |
| `factors` | `Vec<u32>` | the boundary vector |
| `extras` | `LzExtra` | |

!!! danger "`factors_stddev` is not a standard deviation, and `lz_normal_errors` inherits that"

    Reading `lz_normal_errors` as an error bar will misstate your uncertainty. `factors_stddev`
    divides the sum of squares by the **largest factor length** instead of the sample count, and
    centres on `(lzf.last() − 1) / lzf.len()` instead of the mean factor length. It is non-zero
    even for a perfectly uniform factorization: `"abc"` gives `0.8660254037844386` where a
    genuine σ would be `0`. Both it and `lz_normal_errors` are heuristic dispersion indicators
    with no sampling interpretation.

!!! warning "`paired_shuffle_complexity` is always zero"

    Reading it gives `LzShuffle::default()` — `max_block_size: -1`, `emc_value: 0.0`,
    `multi_information: 0.0`, empty `summands` — for every input. `metrics::lz76` assigns
    `Default::default()` to it unconditionally. The function that would fill it,
    `shuffle::lz76_paired_shuffle_complexity`, exists and works; nothing calls it. Call it
    yourself if you want the value.

<div class="lz-tickrule"></div>

## Threads and the rayon pool

The crate uses `rayon` in three places, and **`factorize` is not one of them**.

| Entry point | Parallelism | Shape |
|---|---|---|
| `lz76::factorize`, `lz76_factorization`, `lz76_factors`, `lz76_entropy_density` | **none** | single-threaded end to end: suffix array, LCP, LPF, factor walk |
| `metrics::lz76_information_distance` | nested `rayon::join` | the 4 independent factorizations `C(X)`, `C(Y)`, `C(XY)`, `C(YX)` |
| `metrics::lz76_extras_with` | `rayon::join` | the 2 half-sequence factorizations |
| `shuffle::shuffle_factorization` | `into_par_iter` | the *mm* block-shuffle factorizations of the EMC ladder |

A single `lz76_factors` or `lz76_entropy_density` call therefore saturates exactly one core no
matter how large the pool is — the throughput figures on
[Performance](../project/performance.md) are single-core figures for that reason. EMC and NID
are where the pool earns its keep.

The crate never builds its own pool; it inherits rayon's global one. Three ways to control it:

```rust
// 1. Environment, no code change:  RAYON_NUM_THREADS=2 cargo run --release

// 2. Size the global pool once, before any parallel call.
rayon::ThreadPoolBuilder::new().num_threads(4).build_global().unwrap();

// 3. Scope a pool to one call site — composes with other libraries in the process.
let pool = rayon::ThreadPoolBuilder::new().num_threads(4).build().unwrap();
let s = pool.install(|| shuffle::lz76_random_shuffle_complexity(&seq, &args));
```

<div class="lz-run" markdown>

```text
$ cargo run --release --bin threads
default pool = 16
in 4-thread pool: emc = 2.4609375
pool threads = 4

$ RAYON_NUM_THREADS=2 cargo run --release --bin threads
default pool = 2
in 4-thread pool: emc = 2.4609375
pool threads = 4
```

</div>

Thread count changes timing and nothing else. Every parallel section computes independent pure
functions and reduces them in a fixed order, so results are bit-identical across pool sizes —
including the EMC value above, unchanged at 16, 4 and 2 threads. See
[Determinism](../project/determinism.md).

<div class="lz-tickrule"></div>

## Two conventions that will bite you

### `complexity` counts complete components only

The greedy parse advances `i += LPF[i] + 1`, so the final boundary is always exactly `n` or
`n + 1`. When it is `n + 1` the last production ran past the end of the sequence — its
innovation symbol is not there — and the crate does **not** count it. The textbook
exhaustive-history count — the Kaspar–Schuster loop that `antropy` and NeuroKit2 implement — does
count it, so it is `0` or `+1` above this crate's number.

The conversion is exact for any sequence with at least two distinct symbols. Re-checked against a
naive O(n³) reference implementation of the exhaustive history over a 4000-string sweep
(n = 2…60, alphabets `01`, `012`, `ACGT`): of the 3955 non-constant cases, the boundary vectors
matched with zero mismatches and the converted count matched the reference in all 3955. The 45
constant cases are the exception below.

<div class="lz-formula">
  <p class="lz-math"><i>c</i><sub>textbook</sub> = <i>c</i> + [ lzf.last() &gt; <i>n</i> ]</p>
  <dl class="lz-formula__key">
    <dt><i>c</i></dt><dd><code>Lz76Result::factorization</code> as returned by this crate</dd>
    <dt>lzf.last()</dt><dd>the final entry of <code>Lz76Result::lzf</code>, always <i>n</i> or <i>n</i> + 1</dd>
    <dt><i>n</i></dt><dd><code>seq.len()</code></dd>
  </dl>
</div>

```rust
let r = lz76::lz76_factors(&seq, &args);
let overshoot = (*r.lzf.last().unwrap() as usize) > seq.len();
let c_textbook = r.factorization + overshoot as u32;
```

| Sequence | `factorization` | `lzf.last()` | `n` | textbook |
|---|---|---|---|---|
| `banana` | 3 | 7 | 6 | **4** |
| `1011010100010` | 5 | 14 | 13 | **6** |
| `1001111011000010` | 6 | 16 | 16 | 6 |
| `010101010101` | 2 | 13 | 12 | **3** |

!!! warning "The conversion does not hold for a constant sequence"

    `lz76_factorization` and `lz76_factors` short-circuit when the input has fewer than two
    distinct symbols: they return `factorization = 1`, `epsilon = 0.0` and a fabricated
    `lzf = [0, 1, n]` without running the parse at all. Because that vector never overshoots, the
    formula returns `1` — but the exhaustive history of `"aaaa"` is `a | aaa`, so the textbook
    count is `2`. The empty sequence takes the same branch and yields `lzf = [0, 1, 0]`, a
    descending vector. Guard on "fewer than two distinct bytes" before converting. Background:
    [LZ76 factorization](../concepts/lz76.md).

!!! danger "`lzf.len() - 1` is not the complexity, and `lzf.last()` can index past the end"

    Slicing with the raw boundary vector reads out of bounds and panics. `lzf.len() - 1` is the
    number of *parsed* factors, which exceeds `factorization` by one whenever the parse
    overshoots — the common case. Clamp every span: factor *k* is
    `&seq.as_bytes()[lzf[k-1] as usize .. (lzf[k] as usize).min(n)]`. Python slicing happens to
    clamp for you; Rust does not.

### The EMC sum telescopes to a single term

`shuffle_entropy_calculation` accumulates `Ê = Σ_{l=1..mm} [(H_l − H_{l−1}) − ĥ]`. With
`H_0 = 0` every intermediate `H_l` cancels algebraically and the total reduces to:

<div class="lz-formula">
  <p class="lz-math">Ê = <i>mm</i> · <i>g</i> · ( C<sub>LZ</sub>(<i>u</i><sup>RS(<i>mm</i>)</sup>) − C<sub>LZ</sub>(<i>u</i>) ), &nbsp; <i>g</i> = log<sub><i>k</i></sub>(<i>N</i>) ⁄ <i>N</i></p>
  <dl class="lz-formula__key">
    <dt><i>mm</i></dt><dd><code>LzShuffle::max_block_size</code> — the largest block size in the ladder</dd>
    <dt><i>u</i><sup>RS(<i>mm</i>)</sup></dt><dd>the sequence block-shuffled at scale <i>mm</i></dd>
    <dt><i>k</i></dt><dd>the log base: the detected alphabet size, or <code>LzArgs::log_base</code></dd>
    <dt><i>N</i></dt><dd><code>seq.len()</code></dd>
  </dl>
  <p class="lz-formula__cite">Only the largest block size reaches the total. The other <i>mm</i> − 1 factorizations are computed and then algebraically discarded — they survive only in <code>summands</code>.</p>
</div>

For `"01" * 64`: `N = 128`, `k = 2`, `mm = 15`, `C_LZ(u) = 2`, and `C_LZ` of the scale-15 shuffle
is `5`, so `15 · (7/128) · (5 − 2) = 2.4609375` — exactly the accumulated `emc_value` printed
above, bit for bit. The agreement is not guaranteed in general: `emc_value` is a running sum, so
it carries rounding the closed form does not, and the two can differ in the last few ulp.

!!! warning "Signs flip from scale to scale — that is the cancellation, not a bug"

    Printing `summands` for `"01" * 64` gives `[+1.0390625, −1.0390625, +1.3125, −1.3125, …]`,
    which reads as numerical garbage. The neat pairing is specific to that input, where every even
    scale happens to reproduce the original complexity. A 512-symbol pseudorandom binary string
    (`random.seed(3)`, `random.choice("01")`) gives
    `[−0.0176, +0.0527, +0.0176, −0.0527, 0.0, +0.1055, −0.3516, …]` with no such pattern. What is
    general is the telescoping: every intermediate `H_l` cancels, so the **total does not depend on
    the intermediate scales**. The per-scale terms remain informative — `H_l` is recoverable from
    the running sum of `summands[j] + ĥ`.

!!! danger "An `emc_value` of `0.0` means resonance, not absence of structure"

    Zero comes out exactly when the scale-*mm* shuffle leaves the complexity unchanged, which is a
    property of *mm* against the input, not a verdict on the input. Measured: `"0011" * 128`
    returns `0.0`, `"01" * 1000` returns `-4.440892098500626e-16`, and `"01" * 64` — no less
    periodic — returns `2.4609375`. Negative totals are normal and mean the shuffle **lowered** the
    complexity; the 512-symbol pseudorandom string above returns `-0.28125`.

The derivation and the surrogate-quality caveats are on
[Effective measure complexity](../concepts/emc.md).

<div class="lz-tickrule"></div>

## Where to go next

<div class="lz-cards" markdown>
<div class="lz-card" markdown>

### Generated API reference

Every signature, field and module, rendered from the source doc-comments and versioned per
release on [docs.rs](https://docs.rs/lzcomplexity-core).

<p class="lz-card__api"><code>docs.rs/lzcomplexity-core</code></p>

</div>
<div class="lz-card" markdown>

### Performance numbers

Wall-clock timings, scaling exponents, throughput and the EMC cost model, all measured on the
core entry points documented here: [Performance](../project/performance.md).

<p class="lz-card__api"><code>~12 MB/s on incompressible data</code></p>

</div>
<div class="lz-card" markdown>

### The Python surface

Five functions mapping onto the calls above, with the coercion rules for non-`str` inputs:
[Python API](python.md) and [Input types](inputs.md).

<p class="lz-card__api"><code>import lzcomplexity as lz</code></p>

</div>
<div class="lz-card" markdown>

### Rust vs C++

What the port changed, what it kept bit-identical and where the two libraries diverge:
[Rust vs C++](../project/cpp-parity.md).

<p class="lz-card__api"><code>289/289 differential sweep</code></p>

</div>
</div>
