#!/usr/bin/env python3
"""Behavioral MM SFX clock regression; product snippets compile from memory.

Default input is a real repository. --source-json - accepts the same files as
JSON strings for read-only/no-checkout investigations. --patch applies a
unified glue patch strictly in memory. No full source files are written.
"""
import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

PATHS = {
    "glue": "soh/mods/sound_translator/mm_sfx_synth_glue.cpp",
    "seq": "soh/mods/sound_translator/mm_sfx_synth_seqplayer.cpp",
    "heap": "mm/src/audio/lib/heap.c",
    "load": "mm/src/audio/lib/load.c",
    "backend": "soh/mods/sound_translator/mm_sfx_synth_backend.cpp",
}


def span(source, first, last):
    start = source.index(first)
    return source[start:source.index(last, start)]


def apply_patch(source, patch):
    lines = source.splitlines(keepends=True)
    offset = 0
    chunks = re.split(r"(?m)^@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@[^\n]*\n", patch)
    if len(chunks) == 1:
        raise ValueError("patch has no unified hunks")
    for i in range(1, len(chunks), 2):
        start = int(chunks[i]) - 1 + offset
        old, new = [], []
        for line in chunks[i + 1].splitlines(keepends=True):
            if line.startswith(("---", "+++", "diff ", "index ")):
                break
            if line.startswith(" "):
                old.append(line[1:]); new.append(line[1:])
            elif line.startswith("-"):
                old.append(line[1:])
            elif line.startswith("+"):
                new.append(line[1:])
            elif line.startswith("\\"):
                continue
            else:
                break
        if lines[start:start + len(old)] != old:
            raise ValueError("patch context mismatch at line " + str(start + 1))
        lines[start:start + len(old)] = new
        offset += len(new) - len(old)
    return "".join(lines)


STUBS = r'''
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
using s8=int8_t; using u8=uint8_t; using s16=int16_t; using u16=uint16_t;
using s32=int32_t; using u32=uint32_t; using f32=float;
constexpr int MMSFX_UPDATES_PER_FRAME=3, MMSFX_NUM_NOTES=32, TATUMS_PER_BEAT=48;
constexpr int SOUNDMODE_STEREO=0, LOAD_STATUS_COMPLETE=2, MUTE_FLAGS_STOP_SCRIPT=0x80;
constexpr int OS_TV_PAL=0, OS_TV_NTSC=1, OS_TV_MPAL=2;
#define ALIGN16(v) (((int)(v)+15)&~15)
#define PORTAMENTO_IS_SPECIAL(p) (((p).mode & 0x80) != 0)
struct AudioBufferParameters {
    s16 specUnk4, samplingFreq, aiSamplingFreq, numSamplesPerFrameTarget;
    s16 numSamplesPerFrameMax, numSamplesPerFrameMin, updatesPerFrame;
    s16 numSamplesPerUpdate, numSamplesPerUpdateMax, numSamplesPerUpdateMin, numSequencePlayers;
    f32 resampleRate,updatesPerFrameInv,updatesPerFrameInvScaled,updatesPerFrameScaled;
};
struct Context {
    AudioBufferParameters audioBufferParameters;
    s32 numNotes; s16 maxTempo; f32 unk_2870,unk_2960; u32 refreshRate;
    s32 soundMode,audioErrorFlags,sampleStateOffset;
} gMmSfx{},gAudioCtx{};
struct DefaultSample { struct { u8 enabled,needsInit; } bitField0; };
DefaultSample gDefaultSampleState{},gZeroedSampleState{};
u8 sMiscPool[16]; u8* sMiscCur;
constexpr int gTatumsPerBeat=48;
struct Spec { int samplingFreq=32000; } specValue; Spec* spec=&specValue;
int osAiSetFrequency(int n) { return n; }
struct SeqScriptState { u8* pc; };
struct SequenceChannel {};
struct SequencePlayer {
    SeqScriptState scriptState{}; bool enabled=true,muted=false,stopScript=false;
    u8 seqId=0,defaultFont=0,muteFlags=0; u32 scriptCounter=0;
    u16 tempo=0,tempoAcc=0,unk_16=0; s16 tempoChange=0;
};
bool AudioLoad_IsSeqLoadComplete(int) { return true; }
bool AudioLoad_IsFontLoadComplete(int) { return true; }
void AudioLoad_SetSeqLoadStatus(int,int) {}
void AudioLoad_SetFontLoadStatus(int,int) {}
void AudioScript_SequencePlayerDisable(SequencePlayer* p) { p->enabled=false; }
u8 AudioScript_ScriptReadU8(SeqScriptState* p) { return *p->pc++; }
struct Portamento { u8 mode; s16 speed; };
struct SequenceLayer { u16 delay,portamentoTime; };
int errors=0;
void check(bool ok,const char* text) { std::printf("%s %s\n",ok?"PASS":"FAIL",text); errors += !ok; }
SequencePlayer* pcmPlayer=nullptr;
unsigned pcmOpportunities=0,pcmFrames=0;
namespace mmsfx {
    using ::gMmSfx; bool sReady=true;
    void DrainIoQueue() {}
    void AudioScript_ProcessSequences(s32);
    void AudioSynth_SyncSampleStates(s32) {}
    void SynthUpdate(s32*,s32 n) { pcmFrames += n & ~7; }
}
int CVarGetInteger(const char*,int fallback) { return fallback; }
void MmSfxSynth_Log(const char*) {}
#define CLAMP(v,lo,hi) std::clamp((v),(lo),(hi))
'''


