//! Shuffle-based complexity measures.

use rayon::prelude::*;

use crate::block_entropy::{block_histogram, chao_shen_entropy_bits};
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
/// use this anymore — see [`chao_shen_block_entropy_ladder`] and
/// [`shuffle_entropy_calculation`]'s doc comment for why shuffle-and-LZ76
/// is the wrong tool for that estimator specifically.
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

/// Per-block-size block entropy `Ĥ_l` (already normalized to the same
/// units as `ĥ` — see [`chao_shen_block_entropy_ladder`]), for `l = 1..mm`.
/// Index 0 is unused (`Ĥ_0 ≡ 0` is handled separately, matching `H_0(u)=0`).
fn chao_shen_block_entropy_ladder(seq: &Sequence, args: &LzArgs) -> (Vec<f64>, usize) {
    let mut mm: i64 = args.block_size as i64;
    if mm <= 0 {
        mm = max_block_size(seq.len()) as i64;
    }
    let mm = mm as usize;

    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;

    let bytes = seq.as_bytes();
    let mut res: Vec<f64> = vec![0.0; mm + 1];
    let computed: Vec<(usize, f64)> = (1..=mm)
        .into_par_iter()
        .map(|l| {
            let counts = block_histogram(bytes, l);
            let h_bits = chao_shen_entropy_bits(&counts);
            // Normalize to the same units as ĥ (fraction of the original
            // alphabet's max entropy): Ĥ_l = h_bits / log2(k) = h_bits *
            // ln(2)/ln(k), k = log_base.
            let h_l = h_bits * 2f64.ln() / log_base.ln();
            (l, h_l)
        })
        .collect();
    for (l, h_l) in computed {
        res[l] = h_l;
    }
    (res, mm)
}

