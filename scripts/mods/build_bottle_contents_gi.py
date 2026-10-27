#!/usr/bin/env python3
"""Bundle the preserved TP casing for filled-bottle GIs in both port archives.

The matching renderer composes live MM mushroom/princess and procedural dust.
Marker roots retain a casing-only fallback for incomplete renderer installations.
"""
import argparse
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile
from build_shared_bottle_gi import BASE, canonical_resources, compose, digest, verify, xml


def add_fairy_glass(resources):
    """Reuse the exact shell, overriding its reflective glass after material setup."""
    glass = BASE + 'shell/mat_gGiBlueFireChamberstickDL_BottleGlassOutside'
    material = ET.Element('DisplayList', Version='0')
    ET.SubElement(material, 'CallDisplayList', Path=glass)
    ET.SubElement(material, 'PipeSync')
    ET.SubElement(material, 'SetPrimColor', M='0', L='0', R='255', G='255', B='255', A='80')
    ET.SubElement(material, 'EndDisplayList')
    resources[BASE + 'FairyGlassMaterial'] = xml(material)
    shell = ET.fromstring(resources[BASE + 'BottleShell'])
    calls = [node for node in shell.findall('CallDisplayList') if node.get('Path') == glass]
    assert len(calls) == 1, 'Expected one glass material in the preserved shell'
    calls[0].set('Path', BASE + 'FairyGlassMaterial')
    resources[BASE + 'FairyXlu'] = xml(shell)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-o2r', required=True, type=Path)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    with zipfile.ZipFile(args.source_o2r) as source:
        resources, preserved = canonical_resources(source)
        add_fairy_glass(resources)
        for marker in ('MushroomBottle', 'PrincessBottle', 'GoldDustBottle'):
            resources[BASE + marker] = compose()
        checks = verify(resources, source, preserved, {})
        checks['fairy_glass_alpha'] = 80
        for host in ('soh', 'mm'):
            for name, data in sorted(resources.items()):
                output = args.repo / host / 'assets/custom' / name
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(data)
        manifest = {
            'source': str(args.source_o2r.resolve().relative_to(args.repo.resolve())),
            'source_sha256': digest(args.source_o2r.read_bytes()),
            'checks': checks,
            'preserved': preserved,
            'resources': {path: digest(blob) for path, blob in sorted(resources.items())},
            'packaging': 'non-Alt custom resources in soh.o2r and 2ship.o2r',
            'runtime': 'untested',
        }
    output = args.repo / 'tools/song_bottle_20261008/bottle_assets.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'PASS {len(resources)} resources per port; closed graph; original casing geometry/UV/texture bytes preserved')


if __name__ == '__main__':
    main()
