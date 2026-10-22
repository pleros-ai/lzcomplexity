#!/usr/bin/env python3
"""Check the binary factorization and the boundaries drawn in each figure."""
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
MODEL = json.loads((ROOT / 'tests/docs/factorization-visual.json').read_text())
SEQUENCE, FACTORS, FOCUS = MODEL['sequence'], MODEL['factors'], MODEL['focus']
BLUE, TEAL = '#4779c5', '#208a80'

# Independent longest-previous-match search, including overlapping copies.
position, boundaries, values = 0, [0], []
while position < len(SEQUENCE):
    longest = 0
    for previous in range(position):
        length = 0
        while position + length < len(SEQUENCE) and SEQUENCE[previous + length] == SEQUENCE[position + length]:
            length += 1
        longest = max(longest, length)
    end = position + longest + 1
    assert end <= len(SEQUENCE), 'This illustration should contain only complete factors.'
    values.append(SEQUENCE[position:end])
    boundaries.append(end)
    position = end
assert values == [factor['value'] for factor in FACTORS]
assert boundaries == MODEL['boundaries']
assert len(values) == MODEL['count']

# The annotated match crosses earlier boundaries and lies entirely in history.
history = SEQUENCE[:FOCUS['start'] - 1]
source_start = FOCUS['source_start'] - 1
source_end = source_start + len(FOCUS['match'])
assert history[source_start:source_end] == FOCUS['match']
assert sum(source_start < boundary < source_end for boundary in boundaries) == 2
assert FOCUS['factor'] == FOCUS['match'] + FOCUS['next']
assert FOCUS['factor'] not in history


def literal(element):
    return ''.join(element.itertext())


def select(root, role):
    return [element for element in root.iter() if element.get('data-role') == role]


for name in ('sequence-workflow.svg', 'sequence-workflow-mobile.svg', 'social-card.svg'):
    root = ET.parse(ROOT / 'docs/assets' / name).getroot()
    drawn = select(root, 'factor')
    assert len(drawn) == MODEL['count']
    for element, factor in zip(drawn, FACTORS):
        assert element.get('data-start') == str(factor['start'])
        assert element.get('data-value') == factor['value']
        assert literal(element) == factor['value']
    dots = select(root, 'boundary')
    assert [int(dot.get('data-position')) for dot in dots] == boundaries[1:-1]
    axis = float(dots[0].get('cy'))
    edge_x = [float(drawn[0].get('x'))] + [float(dot.get('cx')) for dot in dots]
    edge_x.append(float(drawn[-1].get('x')) + float(drawn[-1].get('textLength')))
    arrows = select(root, 'factor-arrow')
    assert len(arrows) == MODEL['count']
    for index, (arrow, factor) in enumerate(zip(arrows, FACTORS)):
        side = 'above' if index % 2 == 0 else 'below'
        assert arrow.get('data-start') == str(factor['start'])
        assert arrow.get('data-side') == side
        assert arrow.get('marker-end'), 'Every curve needs an arrowhead.'
        assert 'C' in arrow.get('d'), 'Use a smooth cubic curve.'
        coordinates = [float(value) for value in re.findall(r'-?\d+(?:\.\d+)?', arrow.get('d'))]
        assert len(coordinates) == 8
        x0, y0, _, control_y0, _, control_y1, x1, y1 = coordinates
        assert abs(x0 - edge_x[index]) < 0.02 and abs(x1 - edge_x[index + 1]) < 0.02
        assert abs(y0 - axis) <= 4 and abs(y1 - axis) <= 4, 'Curves must connect to the boundary dots.'
        sign = -1 if side == 'above' else 1
        assert sign * (control_y0 - axis) > 0 and sign * (control_y1 - axis) > 0
    explanations = select(root, 'explanation')
    assert len(explanations) == 1
    assert ' '.join(literal(explanations[0]).split()) == FOCUS['caption']
    sources = select(root, 'source')
    assert ''.join(literal(element) for element in sources) == FOCUS['match']
    assert all(element.get('fill') == BLUE for element in sources)
    matches, extensions = select(root, 'match'), select(root, 'new')
    assert ''.join(literal(element) for element in matches) == FOCUS['match']
    assert all(element.get('fill') == BLUE for element in matches)
    assert len(extensions) == 1 and literal(extensions[0]) == FOCUS['next'] and extensions[0].get('fill') == TEAL
    for element, factor in zip(drawn, FACTORS):
        offset = factor['start'] - 1
        for span in element:
            if span.get('data-role') == 'source':
                assert source_start <= offset < source_end
            offset += len(literal(span))

print('Five binary factors, four boundary dots, alternating curved arrows and the cross-boundary 100 match are correct in all three figures.')
