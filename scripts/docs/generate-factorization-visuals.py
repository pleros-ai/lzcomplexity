#!/usr/bin/env python3
"""Generate the factorization figures from their shared, inspectable trace."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "tests/docs/factorization-visual.json"
ASSETS = ROOT / "docs/assets"
INK = "#25344d"
MUTED = "#687a91"
BLUE = "#4779c5"
TEAL = "#208a80"
VIOLET = "#7665ac"
SANS = "-apple-system, BlinkMacSystemFont, 'Helvetica Neue', Arial, sans-serif"
MONO = "'SFMono-Regular', Menlo, Consolas, monospace"


def text(x, y, value, *, size=14, color=INK, mono=False, weight=None,
         role=None, start=None, data_value=None, anchor=None, spans=None):
    attrs = {"x": x, "y": y, "font-size": size, "fill": color,
             "font-family": MONO if mono else SANS}
    if weight:
        attrs["font-weight"] = weight
    if role:
        attrs["data-role"] = role
    if start is not None:
        attrs["data-start"] = start
    if data_value is not None:
        attrs["data-value"] = data_value
    if anchor:
        attrs["text-anchor"] = anchor
    attributes = " ".join(f'{k}="{escape(str(v), quote=True)}"' for k, v in attrs.items())
    body = escape(value) if spans is None else "".join(
        f'<tspan fill="{color}">{escape(part)}</tspan>' for part, color in spans
    )
    return f"<text {attributes}>{body}</text>"


def rect(x, y, w, h, *, radius=10, fill="#fff", stroke="#dfe7ef"):
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" '
            f'fill="{fill}" stroke="{stroke}"/>')


def document(width, height, title, description, parts):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title description">
<title id="title">{escape(title)}</title>
<desc id="description">{escape(description)}</desc>
<defs><linearGradient id="paper" x2="1" y2="1"><stop stop-color="#fff"/><stop offset="1" stop-color="#f2f7fa"/></linearGradient></defs>
{chr(10).join(parts)}
</svg>
'''


def history_parts(row):
    if not row["match"]:
        return [(row["history"], INK)]
    offset = row["match_start"] - 1
    end = offset + len(row["match"])
    return [(row["history"][:offset], INK), (row["history"][offset:end], BLUE),
            (row["history"][end:], INK)]


def validate(model):
    rows = model["trace"]
    assert len(rows) == model["count"]
    assert "".join(row["factor"] for row in rows) == model["sequence"]
    assert [row["start"] - 1 for row in rows] + [len(model["sequence"])] == model["boundaries"]
    for row in rows:
        assert row["history"] == model["sequence"][:row["start"] - 1]
        assert row["factor"] == row["match"] + row["new"]
        assert len(row["new"]) == 1
        if row["match"]:
            offset = row["match_start"] - 1
            assert row["history"][offset:offset + len(row["match"])] == row["match"]
        else:
            assert row["match_start"] is None
        assert row["factor"] not in row["history"]


def trace_figure(model, mobile=False):
    lab, acc = model["labels"], model["accessibility"]
    width, height = (360, 440) if mobile else (600, 410)
    margin = 16 if mobile else 24
    p = [rect(.5, .5, width - 1, height - 1, radius=20,
              fill="url(#paper)")]
    p += [text(margin, 32 if mobile else 37, lab["title"], size=18 if mobile else 22, weight=600)]
    badge_x = 280 if mobile else 490
    p += [rect(badge_x, 15 if mobile else 17, 64 if mobile else 86, 28 if mobile else 30,
               fill="#efedf9", stroke="none")]
    p += [text(badge_x + (32 if mobile else 43), 34 if mobile else 38,
               lab["count_template"].format(count=model["count"]),
               size=14 if mobile else 16, color=VIOLET, anchor="middle")]
    p += [text(margin, 71 if mobile else 77, lab["input"], size=13 if mobile else 14, color=MUTED)]
    p += [text(65 if mobile else 84, 71 if mobile else 77, model["sequence"],
               size=19 if mobile else 23, mono=True, role="input", data_value=model["sequence"])]
    x = {"position": 16, "history": 54, "match": 161, "factor": 289} if mobile else {
        "position": 24, "history": 100, "match": 296, "factor": 481}
    header_y, row_y, row_h = (107, 119, 45) if mobile else (112, 123, 37)
    header_size = 11 if mobile else 12
    for key, label in [("position", lab["position_mobile"] if mobile else lab["position"]),
                       ("history", lab["history"]), ("match", lab["match_new"]), ("factor", lab["factor"])]:
        p += [text(x[key], header_y, label, size=header_size, color=MUTED)]
    for i, row in enumerate(model["trace"]):
        top = row_y + i * row_h
        baseline = top + (28 if mobile else 25)
        p += [rect(margin - 4, top, width - margin * 2 + 8, row_h - 3,
                   radius=7, fill="#fff" if i % 2 == 0 else "#f0f4f9", stroke="none")]
        p += [text(x["position"], baseline, str(row["start"]), size=14,
                   color=MUTED, mono=True, role="position", start=row["start"], data_value=str(row["start"]))]
        p += [text(x["history"], baseline, row["history"] or lab["empty_history"],
                   size=14 if mobile else 18, color=MUTED, mono=True,
                   role="history", start=row["start"], data_value=row["history"],
                   spans=history_parts(row) if row["history"] else None)]
        symbol_size = 14 if mobile else 18
        advance = symbol_size * .6
        p += [text(x["match"], baseline, row["match"] or lab["empty_match"],
                   size=symbol_size, color=BLUE if row["match"] else MUTED, mono=True,
                   role="match", start=row["start"], data_value=row["match"])]
        plus_x = x["match"] + (len(row["match"]) or 1) * advance + (7 if mobile else 11)
        p += [text(plus_x, baseline, lab["plus"], size=symbol_size, color=MUTED, mono=True)]
        p += [text(plus_x + (15 if mobile else 21), baseline, row["new"],
                   size=symbol_size, color=TEAL, mono=True, role="new", start=row["start"], data_value=row["new"])]
        p += [text(269 if mobile else 453, baseline, lab["arrow"], size=15 if mobile else 20, color="#97a7ba", anchor="middle")]
        p += [text(x["factor"], baseline, row["factor"], size=symbol_size,
                   mono=True, role="factor", start=row["start"], data_value=row["factor"],
                   spans=[(row["match"], BLUE), (row["new"], TEAL)])]
    legend_y = 417 if mobile else 382
    p += [f'<circle cx="{margin + 3}" cy="{legend_y - 4}" r="3.5" fill="{BLUE}"/>',
          text(margin + 13, legend_y, lab["matched_prefix"], size=11 if mobile else 12, color=MUTED)]
    second = 183 if mobile else 265
    p += [f'<circle cx="{second}" cy="{legend_y - 4}" r="3.5" fill="{TEAL}"/>',
          text(second + 10, legend_y, lab["next_symbol"], size=11 if mobile else 12, color=MUTED)]
    return document(width, height, acc["title"], acc["description"], p)


def social_figure(model):
    lab, acc = model["labels"], model["accessibility"]
    p = [rect(0, 0, 1200, 630, radius=0, fill="url(#paper)", stroke="none"),
         '<circle cx="1130" cy="0" r="290" fill="#e7ecfb" opacity=".45"/>',
         rect(74, 43, 36, 36, fill=BLUE, stroke="none"),
         '<path d="M84 52v17h9m0-17h8l-8 17h8" fill="none" stroke="#fff" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>',
         text(124, 67, lab["brand"], size=17, weight=600),
         text(238, 67, lab["brand_context"], size=14, color=MUTED),
         text(69, 178, lab["product"], size=87, weight=600),
         text(74, 229, lab["subtitle"], size=27, color="#5c6d85"),
         text(74, 283, lab["input"], size=16, color=MUTED),
         text(169, 283, model["sequence"], size=27, mono=True, role="input", data_value=model["sequence"])]
    lefts = [74, 178, 282, 386, 538, 690]
    widths = [88, 88, 88, 136, 136, 280]
    for row, left, width in zip(model["trace"], lefts, widths):
        p += [rect(left, 307, width, 60, radius=13),
              text(left + width / 2, 347, row["factor"], size=28, mono=True,
                   anchor="middle", role="factor", start=row["start"], data_value=row["factor"],
                   spans=[(row["match"], BLUE), (row["new"], TEAL)])]
    p += [text(1084, 331, lab["title"], size=13, color=MUTED, anchor="middle"),
          text(1084, 356, lab["count_template"].format(count=model["count"]), size=20, color=VIOLET, anchor="middle")]
    row = model["trace"][-1]
    p += [rect(74, 402, 1052, 110, radius=15),
          text(94, 426, lab["position_template"].format(start=row["start"]), size=12, color=MUTED),
          text(94, 453, lab["history"], size=13, color=MUTED),
          text(94, 489, row["history"], size=27, mono=True,
               role="history", start=row["start"], data_value=row["history"], spans=history_parts(row)),
          text(503, 453, lab["match_new"], size=13, color=MUTED),
          text(503, 489, row["match"], size=27, mono=True, color=BLUE,
               role="match", start=row["start"], data_value=row["match"]),
          text(584, 489, lab["plus"], size=27, mono=True, color=MUTED),
          text(618, 489, row["new"], size=27, mono=True, color=TEAL,
               role="new", start=row["start"], data_value=row["new"]),
          text(731, 489, lab["arrow"], size=32, color="#97a7ba", anchor="middle"),
          text(837, 453, lab["factor"], size=13, color=MUTED),
          text(837, 489, row["factor"], size=27, mono=True,
               role="factor-detail", start=row["start"], data_value=row["factor"],
               spans=[(row["match"], BLUE), (row["new"], TEAL)]),
          text(74, 561, lab["languages"], size=19, color="#5c6d85"),
          text(74, 599, lab["url"], size=17, color=MUTED)]
    return document(1200, 630, acc["social_title"], acc["social_description"], p)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--proof-dir", type=Path)
    parser.add_argument("--skip-png", action="store_true")
    parser.add_argument("--check", action="store_true", help="check SVG sources without changing files or rendering PNGs")
    args = parser.parse_args()
    model = json.loads(MODEL.read_text())
    validate(model)
    outputs = {"sequence-workflow.svg": trace_figure(model),
               "sequence-workflow-mobile.svg": trace_figure(model, mobile=True),
               "social-card.svg": social_figure(model)}
    if args.check:
        stale = [name for name, source in outputs.items() if (ASSETS / name).read_text() != source]
        if stale:
            raise SystemExit("Regenerate factorization figures: " + ", ".join(stale))
        print("All three factorization SVGs match the shared trace.")
        return
    for filename, source in outputs.items():
        (ASSETS / filename).write_text(source)
    if not args.skip_png:
        renderer = shutil.which("rsvg-convert")
        if not renderer:
            raise SystemExit("rsvg-convert is required to render PNG assets (or pass --skip-png)")
        subprocess.run([renderer, "-o", str(ASSETS / "social-card.png"), str(ASSETS / "social-card.svg")], check=True)
        if args.proof_dir:
            args.proof_dir.mkdir(parents=True, exist_ok=True)
            for source, suffix in [("sequence-workflow.svg", "desktop"),
                                   ("sequence-workflow-mobile.svg", "mobile"),
                                   ("social-card.svg", "social")]:
                subprocess.run([renderer, "-o", str(args.proof_dir / f"lz-factorization-trace-{suffix}.png"), str(ASSETS / source)], check=True)


if __name__ == "__main__":
    main()
