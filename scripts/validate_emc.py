#!/usr/bin/env python3
"""Reproduce finite-window EMC checks against sources with known entropies.

Install the current wheel first, then run, for example::

    python scripts/validate_emc.py --length 100000 --seeds 3
    python scripts/validate_emc.py --length 20000 --seeds 1 --check
    python scripts/validate_emc.py --self-test

Only the installed lzcomplexity extension and Python's standard library are
needed. Every reported value is in bits. Seeds are the integers 0..seeds-1;
Markov/HMM generators start in their stationary distribution. Periodic sources
start at a uniformly drawn phase.

The statistical target is I(past_a; future_b), a=floor(M/2), b=ceil(M/2), for
the reported maximum joint-window length M. For a stationary process this is
H(a)+H(b)-H(M). Its limit is excess entropy, but a small finite-window estimate
need not approximate that limit. See Crutchfield and Feldman (2003),
https://arxiv.org/abs/cond-mat/0102181 . The exact source calculations below are
independent of the library's empirical histogram implementation.

Plug-in mutual information has sampling bias, including a positive iid null.
The --check tolerance is a reproducible smoke-test budget on the seed mean,
not a confidence interval or a distribution-free accuracy guarantee. The script
does not change or tune the estimator.
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


@dataclass(frozen=True)
class Source:
    name: str
    generate: Callable[[int, int], bytes]
    truth: Callable[[int], tuple[float, float]]


def sources() -> list[Source]:
    result = []
    for p in (0.01, 0.1, 0.5):
        result.append(
            Source(
                f"iid p={p:g}",
                lambda n, seed, p=p: iid_sequence(n, p, seed),
                lambda _width: (0.0, 0.0),
            )
        )
    for a, b in ((0.1, 0.4), (0.05, 0.05), (0.3, 0.4)):
        result.append(
            Source(
                f"Markov a={a:g},b={b:g}",
                lambda n, seed, a=a, b=b: markov_sequence(n, a, b, seed),
                lambda width, a=a, b=b: markov_truth(a, b, width),
            )
        )
    for gamma in (0.1, 0.5, 0.9):
        result.append(
            Source(
                f"even gamma={gamma:g}",
                lambda n, seed, gamma=gamma: even_sequence(n, gamma, seed),
                lambda width, gamma=gamma: even_truth(gamma, width),
            )
        )
    motif = b"00010111"
    result.append(
        Source(
            "period 8",
            lambda n, seed: periodic_sequence(n, motif, seed),
            lambda width: periodic_truth(motif, width),
        )
    )
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--length", type=int, default=100_000)
    parser.add_argument("--seeds", type=int, default=3, help="number of seeds, starting at 0")
    parser.add_argument("--max-block-size", type=int, default=-1, help="total joint-window cap")
    parser.add_argument("--check", action="store_true", help="enforce invariants and smoke budgets")
    parser.add_argument("--tolerance", type=float, default=0.1, help="finite-target mean error budget, bits")
    parser.add_argument("--json", action="store_true", help="emit settings and rows as JSON")
    parser.add_argument("--self-test", action="store_true", help="test the exact source oracles only")
    args = parser.parse_args(argv)
    if args.self_test:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(ExactSourceTests)
        return 0 if unittest.TextTestRunner().run(suite).wasSuccessful() else 1
    if args.length < 2 or args.seeds < 1 or not math.isfinite(args.tolerance) or args.tolerance < 0:
        parser.error("length >= 2, seeds >= 1, and a finite nonnegative tolerance are required")

    import lzcomplexity as lz

    started = time.perf_counter()
    rows = []
    failures = []
    for source in sources():
        estimates = []
        targets = []
        widths = []
        infinite = None
        for seed in range(args.seeds):
            data = source.generate(args.length, seed)
            value, summands = lz.emc(data, log_base=2, max_block_size=args.max_block_size)
            width = len(summands)
            target, infinite = source.truth(width)
            estimates.append(value)
            targets.append(target)
            widths.append(width)
            if not math.isfinite(value) or value < 0 or any(not math.isfinite(s) or s < 0 for s in summands):
                failures.append(f"{source.name}, seed={seed}: invalid value or increment")
            if not math.isclose(math.fsum(summands), value, rel_tol=1e-11, abs_tol=1e-11):
                failures.append(f"{source.name}, seed={seed}: increments do not sum to value")
        mean = statistics.mean(estimates)
        target = statistics.mean(targets)
        error = mean - target
        if args.check and abs(error) > args.tolerance:
            failures.append(f"{source.name}: mean finite-target error {error:+.6f} exceeds {args.tolerance:g} bits")
        rows.append(
            {
                "source": source.name,
                "joint_window_lengths": widths,
                "estimate_mean_bits": mean,
                "estimate_sd_bits": statistics.stdev(estimates) if len(estimates) > 1 else 0.0,
                "exact_finite_target_mean_bits": target,
                "infinite_excess_entropy_bits": infinite,
                "mean_error_bits": error,
                "estimates_bits": estimates,
            }
        )
    settings = {
        "package_version": lz.__version__,
        "length": args.length,
        "seeds": list(range(args.seeds)),
        "log_base": 2,
        "max_block_size_argument": args.max_block_size,
        "elapsed_seconds": time.perf_counter() - started,
        "check_tolerance_bits": args.tolerance if args.check else None,
    }
    if args.json:
        print(json.dumps({"settings": settings, "rows": rows, "failures": failures}, indent=2))
    else:
        print(f"lzcomplexity {lz.__version__}; N={args.length}; seeds=0..{args.seeds - 1}; log_base=2")
        print("source                  M    estimate      finite E_M   infinite E   error       seed sd")
        for row in rows:
            width = ",".join(map(str, sorted(set(row["joint_window_lengths"]))))
            print(
                f"{row['source']:<23} {width:>3}  {row['estimate_mean_bits']:10.6f}"
                f"  {row['exact_finite_target_mean_bits']:10.6f}"
                f"  {row['infinite_excess_entropy_bits']:10.6f}"
                f"  {row['mean_error_bits']:+10.6f}  {row['estimate_sd_bits']:9.6f}"
            )
        print(f"Elapsed: {settings['elapsed_seconds']:.2f}s. Seed sd is descriptive, not a confidence interval.")
        if args.check:
            print(f"Finite-target mean error smoke budget: {args.tolerance:g} bits; not an accuracy guarantee.")
    for failure in failures:
        print(f"FAIL: {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
