//! Predictive-state excess entropy and legacy shuffle distances.

use rayon::prelude::*;

use crate::block_entropy::symbol_entropy_bits;
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

/// Legacy random-shuffle factor counts used by the shuffle-distance metrics.
/// The auto-mode +10 is retained for compatibility with released 2.0.0.
/// The predictive-state EMC estimator below does not call this function.
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

/// Estimate full excess entropy with independently evaluated bidirectional CSSR.
/// The historical public name is retained; E estimation performs no shuffle.
/// Unsupported, unstable, or unsynchronized fits have no numerical E value.
pub fn lz76_random_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    lz76_random_shuffle_complexity_with(seq, args, lz76_factorization(seq, args) as i32)
}

/// Reuse an LZ factor count only for the legacy multi_information diagnostic.
pub fn lz76_random_shuffle_complexity_with(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
) -> LzShuffle {
    let mut result = crate::excess_entropy::estimate(seq.as_bytes(), args);
    let bits_to_units = 2f64.ln() / (args.resolved_log_base(seq) as f64).ln();
    result.emc_value = result.emc_value.map(|value| value * bits_to_units);
    for term in &mut result.summands {
        *term *= bits_to_units;
    }
    result.multi_information = symbol_entropy_bits(seq.as_bytes()) * bits_to_units
        - entropy_density_from(complexity.max(0) as u32, seq, args);
    result
}

/// Legacy paired-sequence entry point, using the same predictive-state estimator.
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
    fn periodic_source_recovers_full_phase_and_units() {
        let seq = Sequence::from_bytes(b"00010111".iter().copied().cycle().take(20_000).collect());
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        args.log_base = 2;
        let bits = lz76_random_shuffle_complexity(&seq, &args);
        assert_eq!(bits.status, "estimated", "{:?}", bits.message);
        assert!((bits.emc_value.unwrap() - 3.0).abs() < 1e-6);
        assert_eq!(bits.summands.len(), 8);
        assert!(bits.summands.iter().all(|&s| s >= 0.0));
        assert!((bits.summands.iter().sum::<f64>() - bits.emc_value.unwrap()).abs() < 1e-12);
        for base in [4, 10] {
            args.log_base = base;
            let converted = lz76_random_shuffle_complexity(&seq, &args);
            assert_eq!(converted.max_block_size, bits.max_block_size);
            assert!(
                (converted.emc_value.unwrap() * (base as f64).log2() - bits.emc_value.unwrap())
                    .abs()
                    < 1e-12
            );
        }
    }

    #[test]
    fn insufficient_data_has_no_fabricated_zero() {
        let seq = Sequence::from_str("00110");
        let args = LzArgs::new();
        let first = lz76_random_shuffle_complexity_with(&seq, &args, 1);
        let second = lz76_random_shuffle_complexity_with(&seq, &args, 100);
        assert_eq!(first.status, "insufficient_data");
        assert_eq!(first.emc_value, None);
        assert_eq!(first.emc_value, second.emc_value);
        assert_ne!(first.multi_information, second.multi_information);
    }

    #[test]
    fn repeatability_includes_thread_count() {
        let seq = Sequence::from_bytes(b"001011".iter().copied().cycle().take(8192).collect());
        let mut args = LzArgs::new();
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
        for _ in 0..3 {
            let result = three.install(|| lz76_random_shuffle_complexity(&seq, &args));
            assert_eq!(result.emc_value, expected.emc_value);
            assert_eq!(result.summands, expected.summands);
        }
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
