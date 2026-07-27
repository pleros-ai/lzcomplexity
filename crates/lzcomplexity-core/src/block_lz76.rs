//! Block-level (token-aware) LZ76 factorization.
//!
//! Building `H_l` (the joint entropy of `l` consecutive symbols) via the
//! random-shuffle method requires shuffling the sequence at scale `l` and
//! measuring the LZ76 complexity of the result. Doing that at the byte
//! level — physically cutting the sequence into `l`-byte blocks, swapping
//! blocks around in place, then running ordinary single-symbol LZ76 on the
//! reassembled bitstream — lets the parser match *through* the artificial
//! seams the cut-and-reassemble step creates. That systematically inflates
//! the factor count by an amount that does not shrink with `l` (see
//! `shuffle::shuffle_entropy_calculation`'s doc comment for the full
//! diagnosis of why that made the estimator diverge).
//!
//! This module avoids the seam artifact by treating each block as one
//! atomic token: every distinct block pattern is mapped to a dense integer
//! id, the *token array* (not the raw bytes) is what gets shuffled, and
//! factorization runs over that token array — so a "match" can only ever be
//! between whole tokens. There is no seam for the parser to see partway
//! through, because there is no partway-through a token to be at.
//!
//! Removing that artifact does not make `factorize_tokens` an unbiased
//! entropy estimator in general — no LZ-complexity-based method is, once
//! the number of distinct tokens (`num_distinct`, i.e. the block alphabet)
//! grows large relative to how many tokens there are to observe (`m`). See
//! `large_alphabet_iid_entropy_is_underestimated_at_moderate_sample_ratios`
//! below for a controlled measurement: even on freshly-drawn i.i.d. data
//! with no shuffle involved at all, `m=500,000` samples over `K=28,256`
//! categories recovers only ~2/3 of the true `log2(K)` bits. Larger block
//! sizes `l` shrink `m = N/l` while growing `K_l` (the number of distinct
//! `l`-blocks actually observed), so this bias grows with `l` — which is
//! why `shuffle::shuffle_entropy_calculation` treats the point where its
//! ladder stops improving as the edge of the trustworthy range, rather than
//! extending `l` indefinitely.

use std::collections::HashMap;

use rand::seq::SliceRandom;
use rand::SeedableRng;
use rand_chacha::ChaCha8Rng;

use crate::lpf::lpf;
use crate::suffix_array::{build_lcp, build_suffix_array};

/// Split `data` into non-overlapping blocks of `block_size` bytes (a
/// trailing remainder shorter than `block_size` is dropped, matching the
/// byte-level shuffle's own alignment convention) and map each distinct
/// block pattern to a dense id, assigned in order of first appearance.
/// Returns the token sequence and the number of distinct tokens found.
pub fn tokenize_blocks(data: &[u8], block_size: usize) -> (Vec<u32>, usize) {
    assert!(block_size > 0, "block_size must be positive");
    let num_blocks = data.len() / block_size;
    let mut ids: HashMap<&[u8], u32> = HashMap::with_capacity(num_blocks);
    let mut tokens = Vec::with_capacity(num_blocks);
    for i in 0..num_blocks {
        let block = &data[i * block_size..(i + 1) * block_size];
        let next_id = ids.len() as u32;
        let id = *ids.entry(block).or_insert(next_id);
        tokens.push(id);
    }
    (tokens, ids.len())
}

/// Exact Fisher-Yates shuffle of the token array — uniform over all
/// `tokens.len()!` orderings in `O(tokens.len())` swaps. Unlike the
/// byte-level shuffle (which has to budget swaps against a length-`N`
/// array regardless of block size), there is no mixing-quality tradeoff
/// here: `tokens` is only `N / block_size` elements long, so an exact full
/// permutation is always cheap.
pub fn shuffle_tokens(tokens: &mut [u32], seed: u64) {
    let mut rng = ChaCha8Rng::seed_from_u64(seed);
    tokens.shuffle(&mut rng);
}

