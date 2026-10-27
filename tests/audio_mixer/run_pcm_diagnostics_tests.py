#!/usr/bin/env python3
"""Exercise the actual observer host and gain function; no user music or game boot."""
import os
from pathlib import Path
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_audio_runtime_test import function_body

source = (ROOT / 'mm/2s2h/mixer.c').read_text()
start, end = function_body(source, 'aHiLoGainImpl')
fixture = r'''
#define COMBO_BUILD
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
std::vector<std::string> logs;
template<class... T> void Log(const T&... args) {
    std::ostringstream stream; (stream << ... << args); logs.push_back(stream.str());
}
#define SPDLOG_INFO(...) Log(__VA_ARGS__)
#define SPDLOG_ERROR(...) Log(__VA_ARGS__)
int traceCvar=0, cleanPositional=0;
int CVarGetInteger(const char* name, int value) {
    if (!std::strcmp(name, "gDeveloperTools.MMAudioPCMTrace")) return traceCvar;
    if (!std::strcmp(name, "gEnhancements.Fixes.MMStreamedPositionalAudio")) return cleanPositional;
    return value;
}
#include "combo/audio/MMAudioTraceHost.h"
extern "C" void MM_LogAudioGainMode(int, int, int, int) {}
int16_t buffer[1024];
#define BUF_S16(addr) (buffer + ((addr)-0x330)/2)
#define ROUND_UP_32(v) (((v)+31)&~31)
#define DMEM_BUF_SIZE sizeof(buffer)
int16_t clamp16(int32_t x) { return std::max(-32768, std::min(32767, static_cast<int>(x))); }
'''
# The log helper uses the same standard declaration as the actual host.
fixture = fixture.replace('#include <fstream>', '#include <fstream>\n#include <sstream>\n#include <algorithm>')
fixture += source[start:end]
start, end = function_body(source, 'aAudioTraceDMemImpl')
fixture += source[start:end]
fixture += r'''
bool Has(const std::string& value) {
    for (const auto& line : logs) if (line.find(value) != std::string::npos) return true;
    return false;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    MM_AudioTraceResource(1, 123, "Shop_44.xml", nullptr, 1);
    MM_AudioTraceResource(0, 456, "Replacement.xml", "shop.ogg", 1);
    MM_AudioTraceFormat(456, 44100, 2, 88200);
    MM_AudioTraceAlias(1, 124, 123); // the game uses a relocated sequence cache copy.
    auto& trace = MMAudioTrace::WorkerTrace();
    MMAudioTraceNote note{};
    note.note=0; note.player=0; note.sequence=44; note.font=12; note.codec=1;
    note.sample=456; note.sequenceData=124; note.samples=16; note.gain=127;
    note.tuning=1.378125f;
    int16_t baseline[1024];
    for (int mode=0; mode<2; ++mode) {
        cleanPositional=mode; trace.SetEnabled(false);
        std::fill_n(buffer, 1024, 10000);
        aHiLoGainImpl(127, 64, 0x3B0);
        std::memcpy(baseline, buffer, sizeof(buffer));
        trace.SetEnabled(true); MM_AudioTraceSetNote(&note);
        std::fill_n(buffer, 1024, 10000);
        aHiLoGainImpl(127, 64, 0x3B0);
        assert(std::memcmp(baseline, buffer, sizeof(buffer)) == 0);
        trace.Flush();
        assert(Has("wouldClip=16")); // excludes command padding and legacy spill.
    }
    note.note=0; note.player=2; note.sample=789; // actual slot reused by SFX, preserve the earlier BGM record.
    MM_AudioTraceSetNote(&note);
    MM_AudioTracePCM(MM_TRACE_DECODED, buffer, 16, 0);
    trace.Flush();
    assert(Has("player=0 seq=44") && Has("player=2 seq=44"));
    assert(Has("shop.ogg streamed=1 pcmRate=44100 sourceChannels=2 sourceFrames=88200") &&
           Has("sampleResource=unregistered"));
    // Pitch changes close the previous interval rather than labeling it with
    // the last pitch seen. The snapshot describes the samples actually traced.
    note.pitch=16384; MM_AudioTraceSetNote(&note);
    MM_AudioTracePCM(MM_TRACE_PROCESSED, buffer, 16, 0);
    note.pitch=32768; MM_AudioTraceSetNote(&note);
    assert(Has("pitch=16384"));
    trace.Flush();
    aAudioTraceDMemImpl(MM_TRACE_PROCESSED, 0x330, 16);
    aAudioTraceDMemImpl(MM_TRACE_PROCESSED, 0x32e, 16);
    aAudioTraceDMemImpl(MM_TRACE_PROCESSED, 0xffff, 16);
    aAudioTraceDMemImpl(MM_TRACE_PROCESSED, 0x330, UINT32_MAX);
    trace.Flush();
    assert(Has("pitch=32768") && Has("stage=processed samples=16"));
    MMAudioTrace::Stats a, b;
    a.Add(buffer, 16); b.Add(buffer, 8); b.Add(buffer+8, 8);
    assert(a.fingerprint == b.fingerprint && a.count == b.count);
    trace.SetEnabled(false);
    const auto logCount=logs.size();
    MM_AudioTraceSetNote(&note); MM_AudioTracePCM(MM_TRACE_DECODED, buffer, 16, 0);
    MM_AudioTraceEvent("disabled", 1, 0);
    assert(logs.size() == logCount);
    trace.SetEnabled(true);
    MMAudioTrace::Queue(0, 3); // initial fill is not counted as starvation.
    std::vector<int16_t> pcm(64000);
    for(size_t i=0;i<pcm.size();i+=2) { pcm[i]=1000; pcm[i+1]=-2000; }
    MMAudioTrace::Native(pcm.data(), 32000);
    MMAudioTrace::Final(pcm.data(), 32000, false);
    assert(Has("emptyQueueObservations=0"));
    MMAudioTrace::Queue(0, 75);
    MM_AudioTraceEvent("opus-read-failed", 64, 0);
    MMAudioTrace::Final(pcm.data(), 32000, false);
    assert(Has("emptyQueueObservations=1") && Has("maxWorkerWakeGapMs=75"));
    assert(Has("event=opus-read-failed"));
    assert(Has("stage=device-L samples=32000 peak=1000") && Has("stage=device-R samples=32000 peak=2000"));
    for(int i=0;i<25;++i) MMAudioTrace::Final(pcm.data(), 32000, false);
    assert(trace.capture.size() == 32000*2*20);
    trace.SetEnabled(false); trace.SetEnabled(true);
    assert(trace.capture.size() == 32000*2*20); // A/B toggles retain unsaved evidence.
    MMAudioTrace::SaveCapture(argv[1]);
    assert(trace.capture.empty());
    unsigned wavs=0;
    for(const auto& entry:std::filesystem::directory_iterator(argv[1])) {
        std::ifstream f(entry.path(), std::ios::binary);
        std::vector<unsigned char> bytes(48); f.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        assert(std::memcmp(bytes.data(), "RIFF", 4) == 0);
        assert(bytes[22]==2 && bytes[24]==0 && bytes[25]==125);
        assert(bytes[44]==232 && bytes[45]==3 && bytes[46]==48 && bytes[47]==248);
        assert(std::filesystem::file_size(entry.path()) == 44+32000*2*20*2);
        ++wavs;
    }
    assert(wavs==1);
    trace.SetEnabled(false); trace.SetEnabled(true);
    for(int i=0;i<1000;++i) MM_AudioTraceEvent("burst", 1, 0);
    logs.clear();
    MMAudioTrace::Native(pcm.data(), 16);
    MMAudioTrace::Final(pcm.data(), 16, false);
    trace.Flush(); assert(Has("suppressedRecords=872"));
    assert(Has("stage=native-L") && Has("stage=device-R"));
    std::cout << "PCM diagnostics: passive gain, BGM/SFX attribution, stages, queue, bounded logs/capture and WAV PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-pcm-diagnostics-') as temporary:
    build = Path(temporary)
    path = build / 'trace.cpp'
    path.write_text(fixture)
    binary = build / 'trace'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie',
                    '-I'+str(ROOT), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(build / 'captures')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
    # Execute actual per-subupdate ownership capture with real MM audio types.
    synthesis = (ROOT / 'mm/src/audio/lib/synthesis.c').read_text()
    preamble = (ROOT / 'mm/tests/audio_stream_runtime_test.c').read_text().split('#include "seqcmd.h"')[0]
    ctx_start, ctx_end = function_body(synthesis, 'AudioSynth_TraceContext')
    sync_start, sync_end = function_body(synthesis, 'AudioSynth_SyncSampleStates')
    context = '''
