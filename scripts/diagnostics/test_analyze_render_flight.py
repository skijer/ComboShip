#!/usr/bin/env python3
import json
import unittest
from analyze_render_flight import analyze

class FlightAnalysisTest(unittest.TestCase):
    def test_skips_stalls_gpu_correlation_and_context_boundary(self):
        base={'scene':81,'room':0,'age':1,'alt_assets':True,'paused':False,'context_id':1,
              'tick_id':1,'start_ns':0,'end_ns':1000000,'presented':True,'detail_mode':2}
        first={**base,'id':1}
        skip={**base,'id':2,'presented':False,'pacing':{'reason':'scheduler_late'}}
        second={**base,'id':3,'start_ns':400000000,'end_ns':450000000,'tick_id':8,
                'gpu':{'samples':[{'frame_id':1,'gpu_ms':4.5}]}}
        transition={**base,'id':4,'context_id':2,'start_ns':9000000000,'end_ns':9001000000}
        capture={'attempts':[first,skip,second,transition],'ticks':[{'id':1,'wall_ms':458,'trace':{'events':[]}}]}
        result=analyze('[RenderFlightConfiguration] {}\n[FrameFlightRecorder] '+json.dumps(capture))
        scene=result['scenes'][0]
        self.assertEqual(scene['skipped'],1)
        self.assertEqual(scene['cpu_submission_interval_ms']['max'],449.0)
        self.assertEqual(scene['gpu_samples'],1)
        self.assertEqual(scene['gpu_ms_p95'],4.5)
        self.assertEqual(result['slow_ticks'][0]['wall_ms'],458)
    def test_missing_and_truncated_capture(self):
        r=analyze('[RenderFlightState] {"event":"capture_limit"}\n[FrameFlightRecorder] broken')
        self.assertEqual(r['coverage']['attempts'],0)
        self.assertEqual(r['coverage']['errors']['capture_limit_events'],1)
        self.assertEqual(r['coverage']['errors']['malformed_records'],1)

if __name__=='__main__': unittest.main()