/// Effective measure complexity via the block-entropy estimator.
///
/// For each block size `l`, `Ĥ_l` is the entropy of `u`'s own length-`l`
/// block distribution, estimated directly from the block frequency
/// histogram via a bias-corrected estimator for the undersampled regime
/// ([`chao_shen_block_entropy_ladder`], `Ĥ_0 ≡ 0` matching `H_0(u)=0`).
/// `ĥ = C_LZ(u) · g` is the entropy density of the untouched original
/// sequence (`g = log_k(N)/N`), estimated via ordinary LZ76 exactly as in
/// every earlier version — see "Why `ĥ` is still LZ76, not Chao-Shen"
/// below for why that stays true even though `Ĥ_l` itself no longer is.
///
/// Section IV of the manuscript this mirrors singles out the entropy gain
/// `ΔH_l ≡ H_l − H_{l−1}` as the preferred building block for the effective
/// measure complexity: unlike the finite-order rate `h_l = H_l/l`, it
/// converges to `h` monotonically and at least as fast
/// (`h_l(u) ≥ ΔH_l(u) ≥ h`, proven in that manuscript's Appendix B). The
/// same preference carries over to the estimator: the natural estimator of
/// the entropy gain is simply the first difference of consecutive
/// block-entropy estimates,
///
/// ```text
/// ΔĤ_l ≡ Ĥ_l − Ĥ_{l−1}
/// raw(l) = Ĥ_l − l·ĥ   ( = Σ_{j≤l} (ΔĤ_j − ĥ), telescoping )
/// ```
///
/// Theory pins down the shape of that ladder: every increment `ΔH_l − h`
/// is non-negative (the inequality above), so the exact `raw(l)` is
/// non-negative and non-decreasing in `l`, rising to `E`. The empirical
/// `raw(l)` can still violate both properties in small ways even with an
/// accurate `Ĥ_l` — see below — so it is projected onto that cone
/// (non-negative isotonic regression, [`non_negative_isotonic`]) and the
/// answer read off the projection: `summands[l−1] = fit(l) − fit(l−1) ≥
/// 0`, `emc_value = fit(mm) = Σ summands ≥ 0`. `multi_information` stays
/// the *raw*, unprojected `l=1` contrast `Ĥ_1 − ĥ`.
///
/// # Why `Ĥ_l` moved off LZ76 but `ĥ` didn't
///
/// Every version up to and including 2.0.0 measured `Ĥ_l` itself by
/// physically cutting `u` into `N/l` aligned byte blocks, shuffling them,
/// and running ordinary single-symbol LZ76 on the reassembled bitstream —
/// which let the parser match *through* the artificial seams the
/// reassembly created (a real, provable divergence bug), and, once that
/// was fixed at the token level, was still found to badly underestimate
/// entropy once the block alphabet `K_l` grew large relative to the sample
/// count (500,000 i.i.d. draws over 28,256 categories recovered only
/// ~55–80% of the truth via LZ76 — see `crate::block_entropy`'s doc
/// comment for the full history). [`chao_shen_block_entropy_ladder`]
/// replaces that with entropy read directly off the block frequency
/// histogram through Chao & Shen's coverage-adjusted estimator, built and
/// validated for exactly this regime (recovers the same test to ~3%).
///
/// It is tempting to also replace `ĥ` with something derived from that
/// same, much-more-accurate `Ĥ_l` ladder — e.g. the average of its last
/// few increments, which by the inequality above should sit closer to `h`
/// than any single `h_l` does. Tried directly against the closed-form
/// truth for the even process (`h_μ(γ) = H_b(γ)/(1+γ)`) across
/// `γ∈{0.1,...,0.9]`, `N=1e7`: at `γ=0.1` a tail-averaged `ĥ` from the
/// Chao-Shen ladder lands at `0.4258` against a true `0.4264` (~0.1%
/// error) — far tighter than LZ76's `0.4664` (~9% high). But at `γ=0.9`
/// the same construction gives `0.2693` against a true `0.2468` (~9%
/// error) — while LZ76 lands at `0.2523` (~2% error) — the *opposite*
/// ranking. The even process's correlation length grows with `γ`, so at
/// high `γ` the ladder is nowhere near converged by the block sizes this
/// `N` can support, and averaging its still-descending tail overestimates
/// `h` (`ΔH_l` has not reached its floor yet) by more than LZ76's own
/// slow-but-global convergence does. Since which estimator wins depends on
/// a process's own correlation length — exactly the thing being
/// measured — there is no principled way to pick one over the other ahead
/// of time, so `ĥ` stays what the manuscript specifies: the single, global
/// LZ76 estimate of the *whole* sequence, unaffected by any per-`l`
/// resolution tradeoff. The isotonic projection and peak truncation below
/// are what keep a still-imperfect `ĥ` from being multiplied by a large
/// `l` into a wrecked total — removing them (tried earlier this session)
/// let exactly that happen: several `γ`/`p` values on both the even
/// process and a plain order-1 Markov control collapsed to a flat `0`
/// once summed all the way to `mm=19` (auto, `N=1e7`), even though
/// `Ĥ_l` itself was accurate at every individual `l`.
fn shuffle_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    h_block: &[f64],
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

    // The raw ladder raw(l) = Ĥ_l − l·ĥ. `h_block` is 1-indexed (`Ĥ_0≡0`
    // implicit), so `len` is how many scales it holds.
    let len = mm.min(h_block.len().saturating_sub(1));
    if len == 0 {
        return result;
    }
    let raw: Vec<f64> = (1..=len)
        .map(|l| h_block[l] - (l as f64) * h_hat)
        .collect();

    // The l = 1 contrast, before any projection.
    result.multi_information = raw[0];

    // Auto mode: truncate to where the raw ladder peaks (see the doc
    // comment above — `ĥ`'s own residual bias, multiplied by `l`,
    // eventually overwhelms genuine signal, so raw(l) rises while real
    // finite-l structure still dominates and then declines once it
    // doesn't; the decline carries no information the projection should
    // be allowed to fold in). An explicit `-e n` is a deliberate request
    // to see the whole shape, decline included, so it is left untouched.
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
    let mut prev = 0.0f64; // fit(0) = 0
    for l in 1..=len {
        let term = fitted[l - 1] - prev;
        if args.get_shuffle_terms {
            result.summands[l - 1] = term;
        }
        prev = fitted[l - 1];
    }
    result.emc_value = prev; // = fit(len)
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
    let (h_block, mm) = chao_shen_block_entropy_ladder(seq, args);
    shuffle_entropy_calculation(seq, args, complexity, &h_block, mm)
}

