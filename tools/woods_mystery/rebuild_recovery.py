"""Restore the independent POC recovery package into new source/output folders."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parent)
    parser.add_argument('--native-tests',action='store_true')
    args=parser.parse_args();root=args.root.resolve()
    manifest=json.loads((root/'manifest.json').read_text())
    for name,expected in manifest['files'].items():
        if sha(root/name)!=expected:raise SystemExit('Input hash mismatch: '+name)
    source=root/'source'
    if source.exists():raise SystemExit('Restore requires a fresh package directory; source already exists')
    with tarfile.open(root/'source.tar.gz') as archive:
        archive.extractall(root,filter='data')
    def run(command,**kw):
        return subprocess.run(command,cwd=source,check=True,**kw)
    run(['git','init','-q'])
    run(['git','-c','core.autocrlf=false','add','--all','--force'])
    actual=run(['git','write-tree'],capture_output=True,text=True).stdout.strip()
    if actual!=manifest['source_tree']:raise SystemExit('Restored source tree differs: '+actual)
    output=root/'rebuilt';inputs=root/'inputs'
    run([sys.executable,'-B','tools/woods_mystery/build_candidate.py',
         '--native',str(inputs/'Woods-of-Mystery-Native-Resources.zip'),
         '--poc',str(inputs/'tunnel-source-resources.zip'),
         '--donor-archive-sha256',manifest['original_poc_sha256'],
         '--textures',str(inputs/'textures'),'--output',str(output)])
    candidate=output/manifest['candidate_name']
    if sha(candidate)!=manifest['candidate_sha256']:raise SystemExit('Candidate rebuild hash differs')
    run([sys.executable,'-B','tests/woods_mystery/verify_candidate.py',str(candidate),
         str(inputs/'Woods-of-Mystery-Native-Resources.zip')])
    run([sys.executable,'-B','tests/woods_mystery/test_tunnel_fit.py',str(inputs/'tunnel-source-resources.zip')])
    run([sys.executable,'-B','tests/woods_mystery/run_hex_test.py'])
    if args.native_tests:
        with tarfile.open(root/'native-test-headers.tar.gz') as archive:
            archive.extractall(root,filter='data')
        dependencies=root/'native-test-headers'
        include_paths=[dependencies/name for name in ['include','spdlog','nlohmann_json','imgui']]
        env=dict(os.environ,CPLUS_INCLUDE_PATH=os.pathsep.join(map(str,include_paths)))
        run([sys.executable,'-B','tests/woods_mystery/run_active_pack_test.py'],env=env)
        run([sys.executable,'-B','tests/lost_woods_entrance/run_tests.py'],env=env)
    receipt=dict(source_tree=actual,candidate_sha256=sha(candidate),exact_rebuild=True,
                 native_tests=args.native_tests,runtime_tested=False,user_accepted=False)
    (root/'cold-rebuild.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS: independent source tree restoration and byte-identical candidate rebuild')


if __name__=='__main__':main()
