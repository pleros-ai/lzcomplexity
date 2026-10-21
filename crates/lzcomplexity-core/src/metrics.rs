//! Information distance, mutual information, extras, and the top-level
//! `lz76(seq, args) -> LempelZiv` driver.

use crate::lz76::{
    entropy_density_from, lz76_factorization, lz76_factors, LempelZiv, LzArgs, LzExtra,
};
use crate::sequence::Sequence;
use crate::shuffle::{lz76_random_shuffle_complexity_with, merge_sequences, shuffle_factorization};
use crate::NO_ALPHABET;

/// `lz76InformationDistance(T1, T2, args)` from the C++.
///
/// Formula: `max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))`.
pub fn lz76_information_distance(t1: &Sequence, t2: &Sequence, args: &LzArgs) -> f64 {
    // The four factor counts are independent and deterministic, so computing
    // them concurrently is result-identical. `rayon::join` runs inline when no
    // worker is idle, so this stays cheap even nested inside a parallel caller.
    let ((c_x, c_y), (c_xy, c_yx)) = rayon::join(
        || {
            rayon::join(
                || lz76_factorization(t1, args),
                || lz76_factorization(t2, args),
            )
        },
        || {
            rayon::join(
                || lz76_factorization(&(t1 + t2), args),
                || lz76_factorization(&(t2 + t1), args),
            )
        },
    );
    let (c_x, c_y, c_xy, c_yx) = (c_x as f64, c_y as f64, c_xy as f64, c_yx as f64);
    let denom = c_x.max(c_y);
    if denom == 0.0 {
        return 0.0;
    }
    ((c_xy - c_x).max(c_yx - c_y)) / denom
}

/// `MutualInformation(s1, s2, args)` — shuffle-of-concatenated estimator.
pub fn mutual_information(s1: &Sequence, s2: &Sequence, args: &LzArgs) -> f64 {
    let concat = s1 + s2;
    let complexity = lz76_factorization(&concat, args) as f64;
    let (h_rand, mm) = shuffle_factorization(&concat, args);
    let random_sum: f64 = h_rand.iter().map(|&x| x as f64).sum();
    if random_sum == 0.0 {
        return 0.0;
    }
    1.0 - ((mm as f64) * complexity) / random_sum
}

pub fn lz76_random_shuffle_distance(t1: &Sequence, t2: &Sequence, args: &LzArgs) -> f64 {
    1.0 - mutual_information(t1, t2, args)
}

pub fn mutual_information_z(s1: &Sequence, s2: &Sequence, args: &LzArgs) -> f64 {
    let merged = merge_sequences(s1, s2);
    let complexity = lz76_factorization(&merged, args) as f64;
    let (h_rand, mm) = shuffle_factorization(&merged, args);
    let random_sum: f64 = h_rand.iter().map(|&x| x as f64).sum();
    if random_sum == 0.0 {
        return 0.0;
    }
    1.0 - ((mm as f64) * complexity) / random_sum
}

pub fn lz76_random_shuffle_distance_z(t1: &Sequence, t2: &Sequence, args: &LzArgs) -> f64 {
    1.0 - mutual_information_z(t1, t2, args)
}

/// `lz76ExtraMeasures` — Rajski, redundancy, etc.
pub fn lz76_extras(seq: &Sequence, args: &LzArgs) -> LzExtra {
    lz76_extras_with(seq, args, lz76_factorization(seq, args) as f64)
}

/// Same, but with an already-computed whole-sequence complexity `c_lz`.
pub fn lz76_extras_with(seq: &Sequence, args: &LzArgs, c_lz: f64) -> LzExtra {
    let mid = seq.len() / 2;
    let second_half = seq.drop(mid);
    let first_half = seq.take(mid);
    // The two half-sequence factorizations are independent (order-independent
    // and deterministic → identical values).
    let (lh, fh) = rayon::join(
        || lz76_factorization(&second_half, args) as f64,
        || lz76_factorization(&first_half, args) as f64,
    );
    if c_lz == 0.0 || fh == 0.0 || lh == 0.0 {
        return LzExtra::default();
    }
    let mi = fh + lh - c_lz;
    LzExtra {
        lz_rajski_distance: 2.0 - (fh + lh) / c_lz,
        redundancy: mi / (fh + lh),
        fh_uncertainty: mi / fh,
        lh_uncertainty: mi / lh,
        lz_pearson_coefficient: mi / (fh * lh).sqrt(),
    }
}

/// `lz76(seq, args)` — the complete analysis.
pub fn lz76(seq: &Sequence, args: &LzArgs) -> LempelZiv {
    // Factorize the full sequence ONCE; reuse the count for entropy, the
    // random-shuffle stage, and the extras instead of re-factorizing 3 more times.
    let res = lz76_factors(seq, args);
    let factorization = res.factorization;
    let entropy = if seq.len() <= 1 {
        0.0
    } else {
        entropy_density_from(factorization, seq, args)
    };
    let rsc = lz76_random_shuffle_complexity_with(seq, args, factorization as i32);
    let extras = lz76_extras_with(seq, args, factorization as f64);
    let stddev = factor_stddev_of(&res.lzf); // computed once, used twice below

    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;
    let n = seq.len() as f64;
    let (normal_error, poison_error) = if n > 1.0 {
        let log_n = n.ln() / log_base.ln();
        let div = (n / log_n).sqrt();
        (
            (entropy * entropy * entropy).sqrt() * stddev / div,
            entropy / n,
        )
    } else {
        (0.0, 0.0)
    };

    LempelZiv {
        complexity: factorization,
        entropy_density: entropy,
        random_shuffle_complexity: rsc,
        paired_shuffle_complexity: Default::default(),
        lz_normal_errors: normal_error,
        lz_poison_errors: poison_error,
        epsilon: res.epsilon,
        factors_stddev: stddev,
        factors: res.lzf,
        extras,
    }
}

fn factor_stddev_of(lzf: &[u32]) -> f64 {
    if lzf.len() < 2 {
        return 0.0;
    }
    let mean = (*lzf.last().unwrap() as f64 - 1.0) / lzf.len() as f64;
    let mut max_factor_size = 0u32;
    let mut sq_sum = 0.0;
    for i in 1..lzf.len() {
        let sz = lzf[i].saturating_sub(lzf[i - 1]);
        if sz > max_factor_size {
            max_factor_size = sz;
        }
        let d = sz as f64 - mean;
        sq_sum += d * d;
    }
    if max_factor_size == 0 {
        0.0
    } else {
        (sq_sum / max_factor_size as f64).sqrt()
    }
}
