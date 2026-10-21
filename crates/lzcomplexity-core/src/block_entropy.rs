//! Empirical mutual information between adjacent past and future windows.
//!
//! All scales use the same sample centers. Smaller centered windows are
//! marginals of the largest window's empirical distribution, so their mutual
//! information is nonnegative and nondecreasing by data processing. No rate
//! estimate, surrogate shuffle, missing-category correction, or peak selection
//! is needed. Finite samples introduce positive bias, and finite windows can
//! miss long-range dependence. Overlapping windows are dependent; their number
//! is not an independent-sample count or an error bar.

use std::collections::HashMap;

use rayon::prelude::*;

type Histogram<'a> = HashMap<&'a [u8], usize>;

fn histogram(data: &[u8], block_size: usize) -> Histogram<'_> {
    data.windows(block_size)
        .fold(HashMap::new(), |mut counts, block| {
            *counts.entry(block).or_insert(0) += 1;
            counts
        })
}

/// Shannon entropy of the full sequence's one-symbol empirical distribution.
pub(crate) fn symbol_entropy_bits(data: &[u8]) -> f64 {
    if data.is_empty() {
        return 0.0;
    }
    let mut counts = [0usize; 256];
    for &symbol in data {
        counts[symbol as usize] += 1;
    }
    counts.sort_unstable();
    let n = data.len() as f64;
    compensated_sum(counts.into_iter().filter(|&c| c > 0).map(|count| {
        let p = count as f64 / n;
        -p * p.log2()
    }))
}

/// One-indexed empirical past/future mutual information ladder (bits).
///
/// At scale `l`, the past has `floor(l/2)` symbols and the future `ceil(l/2)`.
/// Every scale uses the `N - M + 1` windows of the resolved maximum size `M`,
/// with smaller windows centered on their common boundary. Index zero and the
/// one-symbol rung are zero. The requested size is capped at `N`.
pub(crate) fn mutual_information_ladder_bits(data: &[u8], max_block_size: usize) -> Vec<f64> {
    let max_block_size = max_block_size.min(data.len());
    if max_block_size == 0 {
        return vec![0.0];
    }

    let full = histogram(data, max_block_size);
    if full.len() == 1 {
        // A point-mass joint distribution has zero information at every
        // scale. In particular, an explicit M=N must not do O(N^2) work.
        return vec![0.0; max_block_size + 1];
    }
    let samples = data.len() - max_block_size + 1;
    let center = max_block_size / 2;
    let mut ladder = vec![0.0; max_block_size + 1];
    let values: Vec<f64> = (2..=max_block_size)
        .into_par_iter()
        .map(|size| {
            let past_len = size / 2;
            let future_len = size - past_len;
            let mut joint = Histogram::new();
            let mut past = Histogram::new();
            let mut future = Histogram::new();
            for (&window, &count) in &full {
                let left = &window[center - past_len..center];
                let right = &window[center..center + future_len];
                let pair = &window[center - past_len..center + future_len];
                *past.entry(left).or_insert(0) += count;
                *future.entry(right).or_insert(0) += count;
                *joint.entry(pair).or_insert(0) += count;
            }

            // Count tuples are integer-valued and independent of hash order.
            // Sort before floating-point arithmetic for reproducibility across
            // runs, thread counts, and bijective symbol relabelings.
            let mut terms: Vec<(usize, usize, usize)> = joint
                .iter()
                .map(|(&pair, &count)| (count, past[&pair[..past_len]], future[&pair[past_len..]]))
                .collect();
            terms.sort_unstable();
            let n = samples as f64;
            compensated_sum(
                terms
                    .into_iter()
                    .map(|(joint_count, past_count, future_count)| {
                        let joint_count = joint_count as f64;
                        let ratio = joint_count * n / (past_count as f64 * future_count as f64);
                        (joint_count / n) * ratio.log2()
                    }),
            )
        })
        .collect();

    let mut previous = 0.0;
    for (size, value) in (2..=max_block_size).zip(values) {
        // Nested empirical distributions guarantee these inequalities;
        // only floating-point roundoff can violate them.
        previous = value.max(previous);
        ladder[size] = previous;
    }
    ladder
}

