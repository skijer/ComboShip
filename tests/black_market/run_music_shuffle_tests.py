#!/usr/bin/env python3
"""Run production shuffle/event callbacks with real collections and RNG."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
def body(source,signature):
    match=re.search(re.escape(signature)+r'[^;{}]*\{',source)
    assert match,signature
    return block_from(source,match.start())
with tempfile.TemporaryDirectory(prefix='music-shuffle-') as temporary:
    for host in ('mm','soh'):
        folder=ROOT/host/('2s2h/Enhancements/Audio' if host=='mm' else 'soh/Enhancements/audio')
        source=(folder/'AudioCollection.cpp').read_text()
        signatures=['AudioCollection::AudioCollection()', 'std::string AudioCollection::GetCvarKey(',
            'std::string AudioCollection::GetCvarLockKey(', 'uint16_t AudioCollection::GetReplacementSequence(',
            'void AudioCollection::AddToCollection(', 'void AudioCollection::InitializeShufflePool(']
        if host=='mm':
            signatures+=['uint16_t AudioCollection::GetMaxOriginalSeqId(',
                'void AudioCollection::ParseSequenceCategory(', 'size_t AudioCollection::CountSequencesByType(']
        collection=source[source.index('#define SEQUENCE_MAP_ENTRY'):source.index('AudioCollection::AudioCollection()')]
        collection+='\n'+'\n'.join(body(source,s) for s in signatures)
        editor=(folder/'AudioEditor.cpp').read_text()
        signatures=['void RandomizeGroup(', 'void ResetGroup(', 'void LockGroup(', 'void UnlockGroup(',
            'void AudioEditor_RandomizeAll(']
        if host=='mm':
            signatures+=['extern "C" void AudioEditor_RandomizeOnSceneLoad(', 'void AudioEditor::InitElement(']
            definitions=''
        else:
            signatures+=['void AudioEditor_AutoRandomizeAll(', 'void AudioEditorRegisterOnSceneInitHook(',
                'void AudioEditorRegisterOnGenerationCompletionHook(']
            definitions=editor[editor.index('#define SEQ_COUNT_BGM_WORLD'):editor.index('static const std::map<int32_t, const char*> audioRandomizerModes')]
        definitions+=body(editor,'static std::array<SeqType, 8> allTypes =' if host=='mm' else 'std::vector<SeqType> allTypes =')+';\n'
        definitions+='\n'.join(body(editor,s) for s in signatures)
        fixture=(ROOT/'tests/black_market/music_shuffle_test.cpp').read_text()
        utils=(ROOT/'soh/soh/ShipUtils.cpp').read_text()
        rng=utils[utils.index('static bool default_init'):utils.index('// Initialize with seed specified')]
        rng+='\n'+'\n'.join(body(utils,s) for s in ['void ShipUtils::RandInit(', 'uint32_t ShipUtils::next32(', 'uint32_t ShipUtils::Random('])
        shuffle=body((ROOT/'soh/soh/ShipUtils.h').read_text(),'template <typename T> void Shuffle(')
        fixture=fixture.replace('/* COLLECTION */',collection).replace('/* EDITOR */',definitions)
        fixture=fixture.replace('/* RNG */',rng).replace('/* SHUFFLE TEMPLATE */',shuffle)
        maximum=re.search(r'^#define MAX_AUTHENTIC_SEQID\s+\d+', (ROOT/host/'include/z64audio.h').read_text(),re.M)
        fixture=fixture.replace('/* AUDIO CONSTANTS */',maximum[0])
        cpp=Path(temporary)/(host+'.cpp');cpp.write_text(fixture);exe=cpp.with_suffix('')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-g','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
            '-I'+str(ROOT),'-I'+str(ROOT/'combo'),'-I'+str(folder),'-I'+str(ROOT/host/'include'),
            '-I'+str(ROOT/host/'include/PR'),'-I'+str(ROOT/'libultraship/include'),
            *(['-DSCENE_MUSIC_MM'] if host=='mm' else []),str(cpp),
            str(ROOT/'libultraship/src/ship/utils/StringHelper.cpp'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
