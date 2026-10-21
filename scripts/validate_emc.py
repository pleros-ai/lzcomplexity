#!/usr/bin/env python3
"""Validate full excess entropy against independent, infinite-past/future truths.

Install the current wheel, then run::

    python scripts/validate_emc.py --self-test
    python scripts/validate_emc.py --length 100000 --seeds 1 --check --allow-unresolved "RRXOR fair"
    python scripts/validate_emc.py --length 1000000 --seeds 3 --check --json

Only the installed extension and Python's standard library are needed. All
values use bits. Seeds are 0..seeds-1, and generators start stationary. Truths
are population infinite-past/future E, never the information of a short window
or the entropy C_mu of a predictive model's states. The separate finite-window
oracle tests demonstrate why those substitutes are inadequate.

The tolerance is a regression budget for each seed and its mean, not a confidence bound or
an accuracy guarantee for arbitrary data. No source truth or source label is
passed to the estimator. Unresolved fits must remain explicit; a finite-window
fallback, missing status, NaN, or an unsupported fit is not a passing full-E
result. Optional model settings can be supplied as JSON with --model-options.
--allow-unresolved explicitly declares a known limitation; these cases are
reported as unresolved, never as numerical accuracy passes. Any numeric value
for an allowed source is still checked against its true E.
"""

from __future__ import annotations

import argparse
import collections
import functools
import json
import math
import random
import statistics
import sys
import time
import unittest
from dataclasses import dataclass
from typing import Callable


def binary_entropy(p: float) -> float:
    if not 0.0 <= p <= 1.0:
        raise ValueError("probability must be in [0, 1]")
    if p in (0.0, 1.0):
        return 0.0
    return -p * math.log2(p) - (1.0 - p) * math.log2(1.0 - p)


def entropy(probabilities: list[float]) -> float:
    return -math.fsum(p * math.log2(p) for p in probabilities if p > 0.0)


def finite_information(block_entropies: tuple[float, ...], width: int) -> float:
    left = width // 2
    right = width - left
    return block_entropies[left] + block_entropies[right] - block_entropies[width]


def iid_sequence(length: int, p: float, seed: int) -> bytes:
    rng = random.Random(seed)
    return bytes(49 if rng.random() < p else 48 for _ in range(length))


def markov_sequence(length: int, a: float, b: float, seed: int) -> bytes:
    """Binary chain: a=P(1|0), b=P(0|1), initialized at equilibrium."""
    rng = random.Random(seed)
    state = int(rng.random() < a / (a + b))
    out = bytearray(length)
    for i in range(length):
        out[i] = 48 + state
        if rng.random() < (b if state else a):
            state = 1 - state
    return bytes(out)


def markov_truth(a: float, b: float, width: int) -> tuple[float, float]:
    pi_one = a / (a + b)
    rate = (1.0 - pi_one) * binary_entropy(a) + pi_one * binary_entropy(b)
    excess = binary_entropy(pi_one) - rate
    # H(m)=H(1)+(m-1)h for m>=1. Every nonempty adjacent pair of
    # windows already shares exactly the first-order Markov excess entropy.
    return (excess if width >= 2 else 0.0), excess


def even_sequence(length: int, gamma: float, seed: int) -> bytes:
    """Even process: A --0,1-gamma--> A; A --1,gamma--> B; B --1--> A."""
    rng = random.Random(seed)
    state = int(rng.random() < gamma / (1.0 + gamma))
    out = bytearray(length)
    for i in range(length):
        if state:
            out[i] = 49
            state = 0
        elif rng.random() < gamma:
            out[i] = 49
            state = 1
        else:
            out[i] = 48
    return bytes(out)


@functools.lru_cache(maxsize=None)
def even_block_entropies(gamma: float, width: int) -> tuple[float, ...]:
    """Exact H(0)..H(width), summing probabilities of all possible HMM words.

    A node holds the probabilities of one output word ending in A and B,
    already summed over the two possible initial states. Zeros synchronize
    the hidden state, while runs of ones retain both possible initial phases.
    Enumeration is deliberately limited; this is a validation oracle, not an
    entropy estimator for unknown sources.
    """
    if not 0.0 < gamma < 1.0:
        raise ValueError("the even-process oracle requires 0 < gamma < 1")
    if width > 24:
        raise ValueError("exact even-process validation supports widths <= 24")
    nodes = [(1.0 / (1.0 + gamma), gamma / (1.0 + gamma))]
    result = [0.0]
    for _ in range(width):
        following = []
        for p_a, p_b in nodes:
            zero = p_a * (1.0 - gamma)
            if zero > 0.0:
                following.append((zero, 0.0))
            # On a 1: B goes to A; A goes to B.
            if p_b + p_a * gamma > 0.0:
                following.append((p_b, p_a * gamma))
        nodes = following
        probabilities = [p_a + p_b for p_a, p_b in nodes]
        if not math.isclose(math.fsum(probabilities), 1.0, abs_tol=1e-12):
            raise AssertionError("exact HMM word probabilities do not sum to 1")
        result.append(entropy(probabilities))
    return tuple(result)


