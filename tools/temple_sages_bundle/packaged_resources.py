"""Dependency inspection for the normal packaged Sages Chamber resources."""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import resource_geometry as geometry


def canonical(path):
    return path.removeprefix('alt/')


def references(data, hashes):
    """Return static resource paths; native dynamic segments remain native."""
    if data.lstrip().startswith(b'<'):
        return {element.get('Path') for element in ET.fromstring(data).iter()
                if element.get('Path') and not element.get('Path').startswith('>')}
    if data[4:8] == b'TLDO':
        result = set()
        for _, _, _, digest in geometry.words(data):
            if digest is not None:
                if digest not in hashes:
                    result.add(f'unresolved-hash:{digest:016x}')
                else:
                    result.add(hashes[digest])
        return result
    if data[4:8] == b'MORO':
        result = set()
        for _, _, _, command in geometry.room_commands(data):
            result.update(path for path in command.get('paths', []) if path)
            result.update(command.get('meshes', []))
        return result
    return set()


def hash_index(paths):
    return {geometry.crc64(path): path for path in paths}


def missing_dependencies(resources, native_paths):
    """Check both resolution modes against bundled resources and native ROM paths."""
    native_paths = set(native_paths)
    available = {canonical(name) for name in resources} | native_paths
    hashes = hash_index(available | set(resources))
    missing = []
    for name, data in resources.items():
        for dependency in references(data, hashes):
            if canonical(dependency) not in available:
                missing.append((name, dependency))
    return sorted(missing)
