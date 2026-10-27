#!/usr/bin/env python3
"""Compile the actual camera adapter against each engine's native structures."""
from pathlib import Path
import os
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

def main():
    with tempfile.TemporaryDirectory(prefix='mirror-camera-') as directory:
        # Compile the private MM table directly from its production declarations.
        source = (ROOT/'mm/src/code/z_camera_data.inc').read_text()
        end = source.index('} CameraSetting;')+len('} CameraSetting;')
        table = source[:end]+'\n#define CAM_SETTING_MODE_ENTRY(f,d) {f, ARRAY_COUNT(d), d}\n#define CAM_SETTING_MODE_NULL {0,0,nullptr}\n'
        for declaration in ('CameraModeValue sSetMagicMirrorModeData[', 'CameraMode sCamSetMagicMirrorModes['):
            start=source.index(declaration); brace=source.index('{',start); depth=1; stop=brace+1
            while depth:
                depth+=(source[stop]=='{')-(source[stop]=='}');stop+=1
            table+=source[start:stop]+';\n'
        entry=re.search(r'^\s*(\{[^\n]*sCamSetMagicMirrorModes[^\n]*\}),',source,re.M)[1]
        table+='CameraSetting mirrorSetting='+entry+';\n'
        (Path(directory)/'native_mirror_camera_table.inc.h').write_text(table)
        manager=(ROOT/'mm/src/code/z_eventmgr.c').read_text()
        manager=re.sub(r'^#include[^\n]*','',manager,flags=re.M)
        (Path(directory)/'native_mirror_manager.inc.h').write_text(manager)
        for host in ('soh', 'mm'):
            includes = ('.', host, host+'/include', host+'/include/PR', host+'/src', host+'/assets',
                        host+('/soh' if host == 'soh' else '/2s2h'), 'libultraship/include', 'combo', 'combo/menu')
            binary = Path(directory) / host
            command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD',
                       '-DLOG_LEVEL_GAME_PRINTS=0', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                       '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                       *['-I'+str(ROOT/p) for p in includes]]
            command += ['-I'+directory]
            if host == 'mm': command.append('-DCOMBO_MAGIC_MIRROR_MM')
            subprocess.run(command + [str(HERE/'camera_native_test.cpp'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
            print('Native camera host:', host, flush=True)

if __name__ == '__main__':
    main()
