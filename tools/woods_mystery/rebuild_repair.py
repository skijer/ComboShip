"""Rebuild the visual POC, or restore its full source using the saved POC1 recovery."""
import argparse,hashlib,json,os,shutil,subprocess,sys,tarfile,zipfile
from pathlib import Path


def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path(__file__).resolve().parent)
    p.add_argument('--base-recovery',type=Path)
    p.add_argument('--native-tests',action='store_true')
    a=p.parse_args();root=a.root.resolve();manifest=json.loads((root/'manifest.json').read_text())
    if a.native_tests and not a.base_recovery:p.error('--native-tests requires --base-recovery')
    for name,expected in manifest['files'].items():
        if sha(root/name)!=expected:raise SystemExit('Package hash mismatch: '+name)
    source=root/'source'
    if source.exists():raise SystemExit('Restore requires a fresh directory; source already exists')
    def run(command,**kw):return subprocess.run(command,cwd=source,check=True,**kw)
    if a.base_recovery:
        base=a.base_recovery.resolve()
        if sha(base)!=manifest['base_recovery']['sha256']:raise SystemExit('POC1 recovery hash mismatch')
        with zipfile.ZipFile(base) as z:
            with z.open('source.tar.gz') as raw,tarfile.open(fileobj=raw,mode='r|gz') as archive:
                archive.extractall(root,filter='data')
            if a.native_tests:
                with z.open('native-test-headers.tar.gz') as raw,tarfile.open(fileobj=raw,mode='r|gz') as archive:
                    archive.extractall(root,filter='data')
        run(['git','init','-q']);run(['git','-c','core.autocrlf=false','add','-Af'])
        tree=run(['git','write-tree'],capture_output=True,text=True).stdout.strip()
        if tree!=manifest['base_tree']:raise SystemExit('POC1 source tree mismatch: '+tree)
        run(['git','apply','--index','--binary',str(root/'source.patch')])
        tree=run(['git','write-tree'],capture_output=True,text=True).stdout.strip()
        if tree!=manifest['source_tree']:raise SystemExit('Repaired source tree mismatch: '+tree)
    else:
        with tarfile.open(root/'asset-source.tar.gz') as archive:archive.extractall(root,filter='data')
        for name,expected in manifest['asset_source_files'].items():
            if sha(source/name)!=expected:raise SystemExit('Asset source mismatch: '+name)
        tree=None
    inputs=root/'inputs';output=root/'rebuilt'
    def build(source_dir,output_dir):
        subprocess.run([sys.executable,'-B','tools/woods_mystery/build_candidate.py',
            '--native',str(inputs/'Woods-of-Mystery-Native-Resources.zip'),
            '--poc',str(inputs/'tunnel-source-resources.zip'),
            '--donor-archive-sha256',manifest['original_poc_sha256'],
            '--textures',str(inputs/'textures'),'--foliage',str(inputs/'foliage'),'--output',str(output_dir)],
            cwd=source_dir,check=True)
    # Tiny complete builder snapshots replace redundant 13 MB control packs.
    # Each historical candidate is rebuilt and checked against its original hash.
    for control in manifest.get('control_rebuilds',[]):
        directory=root/'control-builds'/control['name'];directory.mkdir(parents=True,exist_ok=False)
        with tarfile.open(root/control['source_archive']) as archive:archive.extractall(directory,filter='data')
        build(directory/'source',directory/'rebuilt')
        rebuilt=directory/'rebuilt'/control['candidate_name']
        if sha(rebuilt)!=control['sha256']:raise SystemExit('Historical control differs: '+control['name'])
        shutil.copyfile(rebuilt,inputs/control['input_name'])
    build(source,output)
    candidate=output/manifest['candidate_name'];native=inputs/'Woods-of-Mystery-Native-Resources.zip'
    if sha(candidate)!=manifest['candidate_sha256']:raise SystemExit('Rebuilt candidate SHA256 differs')
    tests=[['verify_candidate.py',candidate,native],['test_prelude_mesh_path.py',candidate],
           ['test_prelude_project.py',candidate,root/'reference/prelude-editor.js'],
           ['test_floor_layers.py',candidate],['test_raw_texture_stride.py',candidate],
           ['test_foliage.py',candidate,native,inputs/'foliage'],
           ['test_tunnel_fit.py',inputs/'tunnel-source-resources.zip']]
    for test,*args in tests:run([sys.executable,'-B','tests/woods_mystery/'+test,*map(str,args)])
    approved=inputs/'approved-bridge-poc2.prelude.o2r'
    if approved.exists():
        run([sys.executable,'-B','tests/woods_mystery/test_tunnel_seams.py',str(candidate),str(native),str(approved),
             '--donor',str(inputs/'tunnel-source-resources.zip')])
    previous=inputs/'coverage-poc3.prelude.o2r'
    if previous.exists():
        run([sys.executable,'-B','tests/woods_mystery/test_wall_fit.py',str(candidate),str(native),str(previous)])
    previous=inputs/'fit-poc4.prelude.o2r'
    if previous.exists():
        run([sys.executable,'-B','tests/woods_mystery/test_backdrop_gaps.py',str(candidate),str(native),str(previous)])
    previous=inputs/'fit-poc5.prelude.o2r'
    if previous.exists():
        run([sys.executable,'-B','tests/woods_mystery/test_shell_grass.py',str(candidate),str(native),str(previous)])
        run([sys.executable,'-B','tests/woods_mystery/run_native_assets_test.py','--native',str(native),'--accepted',str(previous)])
    run([sys.executable,'-B','tests/woods_mystery/test_canopy_join.py',str(candidate),str(native)])
    previous=inputs/'fit-poc6.prelude.o2r'
    if previous.exists():
        run([sys.executable,'-B','tests/woods_mystery/test_floor_contact.py',str(candidate),str(native),str(previous),
             '--previous-native-assets',str(root/'control-builds/poc6/source/mm/assets/custom')])
        run([sys.executable,'-B','tests/woods_mystery/test_wall_fins.py',str(candidate),str(native),str(previous)])
    run([sys.executable,'-B','tests/woods_mystery/test_bridge_canopy_clearance.py',str(candidate),str(native)])
    if a.native_tests:
        deps=root/'native-test-headers'
        env=dict(os.environ,CPLUS_INCLUDE_PATH=os.pathsep.join(str(deps/n) for n in ['include','spdlog','nlohmann_json','imgui']))
        for test in ['woods_mystery/run_hex_test.py','woods_mystery/run_active_pack_test.py',
                     'woods_mystery/run_native_bridge_test.py','woods_mystery/run_seasonal_grass.py',
                     'lost_woods_entrance/run_tests.py']:
            run([sys.executable,'-B','tests/'+test],env=env)
        run([sys.executable,'-B','tests/woods_mystery/run_seasonal_targets.py',str(candidate),
             '--native',str(native)],env=env)
    receipt=dict(exact_rebuild=True,candidate_sha256=sha(candidate),full_source_tree=tree,
                 historical_controls=[c['name'] for c in manifest.get('control_rebuilds',[])],
                 native_tests=a.native_tests,runtime_tested=False,user_accepted=False)
    (root/'cold-rebuild.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS: independent recovery and byte-identical visual archive rebuild')


if __name__=='__main__':main()
