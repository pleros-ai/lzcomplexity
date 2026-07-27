//! Shuffle-based complexity measures.

use rayon::prelude::*;

use crate::block_lz76::{factorize_tokens, shuffle_tokens, tokenize_blocks};
use crate::lz76::{lz76_factorization, LzArgs, LzShuffle};
use crate::sequence::{shuffle_copy_seeded, Sequence};
use crate::NO_ALPHABET;

/// Estimate the "naive M" satisfying `N = M * 2^M`, mirroring `max_block_size` in C++.
pub fn max_block_size(size: usize) -> usize {
    if size < 10 {
        return 1;
    }
    let mut m: usize = 10;
    let mut m_old: usize = 1;
    let mut count: u32 = 0;
    while count < 100 {
        let est = ((size as f64) / (m_old as f64)).ln() / 2f64.ln();
        m = est.round() as usize;
        count += 1;
        if m == m_old {
            break;
        }
        m_old = m;
    }
    m
}

/// Compute the random-shuffle factorization vector and `mm`. Mirrors
/// `ShuffleFactorization` from the C++.
///
/// Used by [`crate::metrics::mutual_information`] and its `_z`/distance
/// variants. The EMC pipeline (`lz76RandomShuffleComplexity`) does **not**
/// use this anymore — see [`block_shuffle_factorization`] and
/// [`shuffle_entropy_calculation`]'s doc comment for why byte-level
/// shuffling is the wrong tool for that estimator specifically.
///
/// In auto mode (`block_size <= 0`) this stops at `max_block_size(N)`
/// exactly; older versions added a flat `+10` here for no stated reason.
pub fn shuffle_factorization(seq: &Sequence, args: &LzArgs) -> (Vec<i32>, usize) {
    let mut mm: i64 = args.block_size as i64;
    if mm <= 0 {
        mm = max_block_size(seq.len()) as i64;
    }
    let mm = mm as usize;

    // Index 0 unused, indices 1..=mm hold the per-block-size complexity.
    let mut res: Vec<i32> = vec![0; mm + 3];

    // Hash the sequence content ONCE; each per-block seed only differs by the
    // `idx` term (bit-identical seeds → identical shuffles → identical EMC).
    let base = fnv1a(seq.as_bytes());
    let computed: Vec<(usize, i32)> = (1..=mm)
        .into_par_iter()
        .map(|idx| {
            let seed = seed_from_base(base, idx);
            let rand_seq = shuffle_copy_seeded(seq, idx as u32, (seq.len() / 2) as u32, seed);
            let c = lz76_factorization(&rand_seq, args);
            (idx, c as i32)
        })
        .collect();
    for (idx, c) in computed {
        res[idx] = c;
    }
    (res, mm)
}

/// Per-block-size result of the block-aware EMC shuffle: the LZ76 factor
/// count of the shuffled *token* array at this block size, plus how many
/// tokens (blocks) it contains. Both are needed to convert back into `Ĥ_l`
/// — see [`shuffle_entropy_calculation`].
#[derive(Clone, Copy, Debug, Default)]
struct BlockLevel {
    factor_count: u32,
    num_blocks: usize,
}

/// Block-aware analogue of [`shuffle_factorization`], used only by the EMC
/// pipeline ([`lz76_random_shuffle_complexity_with`],
/// [`lz76_paired_shuffle_complexity`]).
///
/// For each block size `l`: tokenize `seq` into `l`-byte blocks
/// ([`tokenize_blocks`]), shuffle the *token array* — not the raw bytes —
/// with an exact Fisher-Yates permutation ([`shuffle_tokens`]), and
/// factorize at the token level ([`factorize_tokens`]). Doing it this way
/// (rather than block-shuffling the raw bitstream and running ordinary
/// single-symbol LZ76 on the result, as every earlier version of this
/// estimator did) is what [`shuffle_entropy_calculation`]'s doc comment
/// explains in detail: the byte-level version lets the parser match
/// through the artificial seams created by cutting-and-reassembling, which
/// systematically inflated the factor count in a way that a later
/// `l·(...)` step in the combining formula amplified without bound.
/// Tokenizing removes the seam entirely — there is no partway-through a
/// token for the parser to be at.
fn block_shuffle_factorization(seq: &Sequence, args: &LzArgs) -> (Vec<BlockLevel>, usize) {
    let mut mm: i64 = args.block_size as i64;
    if mm <= 0 {
        mm = max_block_size(seq.len()) as i64;
    }
    let mm = mm as usize;

    let mut res: Vec<BlockLevel> = vec![BlockLevel::default(); mm + 1];

    let base = fnv1a(seq.as_bytes());
    let bytes = seq.as_bytes();
    let computed: Vec<(usize, BlockLevel)> = (1..=mm)
        .into_par_iter()
        .map(|idx| {
            let (mut tokens, num_distinct) = tokenize_blocks(bytes, idx);
            let seed = seed_from_base(base, idx);
            shuffle_tokens(&mut tokens, seed);
            let num_blocks = tokens.len();
            let factor_count = factorize_tokens(&tokens, num_distinct);
            (
                idx,
                BlockLevel {
                    factor_count,
                    num_blocks,
                },
            )
        })
        .collect();
    for (idx, bl) in computed {
        res[idx] = bl;
    }
    (res, mm)
}

