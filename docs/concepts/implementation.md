# Inside the implementation

Python, Rust and the two command-line tools share the current Rust core. This page follows one sequence from bytes to results, separating the current backend from the older C++ implementation described in the supplied manual.

```text
Python inputs / CLI file readers / Rust Sequence
                       ↓
                  byte sequence
                       ↓
       suffix array → LCP → previous-match lengths
                       ↓
                 LZ76 factorization
                ↙        ↓         ↘
         entropy h   block shuffles   count-based comparisons
```

## 1. Sequence storage and alphabet

`Sequence` owns a `Vec<u8>`. The observed symbol list and its size are cached lazily using `OnceLock`. Symbol-changing operations invalidate that cache; the next metadata read reconstructs it. Appending many symbols does not rescan the growing vector after each append.

An explicit size set through `with_alphabet_size` or `set_alphabet_size` is retained until `clear` or `determine_alphabet`. The observed symbol list still describes the actual bytes. Legacy constructor hints are not equivalent to explicit declarations: a mismatching hint is replaced by observed metadata. See the [Rust reference](../api/rust.md) for the public methods.

Operations that merely permute the bytes, such as reversing or block swapping, retain valid observed alphabet metadata.

## 2. Suffix array

A suffix array stores the starting positions of all suffixes in lexicographic order. For `banana`, the suffix order is

| Start | Suffix |
|---:|---|
| 5 | `a` |
| 3 | `ana` |
| 1 | `anana` |
| 0 | `banana` |
| 4 | `na` |
| 2 | `nana` |

The current implementation comparison-sorts suffixes below 2,048 input bytes and uses the `cdivsufsort` dependency for larger inputs. There is no empty sentinel suffix in the returned array.

The suffix-array dependency bundles C code, so building the Rust package from source also requires a C compiler. This backend does not use the manual's CaPS-SA/oneTBB construction.

## 3. Longest common prefixes

The longest-common-prefix array `LCP` records the shared prefix length of neighboring suffixes in suffix-array order. The Rust core constructs it with Kasai's algorithm. For the suffix order above:

```text
SA  = [5, 3, 1, 0, 4, 2]
LCP = [0, 1, 3, 0, 0, 2]
```

The first entry is zero. The comparison between `ana` and `anana`, for example, has a common prefix of length three.

## 4. Longest previous factors and parsing

The LPF stage transforms suffix-array/LCP information into previous-match lengths for positions in the original byte sequence. It uses a stack over array positions, then the factorization walk advances by `LPF[position] + 1`.

The first boundaries are `0, 1`. A final attempted phrase can extend past the input; its boundary stays in the output vector, but it is excluded from the completed-phrase count. Empty and constant sequences have a separate path. [LZ76 factorization](lz76.md) gives the complete public convention and worked examples.

## 5. Entropy density and diagnostics

`h` is calculated directly from the original count and length. `epsilon` uses the requested alphabet parameter and logarithm base. The full analysis also obtains factor-dispersion and legacy error formulas. These fields do not require estimating a source probability table.

The extras split the sequence at `N // 2`, factorize both halves, and form ratios from those counts and the whole-sequence count. They are not separately fitted statistical correlations.

## 6. Shuffle analysis

Each requested block size gets a fresh copy of the original. The core hashes the bytes once, derives a different seed per scale, performs aligned block swaps, and factorizes each surrogate. Different scales can run independently through Rayon.

The result combines the count contrasts using the released scale weighting and nonnegative isotonic fit. [The EMC-like measure](emc.md) explains the exact formula, suffix convention, automatic cutoff and interpretation.

## Work reuse and concurrency

The full `lz76()` call reuses the original factorization for entropy density, the shuffle analysis and extras. It still needs surrogate and half-sequence factorizations; “one call” does not mean one factorization in total.

`nid()` computes the counts of `X`, `Y`, `XY` and `YX` using nested independent Rayon tasks. Thread scheduling changes when tasks run, not their input sequences or factor-count formulas.

Python's `jobs` option is currently reserved and ignored. `partitions` is retained in the API but does not partition this Rust suffix-array implementation. CLI worker configuration is documented separately in the [CLI reference](../cli/index.md). These parameters must not be explained using the old C++ CaPS partition heuristics.

## Memory and representation limits

Suffix, LCP, LPF and factor-boundary arrays use memory proportional to input size. Each concurrent surrogate task also owns sequence and factorization buffers. More concurrency can increase peak memory; do not extrapolate timing or memory guarantees from the manual's historical machine.

Indices and several counts use 32-bit integer types, while host lengths use `usize`; some internal paths also use signed 32-bit count storage. The API does not provide a validated maximum-size contract for inputs near those representation limits. For large workloads, measure actual peak memory and use a practical size comfortably within the representation and available RAM.

## Source map

| Stage | Current source |
|---|---|
| Sequence and shuffling primitives | [`sequence.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/sequence.rs) |
| Suffix array and Kasai LCP | [`suffix_array.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/suffix_array.rs) |
| Longest previous factors | [`lpf.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/lpf.rs) |
| Factor counts, boundaries, normalization | [`lz76.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/lz76.rs) |
| Shuffle score and seed derivation | [`shuffle.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/shuffle.rs) |
| Distances, extras and complete driver | [`metrics.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/metrics.rs) |

For running-time and benchmark reporting, continue to [Performance](../project/performance.md).
