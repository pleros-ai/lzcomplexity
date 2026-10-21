//! Resource-bounded Causal-State Splitting Reconstruction (CSSR).
//!
//! Adapted from `sofic.inference.cssr` in https://github.com/dit/sofic,
//! commit 823741ebd24b4dfd175ef063be30655269ea0f33, Copyright (c) 2026,
//! sofic contributors, under the BSD 3-Clause License. The complete notice is
//! retained in `THIRD_PARTY_LICENSES/sofic-BSD-3-Clause.txt`.
//!
//! The algorithm follows Shalizi and Shalizi (2004),
//! https://arxiv.org/abs/cs/0406011 . Reconstruction assumes a stationary
//! finite-state process with finite synchronizing words. A returned machine is
//! an inferred candidate, not a certificate that those assumptions hold.
//! Candidate selection, independent validation, and excess-entropy estimation
//! belong to the caller. No source-specific topology is recognized here.
//! Morph comparisons use up to three future symbols, rather than just one,
//! so states with equal one-step emissions can remain predictively distinct.
//! This resource-bounded extension is separate from the paper's theorem.

use std::collections::{BTreeMap, BTreeSet, HashMap};

use statrs::distribution::{ChiSquared, ContinuousCDF};

const MAX_STATES: usize = 64;
const MAX_CONTEXTS: usize = 100_000;
const MAX_HISTORY: usize = 64;
const MIN_COUNT: u64 = 5;
const FAMILY_ALPHA: f64 = 0.01;
const MAX_PREDICTIVE_WORDS: usize = 32;
const MAX_LOOKAHEAD: usize = 3;

type History = Vec<u8>;
type State = BTreeSet<History>;
type Counts = HashMap<History, Vec<u64>>;
type Targets = BTreeMap<usize, BTreeSet<History>>;
type Edges = Vec<Vec<Targets>>;

/// A deterministic, symbol-labeled inferred predictive machine.
/// Transition and emission columns have the order of `alphabet`.
#[derive(Debug, Clone)]
pub(crate) struct Machine {
    pub transitions: Vec<Vec<Option<usize>>>,
    pub emissions: Vec<Vec<f64>>,
    pub alphabet: Vec<u8>,
    pub histories: Vec<Vec<Vec<u8>>>,
    pub predictive_horizon: usize,
}

struct Morphs<'a> {
    counts: &'a Counts,
    size: usize,
}

struct Tests {
    critical: Vec<f64>,
}

impl Tests {
    fn new(alphabet_size: usize, eligible: usize) -> Result<Self, String> {
        let alpha = FAMILY_ALPHA / eligible.max(1) as f64;
        let mut critical = vec![f64::INFINITY; alphabet_size.max(1)];
        for (dof, value) in critical.iter_mut().enumerate().skip(1) {
            *value = ChiSquared::new(dof as f64)
                .map_err(|error| format!("invalid CSSR test distribution: {error}"))?
                .inverse_cdf(1.0 - alpha);
            if !value.is_finite() {
                return Err("CSSR significance threshold is not finite".into());
            }
        }
        Ok(Self { critical })
    }

    fn score(&self, left: &[u64], right: &[u64]) -> (f64, usize) {
        let active = left
            .iter()
            .zip(right)
            .filter(|(a, b)| **a + **b > 0)
            .count();
        let left_total = left.iter().sum::<u64>() as f64;
        let right_total = right.iter().sum::<u64>() as f64;
        if active < 2 || left_total == 0.0 || right_total == 0.0 {
            return (0.0, 0);
        }
        let total = left_total + right_total;
        let mut score = 0.0;
        for (&a, &b) in left.iter().zip(right) {
            let column = (a + b) as f64;
            if column == 0.0 {
                continue;
            }
            for (count, row) in [(a as f64, left_total), (b as f64, right_total)] {
                let expected = row * column / total;
                // Match scipy's G-test continuity correction for a 2x2 table.
                let count = if active == 2 {
                    let difference = expected - count;
                    count + difference.signum() * difference.abs().min(0.5)
                } else {
                    count
                };
                if count > 0.0 {
                    score += count * (count / expected).ln();
                }
            }
        }
        ((2.0 * score).max(0.0), active - 1)
    }

