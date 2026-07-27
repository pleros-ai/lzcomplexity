//! Block entropy estimation via frequency histograms.
//!
//! Building `H_l` (the joint entropy of `l` consecutive symbols) requires
//! estimating the entropy of `u`'s own length-`l` block distribution. Two
//! earlier approaches were tried and superseded, in order:
//!
//! 1. Physically cutting `u` into `N/l` aligned byte blocks, shuffling them
//!    in place, and running ordinary single-symbol LZ76 on the reassembled
//!    bitstream. This let the parser match *through* the artificial seams
//!    the reassembly created, systematically inflating the factor count by
//!    an amount that did not shrink with `l` — a real, provable divergence
//!    bug (see `crate::shuffle`'s doc comment for the full history).
//! 2. Tokenizing each block to a dense id and shuffling/factorizing the
//!    *token array* instead of the raw bytes — this removed the seam
//!    artifact (a match can only ever be between whole tokens), but LZ76
//!    itself is still a slow-converging entropy estimator once its
//!    alphabet (`K_l`, the number of distinct `l`-blocks) is large relative
//!    to the sample count (`m_l = N/l`) — confirmed in isolation, no
//!    shuffle involved at all: 500,000 i.i.d. draws over 28,256 categories
//!    recovered only ~55-80% of the true entropy via LZ76-via-tokens.
//!
//! This module implements the replacement: entropy estimated directly from
//! the block *frequency histogram* ([`block_histogram`]), via a proper
//! bias-corrected estimator for the undersampled regime
//! ([`chao_shen_entropy_bits`]) instead of LZ76. This has two advantages
//! over both earlier approaches: no shuffling is needed at all (entropy
//! computed from a frequency histogram is order-insensitive — the whole
//! reason shuffling was ever necessary was to make an *order-sensitive*
//! estimator like LZ76 behave as if the blocks were unordered), and
//! Chao-Shen is specifically designed for and validated in the
//! `K` approaching-or-exceeding-`m` regime that made LZ76 unreliable:
//! the identical 28,256-category test above recovers the true entropy to
//! within ~3% via [`chao_shen_entropy_bits`] (see
//! `chao_shen_recovers_large_alphabet_iid_entropy_accurately`).

use std::collections::HashMap;

/// Build the frequency histogram of `data`'s length-`block_size` windows,
/// sliding by 1 (so there are `data.len() - block_size + 1` of them, not
/// `data.len() / block_size` non-overlapping ones). Only the *counts*
/// matter here — no per-position ids, no ordering — because entropy
/// estimated from a frequency histogram is order-insensitive.
///
/// Melchert & Hartmann's original surrogate construction needed literal
/// non-overlapping blocks, because it had to reassemble them (shuffled)
/// into a valid surrogate sequence for LZ76 to factorize. This module
/// doesn't build a surrogate sequence at all — only the block probability
/// distribution — so that constraint no longer applies, and sliding is
/// the better choice on two independent counts: it uses `data.len() -
/// block_size + 1` samples instead of `data.len() / block_size` (directly
/// easing the undersampling regime [`chao_shen_entropy_bits`] exists to
/// correct), and, unlike a non-overlapping partition, it does not depend
/// on which of the `block_size` possible phases the data happens to start
/// on. That phase-dependence is a real failure mode, not a theoretical
/// nicety: on a length-2000, period-8 test sequence, a non-overlapping
/// partition at `block_size=8` sees only ever the *same* phase and
/// therefore only 1 distinct block (an entropy of exactly 0, even though
/// the true per-block entropy of this source is `log2(8)=3` bits) — a
/// measurement artifact of a single arbitrary alignment, not a real
/// property of the source. Sliding by 1 instead sees all 8 phases and
/// recovers `~3` bits, as it should.
pub fn block_histogram(data: &[u8], block_size: usize) -> Vec<u32> {
    assert!(block_size > 0, "block_size must be positive");
    if data.len() < block_size {
        return Vec::new();
    }
    let num_windows = data.len() - block_size + 1;
    let mut counts: HashMap<&[u8], u32> = HashMap::with_capacity(num_windows);
    for i in 0..num_windows {
        let block = &data[i..i + block_size];
        *counts.entry(block).or_insert(0) += 1;
    }
    counts.into_values().collect()
}