def even_truth(gamma: float, width: int) -> tuple[float, float]:
    # Both semi-infinite past and future determine the hidden parity state
    # almost surely. They are conditionally independent given that state.
    infinite = binary_entropy(gamma / (1.0 + gamma))
    return finite_information(even_block_entropies(gamma, width), width), infinite


def periodic_sequence(length: int, motif: bytes, seed: int) -> bytes:
    phase = random.Random(seed).randrange(len(motif))
    repeated = motif * ((length + phase + len(motif) - 1) // len(motif))
    return repeated[phase : phase + length]


def periodic_truth(motif: bytes, width: int) -> tuple[float, float]:
    period = next(
        p
        for p in range(1, len(motif) + 1)
        if len(motif) % p == 0 and motif == motif[:p] * (len(motif) // p)
    )
    motif = motif[:period]
    repeated = motif * ((width + period - 1) // period + 1)
    entropies = [0.0]
    for size in range(1, width + 1):
        counts = collections.Counter(repeated[i : i + size] for i in range(period))
        entropies.append(entropy([n / period for n in counts.values()]))
    return finite_information(tuple(entropies), width), math.log2(period)


ORDER2_PROBABILITIES = (0.1, 0.7, 0.4, 0.9)


def order2_stationary() -> list[float]:
    probabilities = [0.25] * 4
    for _ in range(10_000):
        following = [0.0] * 4
        for state, p_one in enumerate(ORDER2_PROBABILITIES):
            following[2 * (state % 2)] += probabilities[state] * (1.0 - p_one)
            following[2 * (state % 2) + 1] += probabilities[state] * p_one
        if max(abs(a - b) for a, b in zip(probabilities, following)) < 1e-15:
            return following
        probabilities = following
    raise AssertionError("order-two stationary distribution did not converge")


def order2_sequence(length: int, seed: int) -> bytes:
    rng = random.Random(seed)
    probabilities = order2_stationary()
    residual = rng.random()
    state = 0
    while residual >= probabilities[state]:
        residual -= probabilities[state]
        state += 1
    out = bytearray(length)
    for i in range(length):
        symbol = int(rng.random() < ORDER2_PROBABILITIES[state])
        out[i] = 48 + symbol
        state = 2 * (state % 2) + symbol
    return bytes(out)


def order2_excess_entropy() -> float:
    probabilities = order2_stationary()
    rate = math.fsum(p * binary_entropy(q) for p, q in zip(probabilities, ORDER2_PROBABILITIES))
    # An order-R Markov source has E=H(R)-R*h; C_mu=H(R) here.
    return entropy(probabilities) - 2.0 * rate


def rrxor_sequence(length: int, seed: int) -> bytes:
    """Stationary fair Random-Random-XOR triples, with a uniformly random phase.

    Process definition: Riechers and Crutchfield (2018), Sec. VII,
    https://arxiv.org/abs/1706.00883 . Both infinite halves identify the phase.
    Conditional on it, a cut inside a triple shares one parity bit, while a
    cut between triples shares none. Therefore E=log2(3)+2/3, not a short-window
    approximation. This is derived independently of the fitted model.
    """
    rng = random.Random(seed)
    phase = rng.randrange(3)
    out = bytearray()
    while len(out) < length + phase:
        first, second = rng.randrange(2), rng.randrange(2)
        out.extend((48 + first, 48 + second, 48 + (first ^ second)))
    return bytes(out[phase : phase + length])


@dataclass(frozen=True)
class Source:
    name: str
    generate: Callable[[int, int], bytes]
    infinite_excess_entropy: float
    statistical_complexity: float | None = None


def sources() -> list[Source]:
    result = []
    for p in (0.01, 0.1, 0.5):
        result.append(Source(f"iid p={p:g}", lambda n, seed, p=p: iid_sequence(n, p, seed), 0.0))
    for a, b in ((0.1, 0.4), (0.05, 0.05), (0.3, 0.4), (1.0, 0.5)):
        result.append(
            Source(
                "golden mean" if a == 1.0 else f"Markov a={a:g},b={b:g}",
                lambda n, seed, a=a, b=b: markov_sequence(n, a, b, seed),
                markov_truth(a, b, 2)[1],
                binary_entropy(a / (a + b)),
            )
        )
    result.append(Source("order-2 Markov", order2_sequence, order2_excess_entropy(), entropy(order2_stationary())))
    for gamma in (0.1, 0.5, 0.9):
        state_entropy = binary_entropy(gamma / (1.0 + gamma))
        result.append(
            Source(
                f"even gamma={gamma:g}",
                lambda n, seed, gamma=gamma: even_sequence(n, gamma, seed),
                state_entropy,
                state_entropy,
            )
        )
    result.append(Source("period 8", lambda n, seed: periodic_sequence(n, b"00010111", seed), 3.0, 3.0))
    result.append(Source("RRXOR fair", rrxor_sequence, math.log2(3.0) + 2.0 / 3.0, math.log2(3.0) + 2.0 / 3.0))
    return result


class ExactSourceTests(unittest.TestCase):
    def test_binary_entropy(self) -> None:
        self.assertEqual(binary_entropy(0.0), 0.0)
        self.assertEqual(binary_entropy(1.0), 0.0)
        self.assertEqual(binary_entropy(0.5), 1.0)

    def test_markov_iid_special_case(self) -> None:
        for p in (0.01, 0.1, 0.5):
            finite, infinite = markov_truth(p, 1.0 - p, 12)
            self.assertAlmostEqual(finite, 0.0, places=14)
            self.assertAlmostEqual(infinite, 0.0, places=14)

    def test_markov_closed_form(self) -> None:
        expected = 1.0 - binary_entropy(0.05)
        self.assertAlmostEqual(markov_truth(0.05, 0.05, 2)[0], expected)
        self.assertAlmostEqual(markov_truth(0.05, 0.05, 15)[0], expected)
        self.assertEqual(markov_truth(0.05, 0.05, 1)[0], 0.0)

    def test_even_one_and_two_symbol_probabilities(self) -> None:
        gamma = 0.5
        h = even_block_entropies(gamma, 2)
        self.assertAlmostEqual(h[1], binary_entropy(2.0 * gamma / (1.0 + gamma)))
        # For gamma=.5: P(00)=P(01)=P(10)=1/6, P(11)=1/2.
        self.assertAlmostEqual(h[2], entropy([1 / 6, 1 / 6, 1 / 6, 1 / 2]))

    def test_even_finite_resolution(self) -> None:
        targets = [even_truth(0.5, width)[0] for width in range(1, 17)]
        self.assertTrue(all(a <= b + 1e-12 for a, b in zip(targets, targets[1:])))
        self.assertAlmostEqual(even_truth(0.5, 12)[0], 0.7030702479479594)
        finite, infinite = even_truth(0.9, 12)
        self.assertAlmostEqual(finite, 0.07329418291246448)
        self.assertAlmostEqual(infinite, 0.9980008838722996)
        self.assertLess(finite, 0.1 * infinite)

    def test_periodic_phase_entropy(self) -> None:
        self.assertEqual(periodic_truth(b"0", 12), (0.0, 0.0))
        self.assertEqual(periodic_truth(b"01", 2), (1.0, 1.0))
        self.assertAlmostEqual(periodic_truth(b"00010111", 12)[0], 3.0)
        self.assertEqual(periodic_truth(b"0101", 4)[1], 1.0)

    def test_finite_windows_cannot_identify_infinite_memory(self) -> None:
        # A primitive order-4 de Bruijn cycle visits all 4-bit words equally.
        # At M=4 its adjacent halves are independent, but knowing the infinite
        # past or future identifies one of 16 deterministic phases.
        finite, infinite = periodic_truth(b"0000100110101111", 4)
        self.assertEqual(finite, 0.0)
        self.assertEqual(infinite, 4.0)


    def test_golden_mean_does_not_substitute_state_entropy(self) -> None:
        self.assertAlmostEqual(markov_truth(1.0, 0.5, 2)[1], 0.2516291673878229)
        self.assertAlmostEqual(binary_entropy(2.0 / 3.0), 0.9182958340544896)

    def test_order_two_markov_truth(self) -> None:
        probabilities = order2_stationary()
        self.assertAlmostEqual(math.fsum(probabilities), 1.0)
        self.assertAlmostEqual(order2_excess_entropy(), 0.5029154036470376)
        self.assertGreater(entropy(probabilities), order2_excess_entropy())

    def test_rrxor_generator_and_truth(self) -> None:
        self.assertAlmostEqual(math.log2(3.0) + 2.0 / 3.0, 2.2516291673878226)
        # Five minimal predictive states: one boundary state and two states
        # in each interior phase. Here C_mu equals E, unlike golden mean.
        self.assertAlmostEqual(entropy([1.0 / 3.0] + [1.0 / 6.0] * 4), math.log2(3.0) + 2.0 / 3.0)
        data = rrxor_sequence(30_000, 1)
        self.assertTrue(any(
            all((data[i] - 48) ^ (data[i + 1] - 48) == data[i + 2] - 48
                for i in range(phase, len(data) - 2, 3))
            for phase in range(3)
        ))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--length", type=int, default=100_000)
    parser.add_argument("--seeds", type=int, default=3, help="number of seeds, starting at 0")
    parser.add_argument("--model-options", default="{}", help="JSON keyword arguments for lz76")
    parser.add_argument(
        "--allow-unresolved", action="append", default=[], choices=[source.name for source in sources()],
        help="declare one known unsupported source; repeat for additional sources",
    )
    parser.add_argument("--check", action="store_true", help="require an estimated full E and enforce error budgets")
    parser.add_argument("--tolerance", type=float, default=0.05, help="per-seed and mean infinite-E error budget, bits")
    parser.add_argument("--json", action="store_true", help="emit settings and rows as JSON")
    parser.add_argument("--self-test", action="store_true", help="test the independent source oracles")
    args = parser.parse_args(argv)
    if args.self_test:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(ExactSourceTests)
        return 0 if unittest.TextTestRunner().run(suite).wasSuccessful() else 1
    if args.length < 2 or args.seeds < 1 or not math.isfinite(args.tolerance) or args.tolerance < 0:
        parser.error("length >= 2, seeds >= 1, and a finite nonnegative tolerance are required")
    try:
        options = json.loads(args.model_options)
    except json.JSONDecodeError as exc:
        parser.error(f"model-options must be JSON: {exc}")
    if not isinstance(options, dict):
        parser.error("model-options must be a JSON object")
    options["log_base"] = 2

    import lzcomplexity as lz

    started = time.perf_counter()
    rows = []
    failures = []
    allowed_unresolved = set(args.allow_unresolved)
    for source in sources():
        estimates = []
        diagnostics = []
        unresolved = []
        for seed in range(args.seeds):
            try:
                result = lz.lz76(source.generate(args.length, seed), **options)["emc"]
            except Exception as exc:
                failures.append(f"{source.name}, seed={seed}: {type(exc).__name__}: {exc}")
                continue
            diagnostics.append(result)
            status = result.get("status")
            value = result.get("value")
            if value is None:
                message = result.get("message")
                valid_failure = (status in ("unresolved", "insufficient_data")
                                 and isinstance(message, str) and bool(message.strip()))
                if status == "insufficient_data" and args.length >= 1024:
                    valid_failure = False
                unresolved.append({"seed": seed, "status": status, "message": message})
                if not valid_failure:
                    failures.append(f"{source.name}, seed={seed}: invalid unresolved status/message")
                if source.name not in allowed_unresolved:
                    failures.append(f"{source.name}, seed={seed}: unresolved, status={status!r}")
                continue
            if not math.isfinite(value) or value < 0:
                failures.append(f"{source.name}, seed={seed}: invalid full-E value {value!r}")
                continue
            if args.check and status != "estimated":
                failures.append(f"{source.name}, seed={seed}: full-E status is {status!r}, not estimated")
            if status == "estimated":
                detail = result.get("diagnostics")
                required = {
                    "forward_states", "reverse_states", "forward_history", "reverse_history",
                    "forward_stable_depths", "reverse_stable_depths", "max_history", "predictive_horizon",
                    "training_samples", "selection_samples", "evaluation_samples",
                    "synchronized_samples", "synchronization_fraction",
                }
                if not isinstance(detail, dict) or not required.issubset(detail):
                    failures.append(f"{source.name}, seed={seed}: missing model diagnostics")
                else:
                    state_count = min(detail["forward_states"], detail["reverse_states"])
                    evaluation = detail["evaluation_samples"]
                    # A segment of N symbols has N+1 past/future cuts.
                    evaluation_cuts = evaluation + 1
                    synchronized = detail["synchronized_samples"]
                    fraction = detail["synchronization_fraction"]
                    if state_count < 1 or value > math.log2(max(1, state_count)) + 1e-9:
                        failures.append(f"{source.name}, seed={seed}: E exceeds the fitted state-information bound")
                    if (not 0 < synchronized <= evaluation_cuts or not 0.99 <= fraction <= 1.0
                            or not math.isclose(fraction, synchronized / evaluation_cuts, abs_tol=1e-12)):
                        failures.append(f"{source.name}, seed={seed}: inconsistent synchronization diagnostics")
                    if not (2 <= detail["max_history"] <= 16
                            and 1 <= detail["predictive_horizon"] <= 3
                            and 0 <= detail["forward_history"] <= detail["max_history"]
                            and 0 <= detail["reverse_history"] <= detail["max_history"]):
                        failures.append(f"{source.name}, seed={seed}: invalid history diagnostics")
            method = str(result.get("method", ""))
            if "finite" in method.lower() or "shuffle" in method.lower():
                failures.append(f"{source.name}, seed={seed}: {method!r} is a surrogate target")
            summands = result.get("summands", [])
            if any(not math.isfinite(term) or term < 0 for term in summands):
                failures.append(f"{source.name}, seed={seed}: invalid increment")
            if summands and not math.isclose(math.fsum(summands), value, rel_tol=1e-10, abs_tol=1e-10):
                failures.append(f"{source.name}, seed={seed}: increments do not sum to E")
            if args.check and abs(value - source.infinite_excess_entropy) > args.tolerance:
                failures.append(f"{source.name}, seed={seed}: numeric result has infinite-E error {value - source.infinite_excess_entropy:+.6f} bits")
            estimates.append(value)
        mean = statistics.mean(estimates) if estimates else None
        error = mean - source.infinite_excess_entropy if mean is not None else None
        if args.check and error is not None and abs(error) > args.tolerance:
            failures.append(f"{source.name}: mean infinite-E error {error:+.6f} exceeds {args.tolerance:g} bits")
        rows.append({
            "source": source.name,
            "estimate_mean_bits": mean,
            "estimate_sd_bits": statistics.stdev(estimates) if len(estimates) > 1 else 0.0,
            "infinite_excess_entropy_bits": source.infinite_excess_entropy,
            "statistical_complexity_bits": source.statistical_complexity,
            "mean_error_bits": error,
            "max_absolute_error_bits": max((abs(value - source.infinite_excess_entropy) for value in estimates), default=None),
            "estimates_bits": estimates,
            "estimated_count": len(estimates),
            "unresolved_count": len(unresolved),
            "unresolved": unresolved,
            "declared_limitation": source.name in allowed_unresolved,
            "accuracy_status": "not_evaluated_unresolved" if not estimates else ("partial_estimates" if unresolved else "evaluated"),
            "diagnostics": diagnostics,
        })
    settings = {
        "package_version": lz.__version__, "length": args.length,
        "seeds": list(range(args.seeds)), "model_options": options,
        "elapsed_seconds": time.perf_counter() - started,
        "check_tolerance_bits": args.tolerance if args.check else None,
        "allowed_unresolved_sources": args.allow_unresolved,
    }
    summary = {
        "fully_estimated_sources": sum(row["estimated_count"] == args.seeds for row in rows),
        "source_count": len(rows),
        "unresolved_evaluations": sum(row["unresolved_count"] for row in rows),
    }
    if args.json:
        print(json.dumps({"settings": settings, "summary": summary, "rows": rows, "failures": failures}, indent=2))
    else:
        print(f"lzcomplexity {lz.__version__}; N={args.length}; seeds=0..{args.seeds - 1}; log_base=2")
        print("source                    estimate    infinite E     error       seed sd")
        for row in rows:
            if row["estimate_mean_bits"] is None:
                print(f"{row['source']:<25} UNRESOLVED {row['unresolved_count']}/{args.seeds}; no accuracy pass")
            else:
                print(f"{row['source']:<25} {row['estimate_mean_bits']:10.6f}"
                      f"  {row['infinite_excess_entropy_bits']:10.6f}"
                      f"  {row['mean_error_bits']:+10.6f}  {row['estimate_sd_bits']:9.6f}")
        print(f"Fully estimated sources: {summary['fully_estimated_sources']}/{summary['source_count']}; unresolved evaluations: {summary['unresolved_evaluations']}.")
        print(f"Elapsed: {settings['elapsed_seconds']:.2f}s. Seed sd is descriptive, not a confidence interval.")
        if args.check:
            print(f"Per-seed and mean infinite-E regression budget: {args.tolerance:g} bits; not an accuracy guarantee.")
    for failure in failures:
        print(f"FAIL: {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
