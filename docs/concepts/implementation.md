# Inside the implementation

The Python library and command-line tools use the same Rust core. This page follows a sequence from its byte representation through factorization to the reported measures.

```text
Python inputs / CLI file readers / Rust Sequence
                       ↓
                  byte sequence
                       ↓
       suffix array → LCP → previous-match lengths
                       ↓
                 LZ76 factorization
                ↙        ↓         ↘
         entropy h        E           information distance
```

## 1. Sequence storage and alphabet

`Sequence` owns a `Vec<u8>`. The observed symbol list and its size are cached lazily using `OnceLock`. Symbol-changing operations invalidate that cache; the next metadata read reconstructs it. Appending many symbols does not rescan the growing vector after each append.

An explicit size set through `with_alphabet_size` or `set_alphabet_size` is retained until `clear` or `determine_alphabet`. The observed symbol list still describes the actual bytes. A mismatching constructor hint is replaced by observed metadata. See the [Rust reference](../api/rust.md) for the public methods.

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

Suffixes are comparison-sorted below 2,048 input bytes; larger inputs use `cdivsufsort`. There is no empty sentinel suffix in the returned array.

The suffix-array dependency bundles C code, so source builds require a C compiler.

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

`h` is calculated directly from the original count and length. `epsilon` uses the requested alphabet parameter and logarithm base. The full analysis also computes factor dispersion and the additional diagnostic formulas. These fields do not require estimating a source probability table.

The extras split the sequence at `N // 2`, factorize both halves, and form ratios from those counts and the whole-sequence count. Their formulas are listed in [reading the numbers](../guide/reading-the-numbers.md).

## 6. Structural complexity E

Each requested block size gets a fresh copy of the original. The core hashes the bytes once, derives a different seed per scale, performs aligned block swaps, and factorizes each surrogate. Different scales can run independently through Rayon.

The count differences are weighted by block range and combined through a nonnegative isotonic fit. [Calculating E](../shuffle-measure.md) gives the formula, sampling boundaries and automatic range selection.

## Work reuse and concurrency

The full `lz76()` call reuses the original factorization for entropy density, E and extras. Sampled copies and the two half-sequences are factorized separately.

`nid()` computes the counts of `X`, `Y`, `XY` and `YX` using nested independent Rayon tasks. Thread scheduling changes when tasks run, not their input sequences or factor-count formulas.

Python's `jobs` and `partitions` arguments do not control execution. Configure Rayon workers with `RAYON_NUM_THREADS` before the first native call. The CLI provides its own worker option; see the [CLI reference](../cli/index.md).

## Memory and representation limits

Suffix, LCP, LPF and factor-boundary arrays use memory proportional to input size. Each concurrent surrogate task also owns sequence and factorization buffers. More concurrent tasks can increase peak memory.

Indices and several counts use 32-bit integer types, while host lengths use `usize`; some internal paths also use signed 32-bit count storage. For large workloads, account for these index limits as well as available RAM, and measure peak memory on representative inputs.

## Source map

| Stage | Source |
|---|---|
| Sequence and shuffling primitives | [`sequence.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/sequence.rs) |
| Suffix array and Kasai LCP | [`suffix_array.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/suffix_array.rs) |
| Longest previous factors | [`lpf.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/lpf.rs) |
| Factor counts, boundaries, normalization | [`lz76.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/lz76.rs) |
| E calculation and seed derivation | [`shuffle.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/shuffle.rs) |
| Distances, extras and complete driver | [`metrics.rs`](https://github.com/pleros-ai/lzcomplexity/blob/rust-backend/crates/lzcomplexity-core/src/metrics.rs) |

For running-time and benchmark reporting, continue to [Performance](../project/performance.md).
