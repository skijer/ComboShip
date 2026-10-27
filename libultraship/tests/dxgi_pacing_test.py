#!/usr/bin/env python3
"""Exercise production IsFrameReady against deterministic DXGI statistics.
Usage: python3 dxgi_pacing_test.py /path/to/nlohmann/include
Does not replace native Windows build or presentation runtime verification.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
source = (Path(__file__).resolve().parents[1] / 'src/fast/backends/gfx_dxgi.cpp').read_text()
methods = source[source.index('void GfxWindowBackendDXGI::SetCollectPacingTelemetry('):
                 source.index('void GfxWindowBackendDXGI::SwapBuffersBegin(')]
harness = r'''
#include <nlohmann/json.hpp>
#include <cassert>
#include <map>
#include <set>
#include <cmath>
using HRESULT = int32_t;
using UINT = uint32_t;
constexpr HRESULT S_OK = 0, E_FAIL = -1;
struct LARGE_INTEGER { int64_t QuadPart = 0; };
struct DXGI_FRAME_STATISTICS {
    UINT PresentCount = 0, PresentRefreshCount = 0, SyncRefreshCount = 0;
    LARGE_INTEGER SyncQPCTime, SyncGPUTime;
};
constexpr uint64_t qpc_freq = 1000000000, qpc_init = 0;
uint64_t qpc_to_ns(uint64_t qpc) { return qpc; }
void QueryPerformanceCounter(LARGE_INTEGER* qpc) { qpc->QuadPart = 100000000; }
#define FRAME_INTERVAL_NS_NUMERATOR 1000000000
#define FRAME_INTERVAL_NS_DENOMINATOR (mTargetFps)
struct SwapChain {
    HRESULT result = E_FAIL;
    DXGI_FRAME_STATISTICS stats;
    HRESULT GetFrameStatistics(DXGI_FRAME_STATISTICS* out) { *out = stats; return result; }
};
class GfxWindowBackendDXGI {
public:
    bool mCollectPacingTelemetry = false;
    nlohmann::json mPacingTelemetry;
    uint32_t mTargetFps = 60, mMaxFrameLatency = 1;
    double mDetectedHz = 60;
    bool mVsyncEnabled = true, mTearingSupport = false, mDroppedFrame = false;
    uint64_t mFrameTimeStamp = 0;
    std::map<UINT, DXGI_FRAME_STATISTICS> mFrameStats;
    std::set<std::pair<UINT, UINT>> mPendingFrameStats;
    SwapChain* swap_chain;
    void SetCollectPacingTelemetry(bool);
    nlohmann::json GetPacingTelemetry();
    void RecordFrameStatistics(const DXGI_FRAME_STATISTICS&, HRESULT);
    bool IsFrameReady();
};
void seed(GfxWindowBackendDXGI& g) {
    g.mFrameStats.clear(); g.mPendingFrameStats.clear();
    g.mFrameTimeStamp = 0;
    g.mFrameStats.emplace(1, DXGI_FRAME_STATISTICS{1, 1, 1, {16666666}, {0}});
    g.mFrameStats.emplace(2, DXGI_FRAME_STATISTICS{2, 2, 2, {33333332}, {0}});
}
'''
tests = r'''
int main() {
    SwapChain swap; GfxWindowBackendDXGI g; g.swap_chain = &swap;
    g.SetCollectPacingTelemetry(true);
    assert(g.IsFrameReady());
    auto first = g.GetPacingTelemetry();
    assert(first["ready"] == true && first["frame_statistics_status"] == "unavailable");
    assert(first["observed_queue_end_ns"].is_null() && first["present_id"].is_null());
    seed(g);
    assert(!g.IsFrameReady());
    auto late = g.GetPacingTelemetry();
    assert(late["reason"] == "scheduler_late" && late["scheduler_reset"] == false);
    assert(late["observed_queue_end_ns"] == 33333332);
    seed(g);
    g.mFrameTimeStamp = uint64_t(18000000) * 60;
    assert(!g.IsFrameReady());
    assert(g.GetPacingTelemetry()["reason"] == "rounded_vsync_zero");
    seed(g);
    g.mFrameStats[1].SyncQPCTime.QuadPart = 1000000000;
    g.mFrameStats[2].SyncQPCTime.QuadPart = 1016666666;
    assert(g.IsFrameReady());
    auto reset = g.GetPacingTelemetry();
    assert(reset["scheduler_reset"] == true && reset["ready"] == true);
    assert(reset["effective_timestamp_ns"] != reset["requested_timestamp_ns"]);
    // Per-attempt fields cannot inherit the previous Present or reset marker.
    g.mPacingTelemetry["present_id"] = 99;
    g.mFrameStats.clear(); g.mPendingFrameStats.clear();
    assert(g.IsFrameReady());
    assert(g.GetPacingTelemetry()["present_id"].is_null());
    assert(g.GetPacingTelemetry()["scheduler_reset"] == false);
    swap.result = S_OK;
    swap.stats = {10, 11, 12, {123456}, {0}};
    g.IsFrameReady();
    assert(g.GetPacingTelemetry()["readiness_refresh_statistics"]["present_count"] == 10);
    // Instrumentation does not change the scheduler's decisions or state.
    for (uint64_t offset : {0ULL, 18000000ULL, 100000000ULL}) {
        GfxWindowBackendDXGI plain, traced;
        plain.swap_chain = traced.swap_chain = &swap;
        swap.result = E_FAIL;
        seed(plain); seed(traced);
        plain.mFrameTimeStamp = traced.mFrameTimeStamp = offset * 60;
        traced.SetCollectPacingTelemetry(true);
        assert(plain.IsFrameReady() == traced.IsFrameReady());
        assert(plain.mFrameTimeStamp == traced.mFrameTimeStamp);
        assert(plain.mDroppedFrame == traced.mDroppedFrame);
    }
    g.SetCollectPacingTelemetry(false);
    assert(g.GetPacingTelemetry()["status"] == "disabled");
}
'''
with tempfile.TemporaryDirectory(prefix='dxgi-pacing-test-') as temp:
    cpp = Path(temp) / 'test.cpp'; cpp.write_text(harness + methods + tests)
    exe = Path(temp) / 'test'
    # The existing scheduler contains unused diagnostic locals; preserve its code.
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-variable',
                    '-I', sys.argv[1], str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('DXGI pacing tests passed (production scheduler, fake statistics).')
