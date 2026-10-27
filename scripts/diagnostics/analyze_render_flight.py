#!/usr/bin/env python3
"""Read one complete OoT-RenderFlight-*.log; summarize continuous evidence and gaps.

No third-party dependencies. Raw records remain the authority; CPU submission
intervals are not physical display timestamps, and nested costs are not additive.
"""
import argparse
import collections
import json
import math
from pathlib import Path


def percentile(values, fraction):
    if not values:
        return None
    return sorted(values)[max(0, math.ceil(fraction * len(values)) - 1)]


def analyze(text):
    attempts, ticks, renders, configurations = [], [], [], []
    errors = collections.Counter()
    for line in text.splitlines():
        if '[RenderFlightState] ' in line: errors["capture_limit_events"] += 1
        for marker, output in [("[FrameFlightRecorder] ", None), ("[RenderCostProbe] ", renders),
                               ("[RenderFlightConfiguration] ", configurations)]:
            if marker not in line:
                continue
            try:
                data = json.loads(line.split(marker, 1)[1])
            except (ValueError, IndexError):
                errors['malformed_records'] += 1
                continue
            if output is not None:
                output.append(data)
            else:
                attempts.extend(data.get('attempts', [])); ticks.extend(data.get('ticks', []))
                for key in ['attempt_overflow', 'tick_overflow']:
                    errors[key] += data.get(key, 0)
                errors['diagnostic_errors'] = max(errors['diagnostic_errors'], data.get('diagnostic_errors', 0))
    groups = collections.defaultdict(list)
    for row in attempts:
        groups[(row['scene'], row['room'], row['age'], row['alt_assets'], row['paused'])].append(row)
    summaries = []
    gpu_by_id = {}
    for row in attempts:
        for sample in (row.get('gpu') or {}).get('samples', []):
            if sample.get('gpu_ms') is not None: gpu_by_id[sample['frame_id']] = sample['gpu_ms']
    for key, rows in groups.items():
        rows.sort(key=lambda r: r.get('start_ns', 0))
        drawn = [r for r in rows if r.get('presented')]
        # Reset interval comparison on each context transition or skipped batch gap.
        intervals = []
        for a, b in zip(drawn, drawn[1:]):
            if b.get('context_id') == a.get('context_id'):
                intervals.append((b['end_ns']-a['end_ns'])/1e6)
        repeated = 0
        for a, b in zip(drawn, drawn[1:]):
            if a['tick_id']==b['tick_id'] and a.get('matrix_digest') is not None and a.get('matrix_digest')==b.get('matrix_digest') and a.get('matrix_count',0)>0:
                repeated += 1
        gpu = [gpu_by_id[r['id']] for r in rows if r['id'] in gpu_by_id]
        sums = lambda field: sum(r.get(field, 0) for r in drawn) / len(drawn) if drawn else None
        summaries.append({'scene_room_age_alt_paused': key, 'attempts': len(rows), 'presented':len(drawn),
            'skipped':len(rows)-len(drawn), 'skip_reasons':dict(collections.Counter((r.get('pacing') or {}).get('reason','unavailable') for r in rows if not r.get('presented'))),
            'cpu_submission_interval_ms':{'p50':percentile(intervals,.5),'p95':percentile(intervals,.95),'p99':percentile(intervals,.99),'max':max(intervals,default=None)},
            'commands_ms_mean':sums('commands_ms'), 'present_ms_mean':sums('present_ms'),
            'gpu_ms_p95':percentile(gpu,.95), 'gpu_samples':len(gpu),
            'repeated_matrix_pose_pairs_same_tick':repeated,
            'detail_modes':sorted(set(r.get('detail_mode',-1) for r in rows))})
    resource_events = []
    for tick in ticks:
        if tick.get('trace') is None: errors['missing_tick_trace'] += 1
        trace = tick.get('trace') or {}
        for key in ['overflow_events','overflow_aggregates','overflow_counters','overflow_phases','overflow_actors','collector_errors']:
            errors[key] += trace.get(key,0)
        resource_events.extend(e for e in trace.get('events',[]) if e.get('category','').startswith('resource.'))
    slow = sorted(ticks,key=lambda r:r.get('wall_ms',0),reverse=True)[:20]
    return {'coverage':{'configurations':len(configurations),'attempts':len(attempts),'ticks':len(ticks),
                       'detailed_draws':len(renders),'resource_events':len(resource_events),'errors':dict(errors)},
            'scenes':summaries,'slow_ticks':slow,
            'slow_resource_events':sorted(resource_events,key=lambda e:e.get('duration_ns',0),reverse=True)[:30],
            'semantics':'CPU submission intervals, not scanout; GPU results retain source frame IDs; nested phases overlap; unmatched camera/content prevents causal before/after claims'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    parser.add_argument('--require-complete',action='store_true')
    args=parser.parse_args()
    result=analyze(args.capture.read_text(errors='replace'))
    print(json.dumps(result,indent=2))
    c=result['coverage']
    return int(args.require_complete and (not c['attempts'] or not c['ticks'] or not c['configurations'] or any(c['errors'].values())))

if __name__=='__main__':
    raise SystemExit(main())
