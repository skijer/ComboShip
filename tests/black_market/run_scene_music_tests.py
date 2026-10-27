#!/usr/bin/env python3
"""Exercise both real audio-collection constructors and replacement functions."""
from pathlib import Path
import argparse,os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
def production_block(source,signature):
    match=re.search(re.escape(signature)+r'[^;{}]*\{',source)
    assert match, signature
    return block_from(source,match.start())
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',action='store_true',help='also run scene routing with real game headers (requires native build dependencies)')
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='scene-music-') as temporary:
    for host in ('mm','soh'):
        folder=ROOT/host/('2s2h/Enhancements/Audio' if host=='mm' else 'soh/Enhancements/audio')
        source=(folder/'AudioCollection.cpp').read_text()
        macro=source[source.index('#define SEQUENCE_MAP_ENTRY'):source.index('AudioCollection::AudioCollection()')]
        signatures=[
            'AudioCollection::AudioCollection()', 'std::string AudioCollection::GetCvarKey(',
            'std::string AudioCollection::GetCvarLockKey(', 'uint16_t AudioCollection::GetReplacementSequence(',
            'void AudioCollection::AddToCollection(']
        if host=='mm':
            signatures+=['uint16_t AudioCollection::GetOriginalSequence(',
                         'uint16_t AudioCollection::GetMaxOriginalSeqId(', 'void AudioCollection::ParseSequenceCategory(']
        bodies=macro+'\n'+'\n'.join(production_block(source,signature) for signature in signatures)
        fixture=(ROOT/'tests/black_market/scene_music_collection_test.cpp').read_text().replace('/* PRODUCTION */',bodies)
        cpp=Path(temporary)/(host+'.cpp');cpp.write_text(fixture);binary=Path(temporary)/host
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-g','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
            '-I'+str(ROOT),'-I'+str(ROOT/'combo'),'-I'+str(folder),'-I'+str(ROOT/host/'include'),'-I'+str(ROOT/host/'include/PR'),
            '-I'+str(ROOT/'libultraship/include'),*(['-DSCENE_MUSIC_MM'] if host=='mm' else []),
            str(cpp),str(ROOT/'libultraship/src/ship/utils/StringHelper.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        if args.native:
            fixture=(ROOT/'tests/black_market/scene_music_runtime_test.cpp').read_text().replace('/* PRODUCTION */',bodies)
            environment=(ROOT/host/'src/code/z_kankyo.c').read_text()
            signatures=['void Environment_PlaySceneSequence(']
            if host=='mm':
                signatures=['u8 Environment_IsFinalHours(', 'u8 func_800FE5D0(']+signatures
                audio=(ROOT/'mm/src/audio/code_8019AF00.c').read_text()
                cache='\n'.join(production_block(audio,signature) for signature in
                    ['void Audio_InvalidateSceneSequence(', 'void Audio_PlaySceneSequence('])
                editor=(folder/'AudioEditor.cpp').read_text()
                fixture=fixture.replace('/* NATIVE PREVIEW */','\n'.join(production_block(editor,signature) for signature in
                    ['void CancelForScene(', 'void Update(']))
            else:
                signatures=['s32 Environment_IsForcedSequenceDisabled(']+signatures
                audio=(ROOT/'soh/src/code/code_800EC960.c').read_text()
                cache=''
                fixture=fixture.replace('/* NATIVE CACHE */','')
                fixture=fixture.replace('/* NATIVE ENVIRONMENT */',
                    production_block(audio,'void Audio_CancelPreviewRestore(')+'\n/* NATIVE ENVIRONMENT */')
                fixture=fixture.replace('/* NATIVE EXACT PREVIEW */','\n'.join(production_block(audio,signature) for signature in
                    ['static void Audio_PreviewSequenceInternal(', 'void PreviewSequence(', 'void Audio_PreviewSceneSequence(']))
            fixture=fixture.replace('/* NATIVE CACHE */',cache).replace('/* NATIVE ENVIRONMENT */',
                '\n'.join(production_block(environment,signature) for signature in signatures))
            editor=(folder/'AudioEditor.cpp').read_text()
            current=production_block(editor,'static uint16_t CurrentBGMReplacement(')
            if host=='soh':
                current='bool HyruleFieldNightMusic_IsPlaying(uint16_t) { return false; }\n'+production_block(editor,'static uint16_t CurrentBGMEntry(')+'\n'+current
            fixture=fixture.replace('/* EDITOR CURRENT SELECTION */',current)
            if host=='mm':
                sequence=(ROOT/'mm/src/audio/sequence.c').read_text()
                start=production_block(sequence,'static const uint8_t sClockTownDaySeqIds[')+';\n'
                start+='\n'.join(production_block(sequence,signature) for signature in
                    ['static u16 AudioSeq_ResolveSequence(', 'void AudioSeq_StartSequence('])
            else:
                sequence=(ROOT/'soh/src/code/code_800F9280.c').read_text()
                start=production_block(sequence,'static void Audio_StartSequenceInternal(')
            fixture=fixture.replace('/* NATIVE START */',start)
            # The port's OTR scene handlers are separate from the C handlers.
            # The shared scene-start call must remain on the real Play_Init path.
            play=(ROOT/host/'src/code/z_play.c').read_text()
            configure=play.index('AudioEditor_ConfigureSceneMusic(')
            native_start=play.index('Environment_PlaySceneSequence(',configure)
            assert 0<native_start-configure<180
            cpp.write_text(fixture)
            flags=['-DFMT_HEADER_ONLY','-DLOG_LEVEL_GAME_PRINTS=0','-fpermissive']
            for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
                flags += [f'-D{key}="{value}"' for key,value in re.findall(
                    r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text())]
            includes=[ROOT,ROOT/'combo',folder,ROOT/host,ROOT/host/'include',ROOT/host/'include/PR',
                      ROOT/host/'src',ROOT/host/'assets',ROOT/host/'mods',ROOT/host/('2s2h' if host=='mm' else 'soh'),
                      ROOT/'libultraship/include',ROOT/'libultraship']
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-g','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
                '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*flags,
                *['-I'+str(path) for path in includes],*(['-DSCENE_MUSIC_MM'] if host=='mm' else []),
                str(cpp),str(ROOT/'libultraship/src/ship/utils/StringHelper.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