/// `lz76PairedShuffleComplexity` — shuffle on the merged (Z) sequence.
pub fn lz76_paired_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    let mid = seq.len() / 2;
    let (past, future) = seq.split_at(mid);
    let merged = merge_sequences(&past, &future);
    let complexity = lz76_factorization(&merged, args) as i32;
    let (h_block, mm) = chao_shen_block_entropy_ladder(&merged, args);
    shuffle_entropy_calculation(&merged, args, complexity, &h_block, mm)
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

    /// Hand-fed `Ĥ_l` values (index 0 = `Ĥ_0 = 0`, matching `H_0(u)=0`).
    fn ladder(h_values: &[f64]) -> Vec<f64> {
        let mut v = vec![0.0];
        v.extend_from_slice(h_values);
        v
    }

    #[test]
    fn emc_projects_the_excess_ladder() {
        let (seq, args, g) = fixture();
        // Ĥ = [20g, 30g, 36g] against ĥ = 10g: raw(l) = Ĥ_l − l·10g =
        // [10g, 10g, 6g] — non-monotone at l = 3.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[20.0 * g, 30.0 * g, 36.0 * g]), 3);

        // Pooling all three violators gives one block at (10 + 10 + 6)/3 g.
        let level = (26.0 / 3.0) * g;
        assert!((r.emc_value - level).abs() < 1e-9, "{}", r.emc_value);
        assert!((r.summands[0] - level).abs() < 1e-9);
        assert_eq!(r.summands[1], 0.0);
        assert_eq!(r.summands[2], 0.0);
        // multi_information is the *raw* l = 1 contrast, not the fitted one.
        assert!((r.multi_information - 10.0 * g).abs() < 1e-9);
    }

    #[test]
    fn emc_keeps_the_closed_form_when_the_ladder_is_monotone() {
        let (seq, args, g) = fixture();
        // Ĥ = [11g, 22g, 33g] gives raw(l) = Ĥ_l − l·10g = [g, 2g, 3g],
        // already strictly increasing, so the projection is the identity.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &ladder(&[11.0 * g, 22.0 * g, 33.0 * g]), 3);

        assert!((r.emc_value - 3.0 * g).abs() < 1e-9, "{}", r.emc_value);
        for s in &r.summands {
            assert!((s - g).abs() < 1e-9, "{s}");
        }
    }

    #[test]
    fn emc_is_never_negative() {
        let (seq, args, g) = fixture();
        // Ĥ = [10g, 10g, 10g] against ĥ = 20g: raw(l) = [−10g, −30g, −50g],
        // entirely below zero.
        let r = shuffle_entropy_calculation(&seq, &args, 20, &ladder(&[10.0 * g, 10.0 * g, 10.0 * g]), 3);

        assert_eq!(r.emc_value, 0.0);
        assert!(r.summands.iter().all(|&s| s == 0.0), "{:?}", r.summands);
        // The diagnostic still shows the negative contrast.
        assert!((r.multi_information + 10.0 * g).abs() < 1e-9);
    }

    #[test]
    fn auto_mode_truncates_at_the_ladder_peak() {
        let (seq, mut args, g) = fixture();
        args.block_size = -1; // auto mode: this is what's under test here
                               // Ĥ = [15g, 30g, 36g, 38g] against ĥ = 10g gives
                               // raw(l) = [5g, 10g, 6g, −2g]: rises to a peak at
                               // l=2, then declines. Auto mode should stop at that
                               // peak (l=2) rather than including the decline.
        let r = shuffle_entropy_calculation(
            &seq,
            &args,
            10,
            &ladder(&[15.0 * g, 30.0 * g, 36.0 * g, 38.0 * g]),
            4,
        );

        assert_eq!(r.max_block_size, 2);
        assert_eq!(r.summands.len(), 2);
        // [5g, 10g] is already non-decreasing, so the fit is the identity:
        // fit(2) = 10g.
        assert!((r.emc_value - 10.0 * g).abs() < 1e-9, "{}", r.emc_value);
    }

    #[test]
    fn explicit_block_size_keeps_the_whole_ladder_decline_included() {
        let (seq, mut args, g) = fixture();
        args.block_size = 4; // explicit: no truncation, even though it peaks at l=2
        let r = shuffle_entropy_calculation(
            &seq,
            &args,
            10,
            &ladder(&[15.0 * g, 30.0 * g, 36.0 * g, 38.0 * g]),
            4,
        );

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
            (total - r.emc_value).abs() < 1e-9,
            "{total} vs {}",
            r.emc_value
        );
        // The ladder is non-decreasing, so partial sums climb to the total.
        let mut acc = 0.0;
        for &s in &r.summands {
            acc += s;
            assert!(acc <= r.emc_value + 1e-9);
        }
    }
}
