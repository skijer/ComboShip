#!/usr/bin/env python3
"""Run production VoicePack_Init with filesystem fixtures, without decoding audio."""
from pathlib import Path
import os
import subprocess
import tempfile
import re

def function(source, name):
    match = re.search(r"(?:static )?(?:void|std::vector<std::string>) " + name + r"\(.*?^}", source, re.M | re.S)
    if match is None:
        raise RuntimeError("Cannot locate production function " + name)
    return match[0] + "\n"

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'soh/mods/voice_pack/voice_pack.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='voice-discovery-') as tmp:
    tmp = Path(tmp)
    production = function(source.replace('extern "C" ', ''), 'VoicePack_Init')
    if 'static std::vector<std::string> VoicePack_FindFiles(' in source:
        production = function(source, 'VoicePack_FindFiles') + production
    (tmp / 'voice_init.inc').write_text(production)
    binary = tmp / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    '-I' + str(tmp), str(ROOT / 'soh/tests/voice_pack_discovery_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(tmp / 'mods')], check=True)
