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

/// Effective measure complexity via the block-entropy estimator.
///
/// For each block size `l`, `h_rand[l]` is `C_LZ(u^{RS(l)})`, the LZ complexity
/// of `u` block-shuffled at scale `l`. From it we form a block-entropy estimate
/// `H_l = l * C_LZ(u^{RS(l)}) * log_k(N) / N` (with `H_0 = 0`), take the entropy
/// gain `ΔH_l = H_l - H_{l-1}`, and sum each gain's excess over the original
/// sequence's entropy density `ĥ = C_LZ(u) * log_k(N) / N`:
///
/// ```text
/// Ê = Σ_{l=1}^{mm} (ΔH_l - ĥ)
/// ```
pub fn shuffle_entropy_calculation(
    seq: &Sequence,
    args: &LzArgs,
    complexity: i32,
    h_rand: &[i32],
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
    // g = log_k(N) / N, so ĥ = C_LZ(u)·g and H_l = l·C_LZ(u^{RS(l)})·g.
    let g = (n.ln() / log_base.ln()) / n;
    let h_hat = complexity as f64 * g; // entropy density of the original sequence

    let mut emc = 0.0f64;
    let mut h_prev = 0.0f64; // H_0 = 0
    for l in 1..=mm {
        if l >= h_rand.len() {
            break;
        }
        let h_l = l as f64 * h_rand[l] as f64 * g;
        let term = (h_l - h_prev) - h_hat; // ΔH_l - ĥ
        emc += term;
        if args.get_shuffle_terms {
            result.summands[l - 1] = term;
        }
        if l == 1 {
            result.multi_information = term;
        }
        h_prev = h_l;
    }
    result.emc_value = emc;
    result
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
    fn emc_block_entropy_formula() {
        // Binary sequence (alphabet 2 -> log_base auto = 2), n = 100.
        let mut data = vec![b'0'; 50];
        data.extend(std::iter::repeat(b'1').take(50));
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;

        let complexity = 10i32; // C_LZ(u)
        let h_rand = vec![0, 20, 15, 12]; // index 0 unused; c_1=20, c_2=15, c_3=12
        let mm = 3usize;
        let r = shuffle_entropy_calculation(&seq, &args, complexity, &h_rand, mm);

        // Hand-computed: g = log_2(100)/100, ĥ = 10·g, H_l = l·c_l·g (H_0 = 0).
        let n = 100.0f64;
        let g = (n.ln() / 2f64.ln()) / n;
        let h_hat = 10.0 * g;
        let (h1, h2, h3) = (1.0 * 20.0 * g, 2.0 * 15.0 * g, 3.0 * 12.0 * g);
        let expected = (h1 - 0.0 - h_hat) + (h2 - h1 - h_hat) + (h3 - h2 - h_hat);
        assert!(
            (r.emc_value - expected).abs() < 1e-12,
            "{} vs {}",
            r.emc_value,
            expected
        );
        assert!((r.summands[0] - (h1 - h_hat)).abs() < 1e-12);
        assert!((r.multi_information - (h1 - h_hat)).abs() < 1e-12);
    }
}
