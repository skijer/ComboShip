"""An editable scene layer must survive Prelude's continue-editing export path."""
import json
import base64
from pathlib import Path
import sys
import subprocess
import zipfile

with zipfile.ZipFile(Path(sys.argv[1])) as archive:
    names=set(archive.namelist())
    assert 'prelude/manifest.json' in names, 'No Prelude project manifest; archive is only a runtime patch'
    manifest=json.loads(archive.read('prelude/manifest.json'))
    assert manifest['game']=='mm' and manifest['scenes']==['Z2_26SARUNOMORI']
    assert manifest['altAssets']
    assert isinstance(manifest['project'].get('previewedAgainst'),list), 'Prelude rejects a project without previewedAgainst'
    layers=manifest['project']['layers']
    assert len(layers)==1 and layers[0]['kind']=='scene-replace'
    assert layers[0]['targetScene']=='Z2_26SARUNOMORI'
    canonical=lambda n:n.removeprefix('alt/')
    assert set(layers[0]['entries'])=={canonical(n) for n in names if n.startswith('alt/')}, 'Layer entries must be canonical for reopening a Prelude export'
    edits=json.loads(archive.read('prelude/project/edits.json'))
    assert edits=={'codec':2,'blobs':[],'edits':{}}
    assert archive.read('prelude/project/blobs.bin')==b''
    resources={n:base64.b64encode(archive.read(n)).decode() for n in names if n.startswith('alt/')}
if len(sys.argv)>2:
    subprocess.run(['node',str(Path(__file__).with_name('probe_prelude_project.cjs')),sys.argv[2]],
                   input=json.dumps(dict(manifest=manifest,files=resources)),text=True,check=True)
print('PASS: complete editable Prelude scene layer and empty baked edit state')