    fn different(&self, left: &[u64], right: &[u64]) -> bool {
        let (score, dof) = self.score(left, right);
        dof > 0 && score > self.critical[dof]
    }
}

fn aggregate(counts: &Counts, histories: &State, alphabet_size: usize) -> Vec<u64> {
    let mut result = vec![0u64; alphabet_size];
    for history in histories {
        if let Some(observed) = counts.get(history) {
            for (sum, &count) in result.iter_mut().zip(observed) {
                *sum += count;
            }
        }
    }
    result
}

fn count_histories(data: &[u8], max_history: usize, alphabet: &[u8]) -> Result<Counts, String> {
    let mut symbol_index = [0usize; 256];
    for (index, &symbol) in alphabet.iter().enumerate() {
        symbol_index[symbol as usize] = index;
    }
    let mut counts = Counts::new();
    for position in 0..data.len() {
        let column = symbol_index[data[position] as usize];
        for length in 0..=position.min(max_history + 1) {
            let history = &data[position - length..position];
            if !counts.contains_key(history) && counts.len() >= MAX_CONTEXTS {
                return Err(format!(
                    "CSSR exceeds the {MAX_CONTEXTS}-context resource limit"
                ));
            }
            counts
                .entry(history.to_vec())
                .or_insert_with(|| vec![0; alphabet.len()])[column] += 1;
        }
    }
    Ok(counts)
}

/// Bound the full predictive-word alphabet before allocating count rows.
/// A wide alphabet retains single-symbol morphs rather than allocating sigma^3.
fn predictive_horizon(alphabet_size: usize) -> usize {
    let mut horizon = 1;
    let mut words = alphabet_size;
    while horizon < MAX_LOOKAHEAD && words.saturating_mul(alphabet_size) <= MAX_PREDICTIVE_WORDS {
        words *= alphabet_size;
        horizon += 1;
    }
    horizon
}

fn count_predictive_words(
    data: &[u8],
    max_history: usize,
    horizon: usize,
) -> Result<(Counts, usize), String> {
    let outcomes: BTreeSet<&[u8]> = data.windows(horizon).collect();
    if outcomes.len() > MAX_PREDICTIVE_WORDS {
        return Err("CSSR predictive-word alphabet exceeds its resource limit".into());
    }
    let columns: HashMap<&[u8], usize> = outcomes
        .iter()
        .enumerate()
        .map(|(column, &word)| (word, column))
        .collect();
    let mut counts = Counts::new();
    for (position, word) in data.windows(horizon).enumerate() {
        let column = columns[word];
        for length in 0..=position.min(max_history + 1) {
            let history = &data[position - length..position];
            if !counts.contains_key(history) && counts.len() >= MAX_CONTEXTS {
                return Err(format!(
                    "CSSR exceeds the {MAX_CONTEXTS}-context resource limit"
                ));
            }
            counts
                .entry(history.to_vec())
                .or_insert_with(|| vec![0; outcomes.len()])[column] += 1;
        }
    }
    Ok((counts, outcomes.len()))
}

