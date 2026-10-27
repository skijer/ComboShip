#!/usr/bin/env python3
"""Package only Wolf's implemented cues as exact mono s16 PCM from supplied WAVs.

Usage: build_audio_assets.py <extracted-source-directory> [--check]
The source archive remains outside the repository. No cropping, normalization or
synthetic audio is performed; native playback handles each original sample rate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[2]
FILES = ['TP_Transform_Wolf.wav', 'TP_Transform_Human.wav']
FILES += [f'TP_WolfLink_Lunge{i}.wav' for i in range(1, 6)]
FILES += [f'TP_WolfLink_JumpAttack{i}.wav' for i in range(1, 3)]
FILES += [f'TP_WolfLink_Charge_Attack_A{i}.wav' for i in range(1, 4)]
FILES += [f'TP_WolfLink_Huff_A{i}.wav' for i in range(1, 4)]


def package(source):
    values, clips, provenance = [], [], []
    for name in FILES:
        path = source / name
        with wave.open(str(path)) as wav:
            if wav.getnchannels() != 1 or wav.getsampwidth() != 2 or wav.getcomptype() != 'NONE':
                raise ValueError(f'{name}: expected mono uncompressed s16 PCM')
            rate, frames = wav.getframerate(), wav.getnframes()
            pcm = wav.readframes(frames)
        samples = struct.unpack('<' + 'h' * frames, pcm)
        if len(pcm) != frames * 2 or frames < 2 or not any(samples):
            raise ValueError(f'{name}: incomplete or silent source')
        clips.append((name, len(values), frames, rate))
        values.extend(samples)
        provenance.append({'source': name, 'wav_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                           'pcm_sha256': hashlib.sha256(pcm).hexdigest(), 'sample_rate': rate,
                           'sample_count': frames, 'duration_seconds': frames / rate})
    lines = ['// Exact PCM from the user-supplied Wolf Link.zip and TP_Transform_Wolf.wav.',
             '// Regenerate: python3 tests/mm_wolf/build_audio_assets.py <extracted-source-directory>',
             '// Original rates and all samples preserved; no synthesized or substituted audio.',
             'static constexpr std::int16_t kWolfPcm[] = {']
    for offset in range(0, len(values), 16):
        lines.append('    ' + ', '.join(map(str, values[offset:offset + 16])) + ',')
    lines += ['};', '', 'static constexpr WolfAudioClip kWolfClips[] = {']
    for name, offset, frames, rate in clips:
        lines.append(f'    {{ "{name}", kWolfPcm + {offset}, {frames}, {rate}.0f }},')
    lines.append('};')
    return '\n'.join(lines) + '\n', json.dumps({'format': 'mono-s16-pcm-exact', 'clips': provenance}, indent=2) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    pcm, manifest = package(args.source)
    outputs = [(ROOT / 'combo/assets/wolf_link_sfx_pcm.inc', pcm),
               (ROOT / 'combo/assets/wolf_link_sfx_sources.json', manifest)]
    for path, content in outputs:
        if args.check:
            if not path.exists() or path.read_text() != content:
                raise SystemExit(f'FAIL source-derived Wolf audio differs: {path.name}')
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
    print(f'PASS exact supplied Wolf PCM: {len(FILES)} clips, original rates, WAV/PCM SHA256 provenance')


if __name__ == '__main__':
    main()