/// Effective measure complexity via the block-entropy estimator.
///
/// For each block size `l`, `h_rand[l]` gives `Ĥ_l` directly: the LZ76
/// factor count of `u` tokenized into `l`-byte blocks and shuffled at the
/// *token* level ([`block_shuffle_factorization`]), converted with that
/// token sequence's own `g_l = log_k(m_l)/m_l` (`m_l` blocks, same `k` as
/// the original alphabet so the units line up with `ĥ` below). That is a
/// direct estimate of the block entropy `H_l` — no extra rescaling by `l`
/// needed, because the token-level factor count is already extensive over
/// the `l`-symbol block, not a per-symbol density that has to be scaled
/// back up. Each block size gives one *finite-scale excess entropy* — the
/// subextensive part of the block entropy at scale `l`:
///
/// ```text
/// Ê(l) = Ĥ_l − l·ĥ
/// ```
///
/// with `ĥ = C_LZ(u) · g` the entropy density of the untouched original
/// sequence (`g = log_k(N)/N`), computed exactly as before — the boundary
/// problem below was only ever in how `Ĥ_l` was measured, never in `ĥ`.
///
/// Theory pins down the shape of that ladder: `E(l) = Σ_{j≤l} [h(j) − h]`
/// with every increment `h(j) − h ≥ 0`, so `E(l)` is non-negative and
/// non-decreasing in `l` and rises to `E` (Crutchfield & Feldman, *Chaos*
/// 13:25 (2003), Lemma 1 and Eq. 52). The raw ladder can still violate both
/// properties in small ways (each `Ê(l)` is an independent surrogate draw,
/// not a slice of one consistent curve), so we project it onto that cone —
/// non-negative isotonic regression, see [`non_negative_isotonic`] — and
/// read the answer off the projection:
///
/// ```text
/// summands[l−1] = Ê_fit(l) − Ê_fit(l−1)   ≥ 0     (the estimate of h(l) − h)
/// Ê             = Ê_fit(mm) = Σ summands  ≥ 0
/// ```
///
/// `multi_information` stays the *raw* `l = 1` contrast `H_1 − ĥ`, unchanged
/// from earlier versions: it is a separate quantity (the multi-information
/// rate of the source), not a term of the sum.
///
/// # Why this isn't measured by block-shuffling the raw bitstream
///
/// Every version up to and including 2.0.0 measured `Ĥ_l` by physically
/// cutting `u` into `N/l` aligned byte blocks, swapping blocks around in
/// place, and running ordinary single-symbol LZ76 on the reassembled
/// bitstream. That lets the parser match *through* the artificial seams the
/// reassembly creates — confirmed to be a genuine, reproducible artifact of
/// the cut-and-reassemble step and not shuffle noise (15 independent
/// reshuffles at the same `l` agreed to four decimal places; an exact
/// Fisher-Yates permutation of the blocks reproduced the identical bias,
/// ruling out shuffle quality as the cause). Converting that inflated count
/// to a density and multiplying back by `l` — the only way to get an
/// extensive quantity out of a byte-level factorization of a length-`N`
/// sequence — canceled the `l` out of the artifact's own `N/l` scaling and
/// left a per-block-size contamination that did **not** shrink with `l` at
/// all. Concretely, this made the estimator provably diverge: sweeping `-e`
/// from 29 to 80 on the even process at `p=0.5` (true `E=0.9183`) climbed
/// `1.73 → 2.18` with no sign of a plateau, and 29→90 on a plain order-1
/// Markov control (true `E` captured entirely at `l=1`) climbed `0.40 →
/// 0.42` — a bare convergence-to-a-constant is the one property this
/// estimator is required to have, and it did not hold once the ladder ran
/// past a fairly small `l`.
///
/// [`block_shuffle_factorization`] fixes this at the source instead of
/// papering over it: shuffling the *token* array (one token per block) and
/// factorizing that means a match can only ever be between whole tokens —
/// there is no partway-through-a-block position for the parser to match
/// across, so there is no seam artifact to inflate the count in the first
/// place, and no `l`-multiplication step left to amplify it even if there
/// were. Confirmed: sweeping `-e` far past where the byte-level version
/// diverged no longer grows without bound — it rises, then declines, and
/// stays bounded (see the next section for why it declines, and
/// `crate::block_lz76`'s tests for controlled measurements backing this).
///
/// # Why the ladder is still capped in auto mode
///
/// Removing the seam artifact does not make `Ĥ_l` unbiased at every `l`.
/// LZ76 is a slow-converging entropy estimator once its alphabet is large
/// relative to the sample count, and here that alphabet is `K_l` — the
/// number of *distinct* `l`-blocks actually observed — which grows with `l`
/// while `m = N/l` (the number of blocks to estimate it from) shrinks.
/// [`crate::block_lz76`]'s
/// `large_alphabet_iid_entropy_is_underestimated_at_moderate_sample_ratios`
/// test measures this in isolation, with no shuffle involved at all:
/// 500,000 freshly-drawn i.i.d. samples over 28,256 equally-likely
/// categories recovers only ~2/3 of the true `log2(28256)` bits. The
/// practical effect on the ladder: `raw(l)` rises while real finite-`l`
/// excess entropy still dominates, peaks, and then declines as this
/// undersampling bias takes over — a shape confirmed on both the even
/// process (peak around `l=3`, true `E` needs structure out to `l≈15–20`
/// to fully converge, so the peak is a real but incomplete lower bound —
/// `0.385` against a true `0.918`) and a plain order-1 Markov control
/// (peak *exactly* at `l=1`, matching that its true `E` is captured
/// entirely there — `0.191` against a true `0.189`, i.e. accurate to
/// ~1%). Trusting the ladder past its own peak only adds decline, which
/// the non-negative isotonic projection can only ever fold *down* into the
/// running fit — in the worst case (even process, `mm` pushed far enough)
/// collapsing the whole thing to `0`. So in auto mode
/// ([`argmax_index`]) the ladder is cut at its peak, which is always at
/// least as good as, and usually much better than, trusting the full
/// computed range. This is a *bound*, not a promise of accuracy: for
/// processes whose correlation length is long relative to what `N` can
/// support at this block-count-vs-alphabet-size tradeoff (the even
/// process above), the true `E` is genuinely out of reach of this
/// estimator at this `N` — more data (larger `N`) is what buys back the
/// missing range, not a smarter stopping rule. Explicit `-e n` requests
/// bypass truncation entirely: that is how to see the full rise-and-decline
/// shape for diagnostic purposes.
fn shuffle_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    h_rand: &[BlockLevel],
    mm: usize,
) -> LzShuffle {
    let mut result = LzShuffle {
        max_block_size: mm as i32,
        ..Default::default()
    };
    if args.get_shuffle_terms {
        result.summands = vec![0.0; mm];
    }
    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;

    let n = seq.len() as f64;
    if n <= 1.0 || mm == 0 {
        return result;
    }
    // g = log_k(N) / N, so ĥ = C_LZ(u)·g is the entropy density of u.
    let g = (n.ln() / log_base.ln()) / n;
    let h_hat = complexity as f64 * g;

    // The raw ladder Ê(l) = Ĥ_l − l·ĥ. `h_rand` is 1-indexed, so `len` is
    // how many scales it holds.
    let len = mm.min(h_rand.len().saturating_sub(1));
    if len == 0 {
        return result;
    }
    let raw: Vec<f64> = (1..=len)
        .map(|l| {
            let bl = &h_rand[l];
            let m = bl.num_blocks as f64;
            // Ĥ_l = C_LZ(tokens) · log_k(m) / m — same k (original
            // alphabet) as g above, so this lines up in the same units as
            // ĥ, but using this block size's own block count m = N/l, not N.
            let h_l = if m > 1.0 {
                bl.factor_count as f64 * (m.ln() / log_base.ln()) / m
            } else {
                0.0
            };
            h_l - (l as f64) * h_hat
        })
        .collect();

    // The l = 1 contrast, before any projection.
    result.multi_information = raw[0];

    // Auto mode: truncate to where the raw ladder peaks (see the doc
    // comment above — Ĥ_l's own large-alphabet undersampling bias grows
    // with l and eventually overwhelms genuine signal, so raw(l) rises
    // while real finite-l structure still dominates and then declines once
    // it doesn't; the decline carries no information the projection should
    // be allowed to fold in). An explicit `-e n` is a deliberate request to
    // see the whole shape, decline included, so it is left untouched.
    let len = if args.block_size <= 0 {
        argmax_index(&raw) + 1
    } else {
        len
    };
    let raw = &raw[..len];
    result.max_block_size = len as i32;
    if args.get_shuffle_terms {
        result.summands.truncate(len);
    }

    // Project onto {non-negative, non-decreasing} and read off the increments.
    let fitted = non_negative_isotonic(raw);
    let mut prev = 0.0f64; // Ê_fit(0) = 0
    for l in 1..=len {
        let term = fitted[l - 1] - prev;
        if args.get_shuffle_terms {
            result.summands[l - 1] = term;
        }
        prev = fitted[l - 1];
    }
    result.emc_value = prev; // = Ê_fit(len)
    result
}

