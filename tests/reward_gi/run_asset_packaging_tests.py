"""Exercise the real CMake dependency with a clean asset tree and exact bytes."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
manifest = json.loads((ROOT / 'tools/reward_gi/material_manifest.json').read_text())

def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()

# Python 3.10 is valid for this project and has no hashlib.file_digest.
spec = importlib.util.spec_from_file_location('reward_unpack', ROOT / 'tools/reward_gi/unpack_assets.py')
unpacker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(unpacker)
newer_api = getattr(hashlib, 'file_digest', None)
if newer_api is not None:
    del hashlib.file_digest
try:
    assert unpacker.sha256(ROOT / 'tools/reward_gi/assets.zip') == manifest['pack']['sha256']
finally:
    if newer_api is not None:
        hashlib.file_digest = newer_api

with tempfile.TemporaryDirectory(prefix='reward-packaging-') as folder:
    source = Path(folder) / 'source'
    build = Path(folder) / 'build'
    source.mkdir()
    # The real module must restore assets before its dependent check executes.
    sentinel = source / 'assert_ready.py'
    sentinel.write_text(
        "from pathlib import Path\n"
        "root = Path(__file__).parent / 'soh/assets/custom/objects/nei_reward_gi'\n"
        "assert len(list(root.iterdir())) == 7\n")
    (source / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.22)\nproject(RewardPackaging NONE)\n'
        'find_package(Python3 REQUIRED COMPONENTS Interpreter)\n'
        'add_custom_target(CheckAssetCollisions COMMAND "${Python3_EXECUTABLE}" '
        '"${CMAKE_CURRENT_SOURCE_DIR}/assert_ready.py")\n'
        f'include("{(ROOT / "CMake/NeiRewardGiAssets.cmake").as_posix()}")\n')
    subprocess.run(['cmake', '-S', str(source), '-B', str(build)], check=True)
    command = ['cmake', '--build', str(build), '--target', 'CheckAssetCollisions', '--parallel', '2']
    subprocess.run(command, check=True)
    assets = source / 'soh/assets/custom'
    assert {p.relative_to(assets).as_posix() for p in assets.rglob('*') if p.is_file()} == set(manifest['resource_scope'])
    timestamps = {}
    for slug, entry in manifest['entries'].items():
        path = assets / entry['target']
        assert path.stat().st_size == entry['resource_bytes']
        assert sha256(path) == entry['resource_sha256']
        timestamps[slug] = path.stat().st_mtime_ns
    # Repeated builds preserve unchanged files; stale output is restored exactly.
    damaged = assets / manifest['entries']['fire']['target']
    with damaged.open('r+b') as stream:
        stream.write(b'broken')
    subprocess.run(command, check=True)
    for slug, entry in manifest['entries'].items():
        path = assets / entry['target']
        assert sha256(path) == entry['resource_sha256']
        if slug != 'fire':
            assert path.stat().st_mtime_ns == timestamps[slug]
    collision = source / 'mm/assets/custom' / manifest['entries']['fire']['target']
    collision.parent.mkdir(parents=True, exist_ok=True)
    collision.write_bytes(b'unexpected shared resource')
    rejected = subprocess.run(command, capture_output=True, text=True)
    assert rejected.returncode != 0 and 'Reward private resource collision' in rejected.stdout + rejected.stderr
    print('PASS real CMake archive dependency: seven exact 4K resources, clean builds, stale recovery, unchanged-file preservation, private-key collision rejection')
