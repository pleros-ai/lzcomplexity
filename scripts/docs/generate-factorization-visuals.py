#!/usr/bin/env python3
"""Generate the binary factorization figures from one checked example."""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / 'tests/docs/factorization-visual.json'
ASSETS = ROOT / 'docs/assets'
INK, MUTED = '#25344d', '#687a91'
BLUE, TEAL, VIOLET = '#4779c5', '#208a80', '#7665ac'
SANS = "-apple-system, BlinkMacSystemFont, 'Helvetica Neue', Arial, sans-serif"
MONO = "'SFMono-Regular', Menlo, Consolas, monospace"


def attrs(values):
    return ' '.join(f'{key}="{escape(str(value), quote=True)}"' for key, value in values.items())


def text(x, y, value, size=14, color=INK, mono=False, weight=None, anchor=None, **data):
    values = {'x': x, 'y': y, 'font-size': size, 'fill': color,
              'font-family': MONO if mono else SANS}
    if weight: values['font-weight'] = weight
    if anchor: values['text-anchor'] = anchor
    values.update(data)
    return f'<text {attrs(values)}>{escape(value)}</text>'


def rect(x, y, width, height, fill='#fff', stroke='#dfe7ef', radius=12, **data):
    return f'<rect {attrs(dict(x=x,y=y,width=width,height=height,rx=radius,fill=fill,stroke=stroke,**data))}/>'


