//! Held-out bidirectional predictive-state estimate of full excess entropy.
//!
//! The identity E = I(S+; S-) concerns causal states, not finite word windows
//! and not H(S+). CSSR supplies candidate machines; independent prediction,
//! topology stability, and synchronization checks can reject a fit. These
//! checks do not prove that a source has finitely many causal states.

use std::collections::BTreeMap;

use crate::block_entropy::state_information_bits;
use crate::causal_states::{infer, Machine};
use crate::lz76::{EmcDiagnostics, LzArgs, LzShuffle};

const MIN_SAMPLES: usize = 1024;
const MIN_SYNCHRONIZATION: f64 = 0.99;

struct Candidate {
    machine: Machine,
    depth: usize,
    score: f64,
    topology: Vec<usize>,
}

fn columns(machine: &Machine) -> [Option<usize>; 256] {
    let mut columns = [None; 256];
    for (column, &symbol) in machine.alphabet.iter().enumerate() {
        columns[symbol as usize] = Some(column);
    }
    columns
}

/// State at each cut, starting with no knowledge of the initial state. A
/// synchronizing word collapses this set to one state; recursive transitions
/// then retain memory even when that word is far outside the history cap.
fn trace(machine: &Machine, data: &[u8]) -> Result<Vec<Option<usize>>, String> {
    let columns = columns(machine);
    let mut possible = vec![true; machine.transitions.len()];
    let mut labels = Vec::with_capacity(data.len() + 1);
    for cut in 0..=data.len() {
        let mut active = possible.iter().enumerate().filter(|(_, p)| **p);
        let first = active.next().map(|(state, _)| state);
        if first.is_none() {
            return Err("observed sequence contains an unsupported transition".into());
        }
        labels.push(if active.next().is_none() { first } else { None });
        if cut < data.len() {
            let column = columns[data[cut] as usize]
                .ok_or("held-out sequence contains an untrained symbol")?;
            let mut next = vec![false; possible.len()];
            for (state, &present) in possible.iter().enumerate() {
                if present {
                    if let Some(target) = machine.transitions[state][column] {
                        next[target] = true;
                    }
                }
            }
            possible = next;
        }
    }
    Ok(labels)
}

fn refit(machine: &mut Machine, data: &[u8]) -> Result<(), String> {
    let labels = trace(machine, data)?;
    let synchronized = labels.iter().filter(|state| state.is_some()).count();
    if synchronized as f64 / ((data.len() + 1) as f64) < MIN_SYNCHRONIZATION {
        return Err("training path does not synchronize sufficiently".into());
    }
    let columns = columns(machine);
    let mut counts = vec![vec![0usize; machine.alphabet.len()]; machine.transitions.len()];
    for (&symbol, state) in data.iter().zip(labels) {
        if let Some(state) = state {
            counts[state][columns[symbol as usize].unwrap()] += 1;
        }
    }
    for (state, row) in counts.iter().enumerate() {
        let total = row.iter().sum::<usize>();
        if total == 0 {
            return Err("inferred recurrent state was never visited after synchronization".into());
        }
        let edges = machine.transitions[state]
            .iter()
            .filter(|edge| edge.is_some())
            .count();
        for (symbol, &count) in row.iter().enumerate() {
            machine.emissions[state][symbol] = if machine.transitions[state][symbol].is_some() {
                (count as f64 + 0.5) / (total as f64 + 0.5 * edges as f64)
            } else {
                0.0
            };
        }
    }
    Ok(())
}

fn log_loss(machine: &Machine, data: &[u8]) -> Result<f64, String> {
    let columns = columns(machine);
    let states = machine.transitions.len();
    let mut belief = vec![1.0 / states as f64; states];
    let mut loss = 0.0;
    for &symbol in data {
        let column =
            columns[symbol as usize].ok_or("held-out sequence contains an untrained symbol")?;
        let mut next = vec![0.0; states];
        for (state, &mass) in belief.iter().enumerate() {
            if let Some(target) = machine.transitions[state][column] {
                next[target] += mass * machine.emissions[state][column];
            }
        }
        let probability = next.iter().sum::<f64>();
        if probability <= 0.0 || !probability.is_finite() {
            return Err("held-out sequence contains an unsupported transition".into());
        }
        loss -= probability.ln();
        for p in &mut next {
            *p /= probability;
        }
        belief = next;
    }
    Ok(loss)
}