/// Index of the largest value in `raw` (ties keep the earliest — no reason
/// to trust a later, more-block-size-contaminated tie over an earlier one).
/// `raw` is always non-empty here (guarded by the `len == 0` early return
/// above it).
fn argmax_index(raw: &[f64]) -> usize {
    let mut best = 0;
    for i in 1..raw.len() {
        if raw[i] > raw[best] {
            best = i;
        }
    }
    best
}

/// L2 projection of `y` onto the cone of non-negative, non-decreasing sequences.
///
/// The isotonic part is the pool-adjacent-violators algorithm (Ayer et al.,
/// *Ann. Math. Statist.* 26:641 (1955)): walk left to right, and whenever the
/// newest block averages below its predecessor, merge the two and re-check.
/// Blocks are stored as `(sum, count)` pairs, so this is O(n) overall. The
/// non-negative projection is then the pointwise positive part, which preserves
/// monotonicity.
fn non_negative_isotonic(y: &[f64]) -> Vec<f64> {
    let mut sums: Vec<f64> = Vec::with_capacity(y.len());
    let mut counts: Vec<usize> = Vec::with_capacity(y.len());
    for &v in y {
        sums.push(v);
        counts.push(1);
        while sums.len() > 1 {
            let k = sums.len();
            let last = sums[k - 1] / counts[k - 1] as f64;
            let prev = sums[k - 2] / counts[k - 2] as f64;
            if prev <= last {
                break;
            }
            sums[k - 2] += sums[k - 1];
            counts[k - 2] += counts[k - 1];
            sums.pop();
            counts.pop();
        }
    }
    let mut out = Vec::with_capacity(y.len());
    for (s, c) in sums.iter().zip(counts.iter()) {
        let level = (s / *c as f64).max(0.0);
        out.extend(std::iter::repeat(level).take(*c));
    }
    out
}

