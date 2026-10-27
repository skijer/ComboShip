#!/usr/bin/env python3
"""Compile the production silver resolver and hooks against a native actor fixture."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
source = (ROOT / 'soh/soh/Enhancements/randomizer/ShuffleSilver.cpp').read_text()


def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


signatures = (
    'RandomizerGet SilverFromSwitchFlag(',
    's8* Randomizer::SilverFieldFromSaveContext(',
    's8 Randomizer::SilverTotal(',
    'bool IsSilverCleared(',
    'static void SetSilverCleared(',
    'void RegisterShuffleSilver(',
)
with tempfile.TemporaryDirectory(prefix='silver-rupee-hooks-') as tmp:
    directory = Path(tmp)
    (directory / 'production.inc').write_text('\n\n'.join(map(function, signatures)))
    binary = directory / 'silver-test'
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-g', '-I', str(ROOT), '-I', str(directory)]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer',
                  '-fno-pie', '-no-pie']
    subprocess.run(['g++', *flags, str(ROOT / 'tests/silver_rupee/native_test.cpp'), '-o', str(binary)], check=True)
    failed = []
    for case in ('mapping', 'domain-tracker', 'domain-rupee', 'unknown-flag', 'unknown-check',
                 'known-tracker', 'known-rupee', 'missing-play', 'invalid-group'):
        print(f'Silver hooks: {case}', flush=True)
        if subprocess.run([str(binary), case]).returncode:
            failed.append(case)
    if failed:
        raise SystemExit('Silver hook regressions failed: ' + ', '.join(failed))
print('Silver-rupee resolver and hook regressions PASS')
