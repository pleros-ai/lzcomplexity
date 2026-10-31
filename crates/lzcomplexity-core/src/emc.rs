//! Effective Measure Complexity from a Chao–Shen block-entropy ladder.
//!
//! Restores the July 28 estimator (88f8d12): overlapping window histograms,
//! coverage correction, an LZ76 rate estimate, and a nonnegative isotonic fit.
//! Automatic range selection stops at the raw ladder's earliest maximum;
//! an explicit range includes the whole requested ladder.

use crate::block_entropy::{block_histogram, chao_shen_entropy_bits};
use crate::lz76::{lz76_factorization, EmcResult, LzArgs};
use crate::sequence::Sequence;
use crate::shuffle::non_negative_isotonic;
use crate::NO_ALPHABET;
use rayon::prelude::*;

/// E estimator. Chao–Shen is the default; Shuffle preserves the earlier results.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum EmcMethod {
    #[default]
    ChaoShen,
    Shuffle,
}

impl EmcMethod {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::ChaoShen => "chao-shen",
            Self::Shuffle => "shuffle",
        }
    }
}

/// Compute Effective Measure Complexity using the selected estimator.
pub fn emc(seq: &Sequence, args: &LzArgs) -> EmcResult {
    emc_with(seq, args, lz76_factorization(seq, args) as i32)
}

/// Compute E while reusing the original sequence's LZ76 count.
pub fn emc_with(seq: &Sequence, args: &LzArgs, complexity: i32) -> EmcResult {
    match args.emc_method {
        EmcMethod::ChaoShen => {
            let (ladder, mm) = chao_shen_block_entropy_ladder(seq, args);
            block_entropy_calculation(seq, args, complexity, &ladder, mm)
        }
        EmcMethod::Shuffle => {
            crate::shuffle::lz76_random_shuffle_complexity_with(seq, args, complexity)
        }
    }
}

/// Largest range with l * sigma^l <= N, floored at one.
fn reliable_block_size(n_symbols: usize, alphabet: f64) -> usize {
    let budget = n_symbols as f64;
    let sigma = alphabet.max(2.0);
    let mut n: usize = 1;
    // σ^n overflows fast, so step in f64 and stop as soon as the bound bites.
    while ((n + 1) as f64) * sigma.powi(n as i32 + 1) <= budget {
        n += 1;
        if n >= 64 {
            break;
        }
    }
    n
}

fn chao_shen_block_entropy_ladder(seq: &Sequence, args: &LzArgs) -> (Vec<f64>, usize) {
    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;

    let mut mm: i64 = args.block_size as i64;
    if mm <= 0 {
        mm = reliable_block_size(seq.len(), seq.alphabet_size() as f64) as i64;
    }
    let mm = mm as usize;

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

/// Fit raw(l) = H_CS(l) - l * h_LZ. The first raw contrast is retained
/// as multi_information; the nonnegative fitted increments sum to E.
fn block_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    h_block: &[f64],
    mm: usize,
) -> EmcResult {
    let mut result = EmcResult {
        max_block_size: mm as i32,
        ..Default::default()
    };
    if args.get_emc_terms {
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
    let raw: Vec<f64> = (1..=len).map(|l| h_block[l] - (l as f64) * h_hat).collect();

    // The l = 1 contrast, before any projection.
    result.multi_information = raw[0];

    // Retain the historical automatic peak cutoff so later decline from the
    // rate estimate's residual bias does not pull the fitted total downward.
    // An explicit range includes the whole ladder, decline included.
    let len = if args.block_size <= 0 {
        argmax_index(&raw) + 1
    } else {
        len
    };
    let raw = &raw[..len];
    result.max_block_size = len as i32;
    if args.get_emc_terms {
        result.summands.truncate(len);
    }

    // Project onto {non-negative, non-decreasing} and read off the increments.
    let fitted = non_negative_isotonic(raw);
    let mut prev = 0.0f64; // fit(0) = 0
    for l in 1..=len {
        let term = fitted[l - 1] - prev;
        if args.get_emc_terms {
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
        data.extend(std::iter::repeat_n(b'1', 50));
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.get_emc_terms = true;
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
        let r =
            block_entropy_calculation(&seq, &args, 10, &ladder(&[20.0 * g, 30.0 * g, 36.0 * g]), 3);

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
        let r =
            block_entropy_calculation(&seq, &args, 10, &ladder(&[11.0 * g, 22.0 * g, 33.0 * g]), 3);

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
        let r =
            block_entropy_calculation(&seq, &args, 20, &ladder(&[10.0 * g, 10.0 * g, 10.0 * g]), 3);

        assert_eq!(r.emc_value, 0.0);
        assert!(r.summands.iter().all(|&s| s == 0.0), "{:?}", r.summands);
        // The diagnostic still shows the negative contrast.
        assert!((r.multi_information + 10.0 * g).abs() < 1e-9);
    }

    #[test]
    fn auto_mode_truncates_at_the_ladder_peak() {
        let (seq, mut args, g) = fixture();
        // Auto mode is what's under test here.
        args.block_size = -1;
        // Ĥ = [15g, 30g, 36g, 38g] against ĥ = 10g gives raw(l) =
        // [5g, 10g, 6g, −2g]: rises to a peak at l=2, then declines. Auto
        // mode should stop at that peak (l=2) rather than including the
        // decline.
        let r = block_entropy_calculation(
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
        let r = block_entropy_calculation(
            &seq,
            &args,
            10,
            &ladder(&[15.0 * g, 30.0 * g, 36.0 * g, 38.0 * g]),
            4,
        );

        assert_eq!(r.max_block_size, 4);
        assert_eq!(r.summands.len(), 4);
    }

    /// The reliability bound must track the *actual* alphabet, not assume
    /// binary. `max_block_size` (which this replaced for the EMC ladder)
    /// is alphabet-blind: at N = 1e6 it answers 16 whatever σ is, which for
    /// DNA (σ=4) is roughly twice the largest block size the data supports.
    #[test]
    fn reliable_block_size_tracks_the_alphabet() {
        let n = 1_000_000;
        assert_eq!(reliable_block_size(n, 2.0), 15);
        assert_eq!(reliable_block_size(n, 4.0), 8);
        assert_eq!(reliable_block_size(n, 20.0), 4);
        // ...whereas the old, alphabet-blind bound answers 16 for all three.
        assert_eq!(crate::shuffle::max_block_size(n), 16);
        // Degenerate inputs stay in range rather than under/overflowing.
        assert_eq!(reliable_block_size(0, 2.0), 1);
        // `n·σ^n` is solved in f64, so a huge N must terminate at the guard
        // rather than run away or overflow.
        assert!((1..=64).contains(&reliable_block_size(usize::MAX, 2.0)));
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
        args.get_emc_terms = true;
        let r = emc(&seq, &args);

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
