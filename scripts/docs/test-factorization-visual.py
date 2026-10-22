#!/usr/bin/env python3
"""Check the example's substring trace and the text/colors actually drawn."""
import json
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
MODEL = json.loads((ROOT / "tests/docs/factorization-visual.json").read_text())
SEQUENCE = MODEL["sequence"]
ROWS = MODEL["trace"]
BLUE, TEAL, INK = "#4779c5", "#208a80", "#25344d"
SVG = "{http://www.w3.org/2000/svg}"

# Independent substring search for this example, whose matches all lie wholly
# before their factors. In particular, do not search only previous factors.
position, factors, boundaries = 0, [], [0]
for row in ROWS:
    history = SEQUENCE[:position]
    length = 0
    while position + length < len(SEQUENCE) and SEQUENCE[position:position + length + 1] in history:
        length += 1
    match = SEQUENCE[position:position + length]
    next_symbol = SEQUENCE[position + length]
    factor = match + next_symbol
    assert row["start"] == position + 1
    assert (row["history"], row["match"], row["new"], row["factor"]) == (history, match, next_symbol, factor)
    assert row["match_start"] == (history.index(match) + 1 if match else None)
    position += len(factor)
    factors.append(factor)
    boundaries.append(position)
assert position == len(SEQUENCE)
assert boundaries == MODEL["boundaries"]
assert len(factors) == MODEL["count"]


def literal(element):
    return "".join(element.itertext())


def find(root, role, row=None):
    elements = [el for el in root.iter(SVG + "text") if el.get("data-role") == role
                and (row is None or el.get("data-start") == str(row["start"]))]
    assert len(elements) == 1, (role, row, len(elements))
    return elements[0]


def check_value(element, value, displayed=None):
    assert element.get("data-value") == value
    assert literal(element) == (value if displayed is None else displayed)


def check_factor(element, row):
    check_value(element, row["factor"])
    assert [(literal(span), span.get("fill")) for span in element] == [(row["match"], BLUE), (row["new"], TEAL)]


for name in ("sequence-workflow.svg", "sequence-workflow-mobile.svg", "social-card.svg"):
    root = ET.parse(ROOT / "docs/assets" / name).getroot()
    check_value(find(root, "input"), SEQUENCE)
    assert len([el for el in root.iter(SVG + "text") if el.get("data-role") == "factor"]) == MODEL["count"]
    for row in ROWS:
        check_factor(find(root, "factor", row), row)
    for row in ROWS if name != "social-card.svg" else ROWS[-1:]:
        history = find(root, "history", row)
        check_value(history, row["history"], row["history"] or MODEL["labels"]["empty_history"])
        if row["history"]:
            offset = row["match_start"] - 1 if row["match"] else 0
            assert "".join(literal(span) for span in history if span.get("fill") == BLUE) == row["match"]
            for span in history:
                assert span.get("fill") in (BLUE, INK)
            # The highlighted occurrence must start at the stated source position.
            before = ""
            for span in history:
                if span.get("fill") == BLUE:
                    assert len(before) == offset
                    break
                before += literal(span)
        match = find(root, "match", row)
        check_value(match, row["match"], row["match"] or MODEL["labels"]["empty_match"])
        if row["match"]:
            assert match.get("fill") == BLUE
        new = find(root, "new", row)
        check_value(new, row["new"])
        assert new.get("fill") == TEAL
        if name != "social-card.svg":
            check_value(find(root, "position", row), str(row["start"]))
    if name == "social-card.svg":
        check_factor(find(root, "factor-detail", ROWS[-1]), ROWS[-1])

print("All six substring steps, visible factors, source highlights and next-symbol colors match in the three figures.")