fn compensated_sum(values: impl IntoIterator<Item = f64>) -> f64 {
    let mut sum = 0.0;
    let mut correction = 0.0;
    for value in values {
        let adjusted = value - correction;
        let next = sum + adjusted;
        correction = (next - sum) - adjusted;
        sum = next;
    }
    sum
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn independent_empirical_pairs_have_zero_information() {
        // Four equally frequent adjacent pairs: 00, 01, 11, 10.
        assert_eq!(
            mutual_information_ladder_bits(b"00110", 2),
            vec![0.0, 0.0, 0.0]
        );
    }

    #[test]
    fn periodic_source_shares_its_phase() {
        assert_eq!(
            mutual_information_ladder_bits(b"01010101", 3),
            vec![0.0, 0.0, 1.0, 1.0]
        );
        assert_eq!(mutual_information_ladder_bits(b"aaaaaaaa", 3), vec![0.0; 4]);
    }

    #[test]
    fn nested_scales_share_one_empirical_distribution() {
        // Every binary four-letter word occurs once in this linearized
        // de Bruijn cycle. Every centered marginal is independent too.
        let data = b"0000100110101111000";
        assert_eq!(histogram(data, 4).len(), 16);
        assert_eq!(mutual_information_ladder_bits(data, 4), vec![0.0; 5]);
    }

    #[test]
    fn small_binary_inputs_match_a_direct_joint_distribution() {
        for size in 2..=8 {
            for mask in 0..1usize << size {
                let data: Vec<u8> = (0..size).map(|i| ((mask >> i) & 1) as u8).collect();
                for maximum in 2..=size {
                    let ladder = mutual_information_ladder_bits(&data, maximum);
                    let past_len = maximum / 2;
                    let samples = size - maximum + 1;
                    let mut past = Histogram::new();
                    let mut future = Histogram::new();
                    let joint = histogram(&data, maximum);
                    // Independently count marginals directly from positions.
                    for i in 0..samples {
                        *past.entry(&data[i..i + past_len]).or_insert(0) += 1;
                        *future.entry(&data[i + past_len..i + maximum]).or_insert(0) += 1;
                    }
                    let entropy = |counts: &Histogram<'_>| {
                        counts
                            .values()
                            .map(|&c| {
                                let p = c as f64 / samples as f64;
                                -p * p.log2()
                            })
                            .sum::<f64>()
                    };
                    let reference = entropy(&past) + entropy(&future) - entropy(&joint);
                    assert!((ladder[maximum] - reference).abs() < 1e-12);
                    assert!(ladder.windows(2).all(|pair| pair[0] <= pair[1]));
                }
            }
        }
    }

    #[test]
    fn entropy_and_information_are_reproducible() {
        let data = b"aabcaabdaabcabcabdaabcd";
        let expected = mutual_information_ladder_bits(data, 6);
        let entropy = symbol_entropy_bits(data);
        for _ in 0..30 {
            assert_eq!(mutual_information_ladder_bits(data, 6), expected);
            assert_eq!(symbol_entropy_bits(data), entropy);
        }
        let renamed: Vec<u8> = data.iter().map(|b| 255 - b).collect();
        assert_eq!(mutual_information_ladder_bits(&renamed, 6), expected);
    }

    #[test]
    fn empty_short_and_oversized_requests_are_finite() {
        assert_eq!(mutual_information_ladder_bits(b"", 20), vec![0.0]);
        assert_eq!(mutual_information_ladder_bits(b"a", 20), vec![0.0, 0.0]);
        assert_eq!(mutual_information_ladder_bits(b"abcd", 20), vec![0.0; 5]);
        assert_eq!(symbol_entropy_bits(b""), 0.0);
    }
}
