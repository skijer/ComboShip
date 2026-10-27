"""Exercise native point-source policy with actual MM note initialization and release."""
import os
from pathlib import Path
import subprocess
import tempfile
import sys
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "mm/src/audio/lib/playback.c").read_text()
preamble = (ROOT / "mm/tests/audio_stream_runtime_test.c").read_text().split('#include "seqcmd.h"')[0]
fixture = preamble + r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "audio/streamed_positional.h"
AudioContext gAudioCtx;
u8 sSequenceFilter[32];
u16 gHaasEffectDelaySize[128];
f32 gHeadsetPanVolume[128],gStereoPanVolume[128],gDefaultPanVolume[128];
static int enabled;
int32_t CVarGetInteger(const char* name,int32_t fallback) {
    assert(!strcmp(name,"gEnhancements.Fixes.MMStreamedPositionalAudio"));
    assert(fallback==0); return enabled;
}
float CVarGetFloat(const char* name,float fallback) { (void)name;return fallback; }
#define CLAMP_MAX(v,m) ((v)<(m)?(v):(m))
void AudioPlayback_AudioListRemove(AudioListItem* item) { (void)item;assert(0); }
void AudioPlayback_AudioListPushFront(AudioListItem* list, AudioListItem* item) { (void)list;(void)item;assert(0); }
void AudioPlayback_NoteSetResamplingRate(NoteSampleState*, f32);
'''
for name in ("AudioPlayback_NoteSetResamplingRate", "AudioPlayback_InitSampleState", "AudioPlayback_SeqLayerDecayRelease"):
    a,b = function_body(source,name)
    fixture += source[a:b] + "\n"
fixture += r'''
int main(void) {
    s16 authoredFilter[8]={0};
    s16* native=(s16*)(((uintptr_t)sSequenceFilter & ~(uintptr_t)15)+16);
    gAudioCtx.soundMode=SOUNDMODE_MONO;
    for(int codec=0;codec<=8;++codec) for(int opt=0;opt<2;++opt) for(int authored=0;authored<2;++authored) {
        Sample sample={0}; sample.codec=codec; TunedSample tuned={.sample=&sample};
        s16 releasedFilter[8]={0};Note note={0}; note.sampleState.tunedSample=&tuned;
        note.playbackState.attributes.filterBuf=releasedFilter;
        NoteSubAttributes sub={0};sub.frequency=1;sub.velocity=.7f;sub.pan=34;
        sub.gain=127;sub.filter=authored?authoredFilter:native;sub.targetReverbVol=55;
        sub.combFilterGain=37;sub.combFilterSize=16;sub.surroundEffectIndex=61;
        enabled=opt;NoteSampleState result={0};
        AudioPlayback_InitSampleState(&note,&result,&sub);
        int clean=opt && !authored && (codec==CODEC_S16 || codec==CODEC_OPUS);
        assert(result.gain==(clean?0:127)); assert(result.filter==(clean?NULL:sub.filter));
        assert(sub.gain==127 && sub.filter==(authored?authoredFilter:native));
        NoteSampleState control={0}; enabled=0;AudioPlayback_InitSampleState(&note,&control,&sub); enabled=opt;
        control.gain=result.gain;control.filter=result.filter;
        assert(!memcmp(&control,&result,sizeof(result))); // pitch/pan/volume/reverb/comb untouched
        SequenceLayer layer={0};SequenceChannel channel={0};SequencePlayer player={0};
        layer.note=&note;layer.channel=&channel;layer.unk_0A.s.bit_9=1;layer.unk_0A.s.bit_2=1;
        channel.seqPlayer=&player;channel.gain=127;channel.filter=sub.filter;channel.targetReverbVol=55;
        note.playbackState.parentLayer=&layer;note.playbackState.adsr.action.s.status=ADSR_STATUS_SUSTAIN;
        AudioPlayback_SeqLayerDecayRelease(&layer,ADSR_STATUS_RELEASE);
        assert(note.playbackState.parentLayer==NO_LAYER);
        assert(note.playbackState.attributes.gain==(clean?0:127));
        assert((note.playbackState.attributes.filter==NULL)==clean);
        if(!clean) assert(note.playbackState.attributes.filter==note.playbackState.attributes.filterBuf);
    }
    NoteSampleState invalid={0};u8 gain=127;s16* filter=native;enabled=1;
    Audio_ApplyStreamedPositionalPolicy(&invalid,&gain,&filter);assert(gain==127 && filter==native);
    invalid.bitField1.isSyntheticWave=1;invalid.waveSampleAddr=(s16*)1;
    Audio_ApplyStreamedPositionalPolicy(&invalid,&gain,&filter);assert(gain==127 && filter==native);
    Sample sample={0};sample.codec=CODEC_OPUS;TunedSample tuned={.sample=&sample};
    invalid.bitField1.isSyntheticWave=0;invalid.tunedSample=&tuned;
    gain=32;filter=native;Audio_ApplyStreamedPositionalPolicy(&invalid,&gain,&filter);
    assert(gain==32 && filter==NULL); // preserve a separately authored gain
    puts("PASS production streamed positional policy: all codecs, default-off/on, authored filters/gain, note release, spatial attributes and synthetic/null guards");
}
'''
with tempfile.TemporaryDirectory(prefix="streamed-positional-") as td:
    path=Path(td)/"test.c";path.write_text(fixture);binary=Path(td)/"test"
    flags=["-std=gnu11","-Wall","-Wextra","-Wno-unused-variable","-Wno-unused-but-set-variable"]
    if "--sanitize" in sys.argv: flags += ["-g","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
    subprocess.run([os.environ.get("CC","cc"),*flags,*["-I"+str(ROOT/p) for p in ("mm/tests","mm/include","libultraship/include")],str(path),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":os.environ.get("ASAN_OPTIONS","detect_leaks=0")})

menu=(ROOT/"mm/2s2h/BenGui/BenMenu.cpp").read_text()
assert '"Clean Streamed Positional Music"' in menu
assert '"Fix MM Audio Gain Buffer"' not in menu and '"Trace MM Shop Audio"' not in menu
synth=(ROOT/"mm/src/audio/code_8019AF00.c").read_text()
assert "0x54" in synth and "Audio_SetSequenceProperties" in synth
print("PASS menu cleanup and native point-source processing retained")