fn homogenize(
    counts: &Counts,
    alphabet: &[u8],
    morph_size: usize,
    max_history: usize,
    tests: &Tests,
) -> Result<Vec<State>, String> {
    let mut states = vec![BTreeSet::from([Vec::new()])];
    for length in 0..max_history {
        // States created while processing this depth are visited at the next
        // depth, as in CSSR's sufficiency phase.
        let existing_states = states.len();
        for parent_id in 0..existing_states {
            let parents: Vec<History> = states[parent_id]
                .iter()
                .filter(|history| history.len() == length)
                .cloned()
                .collect();
            for parent in parents {
                for &symbol in alphabet {
                    let mut child = Vec::with_capacity(parent.len() + 1);
                    child.push(symbol);
                    child.extend_from_slice(&parent);
                    let Some(observed) = counts.get(&child) else {
                        continue;
                    };
                    if observed.iter().sum::<u64>() < MIN_COUNT {
                        continue;
                    }
                    let parent_counts = aggregate(counts, &states[parent_id], morph_size);
                    if !tests.different(&parent_counts, observed) {
                        states[parent_id].insert(child);
                        continue;
                    }
                    let mut best: Option<(usize, f64)> = None;
                    for (candidate_id, candidate) in states.iter().enumerate() {
                        if candidate_id == parent_id {
                            continue;
                        }
                        let candidate_counts = aggregate(counts, candidate, morph_size);
                        if tests.different(&candidate_counts, observed) {
                            continue;
                        }
                        let score = tests.score(&candidate_counts, observed).0;
                        if best.is_none_or(|(_, previous)| score < previous) {
                            best = Some((candidate_id, score));
                        }
                    }
                    if let Some((candidate_id, _)) = best {
                        states[candidate_id].insert(child);
                    } else {
                        if states.len() >= MAX_STATES {
                            return Err(format!(
                                "CSSR exceeds the {MAX_STATES}-state resource limit"
                            ));
                        }
                        states.push(BTreeSet::from([child]));
                    }
                }
            }
        }
    }
    Ok(states)
}

fn edges(
    states: &[State],
    counts: &Counts,
    morphs: &Morphs<'_>,
    alive: &BTreeSet<usize>,
    alphabet: &[u8],
    max_history: usize,
    tests: &Tests,
) -> Result<Edges, String> {
    let history_to_state: HashMap<&[u8], usize> = alive
        .iter()
        .flat_map(|&id| {
            states[id]
                .iter()
                .map(move |history| (history.as_slice(), id))
        })
        .collect();
    let aggregates: Vec<Vec<u64>> = states
        .iter()
        .map(|state| aggregate(morphs.counts, state, morphs.size))
        .collect();
    let mut result = vec![vec![Targets::new(); alphabet.len()]; states.len()];
    for &id in alive {
        for history in &states[id] {
            let Some(observed) = counts.get(history) else {
                continue;
            };
            for (column, &symbol) in alphabet.iter().enumerate() {
                if observed[column] == 0 {
                    continue;
                }
                let mut extended = history.clone();
                extended.push(symbol);
                let mut target = if extended.len() <= max_history {
                    history_to_state.get(extended.as_slice()).copied()
                } else {
                    history_to_state.get(&extended[1..]).copied()
                };
                if extended.len() > max_history {
                    if let Some(next_counts) = morphs.counts.get(&extended) {
                        if target.is_none_or(|candidate| {
                            tests.different(&aggregates[candidate], next_counts)
                        }) {
                            let mut best: Option<(usize, f64)> = None;
                            for &candidate in alive {
                                if tests.different(&aggregates[candidate], next_counts) {
                                    continue;
                                }
                                let score = tests.score(&aggregates[candidate], next_counts).0;
                                if best.is_none_or(|(_, previous)| score < previous) {
                                    best = Some((candidate, score));
                                }
                            }
                            // Retain the observed suffix successor when no
                            // compatible alternative exists yet. Subsequent
                            // determinization can split this provisional
                            // state; deleting it here prevents that refinement.
                            if let Some((candidate, _)) = best {
                                target = Some(candidate);
                            }
                            if target.is_none() {
                                return Err(
                                    "CSSR cannot resolve an observed history extension".into()
                                );
                            }
                        }
                    }
                }
                if let Some(target) = target {
                    result[id][column]
                        .entry(target)
                        .or_default()
                        .insert(history.clone());
                }
            }
        }
    }
    Ok(result)
}

