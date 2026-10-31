"""Regenerate browser parity fixtures with the installed native Python wheel.

Run with a venv containing a wheel built from the current Rust source:
    python scripts/docs/generate-demo-fixtures.py

The separate rng-cases.json records native rand 0.8.6 / rand_chacha 0.3.1
next_u64 outputs and block-swapped bytes on a 64-bit Rust target. Keep those
vectors when changing this corpus: they independently catch RNG differences
that can otherwise be hidden by identical LZ factor counts.
"""
import json
import random
import re
from pathlib import Path

import lzcomplexity as lz

ROOT = Path(__file__).resolve().parents[2]
rng = random.Random(72194)
inputs = [
    "", "0", "00", "01", "banana", "banana" * 20, "ABRACADABRA",
    "éééàé", "👩‍🔬science", "white space\nwith tabs\t", "<script>alert(1)</script>",
    "00010111" * 32, "01" * 128, "ACGT" * 64, "éà" * 64,
    "👩‍🔬" * 20, "a" * 256,
]
for n in [9, 10, 11, 12, 16, 31, 32, 49, 50, 51, 63, 64, 65, 100, 127, 128, 129, 200, 255, 256]:
    for alphabet in ["01", "ACGT", "abc012! "]:
        inputs.append("".join(rng.choice(alphabet) for _ in range(n)))
for _ in range(50):
    n = rng.randrange(1, 257)
    inputs.append("".join(rng.choice("01") for _ in range(n)))

cases = []
for text in inputs:
    result = lz.lz76(text, log_base=2)
    cases.append({"input": text, "count": result["complexity"], "boundaries": result["factors"],
                  "h": result["h"], "emc": result["emc"]})
for text in [inputs[4], inputs[11], inputs[12], inputs[13], inputs[-1], inputs[-2]]:
    for cap in [1, 2, 3, 7, 16, 64, 256]:
        for base in [2, 4]:
            result = lz.lz76(text, log_base=base, max_block_size=cap)
            cases.append({"input": text, "options": {"maxBlockSize": cap, "logBase": base}, "emc": result["emc"]})

distances = []
pairs = [("", ""), ("", "banana"), ("banana", "banana"), ("0000", "1111"),
         ("01" * 128, "01" * 128), ("👩‍🔬" * 20, "éà" * 64)]
for _ in range(70):
    a, b = rng.choice(inputs), rng.choice(inputs)
    pairs.append((a, b))
for a, b in pairs:
    distances.append({"a": a, "b": b, "value": lz.nid(a, b), "counts": {
        "x": lz.factorization(a)[0], "y": lz.factorization(b)[0],
        "xy": lz.factorization(a + b)[0], "yx": lz.factorization(b + a)[0]}})

output = {"source": "Native lzcomplexity local Chao-Shen default from current Rust core; base 2 unless options override.",
          "cases": cases, "distances": distances}
(ROOT / "tests/docs/measure-cases.json").write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n")
print(f"Wrote {len(cases)} native E cases and {len(distances)} distance cases.")

# Automatic normalization follows the same observed-byte alphabet as Python.
preset_source = (ROOT / "docs/assets/site.js").read_text()
dna_parts = re.search(r"const dna =\s*((?:\"[ACGT]+\"\s*\+?\s*)+);", preset_source)
if dna_parts is None:
    raise ValueError("Could not read the DNA preset for automatic-base fixtures")
dna = "".join(re.findall(r'"([ACGT]+)"', dna_parts.group(1)))
auto_inputs = [
    ("empty", ""),
    ("single symbol", "A"),
    ("constant", "a" * 64),
    ("binary", "00010111" * 32),
    ("banana", "banana"),
    ("three symbols", "abcaacbcbbaacabc" * 8),
    ("four symbols", "ACGT" * 64),
    ("DNA reference", dna),
    ("typed text", "the quick brown fox jumps over the lazy dog" * 4),
    ("Unicode bytes", "éà猫👩‍🔬" * 8),
]
auto_cases = []
for name, text in auto_inputs:
    alphabet_size = len(set(text.encode("utf-8")))
    result = lz.lz76(text, max_block_size=8)
    assert result["h"] == lz.h(text)
    auto_cases.append({
        "name": name, "input": text, "alphabetSize": alphabet_size,
        "logBase": max(2, alphabet_size), "count": result["complexity"],
        "boundaries": result["factors"], "h": result["h"],
        "maxBlockSize": 8, "emc": result["emc"],
    })
auto_output = {
    "source": "Native Python defaults: observed UTF-8 byte alphabet, minimum base two; explicit maximum range eight.",
    "cases": auto_cases,
}
(ROOT / "tests/docs/auto-base-cases.json").write_text(json.dumps(auto_output, ensure_ascii=False, indent=2) + "\n")
print(f"Wrote {len(auto_cases)} native automatic-base cases.")