/// Chao & Shen's coverage-adjusted entropy estimate (bits), from a
/// frequency histogram alone (`counts`, any order, zero-count entries
/// ignored).
///
/// The naive plug-in/MLE estimate `-Σ (n_i/m)·log2(n_i/m)` is well known to
/// be biased *downward* once the number of distinct categories `K` is
/// large relative to the sample count `m` — exactly the regime this
/// crate's block-entropy ladder runs into at larger block sizes. Chao-Shen
/// corrects for it using the Good-Turing coverage estimate `Ĉ = 1 - f₁/m`
/// (`f₁` = number of singleton categories) to reweight each category's
/// probability, then applies a Horvitz-Thompson inclusion-probability
/// correction `1 - (1-p̂ᵢ)^m` per category to account for categories likely
/// missed entirely by a sample of this size:
///
/// ```text
/// p̂ᵢ = Ĉ·(nᵢ/m)
/// Ĥ_CS = - Σᵢ p̂ᵢ·log2(p̂ᵢ) / (1 - (1-p̂ᵢ)^m)
/// ```
///
/// Unlike a first-order correction such as Miller-Madow (`Ĥ_MLE +
/// (K-1)/(2m ln 2)`), Chao-Shen remains reliable well into the severely
/// undersampled regime (`K` approaching or exceeding `m`) — this is
/// precisely what it was designed and validated for (Chao & Shen,
/// *Environmental and Ecological Statistics* 10:429 (2003)).
///
/// `1 - (1-p̂ᵢ)^m` is computed via `ln_1p`/`exp_m1` rather than a direct
/// `powf` + subtraction, since `p̂ᵢ` can be tiny and `m` large (up to ~1e7
/// in this crate's use), where the naive form loses precision to
/// catastrophic cancellation.
pub fn chao_shen_entropy_bits(counts: &[u32]) -> f64 {
    let m: f64 = counts.iter().map(|&c| c as f64).sum();
    if m <= 0.0 {
        return 0.0;
    }
    let f1 = counts.iter().filter(|&&c| c == 1).count() as f64;
    let coverage = 1.0 - f1 / m;
    if coverage <= 0.0 {
        // Degenerate all-singleton sample: the coverage estimate collapses
        // to (near) zero and every term below would divide by (near) zero.
        // Fall back to the plain plug-in estimate — known-biased-low, but
        // at least finite — rather than produce NaN/infinity.
        return plugin_entropy_bits(counts, m);
    }
    let mut h = 0.0;
    for &n_i in counts {
        if n_i == 0 {
            continue;
        }
        let p_i = coverage * (n_i as f64 / m);
        // 1 - (1-p_i)^m = -expm1(m * ln1p(-p_i)).
        let inclusion_prob = -(m * (-p_i).ln_1p()).exp_m1();
        if inclusion_prob > 0.0 {
            h -= p_i * p_i.log2() / inclusion_prob;
        }
    }
    h
}

fn plugin_entropy_bits(counts: &[u32], m: f64) -> f64 {
    let mut h = 0.0;
    for &n_i in counts {
        if n_i == 0 {
            continue;
        }
        let p_i = n_i as f64 / m;
        h -= p_i * p_i.log2();
    }
    h
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn histogram_slides_by_one_and_counts_repeats() {
        // "aabbaabbccdd" (len 12), block_size 2 -> 11 overlapping windows:
        // aa,ab,bb,ba,aa,ab,bb,bc,cc,cd,dd -> aa:2,ab:2,bb:2,ba:1,bc:1,cc:1,cd:1,dd:1
        let data = b"aabbaabbccdd";
        let mut counts = block_histogram(data, 2);
        counts.sort_unstable();
        assert_eq!(counts, vec![1, 1, 1, 1, 1, 2, 2, 2]);
        assert_eq!(counts.iter().sum::<u32>(), 11);
    }

    #[test]
    fn histogram_returns_empty_when_block_size_exceeds_data_len() {
        assert_eq!(block_histogram(b"ab", 5), Vec::<u32>::new());
    }

    /// Regression test for the phase-alignment bug a non-overlapping
    /// partition had: at `block_size` equal to the period, every aligned
    /// block is identical (entropy collapses to 0) purely because the
    /// partition only ever samples one of the `block_size` possible
    /// phases. Sliding by 1 samples all of them.
    #[test]
    fn histogram_is_insensitive_to_phase_alignment() {
        let data: Vec<u8> = b"00010111".iter().copied().cycle().take(2000).collect();
        let counts = block_histogram(&data, 8);
        assert_eq!(counts.len(), 8, "{counts:?}");
        let h = chao_shen_entropy_bits(&counts);
        assert!((h - 3.0).abs() < 0.01, "{h}");
    }

    #[test]
    fn chao_shen_matches_plugin_when_well_sampled() {
        // Fair coin over 4 categories, 10,000 samples each: true H = 2 bits.
        // Chao-Shen on a fully-observed distribution (coverage ≈ 1, every
        // inclusion probability ≈ 1) should reduce to plain plug-in entropy.
        let counts = vec![10_000u32, 10_000, 10_000, 10_000];
        let h = chao_shen_entropy_bits(&counts);
        assert!((h - 2.0).abs() < 1e-6, "{h}");
    }

    #[test]
    fn chao_shen_falls_back_on_all_singletons() {
        // Every category seen exactly once: coverage estimate collapses to
        // 0. Must fall back to a finite value (the plug-in estimate: a
        // uniform distribution over 100 categories, 2 bits... i.e. log2(100)),
        // not NaN/infinity.
        let counts = vec![1u32; 100];
        let h = chao_shen_entropy_bits(&counts);
        assert!(h.is_finite(), "{h}");
        assert!((h - 100f64.log2()).abs() < 1e-9);
    }

    /// The decisive comparison: identical synthetic scenario to the one
    /// that showed LZ76-via-tokens recovering only ~55-80% of the true
    /// entropy (m=500,000 i.i.d. uniform draws over K=28,256 categories,
    /// true entropy exactly `log2(K)`, no shuffle-mixing question at all —
    /// this is freshly-drawn i.i.d. data). Chao-Shen recovers it to within
    /// ~3% — the concrete evidence for using it instead of LZ76 as the
    /// block-entropy estimator in `crate::shuffle`.
    #[test]
    fn chao_shen_recovers_large_alphabet_iid_entropy_accurately() {
        let m = 500_000usize;
        let k = 28_256u32;
        let mut state: u64 = 0x1234_5678;
        let mut next = || {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            state
        };
        let tokens: Vec<u32> = (0..m).map(|_| (next() as u32) % k).collect();
        let mut counts = vec![0u32; k as usize];
        for &t in &tokens {
            counts[t as usize] += 1;
        }
        let h_est = chao_shen_entropy_bits(&counts);
        let true_h = (k as f64).log2();
        let ratio = h_est / true_h;
        assert!(
            (0.97..1.03).contains(&ratio),
            "ratio {ratio:.4} (H_est={h_est:.4}, true_H={true_h:.4})"
        );
    }
}
