"""Verify candidate provenance and retained sword designs against git."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import numpy as np
import build as recipe

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'c0181856968dd6f3869be0d1aa240743091fd6a8'
CHANGED = set(recipe.SLUGS)
RETAINED = recipe.depth.RETAINED
ASSETS = 'soh/assets/custom/objects/nei_gi_redesign/'
CHECKPOINTS = 'tools/nei_gi/CHECKPOINTS/'


def git(*arguments):
    return subprocess.check_output(['git', *arguments], cwd=ROOT)


def check():
    report = {'baseline_commit': BASELINE, 'retained_designs': {}, 'candidates': {}, 'runtime_tested': False}
    for slug in RETAINED:
        paths = git('ls-tree', '-r', '--name-only', BASELINE, ASSETS + slug, CHECKPOINTS + slug).decode().splitlines()
        assert paths, slug
        hashes = {}
        for path in paths:
            if path.startswith(CHECKPOINTS) or path.endswith('mesh_opa_vtx') or path.endswith('mesh_xlu_vtx'):
                continue
            original = git('show', BASELINE + ':' + path)
            current = (ROOT / path).read_bytes()
            assert original == current, ('retained texture, matrix or display list changed', path)
            hashes[path] = hashlib.sha256(current).hexdigest()
        before, before_binary = recipe.depth.decode_glb(git('show', BASELINE + ':' + CHECKPOINTS + slug + '/' + slug + '.glb'))
        after, after_binary = recipe.depth.decode_glb((ROOT / CHECKPOINTS / slug / (slug + '.glb')).read_bytes())
        for key in ('materials', 'images', 'textures', 'samplers', 'meshes', 'nodes', 'scenes'):
            assert before[key] == after[key], (slug, 'retained GLB binding changed', key)
        metadata = json.loads((ROOT / CHECKPOINTS / slug / 'checkpoint.json').read_text())
        for mesh, marker in zip(before['meshes'], metadata['markers']['depth_parts'], strict=True):
            attributes = mesh['primitives'][0]['attributes']
            depth = marker['factor']
            assert depth in (1, 1.2, 1.5)
            p = recipe.depth.accessor(before, before_binary, attributes['POSITION'])
            q = recipe.depth.accessor(after, after_binary, attributes['POSITION'])
            assert np.array_equal(p[:, :2], q[:, :2]), (slug, 'accepted face-on silhouette changed')
            assert np.array_equal(np.rint(p[:, 2] * depth), q[:, 2]), (slug, 'unexpected depth')
            for index in (attributes['TEXCOORD_0'], mesh['primitives'][0]['indices']):
                assert np.array_equal(recipe.depth.accessor(before, before_binary, index),
                                      recipe.depth.accessor(after, after_binary, index)), (slug, 'UV or topology changed')
        report['retained_designs'][slug] = {'unchanged_resource_files': len(hashes), 'sha256': hashes,
                                           'face_on_positions_uvs_topology_materials_preserved': True}
    for slug in CHANGED:
        checkpoint = ROOT / CHECKPOINTS / slug
        metadata = json.loads((checkpoint / 'checkpoint.json').read_text())
        provenance = metadata['source_provenance']
        assert provenance['baseline_commit'] == BASELINE
        assert provenance['bundle'] == recipe.BUNDLE
        resources = ROOT / ASSETS / slug
        hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in resources.iterdir()}
        expected = provenance['resource_sha256']
        assert all(hashes.get(name) == digest for name, digest in expected.items()), (slug, 'resource graph differs from candidate')
        for name in hashes.keys() - expected.keys():
            path = ASSETS + slug + '/' + name
            assert name.endswith('_tex') and (ROOT / path).read_bytes() == git('show', BASELINE + ':' + path), ('unreferenced baseline texture changed', path)
        assert hashlib.sha256((checkpoint / (slug + '.glb')).read_bytes()).hexdigest() == provenance['glb_sha256']
        report['candidates'][slug] = provenance
    for prefix in (ASSETS, CHECKPOINTS):
        changed = git('diff', '--name-only', BASELINE, '--', prefix).decode().splitlines()
        new = git('ls-files', '--others', '--exclude-standard', '--', prefix).decode().splitlines()
        assert all(path[len(prefix):].split('/')[0] in CHANGED for path in changed + new), ('unrelated asset edit', prefix)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = check()
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS five retained sword designs preserve front positions, UVs, topology, materials and resource bindings; ten candidate graphs match provenance; no unrelated GI edits')
