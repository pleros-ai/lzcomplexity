//! Finite-window excess-entropy estimates and legacy shuffle distances.

use rayon::prelude::*;

use crate::block_entropy::{mutual_information_ladder_bits, symbol_entropy_bits};
use crate::lz76::{entropy_density_from, lz76_factorization, LzArgs, LzShuffle};
use crate::sequence::{shuffle_copy_seeded, Sequence};

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

/// Conservative histogram ceiling: largest M with M * alphabet^M <= N.
/// This is an occupancy heuristic, not an independent-sample guarantee.
/// It depends on the alphabet cardinality, never the output logarithm base.
/// At least one symbol is retained for nonempty input; empty input has no scales.
fn reliable_block_size(n_symbols: usize, alphabet: u32) -> usize {
    if n_symbols == 0 {
        return 0;
    }
    let budget = n_symbols as f64;
    let sigma = alphabet.max(2) as f64;
    let mut size = 1usize;
    while size < 64 && (size + 1) as f64 * sigma.powi(size as i32 + 1) <= budget {
        size += 1;
    }
    size.min(n_symbols)
}

/// Legacy random-shuffle factor counts used by the shuffle-distance metrics.
/// The auto-mode +10 is retained for compatibility with released 2.0.0.
/// The finite-window EMC estimator below does not call this function.
pub fn shuffle_factorization(seq: &Sequence, args: &LzArgs) -> (Vec<i32>, usize) {
    let mut mm: i64 = args.block_size as i64;
    if mm <= 0 {
        mm = max_block_size(seq.len()) as i64;
        if seq.len() > 50 {
            mm += 10;
        }
    }
    let mm = mm as usize;

    // Index 0 unused, indices 1..=mm hold the per-block-size complexity.
    let mut res: Vec<i32> = vec![0; mm + 3];

    // Hash the sequence content ONCE; each per-block seed only differs by the
    // `idx` term (bit-identical seeds → identical shuffles → identical shuffle distances).
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

/// Empirical finite-window past/future mutual information.
///
/// The public name is retained for compatibility; this performs no shuffle.
/// The total window length M is resolved from the alphabet-dependent occupancy
/// ceiling, or the explicit block_size capped at the input length. The past
/// contains floor(M/2) symbols and the future ceil(M/2). For a stationary source,
/// their population mutual information is a lower approximation to excess
/// entropy, approaching E as both windows grow. The empirical estimate has
/// finite-sample bias and can miss memory beyond the chosen windows.
///
/// All scales use nested marginals of the same empirical maximum-window
/// distribution. This supplies nonnegative, nondecreasing estimates directly,
/// without rate subtraction, peak selection, or isotonic regression. See
/// docs/emc-estimator.md for the target, sampling convention, and limitations.
pub fn lz76_random_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    lz76_random_shuffle_complexity_with(seq, args, lz76_factorization(seq, args) as i32)
}

/// Same estimator, reusing a factor count only for the legacy
/// multi_information diagnostic. The EMC value does not depend on that count.
pub fn lz76_random_shuffle_complexity_with(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
) -> LzShuffle {
    let maximum = if args.block_size <= 0 {
        reliable_block_size(seq.len(), args.resolved_alphabet(seq))
    } else {
        (args.block_size as usize).min(seq.len())
    };
    let ladder = mutual_information_ladder_bits(seq.as_bytes(), maximum);
    let bits_to_units = 2f64.ln() / (args.resolved_log_base(seq) as f64).ln();
    let mut result = LzShuffle::new(
        maximum as i32,
        ladder[maximum] * bits_to_units,
        symbol_entropy_bits(seq.as_bytes()) * bits_to_units
            - entropy_density_from(complexity.max(0) as u32, seq, args),
    );
    if args.get_shuffle_terms {
        result.summands = ladder
            .windows(2)
            .map(|pair| (pair[1] - pair[0]) * bits_to_units)
            .collect();
    }
    result
}