/// `lz76RandomShuffleComplexity` — full pipeline.
pub fn lz76_random_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    lz76_random_shuffle_complexity_with(seq, args, lz76_factorization(seq, args) as i32)
}

/// Same, but with an already-computed original-sequence complexity (so callers
/// that already have it — e.g. the `lz76` driver — skip one factorization).
pub fn lz76_random_shuffle_complexity_with(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
) -> LzShuffle {
    let (h_rand, mm) = block_shuffle_factorization(seq, args);
    shuffle_entropy_calculation(seq, args, complexity, &h_rand, mm)
}

/// `lz76PairedShuffleComplexity` — shuffle on the merged (Z) sequence.
pub fn lz76_paired_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    let mid = seq.len() / 2;
    let (past, future) = seq.split_at(mid);
    let merged = merge_sequences(&past, &future);
    let complexity = lz76_factorization(&merged, args) as i32;
    let (h_rand, mm) = block_shuffle_factorization(&merged, args);
    shuffle_entropy_calculation(&merged, args, complexity, &h_rand, mm)
}

/// Mirror of `internal::MergeSequences` — element-wise pairing into a new alphabet.
pub fn merge_sequences(s1: &Sequence, s2: &Sequence) -> Sequence {
    let max_iter = s1.len().min(s2.len());
    let mut buf = Vec::with_capacity(max_iter);
    let mut lookup = vec![0u8; 65536];
    let mut next_symbol: u8 = b'0';
    let a = s1.as_bytes();
    let b = s2.as_bytes();
    for i in 0..max_iter {
        let key = ((a[i] as u16) << 8) | (b[i] as u16);
        let entry = &mut lookup[key as usize];
        if *entry == 0 {
            *entry = next_symbol;
            next_symbol = next_symbol.wrapping_add(1);
        }
        buf.push(*entry);
    }
    Sequence::from_bytes_with_alphabet(buf, s1.alphabet_size().saturating_mul(s2.alphabet_size()))
}