/// Closed communicating classes of a potentially nondeterministic graph.
fn recurrent_classes(edges: &Edges, alive: &BTreeSet<usize>) -> Vec<BTreeSet<usize>> {
    let n = edges.len();
    let mut reachable = vec![vec![false; n]; n];
    for &id in alive {
        reachable[id][id] = true;
        for targets in &edges[id] {
            for &target in targets.keys() {
                reachable[id][target] = true;
            }
        }
    }
    for &middle in alive {
        for &source in alive {
            if reachable[source][middle] {
                for &target in alive {
                    reachable[source][target] |= reachable[middle][target];
                }
            }
        }
    }
    let mut unseen = alive.clone();
    let mut classes = Vec::new();
    while let Some(&first) = unseen.first() {
        let class: BTreeSet<usize> = alive
            .iter()
            .copied()
            .filter(|&id| reachable[first][id] && reachable[id][first])
            .collect();
        for id in &class {
            unseen.remove(id);
        }
        let has_edge = class
            .iter()
            .any(|&id| edges[id].iter().any(|targets| !targets.is_empty()));
        let closed = class.iter().all(|&id| {
            edges[id]
                .iter()
                .all(|targets| targets.keys().all(|target| class.contains(target)))
        });
        if has_edge && closed {
            classes.push(class);
        }
    }
    classes
}

fn determinize(
    states: &mut Vec<State>,
    counts: &Counts,
    morphs: &Morphs<'_>,
    alive: &mut BTreeSet<usize>,
    alphabet: &[u8],
    max_history: usize,
    tests: &Tests,
) -> Result<Edges, String> {
    loop {
        let graph = edges(states, counts, morphs, alive, alphabet, max_history, tests)?;
        let split = alive.iter().find_map(|&id| {
            graph[id]
                .iter()
                .find(|targets| targets.len() > 1)
                .map(|targets| (id, targets))
        });
        let Some((id, targets)) = split else {
            return Ok(graph);
        };
        let mut groups: Vec<State> = targets.values().cloned().collect();
        groups.sort_by(|a, b| b.len().cmp(&a.len()).then_with(|| a.cmp(b)));
        for group in groups.into_iter().skip(1) {
            if states.len() >= MAX_STATES {
                return Err(format!(
                    "CSSR exceeds the {MAX_STATES}-state resource limit"
                ));
            }
            for history in &group {
                states[id].remove(history);
            }
            states.push(group);
            alive.insert(states.len() - 1);
        }
    }
}

fn build_machine(
    data: &[u8],
    counts: &Counts,
    states: &[State],
    graph: &Edges,
    alive: &BTreeSet<usize>,
    alphabet: &[u8],
    max_history: usize,
) -> Result<Machine, String> {
    let history_to_state: HashMap<&[u8], usize> = alive
        .iter()
        .flat_map(|&id| {
            states[id]
                .iter()
                .map(move |history| (history.as_slice(), id))
        })
        .collect();
    let mut visits = vec![0u64; states.len()];
    for position in 0..=data.len() {
        for length in (0..=position.min(max_history)).rev() {
            if let Some(&id) = history_to_state.get(&data[position - length..position]) {
                visits[id] += 1;
                break;
            }
        }
    }
    let classes = recurrent_classes(graph, alive);
    let keep = classes
        .iter()
        .max_by(|a, b| {
            let a_visits: u64 = a.iter().map(|&id| visits[id]).sum();
            let b_visits: u64 = b.iter().map(|&id| visits[id]).sum();
            a_visits
                .cmp(&b_visits)
                .then_with(|| b.first().cmp(&a.first()))
        })
        .ok_or("CSSR found no recurrent inferred states")?;
    let labels: HashMap<usize, usize> = keep
        .iter()
        .enumerate()
        .map(|(new, &old)| (old, new))
        .collect();
    let mut machine = Machine {
        transitions: vec![vec![None; alphabet.len()]; keep.len()],
        emissions: vec![vec![0.0; alphabet.len()]; keep.len()],
        alphabet: alphabet.to_vec(),
        histories: Vec::with_capacity(keep.len()),
        predictive_horizon: 1,
    };
    for (new_id, &old_id) in keep.iter().enumerate() {
        let longest = states[old_id]
            .iter()
            .map(Vec::len)
            .max()
            .ok_or("CSSR produced an empty state")?;
        let longest_histories: State = states[old_id]
            .iter()
            .filter(|h| h.len() == longest)
            .cloned()
            .collect();
        let observed = aggregate(counts, &longest_histories, alphabet.len());
        let mut total = 0u64;
        for (column, &count) in observed.iter().enumerate() {
            if count == 0 {
                continue;
            }
            let targets = &graph[old_id][column];
            if targets.len() != 1 {
                return Err("CSSR has an ambiguous or missing observed transition".into());
            }
            let target = *targets.keys().next().unwrap();
            machine.transitions[new_id][column] = Some(
                *labels
                    .get(&target)
                    .ok_or("CSSR transition leaves its recurrent class")?,
            );
            machine.emissions[new_id][column] = count as f64;
            total += count;
        }
        if total == 0 {
            return Err("CSSR recurrent state has no observed emissions".into());
        }
        for probability in &mut machine.emissions[new_id] {
            *probability /= total as f64;
        }
        machine
            .histories
            .push(states[old_id].iter().cloned().collect());
    }
    if (0..alphabet.len()).any(|column| machine.emissions.iter().all(|row| row[column] == 0.0)) {
        return Err("CSSR recurrent class omits an observed alphabet symbol".into());
    }
    Ok(machine)
}