/// LZ76 factor count of a token sequence with `num_distinct` distinct
/// token values.
///
/// Encodes each token as a fixed-width big-endian byte string (so
/// byte-level lexicographic order matches token order exactly, with no
/// ambiguity about where one token's encoding ends and the next begins),
/// reuses the existing byte-level suffix array / LCP construction on that
/// encoding, then filters the result back down to token-aligned positions:
/// the LCP between two token-aligned suffix-array ranks is the minimum of
/// the byte-level LCP array over the range between them (the standard
/// "LCP of two ranks = min of the LCP array between them" identity), which
/// a single left-to-right scan computes in one pass; floor-dividing by the
/// encoding width then converts a byte-match length into a whole-token
/// match count (a match that stops partway through one token's encoding is
/// not a real token match). That filtered (sa, lcp) pair is a fully valid
/// suffix array / LCP array for the token sequence, so the existing
/// [`lpf`] routine — which only ever looks at `(sa, lcp, n)`, never the
/// underlying alphabet — accepts it completely unchanged.
pub fn factorize_tokens(tokens: &[u32], num_distinct: usize) -> u32 {
    let m = tokens.len();
    if m == 0 {
        return 0;
    }
    if num_distinct <= 1 {
        return 1;
    }

    let width = byte_width(num_distinct);
    let mut encoded = Vec::with_capacity(m * width);
    for &t in tokens {
        let be = t.to_be_bytes();
        encoded.extend_from_slice(&be[4 - width..]);
    }

    let sa_bytes = build_suffix_array(&encoded);
    let lcp_bytes = build_lcp(&encoded, &sa_bytes);

    let mut sa_tok: Vec<u32> = Vec::with_capacity(m);
    let mut lcp_tok: Vec<u32> = Vec::with_capacity(m);
    let mut running_min = u32::MAX;
    for i in 0..sa_bytes.len() {
        if i > 0 {
            running_min = running_min.min(lcp_bytes[i]);
        }
        if sa_bytes[i] as usize % width == 0 {
            let tok_lcp = if sa_tok.is_empty() {
                0
            } else {
                running_min / width as u32
            };
            sa_tok.push(sa_bytes[i] / width as u32);
            lcp_tok.push(tok_lcp);
            running_min = u32::MAX;
        }
    }
    debug_assert_eq!(sa_tok.len(), m);

    let mut lpf_arr = vec![0u32; m];
    lpf(&mut lpf_arr, sa_tok, lcp_tok, m);

    let mut lzf: Vec<u32> = Vec::with_capacity(m);
    lzf.push(0);
    lzf.push(1);
    let mut i: usize = 1;
    while i < m {
        let last = *lzf.last().unwrap() as usize;
        let advance = lpf_arr.get(i).copied().unwrap_or(0) as usize;
        i = last + advance + 1;
        lzf.push(i as u32);
    }
    if (*lzf.last().unwrap() as usize) <= m {
        (lzf.len() - 1) as u32
    } else {
        (lzf.len() - 2) as u32
    }
}