/// FNV-1a hash of the sequence bytes (the content-dependent part of the seed).
fn fnv1a(bytes: &[u8]) -> u64 {
    let mut h: u64 = 14695981039346656037; // FNV offset basis (64-bit)
    for &c in bytes {
        h ^= c as u64;
        h = h.wrapping_mul(1099511628211);
    }
    h
}

/// Derive the per-block-size seed from the (once-computed) content hash.
fn seed_from_base(base: u64, idx: usize) -> u64 {
    base ^ ((idx as u64).wrapping_mul(0x9E3779B97F4A7C15))
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A binary sequence (alphabet 2 -> log_base auto = 2) of length 100, plus
    /// `g = log_2(100)/100`. The block-entropy arithmetic only reads the length
    /// and the alphabet, so the content is irrelevant to these tests.
    ///
    /// `block_size` is set explicitly (not auto) so these hand-fed ladders
    /// exercise the isotonic-projection arithmetic in isolation, without
    /// auto mode's peak-truncation (`argmax_index`) also kicking in — that
    /// has its own dedicated test below.
    fn fixture() -> (Sequence, LzArgs, f64) {
        let mut data = vec![b'0'; 50];
        data.extend(std::iter::repeat(b'1').take(50));
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        args.block_size = 3;
        let n = 100.0f64;
        (seq, args, (n.ln() / 2f64.ln()) / n)
    }

    /// `BlockLevel` entries with `num_blocks = 100` for every `l`, so each
    /// one's `g_m` equals the fixture's `g` — lets these hand-picked tests
    /// reason in the same single `g` as the rest of the arithmetic below,
    /// even though a real ladder has `num_blocks = N/l` (these tests are
    /// only exercising `shuffle_entropy_calculation`'s isotonic-projection
    /// arithmetic, not the realistic block-count relationship — that's
    /// covered by the end-to-end `summands_are_non_negative_and_sum_to_the_value`
    /// test below).
    fn ladder(factor_counts: &[u32]) -> Vec<BlockLevel> {
        let mut v = vec![BlockLevel::default()];
        v.extend(factor_counts.iter().map(|&c| BlockLevel {
            factor_count: c,
            num_blocks: 100,
        }));
        v
    }

    #[test]
    fn emc_projects_the_excess_ladder() {
        let (seq, args, g) = fixture();
        // Ĥ_l = c_l·g (num_blocks=100=n for every l, so g_m = g here), and
        // Ê(l) = Ĥ_l − l·10g: c = [20, 30, 36] against C_LZ(u) = 10 gives
        // Ê = [10g, 10g, 6g] — non-monotone at l = 3.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[20, 30, 36]), 3);

        // Pooling all three violators gives one block at (10 + 10 + 6)/3 g.
        let level = (26.0 / 3.0) * g;
        assert!((r.emc_value - level).abs() < 1e-12, "{}", r.emc_value);
        assert!((r.summands[0] - level).abs() < 1e-12);
        assert_eq!(r.summands[1], 0.0);
        assert_eq!(r.summands[2], 0.0);
        // multi_information is the *raw* l = 1 contrast, not the fitted one.
        assert!((r.multi_information - 10.0 * g).abs() < 1e-12);
    }

    #[test]
    fn emc_keeps_the_closed_form_when_the_ladder_is_monotone() {
        let (seq, args, g) = fixture();
        // c = [11, 22, 33] gives Ê(l) = c_l·g − l·10g = [g, 2g, 3g], already
        // strictly increasing, so the projection is the identity.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[11, 22, 33]), 3);

        assert!((r.emc_value - 3.0 * g).abs() < 1e-12, "{}", r.emc_value);
        for s in &r.summands {
            assert!((s - g).abs() < 1e-12, "{s}");
        }
    }

    #[test]
    fn emc_is_never_negative() {
        let (seq, args, g) = fixture();
        // c = [10, 10, 10] against C_LZ(u) = 20 gives Ê = [−10g, −30g, −50g],
        // entirely below zero.
        let r = shuffle_entropy_calculation(&seq, &args, 20, &ladder(&[10, 10, 10]), 3);

        assert_eq!(r.emc_value, 0.0);
        assert!(r.summands.iter().all(|&s| s == 0.0), "{:?}", r.summands);
        // The diagnostic still shows the negative contrast.
        assert!((r.multi_information + 10.0 * g).abs() < 1e-12);
    }

    #[test]
    fn auto_mode_truncates_at_the_ladder_peak() {
        let (seq, mut args, g) = fixture();
        args.block_size = -1; // auto mode: this is what's under test here
                               // c = [15, 30, 36, 38] against C_LZ(u) = 10 gives
                               // Ê = [5g, 10g, 6g, −2g]: rises to a peak at l=2,
                               // then declines. Auto mode should stop at that peak
                               // (l=2) rather than including the declining tail.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[15, 30, 36, 38]), 4);

        assert_eq!(r.max_block_size, 2);
        assert_eq!(r.summands.len(), 2);
        // [5g, 10g] is already non-decreasing, so the fit is the identity:
        // Ê_fit(2) = 10g.
        assert!((r.emc_value - 10.0 * g).abs() < 1e-12, "{}", r.emc_value);
    }

    #[test]
    fn explicit_block_size_keeps_the_whole_ladder_decline_included() {
        let (seq, mut args, _g) = fixture();
        args.block_size = 4; // explicit: no truncation, even though it peaks at l=2
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[15, 30, 36, 38]), 4);

        assert_eq!(r.max_block_size, 4);
        assert_eq!(r.summands.len(), 4);
    }

    #[test]
    fn isotonic_pools_adjacent_violators() {
        // 3.0 then 2.0 violates monotonicity; they pool to 2.5.
        assert_eq!(
            non_negative_isotonic(&[1.0, 3.0, 2.0, 4.0]),
            vec![1.0, 2.5, 2.5, 4.0]
        );
        // Already isotonic: unchanged apart from the non-negativity clamp.
        assert_eq!(non_negative_isotonic(&[-1.0, 0.5]), vec![0.0, 0.5]);
        // A strictly decreasing input pools into one block.
        assert_eq!(non_negative_isotonic(&[6.0, 3.0, 0.0]), vec![3.0, 3.0, 3.0]);
        assert_eq!(non_negative_isotonic(&[]), Vec::<f64>::new());
    }

    #[test]
    fn summands_are_non_negative_and_sum_to_the_value() {
        // A real end-to-end run: period-8 motif, n = 2000.
        let data: Vec<u8> = b"00010111".iter().copied().cycle().take(2000).collect();
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        let r = lz76_random_shuffle_complexity(&seq, &args);

        assert_eq!(r.summands.len(), r.max_block_size as usize);
        assert!(r.emc_value > 0.0, "expected structure, got {}", r.emc_value);
        assert!(r.summands.iter().all(|&s| s >= 0.0), "{:?}", r.summands);
        let total: f64 = r.summands.iter().sum();
        assert!(
            (total - r.emc_value).abs() < 1e-12,
            "{total} vs {}",
            r.emc_value
        );
        // The ladder is non-decreasing, so partial sums climb to the total.
        let mut acc = 0.0;
        for &s in &r.summands {
            acc += s;
            assert!(acc <= r.emc_value + 1e-12);
        }
    }
}
