//! Deterministic empirical entropy and predictive-state mutual information.
use std::collections::BTreeMap;

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

/// One nonnegative p(s+) KL[p(s-|s+) || p(s-)] contribution per forward state.
pub(crate) fn state_information_bits(
    counts: &BTreeMap<(usize, usize), usize>,
    forward: usize,
    reverse: usize,
) -> Vec<f64> {
    let mut rows = vec![0usize; forward];
    let mut columns = vec![0usize; reverse];
    let total = counts.values().sum::<usize>() as f64;
    if total == 0.0 {
        return vec![0.0; forward];
    }
    for (&(a, b), &count) in counts {
        rows[a] += count;
        columns[b] += count;
    }
    (0..forward)
        .map(|state| {
            let mut terms: Vec<_> = counts
                .iter()
                .filter(|((a, _), _)| *a == state)
                .map(|(&(a, b), &count)| (count, rows[a], columns[b]))
                .collect();
            terms.sort_unstable();
            compensated_sum(terms.into_iter().map(|(count, row, column)| {
                let count = count as f64;
                (count / total) * (count * total / (row as f64 * column as f64)).log2()
            }))
            .max(0.0)
        })
        .collect()
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
    fn state_information_is_not_state_entropy() {
        let independent = BTreeMap::from([((0, 0), 1), ((0, 1), 1), ((1, 0), 1), ((1, 1), 1)]);
        assert_eq!(state_information_bits(&independent, 2, 2), vec![0.0, 0.0]);
        let identical = BTreeMap::from([((0, 0), 1), ((1, 1), 1)]);
        assert_eq!(state_information_bits(&identical, 2, 2), vec![0.5, 0.5]);
    }
}
