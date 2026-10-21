# Excess-entropy estimator on this branch

This note describes the revised estimator in PR #14. The
[published website](https://pleros-ai.github.io/lzcomplexity/) describes the
released shuffle estimator, including historical measurements. Those E tables,
timings, RNG explanations and C++ parity results do not apply to this method.
Record the package version and, for an unreleased checkout, the commit used.

## The quantity being estimated

For a stationary finite-alphabet source, excess entropy is the information
shared by the infinite past and future:

```text
E = I(X[-infinity:0]; X[0:infinity]) = I(S+; S-).
```

`S+` groups pasts with the same predictive distribution; `S-` groups futures
with the same retrodictive distribution. Their mutual information equals E.
The entropy of the predictive state alone, `C_mu = H(S+)`, is generally a
different quantity. The estimator does not substitute `C_mu` for E.
See [Ellison, Mahoney and Crutchfield (2009)](https://arxiv.org/abs/0905.3587).

The implementation reconstructs models in both directions with causal-state
splitting reconstruction (CSSR). Suffixes with matching predictive behavior
are grouped, then states are split as needed to make their symbol-labelled
transitions deterministic. A state can carry information through arbitrarily
long observations: a finite reconstruction depth is not a finite Markov-order
assumption. CSSR's consistency results require finitely many causal states
and a finite synchronizing suffix for each state, with a sufficient history
depth and other regularity conditions. Here those requirements apply in
**both** time directions. See [Shalizi and Shalizi (2004), Sections 3–4](https://arxiv.org/abs/cs/0406011).
The paper's asymptotic theorem does not provide an accuracy guarantee for this
capped implementation and its finite-sample model-selection rules.

This implementation extends the next-symbol morph comparison to predictive
words of up to three symbols. This distinguishes states that have identical
one-step emissions but different predictions farther ahead. The horizon is
the largest `h <= 3` with `k**h <= 32`, using the observed training alphabet
size `k`; alphabets larger than 32 retain one-symbol morphs. Count rows use
only observed words. Transitions and emission probabilities still describe
single symbols. This bounded extension is not an unchanged implementation of
the paper's inference procedure.

## Data flow and checks

The input is divided into consecutive, non-overlapping segments:

1. The first 50% trains candidate forward and reverse models.
2. The next 25% selects models using penalized predictive log loss.
3. The final 25% measures the mutual information of synchronized state pairs.

At least 1,024 input bytes are required. Each direction tests depths `1..D`.
On the training path, supported transition probabilities are refitted with
a `1/2` pseudocount per supported edge. A candidate's selection score is
its total natural-log predictive loss plus
`0.5 * (number_of_states + number_of_free_emission_probabilities) * ln(N_select)`.
The best-scoring topology must also occur at another tested depth; an unstable
best candidate is rejected rather than replaced with a worse stable one.

Forward and reverse states are aligned at the same past/future cuts. Only
cuts where both states have synchronized enter their empirical joint counts;
the marginals come from those same counts. E is computed directly from that
joint distribution. Neither the public LZ76 rate `h` nor the entropy of either
state distribution is subtracted from another estimator.

Selection tests multiple history depths, requires a stable topology across
at least two tested depths, rejects unsupported held-out transitions, and
requires at least 99% synchronization on the training and selection paths
and 99% joint synchronization on the evaluation segment. Evaluation also
requires at least `32 * number_of_forward_states * number_of_reverse_states`
synchronized cuts. This is a sample-budget safeguard against a large state
grid; it is not an error bound.
These are practical acceptance criteria. Consecutive segments of a time series
need not be statistically independent, and passing these criteria does not
prove that the inferred models are the source's true causal states.

This E path uses no random shuffle, Chao–Shen coverage correction, peak
selection, or isotonic projection. Shuffle-distance metrics retain their
separate surrogate calculation.

## Python API and migration

- `max_history` is the maximum CSSR suffix-history depth to test, from 2 to 16.
  `None` selects `floor(log_k(N_train / 32)) - 1`, clamped to the range 3–8,
  where `k` is the number of distinct training symbols, floored at 2. The cap
  limits reconstruction work; it is not a cutoff on the memory represented
  by the inferred states. This default is a resource heuristic.
- A positive `max_block_size` is a **deprecated alias** for `max_history`.
  If both positive values are supplied and disagree, Python raises `ValueError`.
  This changes the argument's meaning from the released shuffle estimator.
- `log_base` changes units. Its default is the effective alphabet size;
  `log_base=2` gives E in **bits** and `h` in **bits per byte**. Values below
  2 are treated as 2. Changing units does not change model reconstruction.
- `alphabet` controls the effective alphabet for default units. It defaults
  to the number of distinct input bytes, floored at 2. CSSR works on the
  observed byte symbols; a declared alphabet does not invent unseen data.

`emc(...)` returns `(value, summands)` when its checks pass. Otherwise it raises
`ValueError`, including on inputs too short for reconstruction. A short input
is not reported as a successfully estimated zero. To retain `h` and the other
LZ76 results while inspecting an unresolved E, use `lz76(...)`:

```python
import lzcomplexity as lz

analysis = lz.lz76(b"01" * 4096, max_history=6, log_base=2)
result = analysis["emc"]
if result["value"] is None:
    print(result["status"], result["message"])
else:
    print(result["value"], result["diagnostics"])
```

The nested E result includes:

- `value`: the estimated state-pair mutual information, or `None`.
- `method`: `bidirectional_cssr`, identifying the estimator used.
- `status`: `estimated` when the checks pass; `insufficient_data` or
  `unresolved` when they fail. `message` explains the result.
- `diagnostics`: model sizes, selected histories, tested history cap,
  predictive-word horizon, topology-stability counts, segment sizes,
  synchronized sample count and joint synchronization fraction. Inspect
  these with the estimate.
- `summands`: one contribution per inferred forward state,
  `P_hat(s+) * KL(P_hat(S- | s+) || P_hat(S-))`. Terms are non-negative up to
  roundoff and sum to `value`. Their meaning has changed from the old
  block-length terms `h(m)-h`.
- `max_block_size`: a legacy field recording the larger selected history depth.
- `multi_information`: the separate legacy diagnostic `H_plugin(1) - h_LZ`
  over the complete input. It can be negative and does not enter E.

Python requests the terms automatically. Rust callers must enable
`LzArgs::get_shuffle_terms` to obtain them; `LzArgs::max_context` is the preferred
history cap. Historical Rust names such as `LzShuffle` and
`lz76_random_shuffle_complexity`, and CLI JSON keys such as
`lz76RandomShuffleComplexity`, remain for compatibility. Their names no longer
describe the E algorithm. CLI unresolved values serialize as JSON `null`.

## What the estimate can support

The target is full excess entropy, not a finite-window mutual information
renamed as E. Recovering it from finite observations is conditional on model
identification. The method is suited to stationary sources with finite,
synchronizable causal-state representations in both directions. A finite
forward model alone does not establish the reverse-model assumption.

Finite data can miss rare states or long distinguishing histories, and the
empirical state-pair mutual information has sampling bias. Synchronization
coverage describes the fitted models; it does not certify their correctness.
Some misspecified models can pass the checks. In particular, a repeated
binary de Bruijn cycle of order `r` with random phase has true E of `r` bits,
but its word frequencies through length `r` match a fair coin. A cap below
the needed distinguishing depth can therefore miss this structure. No finite
data diagnostic supplies a universal accuracy certificate for unrestricted
sources.

An unresolved status means that this run does not support a numeric E; it is
not evidence that the source has zero memory. More data, a justified larger
history cap, or a source-specific model may be needed. For successful fits,
compare matched lengths, symbol encodings, resource caps and estimator
versions. The API supplies no calibrated confidence interval.

Validation must compare against **full-E** controls, including processes
with hidden-state memory and cases where `C_mu != E`. The repository's
`scripts/validate_emc.py` defines analytic controls and checks reconstruction
status as well as numerical error. A passing unit test or one source family
does not establish accuracy for an unknown process.

For example, the binary golden-mean Markov chain with `P(1|0)=1/2` and
`P(0|1)=1` has stationary probabilities `(2/3, 1/3)`. Its state entropy is
`C_mu = H_2(1/3) = 0.918296` bits, while its entropy rate is `2/3` bits per
symbol and its full excess entropy is `E = H_2(1/3) - 2/3 = 0.251629` bits.
Returning state entropy for this source would overestimate E by `2/3` bit.

The additional `lz76()` fields `factors_stddev`, `normal_error`, and
`poison_error` are legacy formulas, not uncertainty on E or calibrated error
bars on `h`. The `extras` fields are ratios of LZ factor counts from the two
sequence halves, not Shannon information or statistical correlations.

## Why the method changed

Released 2.0.0 built block-shuffled surrogates and estimated their entropy
rates with LZ76. The initial PR replaced that step with Chao–Shen block
entropies, subtracted `m * h_LZ`, and truncated the ladder at a peak. Combining
estimators with different finite-sample biases and selecting a peak did not
establish an accurate full-E estimator.

A direct finite-window mutual-information revision was also evaluated. It
can estimate a useful lower-dimensional quantity, but short windows can
miss most of a hidden-state source's full excess entropy. This branch instead
uses the causal-state identity to target full E and reports failure when its
reconstruction checks do not support a number. Values and summands from
these different estimators must be labelled and analysed separately.

Before a matching website deployment, update the documentation branch's
method descriptions, API details, examples and performance measurements
together. Until then, use this note and runtime help for this branch, and
versioned website material for the corresponding release.
