#!/usr/bin/env python3
"""Inspect the existing EMC-like shuffle measure; do not calibrate it to exact EMC.

Build the diagnostic-only Rust example, then run::

    cargo build --release -p lzcomplexity-core --example shuffle_diagnostics
    python scripts/validate_shuffle.py --self-test
    python scripts/validate_shuffle.py --lengths 2048 --seeds 1 --check
    python scripts/validate_shuffle.py --check --json > shuffle-report.json

`--check` checks implementation invariants, independent isotonic arithmetic,
constant-input behavior, repeatability, thread-count parity and public-entry
point parity. No source ordering or agreement with theoretical EMC is asserted.
The existing production formula, surrogate generator and default caps are
unchanged. The separately reported unweighted contrast sum is an alternative
formula from the 2019 paper, not a replacement or a correctness oracle.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import pathlib
import platform
import random
import statistics
import subprocess
import sys
import tempfile
import time
import unittest
from dataclasses import dataclass
from typing import Callable


@dataclass(frozen=True)
class Source:
    name: str
    generate: Callable[[int, int], bytes]


def iid(length: int, probability: float, seed: int) -> bytes:
    rng = random.Random(seed)
    return bytes(49 if rng.random() < probability else 48 for _ in range(length))


def periodic(length: int, motif: bytes, phase: int) -> bytes:
    phase %= len(motif)
    return (motif * ((length + phase + len(motif) - 1) // len(motif)))[phase:phase + length]


def markov(length: int, flip_probability: float, seed: int) -> bytes:
    rng = random.Random(seed)
    state = rng.randrange(2)  # stationary for the symmetric chain
    output = bytearray(length)
    for position in range(length):
        output[position] = 48 + state
        state ^= rng.random() < flip_probability
    return bytes(output)


def even(length: int, gamma: float, seed: int) -> bytes:
    rng = random.Random(seed)
    state = int(rng.random() < gamma / (1 + gamma))  # stationary A/B weights
    output = bytearray(length)
    for position in range(length):
        if state:
            output[position] = 49
            state = 0
        elif rng.random() < gamma:
            output[position] = 49
            state = 1
        else:
            output[position] = 48
    return bytes(output)


def sources() -> list[Source]:
    return [
        Source("constant", lambda n, seed: b"0" * n),
        Source("iid p=0.1", lambda n, seed: iid(n, 0.1, seed)),
        Source("iid p=0.5", lambda n, seed: iid(n, 0.5, seed)),
        Source("period 2", lambda n, seed: periodic(n, b"01", seed)),
        Source("period 8", lambda n, seed: periodic(n, b"00010111", seed)),
        Source("Markov flip=0.1", lambda n, seed: markov(n, 0.1, seed)),
        Source("even gamma=0.1", lambda n, seed: even(n, 0.1, seed)),
        Source("even gamma=0.5", lambda n, seed: even(n, 0.5, seed)),
        Source("even gamma=0.9", lambda n, seed: even(n, 0.9, seed)),
    ]


def isotonic_reference(values: list[float]) -> list[float]:
    """Independent min-max characterization of the L2 isotonic projection.

    This does not use the implementation's pool-adjacent-violators algorithm.
    Each fitted coordinate is max_a<=i min_b>=i average(values[a:b+1]).
    Taking its positive part adds the nonnegativity constraint.
    """
    prefix = [0.0]
    for value in values:
        prefix.append(prefix[-1] + value)
    return [
        max(0.0, max(
            min((prefix[b + 1] - prefix[a]) / (b - a + 1)
                for b in range(position, len(values)))
            for a in range(position + 1)
        ))
        for position in range(len(values))
    ]


def unweighted_contrast_sum(result: dict) -> float:
    """Diagnostic paper-style sum using precisely the same finite surrogates."""
    n = result["length"]
    if n <= 1:
        return 0.0
    g = math.log(n, result["log_base"]) / n
    return math.fsum((count - result["complexity"]) * g
                     for count in result["surrogate_counts"])


def check_result(result: dict) -> list[str]:
    failures = []
    maximum = result["max_block_size"]
    terms = result["summands"]
    raw = result["raw_weighted_ladder"]
    value = result["value"]
    if not (len(terms) == len(raw) == len(result["surrogate_counts"]) == maximum):
        return ["ladder/count/term lengths disagree"]
    if not all(math.isfinite(x) for x in [value, result["multi_information"], *terms, *raw]):
        return ["non-finite output"]
    if value < 0 or any(term < 0 for term in terms):
        failures.append("projected score or increment is negative")
    if not math.isclose(math.fsum(terms), value, rel_tol=1e-11, abs_tol=1e-11):
        failures.append("summands do not sum to score")
    fitted = isotonic_reference(raw)
    for position, expected in enumerate(fitted):
        cumulative = math.fsum(terms[:position + 1])
        if not math.isclose(cumulative, expected, rel_tol=1e-10, abs_tol=1e-10):
            failures.append(f"independent isotonic reference mismatch at block {position + 1}")
            break
    if raw and not math.isclose(result["multi_information"], raw[0], abs_tol=1e-12):
        failures.append("multi_information is not the raw one-symbol contrast")
    if result["direct_equals_reused"] is False:
        failures.append("full public pipeline differs from reused-factorization pipeline")
    return failures


def fingerprint(result: dict) -> tuple:
    return (result["complexity"], result["max_block_size"], result["value"].hex(),
            result["multi_information"].hex(), tuple(x.hex() for x in result["summands"]),
            tuple(x.hex() for x in result["raw_weighted_ladder"]),
            tuple(result["surrogate_counts"]))


class ReferenceTests(unittest.TestCase):
    def test_isotonic_minmax(self) -> None:
        self.assertEqual(isotonic_reference([1, 3, 2, 4]), [1, 2.5, 2.5, 4])
        self.assertEqual(isotonic_reference([6, 3, 0]), [3, 3, 3])
        self.assertEqual(isotonic_reference([-1, 0.5]), [0, 0.5])
        self.assertEqual(isotonic_reference([]), [])

    def test_periodic_phases(self) -> None:
        self.assertEqual(periodic(5, b"01", 0), b"01010")
        self.assertEqual(periodic(5, b"01", 1), b"10101")
        self.assertEqual(periodic(5, b"01", 3), b"10101")

    def test_generators_are_reproducible(self) -> None:
        for source in sources():
            self.assertEqual(source.generate(1024, 7), source.generate(1024, 7))
            self.assertEqual(len(source.generate(1024, 7)), 1024)

    def test_even_process_runs(self) -> None:
        data = even(10_000, 0.5, 3)
        zeros = [i for i, value in enumerate(data) if value == 48]
        self.assertGreater(len(zeros), 1)
        self.assertTrue(all((b - a - 1) % 2 == 0 for a, b in zip(zeros, zeros[1:])))

    def test_formula_distinction(self) -> None:
        result = {"length": 16, "log_base": 2, "complexity": 3,
                  "surrogate_counts": [5, 4, 3]}
        self.assertEqual(unweighted_contrast_sum(result), 0.75)
        # The weighted ladder would be [.5,.5,0], whose PAV endpoint is1/3.
        self.assertAlmostEqual(isotonic_reference([0.5, 0.5, 0])[-1], 1 / 3)


def source_metadata(root: pathlib.Path) -> dict:
    """Read-only source context; a dirty tree is not represented as clean HEAD."""
    try:
        revision = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
        status = subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=root, text=True,
            stderr=subprocess.DEVNULL,
        )
        return {"source_revision": revision, "source_dirty": bool(status)}
    except (OSError, subprocess.CalledProcessError):
        return {"source_revision": None, "source_dirty": None}


def main() -> int:
    root = pathlib.Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=pathlib.Path,
                        default=root / "target/release/examples" / ("shuffle_diagnostics.exe" if os.name == "nt" else "shuffle_diagnostics"))
    parser.add_argument("--lengths", type=int, nargs="+", default=[2048, 16384, 65536])
    parser.add_argument("--seeds", type=int, default=3, help="seeds 0..N-1; periodic fixtures use seed as phase")
    parser.add_argument("--caps", type=int, nargs="+", default=[8, 16, 24], help="fixed caps for sensitivity at largest length, seed0")
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--check", action="store_true", help="enforce implementation checks, not theoretical EMC accuracy")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return 0 if unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(ReferenceTests)).wasSuccessful() else 1
    if any(n < 16 for n in args.lengths) or not 1 <= args.seeds <= 100 or args.jobs < 1 or any(not 1 <= cap <= 128 for cap in args.caps):
        parser.error("lengths>=16, seeds1..100, jobs>=1 and caps1..128 required")
    if not args.driver.is_file():
        parser.error("build driver: cargo build --release -p lzcomplexity-core --example shuffle_diagnostics")
    source_context = source_metadata(root)
    driver_sha256 = hashlib.sha256(args.driver.read_bytes()).hexdigest()
    started = time.perf_counter()
    rows, sensitivity, phases, failures = [], [], [], []
    checks = {"repeatability_cases": 0, "thread_count_cases": 0, "public_entry_point_cases": 0}
    with tempfile.TemporaryDirectory(prefix="lz-shuffle-") as directory:
        input_path = pathlib.Path(directory) / "input.bin"

        def measure(data: bytes, cap: int = -1, jobs: int = args.jobs, base: int = 2, verify: bool = False) -> dict:
            input_path.write_bytes(data)
            command = [str(args.driver.resolve()), str(input_path), str(cap), str(jobs), str(base)]
            if verify:
                command.append("--verify")
            result = json.loads(subprocess.check_output(command, text=True))
            result["unweighted_contrast_sum"] = unweighted_contrast_sum(result)
            if args.check:
                failures.extend(check_result(result))
            return result

        selected_sources = sources()
        for length in args.lengths:
            for source in selected_sources:
                results = [measure(source.generate(length, seed)) for seed in range(args.seeds)]
                values = [result["value"] for result in results]
                paper_values = [result["unweighted_contrast_sum"] for result in results]
                if args.check and source.name == "constant" and any(result["value"] != 0 or any(result["raw_weighted_ladder"]) for result in results):
                    failures.append("constant input changed under shuffle")
                rows.append({"source": source.name, "length": length,
                             "max_block_size": results[0]["max_block_size"],
                             "score_mean": statistics.mean(values),
                             "score_min": min(values), "score_max": max(values),
                             "unweighted_sum_mean": statistics.mean(paper_values),
                             "results": results})

        largest = max(args.lengths)
        for source in selected_sources:
            if source.name not in {"iid p=0.5", "period 2", "period 8", "even gamma=0.9"}:
                continue
            data = source.generate(largest, 0)
            records = [measure(data, cap) for cap in sorted(set(args.caps))]
            if args.check:
                for left, right in zip(records, records[1:]):
                    n = left["max_block_size"]
                    if left["surrogate_counts"] != right["surrogate_counts"][:n]:
                        failures.append(f"{source.name}: changing cap altered shared surrogate prefix")
            sensitivity.append({"source": source.name, "length": largest, "seed": 0, "results": records})
        for phase in range(8):
            phases.append({"phase": phase, "result": measure(periodic(largest, b"00010111", phase))})

        if args.check:
            for source in selected_sources:
                if source.name not in {"constant", "iid p=0.5", "period 8", "even gamma=0.9"}:
                    continue
                data = source.generate(min(args.lengths), 0)
                first = measure(data, jobs=1, verify=True)
                repeated = measure(data, jobs=1, verify=True)
                threaded = measure(data, jobs=3, verify=True)
                checks["repeatability_cases"] += 1
                checks["thread_count_cases"] += 1
                checks["public_entry_point_cases"] += 3
                if fingerprint(first) != fingerprint(repeated):
                    failures.append(f"{source.name}: repeated result differs")
                if fingerprint(first) != fingerprint(threaded):
                    failures.append(f"{source.name}: thread count changed result")
                converted = measure(data, jobs=1, base=4)
                if first["surrogate_counts"] != converted["surrogate_counts"] or not math.isclose(first["value"], 2 * converted["value"], abs_tol=1e-12):
                    failures.append(f"{source.name}: log-base conversion changed surrogates or scale")
    settings = {**source_context, "driver_sha256": driver_sha256,
                "lengths": args.lengths, "seeds": list(range(args.seeds)), "log_base": 2,
                "jobs": args.jobs, "caps": args.caps, "python": platform.python_version(),
                "elapsed_seconds": time.perf_counter() - started,
                "checks_enforced": args.check,
                "interpretation": "EMC-like shuffle measure; no exact-EMC accuracy assertion"}
    report = {"settings": settings, "controls": rows, "cap_sensitivity": sensitivity,
              "period8_phases": phases, "checks": checks, "failures": failures}
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print("EMC-like shuffle measure; descriptive values, not exact-EMC accuracy tests")
        print("length  max M source                  current mean    seed/phase range       unweighted sum")
        for row in rows:
            print(f"{row['length']:6d} {row['max_block_size']:6d} {row['source']:<23} {row['score_mean']:11.6f}"
                  f" [{row['score_min']:.6f}, {row['score_max']:.6f}] {row['unweighted_sum_mean']:14.6f}")
        print(f"Checks: {checks}; failures={len(failures)}; elapsed={settings['elapsed_seconds']:.2f}s")
        print("Cap and phase sensitivity records are included in --json output.")
    for failure in failures:
        print("FAIL: " + failure, file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