/// Canonical deterministic graph serialization, independent of state numbering.
fn topology(machine: &Machine) -> Vec<usize> {
    let size = machine.transitions.len();
    (0..size)
        .map(|start| {
            let mut ids = vec![usize::MAX; size];
            let mut queue = vec![start];
            ids[start] = 0;
            let mut result = vec![size, machine.alphabet.len()];
            let mut position = 0;
            while position < queue.len() {
                let state = queue[position];
                for edge in &machine.transitions[state] {
                    if let Some(target) = *edge {
                        if ids[target] == usize::MAX {
                            ids[target] = queue.len();
                            queue.push(target);
                        }
                        result.push(ids[target]);
                    } else {
                        result.push(usize::MAX);
                    }
                }
                position += 1;
            }
            result
        })
        .min()
        .unwrap_or_default()
}

fn select(training: &[u8], selection: &[u8], cap: usize) -> Result<(Candidate, usize), String> {
    let mut candidates = Vec::new();
    let mut errors = Vec::new();
    for depth in 1..=cap {
        let candidate = (|| {
            let mut machine = infer(training, depth)?;
            refit(&mut machine, training)?;
            let labels = trace(&machine, selection)?;
            let synchronized = labels.iter().filter(|state| state.is_some()).count();
            if synchronized as f64 / (labels.len() as f64) < MIN_SYNCHRONIZATION {
                return Err("selection path does not synchronize sufficiently".to_string());
            }
            let parameters = machine.transitions.len()
                + machine
                    .transitions
                    .iter()
                    .map(|row| {
                        row.iter()
                            .filter(|edge| edge.is_some())
                            .count()
                            .saturating_sub(1)
                    })
                    .sum::<usize>();
            let score = log_loss(&machine, selection)?
                + 0.5 * parameters as f64 * (selection.len() as f64).ln();
            Ok(Candidate {
                topology: topology(&machine),
                machine,
                depth,
                score,
            })
        })();
        match candidate {
            Ok(candidate) => candidates.push(candidate),
            Err(error) => errors.push(format!("depth {depth}: {error}")),
        }
    }
    let best = candidates
        .iter()
        .enumerate()
        .min_by(|(_, a), (_, b)| a.score.total_cmp(&b.score))
        .map(|(index, _)| index)
        .ok_or_else(|| format!("no supported predictive model ({})", errors.join("; ")))?;
    let stable = candidates
        .iter()
        .filter(|candidate| candidate.topology == candidates[best].topology)
        .count();
    if stable < 2 {
        return Err(
            "best held-out predictive topology is not stable across two history depths".into(),
        );
    }
    Ok((candidates.swap_remove(best), stable))
}

fn unresolved(status: &'static str, message: String) -> LzShuffle {
    LzShuffle {
        status,
        message: Some(message),
        ..LzShuffle::default()
    }
}