/// Infer a candidate recursive predictive machine from one training sequence.
/// Statistical choices are fixed at alpha=.01, Bonferroni correction, minimum
/// history count five. Morphs use the largest horizon up to three with
/// alphabet^horizon <= 32, or one symbol for wider alphabets. Single-symbol
/// counts still define transitions and emissions. G-tests use the chi-squared
/// approximation, so callers
/// must validate candidates independently rather than interpret alpha as a
/// finite-sample certificate for dependent, overlapping history counts.
pub(crate) fn infer(data: &[u8], max_history: usize) -> Result<Machine, String> {
    if data.len() < 2 {
        return Err("CSSR needs at least two training symbols".into());
    }
    if max_history > MAX_HISTORY {
        return Err(format!(
            "CSSR maximum history exceeds the {MAX_HISTORY}-symbol resource limit"
        ));
    }
    let alphabet: Vec<u8> = data
        .iter()
        .copied()
        .collect::<BTreeSet<_>>()
        .into_iter()
        .collect();
    let counts = count_histories(data, max_history, &alphabet)?;
    let horizon = predictive_horizon(alphabet.len());
    if data.len() < horizon {
        return Err("CSSR training data are shorter than the predictive horizon".into());
    }
    let predictive = if horizon > 1 {
        Some(count_predictive_words(data, max_history, horizon)?)
    } else {
        None
    };
    let (morph_counts, morph_size) = predictive
        .as_ref()
        .map_or((&counts, alphabet.len()), |(counts, size)| (counts, *size));
    let morphs = Morphs {
        counts: morph_counts,
        size: morph_size,
    };
    let eligible = counts
        .iter()
        .filter(|(history, next)| {
            let length = history.len();
            (length > 0 && length <= max_history && next.iter().sum::<u64>() >= MIN_COUNT)
                || length == max_history + 1
        })
        .count();
    let tests = Tests::new(morph_size, eligible)?;
    let mut states = homogenize(morphs.counts, &alphabet, morph_size, max_history, &tests)?;
    let everything: BTreeSet<usize> = (0..states.len()).collect();
    let initial_graph = edges(
        &states,
        &counts,
        &morphs,
        &everything,
        &alphabet,
        max_history,
        &tests,
    )?;
    let classes = recurrent_classes(&initial_graph, &everything);
    let mut alive: BTreeSet<usize> = classes.into_iter().flatten().collect();
    if alive.is_empty() {
        alive = everything;
    }
    let graph = determinize(
        &mut states,
        &counts,
        &morphs,
        &mut alive,
        &alphabet,
        max_history,
        &tests,
    )?;
    let mut machine = build_machine(
        data,
        &counts,
        &states,
        &graph,
        &alive,
        &alphabet,
        max_history,
    )?;
    machine.predictive_horizon = horizon;
    Ok(machine)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn g_test_matches_a_known_two_row_table() {
        let tests = Tests::new(2, 1).unwrap();
        assert!((tests.critical[1] - 6.6348966010212145).abs() < 1e-10);
        assert!(!tests.different(&[100, 100], &[100, 100]));
        assert!(tests.different(&[1000, 0], &[500, 500]));
    }

    #[test]
    fn constant_source_is_one_state() {
        let machine = infer(&vec![0xff; 1000], 3).unwrap();
        assert_eq!(machine.alphabet, vec![0xff]);
        assert_eq!(machine.transitions, vec![vec![Some(0)]]);
        assert_eq!(machine.emissions, vec![vec![1.0]]);
    }

    #[test]
    fn iid_biased_and_fair_sources_do_not_need_memory() {
        let mut state = 0x1234_5678_9abc_def0u64;
        for threshold in [0.01, 0.1, 0.5] {
            let data: Vec<u8> = (0..50_000)
                .map(|_| {
                    state ^= state << 13;
                    state ^= state >> 7;
                    state ^= state << 17;
                    u8::from((state as f64 / u64::MAX as f64) < threshold)
                })
                .collect();
            let machine = infer(&data, 3).unwrap();
            assert_eq!(machine.transitions.len(), 1, "threshold={threshold}");
        }
    }

    #[test]
    fn periodic_source_has_deterministic_emissions() {
        let data = b"00010111".repeat(4000);
        let machine = infer(&data, 4).unwrap();
        assert_eq!(machine.transitions.len(), 8);
        for row in machine.emissions {
            assert_eq!(row.iter().filter(|&&p| p > 0.0).count(), 1);
        }
    }

    #[test]
    fn nonbinary_symbols_use_dense_alphabet_columns() {
        let data: Vec<u8> = [0, 17, 255].into_iter().cycle().take(15_000).collect();
        let machine = infer(&data, 2).unwrap();
        assert_eq!(machine.alphabet, vec![0, 17, 255]);
        assert_eq!(machine.transitions.len(), 3);
        for (transitions, emissions) in machine.transitions.iter().zip(machine.emissions) {
            assert_eq!(transitions.iter().filter(|t| t.is_some()).count(), 1);
            assert_eq!(emissions.iter().filter(|&&p| p == 1.0).count(), 1);
        }
    }

    #[test]
    fn predictive_words_separate_hidden_states_with_equal_one_step_emissions() {
        // Random-random-XOR has three different predictive states whose
        // immediate emissions are fair. One-symbol morph matching loses phase.
        let mut seed = 0x1234_5678_9abc_def0u64;
        let mut bit = || {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            (seed & 1) as u8
        };
        let mut data = Vec::with_capacity(60_000);
        for _ in 0..20_000 {
            let first = bit();
            let second = bit();
            data.extend_from_slice(&[first, second, first ^ second]);
        }
        for depth in [5, 6] {
            let machine = infer(&data, depth).unwrap();
            assert_eq!(machine.predictive_horizon, 3);
            assert_eq!(machine.transitions.len(), 5, "depth={depth}");
        }
    }

    #[test]
    fn many_distinct_predictive_states_hit_the_resource_limit() {
        let data: Vec<u8> = (0..13_000).map(|i| (i % 65) as u8).collect();
        let error = infer(&data, 1).unwrap_err();
        assert!(error.contains("64-state resource limit"), "{error}");
    }

    #[test]
    fn invalid_and_excessive_requests_fail_explicitly() {
        assert!(infer(b"", 3).is_err());
        assert!(infer(b"a", 3).is_err());
        assert!(infer(b"ab", MAX_HISTORY + 1).is_err());
    }
}
