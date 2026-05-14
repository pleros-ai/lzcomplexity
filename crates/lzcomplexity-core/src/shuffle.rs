//! Shuffle-based complexity measures.

use rayon::prelude::*;

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

    let computed: Vec<(usize, i32)> = (1..=mm)
        .into_par_iter()
        .map(|idx| {
            let rand_seq = shuffle_copy_seeded(seq, idx as u32, (seq.len() / 2) as u32, seed_for(seq, idx));
            let c = lz76_factorization(&rand_seq, args);
            (idx, c as i32)
        })
        .collect();
    for (idx, c) in computed {
        res[idx] = c;
    }
    (res, mm)
}

/// Mirror of `ShuffleEntropyCalculation(seq, args, complexity, H_rand, mm)`.
pub fn shuffle_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    h_rand: &[i32],
    mm: usize,
) -> LzShuffle {
    let mut result = LzShuffle::default();
    result.max_block_size = mm as i32;
    if args.get_shuffle_terms {
        result.summands = vec![0.0; mm];
    }
    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;
    let alphabet = if args.alphabet == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.alphabet.max(2)
    } as f64;

    let n = seq.len() as f64;
    if n <= 1.0 || mm == 0 {
        return result;
    }
    let log_n = n.ln() / log_base.ln();
    let log_a = alphabet.ln() / log_base.ln();
    let denom = n * log_a;
    if denom == 0.0 {
        return result;
    }

    let mut emc = 0.0f64;
    for idx in 1..=mm {
        if idx >= h_rand.len() {
            break;
        }
        let term = log_n * ((h_rand[idx] as f64) - (complexity as f64)).abs() / denom;
        emc += term;
        if args.get_shuffle_terms {
            result.summands[idx - 1] = term;
        }
        if idx == 1 {
            result.multi_information = term;
        }
    }
    result.emc_value = emc;
    result
}

/// `lz76RandomShuffleComplexity` — full pipeline.
pub fn lz76_random_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    let complexity = lz76_factorization(seq, args) as i32;
    let (h_rand, mm) = shuffle_factorization(seq, args);
    shuffle_entropy_calculation(seq, args, complexity, &h_rand, mm)
}

/// `lz76PairedShuffleComplexity` — shuffle on the merged (Z) sequence.
pub fn lz76_paired_shuffle_complexity(seq: &Sequence, args: &LzArgs) -> LzShuffle {
    let mid = seq.len() / 2;
    let (past, future) = seq.split_at(mid);
    let merged = merge_sequences(&past, &future);
    let complexity = lz76_factorization(&merged, args) as i32;
    let (h_rand, mm) = shuffle_factorization(&merged, args);
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

fn seed_for(seq: &Sequence, idx: usize) -> u64 {
    // Deterministic per (sequence content, block size)
    let mut h: u64 = 14695981039346656037; // FNV offset basis (64-bit)
    for &c in seq.as_bytes() {
        h ^= c as u64;
        h = h.wrapping_mul(1099511628211);
    }
    h ^ ((idx as u64).wrapping_mul(0x9E3779B97F4A7C15))
}