pub(crate) fn estimate(data: &[u8], args: &LzArgs) -> LzShuffle {
    if data.len() < MIN_SAMPLES {
        return unresolved("insufficient_data", format!("at least {MIN_SAMPLES} symbols are required for independent training, selection, and evaluation"));
    }
    let requested = if args.max_context > 0 {
        args.max_context
    } else {
        args.block_size
    };
    if (requested > 0 && !(2..=16).contains(&requested))
        || (args.max_context > 0 && args.block_size > 0 && args.max_context != args.block_size)
    {
        return unresolved(
            "unresolved",
            "history cap must be 2..=16; explicit aliases must agree".into(),
        );
    }
    let training_samples = data.len() / 2;
    let selection_samples = data.len() / 4;
    let mut observed = [false; 256];
    for &symbol in &data[..training_samples] {
        observed[symbol as usize] = true;
    }
    let alphabet = observed.iter().filter(|&&present| present).count().max(2) as f64;
    let cap = if requested > 0 {
        requested as usize
    } else {
        // Bound candidate contexts by observed sample support, independently of
        // logarithm output units. This is a resource heuristic, not a proof.
        (((training_samples as f64 / 32.0).ln() / alphabet.ln()).floor() as usize)
            .saturating_sub(1)
            .clamp(3, 8)
    };
    let training = &data[..training_samples];
    let selection = &data[training_samples..training_samples + selection_samples];
    let evaluation = &data[training_samples + selection_samples..];
    let reverse_training: Vec<u8> = training.iter().rev().copied().collect();
    let reverse_selection: Vec<u8> = selection.iter().rev().copied().collect();
    let (forward, reverse) = rayon::join(
        || select(training, selection, cap),
        || select(&reverse_training, &reverse_selection, cap),
    );
    let result = (|| {
        let (forward, forward_stable_depths) =
            forward.map_err(|message| format!("forward: {message}"))?;
        let (reverse, reverse_stable_depths) =
            reverse.map_err(|message| format!("reverse: {message}"))?;
        let reverse_evaluation: Vec<u8> = evaluation.iter().rev().copied().collect();
        let forward_labels = trace(&forward.machine, evaluation)?;
        let reverse_labels = trace(&reverse.machine, &reverse_evaluation)?;
        let mut counts = BTreeMap::new();
        let mut synchronized_samples = 0;
        for (past, future) in forward_labels.iter().zip(reverse_labels.iter().rev()) {
            if let (Some(past), Some(future)) = (past, future) {
                *counts.entry((*past, *future)).or_insert(0usize) += 1;
                synchronized_samples += 1;
            }
        }
        let synchronization_fraction = synchronized_samples as f64 / forward_labels.len() as f64;
        if synchronization_fraction < MIN_SYNCHRONIZATION {
            return Err(format!(
                "only {:.2}% of evaluation cuts synchronize in both directions; 99% required",
                100.0 * synchronization_fraction
            ));
        }
        let state_pairs = forward.machine.transitions.len() * reverse.machine.transitions.len();
        if synchronized_samples < 32 * state_pairs {
            return Err(format!("evaluation sample too small for {state_pairs} state pairs; at least 32 synchronized cuts per possible pair required"));
        }
        let terms = state_information_bits(
            &counts,
            forward.machine.transitions.len(),
            reverse.machine.transitions.len(),
        );
        let mut estimate = LzShuffle::new(
            forward.depth.max(reverse.depth) as i32,
            terms.iter().sum(),
            0.0,
        );
        if args.get_shuffle_terms {
            estimate.summands = terms;
        }
        estimate.diagnostics = Some(EmcDiagnostics {
            forward_states: forward.machine.transitions.len(),
            reverse_states: reverse.machine.transitions.len(),
            forward_history: forward.depth,
            reverse_history: reverse.depth,
            forward_stable_depths,
            reverse_stable_depths,
            max_history: cap,
            predictive_horizon: forward.machine.predictive_horizon,
            training_samples,
            selection_samples,
            evaluation_samples: evaluation.len(),
            synchronized_samples,
            synchronization_fraction,
        });
        Ok(estimate)
    })();
    result.unwrap_or_else(|message| unresolved("unresolved", message))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn uniform(seed: &mut u64) -> f64 {
        *seed ^= *seed << 13;
        *seed ^= *seed >> 7;
        *seed ^= *seed << 17;
        (*seed >> 11) as f64 / ((1u64 << 53) as f64)
    }

    #[test]
    fn high_gamma_even_process_recovers_full_entropy() {
        let mut seed = 12345;
        let mut parity = false;
        let data: Vec<u8> = (0..100_000)
            .map(|_| {
                if parity {
                    parity = false;
                    1
                } else if uniform(&mut seed) < 0.9 {
                    parity = true;
                    1
                } else {
                    0
                }
            })
            .collect();
        let result = estimate(&data, &LzArgs::new());
        assert_eq!(result.status, "estimated", "{:?}", result.message);
        assert!((result.emc_value.unwrap() - 0.9980008838722996).abs() < 0.01);
        let diagnostics = result.diagnostics.unwrap();
        assert_eq!(
            (diagnostics.forward_states, diagnostics.reverse_states),
            (2, 2)
        );
    }

    #[test]
    fn golden_mean_recovers_shared_information_not_state_entropy() {
        let mut seed = 6789;
        let mut previous = false;
        let data: Vec<u8> = (0..100_000)
            .map(|_| {
                previous = !previous && uniform(&mut seed) < 0.5;
                u8::from(previous)
            })
            .collect();
        let result = estimate(&data, &LzArgs::new());
        assert_eq!(result.status, "estimated", "{:?}", result.message);
        assert!((result.emc_value.unwrap() - 0.2516291673878229).abs() < 0.02);
    }

    #[test]
    fn unsupported_or_unstable_models_do_not_get_a_fallback() {
        let data: Vec<u8> = b"00010111".iter().copied().cycle().take(8192).collect();
        let mut args = LzArgs::new();
        args.max_context = 2;
        let result = estimate(&data, &args);
        assert_eq!(result.status, "unresolved");
        assert_eq!(result.emc_value, None);
        assert!(result.message.is_some());
        args.max_context = 6;
        args.block_size = 8;
        assert_eq!(estimate(&data, &args).emc_value, None);
    }
}
