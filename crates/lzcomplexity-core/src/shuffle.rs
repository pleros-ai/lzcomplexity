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
/// of `u` block-shuffled at scale `l`. The surrogate is (idealised) an i.i.d.
/// stream of `l`-blocks whose entropy rate is `H(l)/l`, so multiplying back
/// recovers a block-entropy estimate `H_l = l · C_LZ(u^{RS(l)}) · g`, with
/// `g = log_k(N)/N` and `ĥ = C_LZ(u) · g` the entropy density of `u`. That
/// gives one *finite-scale excess entropy* per block size — the subextensive
/// part of the block entropy at scale `l`:
///
/// ```text
/// Ê(l) = H_l − l·ĥ = l · g · ( C_LZ(u^{RS(l)}) − C_LZ(u) )
/// ```
///
/// Theory pins down the shape of that ladder: `E(l) = Σ_{j≤l} [h(j) − h]` with
/// every increment `h(j) − h ≥ 0`, so `E(l)` is non-negative and non-decreasing
/// in `l` and rises to `E` (Crutchfield & Feldman, *Chaos* 13:25 (2003),
/// Lemma 1 and Eq. 52). The raw ladder violates both properties, because each
/// `Ê(l)` is an *independent* surrogate draw rather than a slice of one
/// consistent block-entropy curve. We therefore project it onto that cone —
/// non-negative isotonic regression, see [`non_negative_isotonic`] — and read
/// the answer off the projection:
///
/// ```text
/// summands[l−1] = Ê_fit(l) − Ê_fit(l−1)   ≥ 0     (the estimate of h(l) − h)
/// Ê             = Ê_fit(mm) = Σ summands  ≥ 0
/// ```
///
/// Every block size informs the fit, and the returned scalar can no longer go
/// negative — a mutual information never does.
///
/// # Why not sum the raw differences
///
/// Versions ≤ 1.0.1 summed `(H_l − H_{l−1}) − ĥ` directly. That is the same
/// textbook identity written the other way round, `Σ_l [h(l) − h] = H(L) − L·h`,
/// so the `H_l` telescoped and the total collapsed *exactly* to
/// `mm · g · (C_LZ(u^{RS(mm)}) − C_LZ(u))`: only the largest block size reached
/// the scalar, the other `mm − 1` factorizations cancelled algebraically, and a
/// single unlucky surrogate draw could push the result below zero. Projecting
/// first is what couples the scales together.
///
/// `multi_information` stays the *raw* `l = 1` contrast `H_1 − ĥ`, unchanged
/// from earlier versions: it is a separate quantity (the multi-information rate
/// of the source), not a term of the sum, and it is the best-calibrated number
/// this estimator produces.
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

    // The raw ladder Ê(l) = H_l − l·ĥ, one finite-scale excess entropy per
    // block size. `h_rand` is 1-indexed, so `len` is how many scales it holds.
    let len = mm.min(h_rand.len().saturating_sub(1));
    if len == 0 {
        return result;
    }
    let raw: Vec<f64> = (1..=len)
        .map(|l| l as f64 * (h_rand[l] as f64 * g - h_hat))
        .collect();

    // The l = 1 contrast, before any projection.
    result.multi_information = raw[0];

    // Project onto {non-negative, non-decreasing} and read off the increments.
    let fitted = non_negative_isotonic(&raw);
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

    /// A binary sequence (alphabet 2 -> log_base auto = 2) of length 100, plus
    /// `g = log_2(100)/100`. The block-entropy arithmetic only reads the length
    /// and the alphabet, so the content is irrelevant to these tests.
    fn fixture() -> (Sequence, LzArgs, f64) {
        let mut data = vec![b'0'; 50];
        data.extend(std::iter::repeat(b'1').take(50));
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.get_shuffle_terms = true;
        let n = 100.0f64;
        (seq, args, (n.ln() / 2f64.ln()) / n)
    }

    #[test]
    fn emc_projects_the_excess_ladder() {
        let (seq, args, g) = fixture();
        // c_1 = 20, c_2 = 15, c_3 = 12 against C_LZ(u) = 10, so the raw ladder
        // Ê(l) = l·g·(c_l − 10) is [10g, 10g, 6g] — non-monotone at l = 3.
        let r = shuffle_entropy_calculation(&seq, &args, 10, &[0, 20, 15, 12], 3);

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
        // c_l = 11 for every l gives Ê(l) = l·g, already strictly increasing,
        // so the projection is the identity and the total is the old
        // closed form mm·g·(c_mm − C_LZ(u)).
        let r = shuffle_entropy_calculation(&seq, &args, 10, &[0, 11, 11, 11], 3);

        assert!((r.emc_value - 3.0 * g).abs() < 1e-12, "{}", r.emc_value);
        for s in &r.summands {
            assert!((s - g).abs() < 1e-12, "{s}");
        }
    }

    #[test]
    fn emc_is_never_negative() {
        let (seq, args, g) = fixture();
        // Every surrogate less complex than the original: the raw ladder is
        // [−10g, −20g, −30g], entirely below zero.
        let r = shuffle_entropy_calculation(&seq, &args, 20, &[0, 10, 10, 10], 3);

        assert_eq!(r.emc_value, 0.0);
        assert!(r.summands.iter().all(|&s| s == 0.0), "{:?}", r.summands);
        // The diagnostic still shows the negative contrast.
        assert!((r.multi_information + 10.0 * g).abs() < 1e-12);
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