def translation_unit(src):
    init = span(src["glue"], "void MmSfxSynth_InitEngine(void)", "    gMmSfx.miscPool.start") + "}\n"
    gate = span(src["seq"], "void AudioScript_SequencePlayerProcessSequence(", "    if (seqPlayer->stopScript") + "}\n"
    tempo = span(src["seq"], "                        seqPlayer->tempo = AudioScript_ScriptReadU8", "                        break;")
    startup = span(src["load"], "    switch (osTvType)", "    AudioThread_InitMesgQueues();")
    params = span(src["heap"], "    gAudioCtx.audioBufferParameters.samplingFreq = spec->samplingFreq;", "    // sample dma size")
    clock = span(src["heap"], "    gAudioCtx.maxTempo =", "    gAudioCtx.audioBufferParameters.specUnk4")
    port_start = src["seq"].index("                portamento->extent =")
    port = span(src["seq"][port_start:], "                if (PORTAMENTO_IS_SPECIAL(*portamento))", "                portamento->cur = 0;")
    render = span(src["backend"], 'extern "C" void MmSfxSynth_RenderInto(', "    // Periodic state diagnostic") + "}\n"
    wrappers = "\nvoid NativeClock() { int osTvType=OS_TV_NTSC;\n" + startup + params + clock + "}\n"
    wrappers += "void Tempo(SequencePlayer* seqPlayer,u8 bpm) { SeqScriptState st{&bpm}; auto* seqScript=&st;\n" + tempo + "}\n"
    wrappers += "int Porta(SequencePlayer* seqPlayer,SequenceLayer* layer,bool special) { Portamento p{u8(special?0x80:0),0}; auto* portamento=&p; s32 speed;\n" + port + "return p.speed; }\n"
    wrappers += "namespace mmsfx { void AudioScript_ProcessSequences(s32) { ++pcmOpportunities; ::AudioScript_SequencePlayerProcessSequence(pcmPlayer); } }\n"
    main = r'''
int main() {
    NativeClock(); MmSfxSynth_InitEngine();
    std::printf("native threshold=%d normalization=%.10g imported threshold=%d normalization=%.10g\n",
        gAudioCtx.maxTempo,gAudioCtx.unk_2870,gMmSfx.maxTempo,gMmSfx.unk_2870);
    check(gAudioCtx.maxTempo==10770,"native startup oracle is NTSC frame-period clock");
    check(gMmSfx.maxTempo==gAudioCtx.maxTempo,"imported tempo threshold matches native initialization");
    check(std::fabs(gMmSfx.unk_2870-gAudioCtx.unk_2870)<1e-12f,"sample-duration normalization matches native");
    const Context imported=gMmSfx;
    for (u8 bpm : {u8(24),u8(120),u8(200)}) {
        SequencePlayer actual; Tempo(&actual,bpm);
        const int tempo=std::min(int(bpm)*TATUMS_PER_BEAT,int(gAudioCtx.maxTempo));
        check(actual.tempo==tempo,"authored tempo survives native-equivalent opcode clamping");
        for (int n=0;n<1800;++n) AudioScript_SequencePlayerProcessSequence(&actual);
        const int wanted=1800*tempo/gAudioCtx.maxTempo;
        std::printf("bpm=%u gateTicks=%u nativeTicks=%d nativeRate=%.6f importedRate=%.6f\n",
            unsigned(bpm),actual.unk_16,wanted,180.0*tempo/gAudioCtx.maxTempo,actual.unk_16/10.0);
        check(actual.unk_16==wanted,"actual tempo gate preserves native script cadence over 1800 updates");
        SequencePlayer p; Tempo(&p,bpm); SequenceLayer layer{10,32};
        const int actualSpeed=Porta(&p,&layer,true);
        gMmSfx=gAudioCtx; SequencePlayer native; Tempo(&native,bpm);
        const int expectedSpeed=Porta(&native,&layer,true);
        gMmSfx=imported;
        std::printf("bpm=%u specialPortamento=%d nativePortamento=%d\n",unsigned(bpm),actualSpeed,expectedSpeed);
        check(actualSpeed==expectedSpeed,"special portamento uses native tempo normalization");
        check(Porta(&p,&layer,false)==0x20000/(32*3),"ordinary portamento retains update-count units");
    }
    SequencePlayer onset; Tempo(&onset,120); AudioScript_SequencePlayerProcessSequence(&onset);
    check(onset.unk_16==0,"120-BPM script waits for fractional clock accumulation on first update");
    AudioScript_SequencePlayerProcessSequence(&onset);
    check(onset.unk_16==1,"120-BPM script advances on second update");
    std::printf("PCM scheduler note: fixed %d frames/update gives %.6f asymptotic script ticks/sec at 32k\n",
        gMmSfx.audioBufferParameters.numSamplesPerUpdate,
        32000.0/gMmSfx.audioBufferParameters.numSamplesPerUpdate*
        std::min(5760,int(gMmSfx.maxTempo))/gMmSfx.maxTempo);
    SequencePlayer pcm; Tempo(&pcm,120); pcmPlayer=&pcm;
    s16 output[544*2]{};
    for (int round=0;round<20;++round) {
        for (int frames : {528,544,528}) MmSfxSynth_RenderInto(output,frames);
    }
    const unsigned opportunities=(32000+gMmSfx.audioBufferParameters.numSamplesPerUpdate-1)/
        gMmSfx.audioBufferParameters.numSamplesPerUpdate;
    const unsigned expectedPcmTicks=opportunities*5760/gAudioCtx.maxTempo;
    std::printf("PCM budget: requested=32000 rendered=%u synthOpportunities=%u scriptTicks=%u nativeClockTicks=%u\n",
        pcmFrames,pcmOpportunities,pcm.unk_16,expectedPcmTicks);
    check(pcmFrames==32000,"actual callback partition renders the complete one-second PCM budget");
    check(pcmOpportunities==opportunities,"actual renderer retains its existing 176-frame clock partition");
    check(pcm.unk_16==expectedPcmTicks,"actual renderer uses native tempo gate at the existing PCM cadence");
    return errors?1:0;
}
'''
    return "#include <algorithm>\n#include <initializer_list>\n" + STUBS + init + gate + wrappers + render + main


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source-json",type=str)
    ap.add_argument("--patch",type=pathlib.Path)
    ap.add_argument("--report",type=pathlib.Path)
    ap.add_argument("--repo",type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[2])
    args=ap.parse_args()
    if args.source_json:
        src=json.load(sys.stdin) if args.source_json=="-" else json.loads(pathlib.Path(args.source_json).read_text())
    else:
        src={key:(args.repo/path).read_text() for key,path in PATHS.items()}
    if args.patch:
        src["glue"]=apply_patch(src["glue"],args.patch.read_text())
    results=[]
    with tempfile.TemporaryDirectory(prefix="mm-clock-test-") as temp:
        for host,flags in (("soh",[]),("combo-oot",["-DCOMBO_BUILD","-DSOH_BUILD_DLL"])):
            binary=str(pathlib.Path(temp)/host)
            cc=subprocess.run(["g++","-x","c++","-","-std=c++20","-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer",*flags,"-o",binary],input=translation_unit(src),text=True,capture_output=True)
            if cc.returncode: raise RuntimeError(cc.stderr)
            env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0")
            run=subprocess.run([binary],text=True,capture_output=True,env=env)
            results.append({"host":host,"exit":run.returncode,"stdout":run.stdout,"stderr":run.stderr})
            print(host+":\n"+run.stdout,end="")
    if args.report: args.report.write_text(json.dumps({"results":results},indent=2)+"\n")
    return int(any(r["exit"] for r in results))


if __name__=="__main__":
    raise SystemExit(main())