def document(width, height, title, description, parts):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title description">
<title id="title">{escape(title)}</title>
<desc id="description">{escape(description)}</desc>
<defs>
<linearGradient id="paper" x2="1" y2="1"><stop stop-color="#fff"/><stop offset="1" stop-color="#f2f7fb"/></linearGradient>
<marker id="arrow-muted" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" orient="auto"><path d="M 2 1 L 8 5 L 2 9" fill="none" stroke="#a3b0c3" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></marker>
<marker id="arrow-blue" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" orient="auto"><path d="M 2 1 L 8 5 L 2 9" fill="none" stroke="{BLUE}" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></marker>
</defs>
{chr(10).join(parts)}
</svg>
'''


def validate(model):
    factors = model['factors']
    assert ''.join(row['value'] for row in factors) == model['sequence']
    assert len(factors) == model['count']
    assert [row['start'] - 1 for row in factors] + [len(model['sequence'])] == model['boundaries']
    focus = model['focus']
    assert focus['factor'] == focus['match'] + focus['next']
    assert len(focus['next']) == 1
    history = model['sequence'][:focus['start'] - 1]
    offset = focus['source_start'] - 1
    assert history[offset:offset + len(focus['match'])] == focus['match']
    assert focus['factor'] not in history


def binary_diagram(model, x, y, width, *, mobile=False, social=False):
    """One unwrapped string; cubic arcs span successive factor boundaries."""
    factors, focus = model['factors'], model['focus']
    size = 28 if mobile else 42 if not social else 55
    advance = size * .60
    gap = 12 if mobile else 20 if not social else 30
    total = len(model['sequence']) * advance + (len(factors) - 1) * gap
    left = x + (width - total) / 2
    cursor = left
    positions = []
    boundaries = [left - gap / 2]
    for row in factors:
        positions.append(cursor)
        cursor += len(row['value']) * advance
        boundaries.append(cursor + gap / 2)
        cursor += gap
    # The last endpoint is the end of the sequence rather than another separator.
    boundaries[-1] -= gap / 2
    boundaries[0] += gap / 2
    baseline = y + size * .35
    p = []
    source_start, source_end = focus['source_start'] - 1, focus['source_start'] - 1 + len(focus['match'])
    bit_positions = []
    for row, start_x in zip(factors, positions):
        bit_positions.extend(start_x + i * advance for i in range(len(row['value'])))
    # Source highlighting deliberately includes the intervening factor dots.
    source_x, source_width = bit_positions[source_start], bit_positions[source_end - 1] + advance - bit_positions[source_start]
    p += [rect(source_x - 4, y - size * .64, source_width + 8, size * 1.19,
               fill='#eaf1fd', stroke='none', radius=7,
               **{'data-role':'source-highlight', 'data-source-start':focus['source_start'], 'data-source-length':len(focus['match'])})]
    focus_index = next(i for i, row in enumerate(factors) if row['start'] == focus['start'])
    target_x = positions[focus_index]
    p += [rect(target_x - 4, y - size * .64, len(focus['factor']) * advance + 8, size * 1.19,
               fill='#edf6f5', stroke='none', radius=7)]
    focus_peak = y
    for index, row in enumerate(factors):
        above = index % 2 == 0
        sign = -1 if above else 1
        start, end = boundaries[index], boundaries[index + 1]
        depth = (25 if mobile else 35 if not social else 48) + (end - start) * .25
        endpoint_y = y + sign * (2.8 if mobile else 3.5 if not social else 3.8)
        apex_y = y + sign * depth
        if index == focus_index:
            focus_peak = .25 * endpoint_y + .75 * apex_y
        color = BLUE if index == focus_index else '#a3b0c3'
        marker = 'arrow-blue' if index == focus_index else 'arrow-muted'
        d = f'M {start:.2f} {endpoint_y:.2f} C {start:.2f} {apex_y:.2f}, {end:.2f} {apex_y:.2f}, {end:.2f} {endpoint_y:.2f}'
        p += [f'<path {attrs(dict(d=d,fill="none",stroke=color,**{"stroke-width":1.8 if mobile else 2.1 if not social else 2.6,"stroke-linecap":"round","marker-end":f"url(#{marker})","data-role":"factor-arrow","data-start":row["start"],"data-side":"above" if above else "below"}))}/>']
    for index, boundary in enumerate(model['boundaries'][1:-1], 1):
        p += [f'<circle {attrs(dict(cx=boundaries[index],cy=y,r=3 if mobile else 3.5 if not social else 3.7,fill="#7d8da5",**{"data-role":"boundary","data-position":boundary}))}/>']
    for row, start_x in zip(factors, positions):
        spans = []
        start_offset = row['start'] - 1
        # One tspan per bit makes the precise source and extension auditable.
        for index, bit in enumerate(row['value']):
            offset = start_offset + index
            data = {}
            color = INK
            if source_start <= offset < source_end:
                color = BLUE
                data['data-role'] = 'source'
            if row['start'] == focus['start']:
                color = BLUE if index < len(focus['match']) else TEAL
                data['data-role'] = 'match' if index < len(focus['match']) else 'new'
            spans.append(f'<tspan {attrs(dict(fill=color,**data))}>{bit}</tspan>')
        properties = dict(x=start_x,y=baseline,fill=INK,**{'font-family':MONO,'font-size':size,'textLength':len(row['value'])*advance,'lengthAdjust':'spacingAndGlyphs','data-role':'factor','data-start':row['start'],'data-value':row['value']})
        p += [f'<text {attrs(properties)}>{"".join(spans)}</text>']
    return p, positions[focus_index] + len(focus['factor']) * advance / 2, focus_peak


def explanation(model, x, y, width, *, mobile=False, social=False):
    caption = model['focus']['caption']
    separator_text = '; ' if '; ' in caption else '. '
    first, separator, second = caption.partition(separator_text)
    lines = [first + separator, second] if separator else [caption]
    size, spacing = (13,22) if mobile else (15,25) if not social else (21,30)
    p = [rect(x,y,width,78 if mobile else 91 if not social else 100,
              fill='#f2f5fb',stroke='#e4eaf4',radius=13)]
    baseline = y + (29 if mobile else 35 if not social else 39)
    properties = dict(x=x+width/2,y=baseline,fill=MUTED,**{'font-size':size,'font-family':SANS,'text-anchor':'middle','data-role':'explanation','data-value':caption})
    spans = ''.join(f'<tspan x="{x+width/2}" dy="{0 if index == 0 else spacing}">{escape(line)}</tspan>' for index,line in enumerate(lines))
    p += [f'<text {attrs(properties)}>{spans}</text>']
    return p


def trace_figure(model, mobile=False):
    width,height = (360,350) if mobile else (600,410)
    margin = 18 if mobile else 26
    labels, acc = model.get('labels',{}), model.get('accessibility',{})
    p = [rect(.5,.5,width-1,height-1,fill='url(#paper)',radius=20)]
    p += [text(margin,35 if mobile else 42,labels.get('title','LZ76 factorization'),size=18 if mobile else 22,weight=600)]
    badge_x = width - margin - (61 if mobile else 78)
    p += [rect(badge_x,17 if mobile else 21,61 if mobile else 78,28 if mobile else 31,fill='#efedf9',stroke='none',radius=11),
          text(badge_x+(30.5 if mobile else 39),36 if mobile else 42,f'C = {model["count"]}',size=14 if mobile else 16,color=VIOLET,anchor='middle')]
    body,focus_x,focus_peak = binary_diagram(model,margin,137 if mobile else 174,width-margin*2,mobile=mobile)
    p += body
    p += explanation(model,margin,234 if mobile else 281,width-margin*2,mobile=mobile)
    # A small curved leader relates the single annotation to its factor.
    leader_start_y = focus_peak
    leader_end_y = 234 if mobile else 281
    leader_end_x = width / 2
    p += [f'<path d="M {focus_x:.2f} {leader_start_y} C {focus_x:.2f} {leader_start_y+24}, {leader_end_x:.2f} {leader_end_y-24}, {leader_end_x:.2f} {leader_end_y}" fill="none" stroke="#b9cbe7" stroke-width="1.2" stroke-linecap="round"/>']
    return document(width,height,acc.get('title','LZ76 factorization of 1000110011101'),acc.get('description','Dots separate the five factors 1, 0, 001, 1001 and 1101. Smooth arrows alternate above and below the sequence. The blue source 100 crosses earlier factor boundaries; the fourth factor 1001 adds a teal 1.'),p)


def social_figure(model):
    labels,acc = model.get('labels',{}),model.get('accessibility',{})
    p = [rect(0,0,1200,630,fill='url(#paper)',stroke='none',radius=0),
         '<circle cx="1160" cy="0" r="285" fill="#edf0fa"/>',
         rect(72,40,36,36,fill=BLUE,stroke='none',radius=10),
         '<path d="M82 49v17h9m0-17h8l-8 17h8" fill="none" stroke="#fff" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>',
         text(122,65,labels.get('brand','PLEROS'),size=17,weight=600),
         text(70,165,labels.get('product','lzcomplexity'),size=76,weight=600),
         text(74,214,labels.get('subtitle','Entropy · Effective measure complexity · Information distance'),size=25,color=MUTED)]
    body,_,_=binary_diagram(model,74,334,1052,social=True)
    p += body
    p += [text(74,274,labels.get('title','LZ76 factorization'),size=16,color=MUTED),
          text(1126,274,f'C = {model["count"]}',size=18,color=VIOLET,anchor='end')]
    p += explanation(model,240,440,720,social=True)
    p += [text(74,591,labels.get('languages','Python · Rust · Command line'),size=18,color=MUTED),
          text(1126,591,labels.get('url','pleros-ai.github.io/lzcomplexity'),size=16,color=MUTED,anchor='end')]
    return document(1200,630,acc.get('social_title','lzcomplexity · Pleros'),acc.get('social_description','LZ76 factorization: 1, 0, 001, 1001 and 1101. A matching prefix can cross earlier factor boundaries. Entropy, effective measure complexity and information distance.'),p)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--proof-dir',type=Path)
    parser.add_argument('--skip-png',action='store_true')
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    model=json.loads(MODEL.read_text())
    validate(model)
    outputs={'sequence-workflow.svg':trace_figure(model),
             'sequence-workflow-mobile.svg':trace_figure(model,mobile=True),
             'social-card.svg':social_figure(model)}
    if args.check:
        stale=[name for name,source in outputs.items() if (ASSETS/name).read_text()!=source]
        if stale: raise SystemExit('Regenerate factorization figures: '+', '.join(stale))
        print('All three factorization SVGs match the shared binary example.')
        return
    for name,source in outputs.items(): (ASSETS/name).write_text(source)
    if not args.skip_png:
        renderer=shutil.which('rsvg-convert')
        if not renderer: raise SystemExit('rsvg-convert is required to render PNG assets (or pass --skip-png)')
        subprocess.run([renderer,'-o',str(ASSETS/'social-card.png'),str(ASSETS/'social-card.svg')],check=True)
        if args.proof_dir:
            args.proof_dir.mkdir(parents=True,exist_ok=True)
            for source,suffix in [('sequence-workflow.svg','desktop'),('sequence-workflow-mobile.svg','mobile'),('social-card.svg','social')]:
                subprocess.run([renderer,'-o',str(args.proof_dir/f'lz-binary-factorization-{suffix}.png'),str(ASSETS/source)],check=True)


if __name__=='__main__':
    main()