#define COMBO_BUILD
#include "combo/audio/MMAudioTraceBridge.h"
#include <assert.h>
#include <stdio.h>
AudioContext gAudioCtx;
static MMAudioTraceNote sAudioTraceContexts[1024];
int MM_AudioTraceEnabled(void) { return 1; }
'''
    context += synthesis[ctx_start:ctx_end] + synthesis[sync_start:sync_end]
    context += '''
int main(void) {
    Note notes[1]={0}; NoteSampleState states[2]={0};
    SequenceLayer layer={0}; SequenceChannel channel={0};
    gAudioCtx.numNotes=1; gAudioCtx.notes=notes; gAudioCtx.sampleStateList=states;
    layer.channel=&channel; notes[0].playbackState.parentLayer=&layer;
    states[0].bitField1.isSyntheticWave=states[1].bitField1.isSyntheticWave=1;
    channel.seqPlayer=&gAudioCtx.seqPlayers[0]; channel.seqPlayer->playerIndex=0;
    channel.seqPlayer->seqId=44; AudioSynth_SyncSampleStates(0);
    channel.seqPlayer=&gAudioCtx.seqPlayers[2]; channel.seqPlayer->playerIndex=2;
    channel.seqPlayer->seqId=0; AudioSynth_SyncSampleStates(1);
    assert(sAudioTraceContexts[0].player==0 && sAudioTraceContexts[0].sequence==44);
    assert(sAudioTraceContexts[1].player==2 && sAudioTraceContexts[1].sequence==0);
    notes[0].playbackState.parentLayer=NO_LAYER; AudioSynth_SyncSampleStates(1);
    assert(sAudioTraceContexts[1].player==-1);
    puts("PCM production sub-update attribution: note reuse and released owner PASS");
}
'''
    path = build / 'contexts.c'; path.write_text(preamble + context)
    binary = build / 'contexts'
    subprocess.run([os.environ.get('CC','cc'), '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(ROOT), '-I'+str(ROOT / 'mm/tests'), '-I'+str(ROOT / 'mm/include'), '-I'+str(ROOT / 'libultraship/include'),
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie',
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