/// Legacy paired-sequence entry point, using the same finite-window estimator.
pub fn lz76_paired_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    let mid = seq.len() / 2;
    let (past, future) = seq.split_at(mid);
    let merged = merge_sequences(&past, &future);
    lz76_random_shuffle_complexity(&merged, args)
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

    #[test]
    fn block_ceiling_tracks_the_alphabet() {
        assert_eq!(reliable_block_size(1_000_000, 2), 15);
        assert_eq!(reliable_block_size(1_000_000, 4), 8);
        assert_eq!(reliable_block_size(1_000_000, 20), 4);
        assert_eq!(reliable_block_size(0, 2), 0);
        assert_eq!(reliable_block_size(1, 2), 1);
        assert!((1..=64).contains(&reliable_block_size(usize::MAX, 2)));
    }

    #[test]
    fn output_units_do_not_change_the_window_or_distribution() {
        let seq = Sequence::from_bytes((0..100_000).map(|i| b'A' + ((i / 64) % 4) as u8).collect());
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        args.log_base = 2;
        let bits = lz76_random_shuffle_complexity(&seq, &args);
        assert_eq!(bits.max_block_size, 6);
        for base in [4, 10] {
            args.log_base = base;
            let converted = lz76_random_shuffle_complexity(&seq, &args);
            let scale = (base as f64).log2();
            assert_eq!(converted.max_block_size, bits.max_block_size);
            assert!((converted.emc_value * scale - bits.emc_value).abs() < 1e-12);
            for (a, b) in converted.summands.iter().zip(&bits.summands) {
                assert!((a * scale - b).abs() < 1e-12);
            }
        }
        // Explicit alphabet changes the occupancy ceiling independently of units.
        args.alphabet = 20;
        assert_eq!(
            lz76_random_shuffle_complexity(&seq, &args).max_block_size,
            3
        );
    }

    #[test]
    fn emc_does_not_amplify_the_lz_rate_estimates_bias() {
        let seq = Sequence::from_str("00110");
        let mut args = LzArgs::new();
        args.block_size = 2;
        let first = lz76_random_shuffle_complexity_with(&seq, &args, 1);
        let second = lz76_random_shuffle_complexity_with(&seq, &args, 100);
        assert_eq!(first.emc_value, 0.0);
        assert_eq!(first.emc_value, second.emc_value);
        assert_ne!(first.multi_information, second.multi_information);
    }

    #[test]
    fn periodic_source_recovers_phase_and_increments_sum_to_value() {
        let seq = Sequence::from_bytes(b"00010111".iter().copied().cycle().take(20_000).collect());
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        let result = lz76_random_shuffle_complexity(&seq, &args);
        assert_eq!(result.summands.len(), result.max_block_size as usize);
        assert!((result.emc_value - 3.0).abs() < 1e-6);
        assert_eq!(result.summands[0], 0.0);
        assert!(result.summands.iter().all(|&s| s >= 0.0));
        assert!((result.summands.iter().sum::<f64>() - result.emc_value).abs() < 1e-12);
    }

    #[test]
    fn repeatability_includes_thread_count() {
        let seq = Sequence::from_str("aabcaabdaabcabcabdaabcd");
        let mut args = LzArgs::new();
        args.block_size = 6;
        args.get_shuffle_terms = true;
        let one = rayon::ThreadPoolBuilder::new()
            .num_threads(1)
            .build()
            .unwrap();
        let three = rayon::ThreadPoolBuilder::new()
            .num_threads(3)
            .build()
            .unwrap();
        let expected = one.install(|| lz76_random_shuffle_complexity(&seq, &args));
        for _ in 0..20 {
            let result = three.install(|| lz76_random_shuffle_complexity(&seq, &args));
            assert_eq!(result.emc_value.to_bits(), expected.emc_value.to_bits());
            assert_eq!(result.summands, expected.summands);
            assert_eq!(
                result.multi_information.to_bits(),
                expected.multi_information.to_bits()
            );
        }
    }

    #[test]
    fn explicit_window_is_capped_at_input_length() {
        let mut args = LzArgs::new();
        args.block_size = 100;
        args.get_shuffle_terms = true;
        let result = lz76_random_shuffle_complexity(&Sequence::from_str("abcd"), &args);
        assert_eq!(result.max_block_size, 4);
        assert_eq!(result.emc_value, 0.0); // Only one sampled window.
        let empty = lz76_random_shuffle_complexity(&Sequence::new(), &args);
        assert_eq!(empty.max_block_size, 0);
        assert_eq!(empty.emc_value, 0.0);
        assert!(empty.summands.is_empty());
    }

    #[test]
    fn shuffle_distance_ladder_keeps_released_auto_cutoff() {
        let seq = Sequence::from_str(&"001011".repeat(10));
        let args = LzArgs::new();
        let (_, maximum) = shuffle_factorization(&seq, &args);
        assert_eq!(maximum, max_block_size(seq.len()) + 10);
        let short = Sequence::from_str("001011");
        assert_eq!(
            shuffle_factorization(&short, &args).1,
            max_block_size(short.len())
        );
    }
}
