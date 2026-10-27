#!/usr/bin/env python3
"""Compile the production DX11 timing methods against a deterministic query provider.

Usage: python3 gpu_timing_query_test.py /path/to/nlohmann/include
This tests lifecycle/retirement without Windows; it does not replace a Windows build
or GPU driver/runtime validation.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

source = (Path(__file__).resolve().parents[1] / 'src/fast/backends/gfx_direct3d11.cpp').read_text()
methods = source[source.index('void GfxRenderingAPIDX11::RecordGpuTiming('):
                 source.index('GfxRenderingAPIDX11::GfxRenderingAPIDX11(')]
harness = r'''
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstdint>
#include <memory>
using HRESULT = int32_t;
using UINT64 = uint64_t;
constexpr HRESULT S_OK = 0, S_FALSE = 1, E_FAIL = -1;
#define FAILED(hr) ((hr) < 0)
#define SUCCEEDED(hr) ((hr) >= 0)
constexpr unsigned D3D11_QUERY_TIMESTAMP_DISJOINT = 1, D3D11_QUERY_TIMESTAMP = 2;
constexpr unsigned D3D11_ASYNC_GETDATA_DONOTFLUSH = 1;
struct D3D11_QUERY_DESC { unsigned Query, MiscFlags; };
struct D3D11_QUERY_DATA_TIMESTAMP_DISJOINT { uint64_t Frequency; bool Disjoint; };
struct ID3D11Query {
    static int live;
    unsigned kind;
    uint64_t tick = 0;
    ID3D11Query() { ++live; }
    ~ID3D11Query() { --live; }
};
int ID3D11Query::live = 0;
template<class T> struct ComPtr {
    T* ptr = nullptr;
    ~ComPtr() { delete ptr; }
    ComPtr() = default;
    ComPtr(ComPtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    ComPtr& operator=(ComPtr&& other) { delete ptr; ptr = other.ptr; other.ptr = nullptr; return *this; }
    explicit operator bool() const { return ptr != nullptr; }
    T* Get() { return ptr; }
    T** GetAddressOf() { assert(!ptr); return &ptr; }
    T* operator->() { return ptr; }
};
struct Device {
    bool fail = false;
    int creates = 0;
    HRESULT CreateQuery(const D3D11_QUERY_DESC* desc, ID3D11Query** out) {
        ++creates;
        if (fail) return E_FAIL;
        *out = new ID3D11Query;
        (*out)->kind = desc->Query;
        return S_OK;
    }
};
struct Context {
    HRESULT result = S_FALSE;
    bool disjoint = false;
    int reads = 0, begins = 0, ends = 0;
    uint64_t tick = 100;
    void Begin(ID3D11Query*) { ++begins; }
    void End(ID3D11Query* q) { ++ends; q->tick = tick; tick += 100; }
    HRESULT GetData(ID3D11Query* q, void* out, unsigned, unsigned flags) {
        assert(flags == D3D11_ASYNC_GETDATA_DONOTFLUSH);
        ++reads;
        if (result != S_OK) return result;
        if (q->kind == D3D11_QUERY_TIMESTAMP_DISJOINT)
            *static_cast<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT*>(out) = {100000, disjoint};
        else *static_cast<uint64_t*>(out) = q->tick;
        return S_OK;
    }
};
class GfxRenderingAPIDX11 {
public:
    struct GpuTimingSlot {
        ComPtr<ID3D11Query> disjoint, start, end;
        uint64_t frameId = 0;
        bool pending = false;
    };
    GpuTimingSlot mGpuTimingSlots[8];
    int mActiveGpuTimingSlot = -1;
    bool mGpuTimingEnabled = false;
    uint64_t mGpuTimingFrameId = 0, mGpuTimingDropped = 0;
    const char* mGpuTimingStatus = "disabled";
    nlohmann::json mGpuTimingSamples = nlohmann::json::array();
    Device* mDevice;
    Context* mContext;
    void RecordGpuTiming(uint64_t, const char*, nlohmann::json = nullptr, nlohmann::json = nullptr);
    void ResetGpuTiming();
    void PollGpuTiming();
    void BeginGpuTiming(uint64_t, bool);
    void EndGpuTiming();
    nlohmann::json GetGpuTimingTelemetry();
};
'''
tests = r'''
int main() {
    Device d; Context c; GfxRenderingAPIDX11 g;
    g.mDevice = &d; g.mContext = &c;
    g.BeginGpuTiming(0, false);
    assert(c.reads == 0 && d.creates == 0);
    for (uint64_t i = 1; i <= 8; ++i) { g.BeginGpuTiming(i, true); g.EndGpuTiming(); }
    assert(ID3D11Query::live == 24);
    g.BeginGpuTiming(9, true); g.EndGpuTiming();
    auto full = g.GetGpuTimingTelemetry();
    assert(full["status"] == "dropped" && full["pending_count"] == 8);
    assert(full["samples"][0]["frame_id"] == 9 && full["samples"][0]["gpu_ms"].is_null());
    assert(d.creates == 24);
    c.result = S_OK;
    auto retired = g.GetGpuTimingTelemetry();
    assert(retired["pending_count"] == 0 && retired["samples"].size() == 8);
    for (size_t i = 0; i < 8; ++i) {
        assert(retired["samples"][i]["frame_id"] == i + 1);
        assert(retired["samples"][i]["status"] == "available");
        assert(retired["samples"][i]["gpu_ms"] == 1.0);
    }
    assert(g.GetGpuTimingTelemetry()["samples"].empty()); // results consumed once
    g.BeginGpuTiming(10, true); g.EndGpuTiming(); c.disjoint = true;
    auto disjoint = g.GetGpuTimingTelemetry();
    assert(disjoint["status"] == "disjoint");
    assert(disjoint["samples"][0]["gpu_ms"].is_null());
    c.disjoint = false;
    g.BeginGpuTiming(11, true); g.EndGpuTiming(); c.result = E_FAIL;
    auto error = g.GetGpuTimingTelemetry();
    assert(error["samples"][0]["status"] == "error");
    assert(error["pending_count"] == 0 && ID3D11Query::live == 21);
    d.fail = true;
    g.BeginGpuTiming(12, true);
    assert(g.GetGpuTimingTelemetry()["status"] == "error");
    d.fail = false; c.result = S_FALSE;
    g.BeginGpuTiming(13, true); // disabling also closes an active disjoint query
    auto ends = c.ends;
    g.BeginGpuTiming(14, false);
    assert(c.ends == ends + 2 && ID3D11Query::live == 0);
    auto reads = c.reads;
    assert(g.GetGpuTimingTelemetry()["status"] == "disabled" && c.reads == reads);
    g.BeginGpuTiming(15, true); g.EndGpuTiming(); c.result = S_OK;
    assert(g.GetGpuTimingTelemetry()["samples"][0]["frame_id"] == 15);
    g.ResetGpuTiming();
    assert(ID3D11Query::live == 0);
}
'''
with tempfile.TemporaryDirectory(prefix='gpu-query-test-') as temp:
    cpp = Path(temp) / 'test.cpp'
    cpp.write_text(harness + methods + tests)
    exe = Path(temp) / 'test'
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', sys.argv[1], str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('GPU query lifecycle tests passed (production methods, fake D3D provider).')