/// Smallest number of bytes that can hold `0..num_distinct` as a big-endian
/// unsigned integer (at least 1, at most 4 — `num_distinct` is a `usize`
/// derived from token counts that are themselves `u32`-representable
/// elsewhere in this crate, e.g. `SuffixArray::sa: Vec<u32>`).
fn byte_width(num_distinct: usize) -> usize {
    let mut w = 1usize;
    let mut cap = 256usize;
    while cap < num_distinct && w < 4 {
        w += 1;
        cap = cap.saturating_mul(256);
    }
    w
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tokenize_maps_distinct_blocks_and_repeats_share_ids() {
        let data = b"aabbaabbccdd";
        let (tokens, k) = tokenize_blocks(data, 2);
        // blocks: aa bb aa bb cc dd -> ids 0 1 0 1 2 3
        assert_eq!(tokens, vec![0, 1, 0, 1, 2, 3]);
        assert_eq!(k, 4);
    }

    #[test]
    fn tokenize_drops_trailing_remainder() {
        let (tokens, _) = tokenize_blocks(b"aabbc", 2);
        assert_eq!(tokens.len(), 2); // trailing "c" dropped
    }

    #[test]
    fn factorize_tokens_matches_byte_level_lz76_at_width_one() {
        // With <=256 distinct tokens (width=1), factorizing the token
        // sequence must exactly match ordinary byte-level LZ76 factorizing
        // the tokens-as-bytes directly, since the encoding is the identity.
        let tokens: Vec<u32> = vec![0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0];
        let bytes: Vec<u8> = tokens.iter().map(|&t| t as u8).collect();
        let sa = build_suffix_array(&bytes);
        let lcp = build_lcp(&bytes, &sa);
        let mut lpf_arr = vec![0u32; bytes.len()];
        lpf(&mut lpf_arr, sa, lcp, bytes.len());
        let mut lzf: Vec<u32> = vec![0, 1];
        let mut i = 1;
        while i < bytes.len() {
            let last = *lzf.last().unwrap() as usize;
            let advance = lpf_arr.get(i).copied().unwrap_or(0) as usize;
            i = last + advance + 1;
            lzf.push(i as u32);
        }
        let expected = if (*lzf.last().unwrap() as usize) <= bytes.len() {
            (lzf.len() - 1) as u32
        } else {
            (lzf.len() - 2) as u32
        };
        assert_eq!(factorize_tokens(&tokens, 2), expected);
    }

    #[test]
    fn factorize_tokens_handles_wide_alphabets() {
        // 500 distinct tokens forces width=2; a strictly-increasing token
        // sequence (all distinct, no repeats at all) has no way to form a
        // repeated phrase, so every token is its own factor.
        let tokens: Vec<u32> = (0..500).collect();
        assert_eq!(factorize_tokens(&tokens, 500), 500);
    }

    #[test]
    fn factorize_tokens_constant_sequence_is_one_factor() {
        let tokens = vec![7u32; 100];
        assert_eq!(factorize_tokens(&tokens, 1), 1);
    }

    #[test]
    fn factorize_tokens_empty_is_zero() {
        assert_eq!(factorize_tokens(&[], 0), 0);
    }

    #[test]
    fn shuffle_tokens_is_a_permutation() {
        let mut tokens: Vec<u32> = (0..200).collect();
        let original = tokens.clone();
        shuffle_tokens(&mut tokens, 42);
        let mut sorted = tokens.clone();
        sorted.sort_unstable();
        assert_eq!(sorted, original); // same multiset
        assert_ne!(tokens, original); // (astronomically likely) reordered
    }

    #[test]
    fn shuffle_tokens_is_deterministic_per_seed() {
        let mut a: Vec<u32> = (0..50).collect();
        let mut b = a.clone();
        shuffle_tokens(&mut a, 99);
        shuffle_tokens(&mut b, 99);
        assert_eq!(a, b);
    }

    /// Reference factor count computed directly on the token array (O(m^2
    /// log m) comparison sort of token *slices*, not byte-encoded), so this
    /// is independent of `factorize_tokens`'s width-encoding trick and can
    /// catch a bug specific to width > 1 (i.e. > 256 distinct tokens).
    fn reference_factorize_tokens(tokens: &[u32]) -> u32 {
        let m = tokens.len();
        if m == 0 {
            return 0;
        }
        let mut idx: Vec<u32> = (0..m as u32).collect();
        idx.sort_unstable_by(|&a, &b| tokens[a as usize..].cmp(&tokens[b as usize..]));
        let mut lcp = vec![0u32; m];
        for i in 1..m {
            let a = &tokens[idx[i - 1] as usize..];
            let b = &tokens[idx[i] as usize..];
            lcp[i] = a.iter().zip(b.iter()).take_while(|(x, y)| x == y).count() as u32;
        }
        let mut lpf_arr = vec![0u32; m];
        lpf(&mut lpf_arr, idx, lcp, m);
        let mut lzf: Vec<u32> = vec![0, 1];
        let mut i = 1;
        while i < m {
            let last = *lzf.last().unwrap() as usize;
            let advance = lpf_arr.get(i).copied().unwrap_or(0) as usize;
            i = last + advance + 1;
            lzf.push(i as u32);
        }
        if (*lzf.last().unwrap() as usize) <= m {
            (lzf.len() - 1) as u32
        } else {
            (lzf.len() - 2) as u32
        }
    }

    #[test]
    fn factorize_tokens_matches_reference_with_repeats_and_wide_alphabet() {
        // Random token sequences with 300-900 distinct values (forcing
        // width=2) AND plenty of repeats, cross-checked against a
        // from-scratch reference that never goes through byte encoding.
        let mut state: u64 = 0xC0FFEE;
        let mut next = || {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            state
        };
        for _ in 0..50 {
            let k = 50 + (next() as usize) % 800; // distinct-token budget
            let m = 200 + (next() as usize) % 3000;
            let tokens: Vec<u32> = (0..m).map(|_| (next() as u32) % (k as u32)).collect();
            // Re-densify to 0..num_distinct-1 in first-appearance order, the
            // same convention tokenize_blocks uses.
            let mut seen = std::collections::HashMap::new();
            let mut dense = Vec::with_capacity(m);
            for &t in &tokens {
                let next_id = seen.len() as u32;
                dense.push(*seen.entry(t).or_insert(next_id));
            }
            let num_distinct = seen.len();
            let expected = reference_factorize_tokens(&dense);
            let got = factorize_tokens(&dense, num_distinct);
            assert_eq!(
                got, expected,
                "mismatch: m={} num_distinct={} first20={:?}",
                m,
                num_distinct,
                &dense[..dense.len().min(20)]
            );
        }
    }

    /// `factorize_tokens` is exact (see the tests above), but LZ76 itself is
    /// a known-slow-converging entropy estimator once the alphabet is large
    /// relative to the sample count — this documents that as an expected
    /// characteristic of the estimator, not a defect to chase, so a future
    /// change that makes this number drift doesn't get mistaken for a
    /// regression in `factorize_tokens`'s correctness (which the tests
    /// above already pin down independently).
    ///
    /// m=500,000 i.i.d. uniform draws over K=28,256 categories: true entropy
    /// is exactly `log2(K)` (no shuffle-mixing question at all — this is
    /// freshly-drawn i.i.d. data, not a shuffled block sequence), yet LZ76
    /// recovers only ~2/3 of it at this scale. This is *why*
    /// `shuffle_entropy_calculation` truncates the auto-mode ladder at its
    /// peak rather than trusting arbitrarily large block sizes: `Ĥ_l`'s own
    /// undersampling bias grows with `l` (the block-token alphabet grows
    /// with it) and eventually swamps the real excess-entropy signal.
    #[test]
    fn large_alphabet_iid_entropy_is_underestimated_at_moderate_sample_ratios() {
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
        let c = factorize_tokens(&tokens, k as usize);
        let g_m = (m as f64).ln() / 2f64.ln() / m as f64;
        let h_est = c as f64 * g_m;
        let true_h = (k as f64).log2();
        let ratio = h_est / true_h;
        assert!(
            (0.55..0.80).contains(&ratio),
            "ratio {ratio:.4} moved outside the documented range \
             (H_est={h_est:.4}, true_H={true_h:.4}) — re-examine whether \
             this is still the known LZ76 large-alphabet bias or something \
             new before adjusting this range"
        );
    }
}
