"""Exercise the real night updater, sequence queue and sequence-mode body.

Only the sequence-mode body's unrelated translation-unit dependencies are
fixtures; generated source is confined to the temporary build directory.
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main(build, cc='cc', cxx='c++'):
    build = pathlib.Path(build)
    source = (ROOT / 'soh/src/code/code_800EC960.c').read_text()
    body = re.search(r'^void Audio_SetSequenceMode\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
    if body is None:
        raise RuntimeError('Missing production Audio_SetSequenceMode')
    (build / 'night_sequence_mode.inc').write_text(body[0])
    editor = (ROOT / 'soh/soh/Enhancements/audio/AudioEditor.cpp').read_text()
    replay = re.search(r'^void ReplayCurrentBGM\([^;]*?\)\s*\{.*?^}', editor, re.M | re.S)
    if replay is None:
        raise RuntimeError('Missing production ReplayCurrentBGM')
    update = re.search(r'^void UpdateCurrentBGM\([^;]*?\)\s*\{.*?^}', editor, re.M | re.S)
    entry = re.search(r'^static uint16_t CurrentBGMEntry\([^;]*?\)\s*\{.*?^}', editor, re.M | re.S)
    (build / 'night_editor_replay.inc').write_text(replay[0]+'\n'+entry[0]+'\n'+update[0])
    # Keep the native preview lifecycle and full-width queue in this regression.
    preview_names = ['Audio_PreviewSequenceInternal', 'PreviewSequence', 'Audio_PreviewSceneSequence',
                     'Audio_CancelPreviewRestore', 'func_800F5C2C']
    preview = []
    for name in preview_names:
        match = re.search(r'^(?:static )?void ' + name + r'\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
        if match is None:
            raise RuntimeError('Missing production ' + name)
        preview.append(match[0])
    (build / 'night_preview.inc').write_text('\n'.join(preview))
    includes = ['-I' + str(ROOT / p) for p in
                ['soh/tests/night_runtime_stubs', 'soh/tests/audio_sequence_stubs', 'soh/include', 'soh',
                 'libultraship/include']]
    obj = str(build / 'night_seq.o')
    subprocess.run([cc, '-std=c11', *includes, '-c', str(ROOT / 'soh/src/code/code_800F9280.c'), '-o', obj], check=True)
    exe = str(build / 'night_combat_test')
    subprocess.run([cxx, '-std=c++20', '-DNDEBUG', '-DNIGHT_COMBAT_RUNTIME_TEST', '-DNIGHT_EDITOR_REFRESH_TEST',
                    '-DCVAR_PREFIX_AUDIO="gAudioEditor"', '-I' + str(build), *includes,
                    str(ROOT / 'soh/tests/hyrule_field_night_runtime_test.cpp'),
                    str(ROOT / 'soh/soh/Enhancements/audio/HyruleFieldNightMusic.cpp'), obj, '-o', exe], check=True)
    subprocess.run([exe], check=True)
    print('PASS night combat bridge: handoff, restoration, identity, ownership cleanup and vanilla eligibility')


if __name__ == '__main__':
    main(*sys.argv[1:])
