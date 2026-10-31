"""Native estimator regressions. Run with an installed local wheel:

    python -m unittest discover -s tests -p 'test_*.py'

The shuffle corpus was captured before changing the default implementation.
"""
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import unittest

import lzcomplexity as lz

ROOT = Path(__file__).resolve().parents[1]


class EmcTests(unittest.TestCase):
    def test_default_full_and_standalone_agree(self):
        for data in (b"", b"a", b"01", b"banana", b"00010111" * 256):
            for method in ("chao-shen", "shuffle"):
                for cap in (-1, 1, 8):
                    with self.subTest(data=data[:16], method=method, cap=cap):
                        value, terms = lz.emc(data, max_block_size=cap, method=method)
                        full = lz.lz76(data, max_block_size=cap, method=method)["emc"]
                        self.assertEqual(full["method"], method)
                        self.assertEqual((full["value"], full["summands"]), (value, terms))
                        self.assertEqual(len(terms), full["max_block_size"])
                        self.assertTrue(all(math.isfinite(v) and v >= 0 for v in terms))
                        self.assertAlmostEqual(sum(terms), value, places=12)
            self.assertEqual(lz.emc(data), lz.emc(data, method="chao-shen"))
        # A fair alternating pair distribution has one bit of block entropy;
        # the LZ estimate on length 10 is 2*log2(10)/10.
        expected = 1 / (1 - 0.5 ** 10) - 2 * math.log2(10) / 10
        self.assertAlmostEqual(lz.emc(b"01" * 5, max_block_size=1)[0], expected)
        self.assertEqual(lz.emc(b"01" * 5, method="shuffle")[0], 0)

    def test_matches_built_july_28_chao_shen_binary(self):
        cases = json.loads((ROOT / "tests/chao-shen-history.json").read_text(encoding="utf-8"))["cases"]
        for case in cases:
            with self.subTest(name=case["name"], base=case["log_base"]):
                actual = lz.lz76(case["input"], log_base=case["log_base"],
                                 max_block_size=8)["emc"]
                expected = case["emc"]
                self.assertEqual(actual["max_block_size"], expected["max_block_size"])
                for key in ("value", "multi_information"):
                    self.assertAlmostEqual(actual[key], expected[key], places=12)
                self.assertEqual(len(actual["summands"]), len(expected["summands"]))
                for actual_term, old_term in zip(actual["summands"], expected["summands"]):
                    self.assertAlmostEqual(actual_term, old_term, places=12)

    def test_shuffle_matches_pre_change_corpus(self):
        for case in json.loads((ROOT / "tests/docs/shuffle-cases.json").read_text(encoding="utf-8"))["cases"]:
            options = case.get("options", {})
            actual = lz.lz76(case["input"], method="shuffle",
                             log_base=options.get("logBase", 2),
                             max_block_size=options.get("maxBlockSize", -1))["emc"]
            with self.subTest(input=case["input"][:16], options=options):
                for key, expected in case["emc"].items():
                    if isinstance(expected, list):
                        self.assertEqual(len(actual[key]), len(expected), key)
                        for value, reference in zip(actual[key], expected):
                            self.assertAlmostEqual(value, reference, places=12, msg=key)
                    elif isinstance(expected, float):
                        # Reference fixtures come from macOS; allow libm rounding
                        # differences on Windows and Linux.
                        self.assertAlmostEqual(actual[key], expected, places=12, msg=key)
                    else:
                        self.assertEqual(actual[key], expected, key)

    def test_units_do_not_change_automatic_range(self):
        data = b"ACGT" * 1000
        for method in ("chao-shen", "shuffle"):
            bits = lz.lz76(data, log_base=2, method=method)["emc"]
            base4 = lz.lz76(data, log_base=4, method=method)["emc"]
            self.assertEqual(bits["max_block_size"], base4["max_block_size"])
            self.assertAlmostEqual(bits["value"], base4["value"] * 2, places=12)

    def test_reproducible_across_processes_and_workers(self):
        code = "import json, lzcomplexity as lz; print(json.dumps(lz.lz76(b'00010111'*256)))"
        outputs = [subprocess.check_output([sys.executable, "-c", code],
                   env={**os.environ, "RAYON_NUM_THREADS": str(workers)})
                   for workers in (1, 4, 4)]
        self.assertEqual(outputs[0], outputs[1])
        self.assertEqual(outputs[1], outputs[2])

    def test_method_validation_and_positional_compatibility(self):
        for function in (lz.emc, lz.lz76):
            with self.assertRaises(ValueError):
                function("banana", method="typo")
            with self.assertRaises(TypeError):
                function("banana", method=None)
            self.assertEqual(function("banana", 1, None, 2, 8, 0),
                             function("banana", log_base=2, max_block_size=8))


if __name__ == "__main__":
    unittest.main()
