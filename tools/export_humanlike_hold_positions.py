"""Build-time map content exporter; never sends commands or reads a process.

Optional --route-settings reuses exact saved practice start/expose/retreat by
map/id. Labels, focus, risks and activation evidence must remain explicit.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

RANGES = {None: 'Unknown', 'near': 'Near', 'mid': 'Mid', 'far': 'Far'}
FIELDS = {'id', 'map_id', 'name', 'enabled', 'classification', 'range', 'cover', 'position',
          'retreat', 'focus', 'confidence', 'risk', 'flank_risk', 'exposed_angles', 'source',
          'activation', 'route_ref'}


def number(value, low, high, label):
    if type(value) not in (int, float) or not math.isfinite(value) or not low <= value <= high:
        raise ValueError(f'{label}: finite number in [{low}, {high}] required')
    return struct.unpack('<f', struct.pack('<f', value))[0]


def point(value, label):
    if type(value) is not list or len(value) != 3:
        raise ValueError(f'{label}: three world-feet cm coordinates required')
    return [number(v, -1e7, 1e7, label) for v in value]


def validate(document, route_settings=None):
    if type(document) is not dict or set(document) != {'schema_version', 'revision', 'scope', 'positions'}:
        raise ValueError('exact document fields required')
    if document['schema_version'] != 1 or type(document['revision']) is not int or not 1 <= document['revision'] <= 0xffffffff:
        raise ValueError('invalid version/revision')
    if type(document['scope']) is not str or not document['scope'].strip():
        raise ValueError('scope required')
    positions = document['positions']
    if type(positions) is not list or len(positions) > 256:
        raise ValueError('at most 256 positions')
    seen_ids, seen_points, rows = set(), set(), []
    for original in positions:
        if type(original) is not dict or set(original) != FIELDS:
            raise ValueError('exact position fields required')
        r = dict(original)
        if type(r['id']) is not int or not 1 <= r['id'] <= 0x3fffffff:
            raise ValueError('invalid position id')
        if type(r['map_id']) is not str or not re.fullmatch('[a-z0-9]{1,48}', r['map_id']):
            raise ValueError('invalid exact map id')
        if type(r['name']) is not str or not 1 <= len(r['name'].strip()) <= 120:
            raise ValueError('name required')
        if type(r['enabled']) is not bool or r['classification'] not in ('conventional', 'unconventional'):
            raise ValueError('explicit enabled and author classification required')
        if r['range'] not in RANGES:
            raise ValueError('range must be null/near/mid/far')
        ref = r['route_ref']
        if ref is not None:
            if type(ref) is not dict or set(ref) != {'map_id', 'id'} or ref['map_id'] != r['map_id'] or type(ref['id']) is not int:
                raise ValueError('invalid exact route reference')
            if route_settings is None:
                raise ValueError('--route-settings required for route references')
            route = next((x for x in route_settings.get('routes', {}).get(ref['map_id'], []) if x.get('id') == ref['id']), None)
            if route is None:
                raise ValueError('referenced saved practice route missing')
            if any(r[k] is not None for k in ('cover', 'position', 'retreat')):
                raise ValueError('route coordinates cannot also be silently overridden')
            r.update(cover=route['start'], position=route['expose'], retreat=route['retreat'])
        for k in ('cover', 'position', 'retreat', 'focus'):
            r[k] = point(r[k], k)
        if r['position'] in (r['cover'], r['focus']):
            raise ValueError('hold position must differ from cover and focus')
        r['confidence'] = number(r['confidence'], 0, 1, 'confidence')
        for k in ('risk', 'flank_risk'):
            if r[k] is not None:
                r[k] = number(r[k], 0, 1, k)
        count = r['exposed_angles']
        if count is not None and (type(count) is not int or not 0 <= count <= 16):
            raise ValueError('exposed_angles must be null or integer [0,16]')
        if type(r['source']) is not dict or not r['source'] or type(r['activation']) is not dict or set(r['activation']) != {'status', 'evidence'}:
            raise ValueError('explicit provenance and activation state required')
        if r['enabled'] and (r['activation']['status'] != 'native_geometry_revalidated'
                or type(r['activation']['evidence']) is not str or not r['activation']['evidence'].strip()
                or not r['exposed_angles'] or r['risk'] is None or r['flank_risk'] is None):
            raise ValueError('enabled holds require explicit risk assessment and native revalidation evidence')
        key = (r['map_id'], r['id'])
        physical = (r['map_id'], *r['position'])
        if key in seen_ids or physical in seen_points:
            raise ValueError('duplicate id or duplicate physical hold position; do not relabel one point as two classes')
        seen_ids.add(key); seen_points.add(physical); rows.append(r)
    return rows


def cpp_float(value):
    s = format(value, '.9g')
    return s + ('.0' if '.' not in s and 'e' not in s else '') + 'f'


def generate(document, route_settings=None):
    rows = validate(document, route_settings)
    digest = hashlib.sha256(json.dumps({'document': document, 'resolved_positions': rows}, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    lines = ['#pragma once', '#include "humanlike_bot_space_data.h"', '#include <array>',
             '// Generated map content, not player heat-map facts or runtime proof.', f'// Source/resolved SHA256: {digest}',
             'namespace humanlike_bot_space_data {', f'inline constexpr uint32_t AuthoredRevision={document["revision"]}U;',
             f'inline constexpr std::array<Annotation,{len(rows)}> Authored=[]{{', f'    std::array<Annotation,{len(rows)}> rows{{}};']
    for n, r in enumerate(rows):
        lines.append('    {auto&r=rows[' + str(n) + '];')
        lines.append(f'        r.id={r["id"]}U;const char map[]={json.dumps(r["map_id"])};for(size_t n=0;n<sizeof(map);++n)r.mapId[n]=map[n];')
        lines.append('        r.kind=Kind::Hold;')
        for cpp, key in [('start', 'cover'), ('open', 'position'), ('retreat', 'retreat'), ('focus', 'focus')]:
            lines.append('        r.' + cpp + '={' + ','.join(cpp_float(v) for v in r[key]) + '};')
        lines.append(f'        r.focusKnown=true;r.enabled={str(r["enabled"]).lower()};r.conventional={str(r["classification"] == "conventional").lower()};')
        lines.append(f'        r.range=humanlike_bot::HoldRange::{RANGES[r["range"]]};r.authoredConfidence={cpp_float(r["confidence"])};')
        for key, cpp, known in [('risk', 'authoredRisk', 'riskKnown'), ('flank_risk', 'authoredFlankRisk', 'flankRiskKnown')]:
            lines.append(f'        r.{known}={str(r[key] is not None).lower()};r.{cpp}={cpp_float(r[key] if r[key] is not None else 1)};')
        lines.append(f'        r.exposedAnglesKnown={str(r["exposed_angles"] is not None).lower()};r.authoredExposedAngles={r["exposed_angles"] or 0}U;}}')
    lines += ['    return rows;', '}();', '} // namespace humanlike_bot_space_data', '']
    return '\n'.join(lines), {'source_resolved_sha256': digest, 'revision': document['revision'], 'positions': len(rows),
                              'enabled': sum(r['enabled'] for r in rows), 'map_ids': sorted({r['map_id'] for r in rows}),
                              'not_native_validation': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--route-settings', type=Path)
    args = parser.parse_args()
    routes = json.loads(args.route_settings.read_text(encoding='utf-8')) if args.route_settings else None
    header, result = generate(json.loads(args.source.read_text(encoding='utf-8')), routes)
    args.output.write_text(header, encoding='utf-8', newline='\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
