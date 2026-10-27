#!/usr/bin/env python3
"""Execute real Ship manager/cache/loader methods against controlled archive bytes.

Archive mount/I/O, tracing and unused XML parsing are seams; no game runtime.
The private mutex has a native-lock wrapper to detect destruction during I/O.
Requires the project's public dependency headers (CPATH can supply them).
"""
import argparse, os, re, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--sanitize',action='store_true')
parser.add_argument('--case',choices=('root','alias','lifetime','owner'))
parser.add_argument('--source-root',type=Path,default=ROOT,help='optional pre-fix production source for regression control')
args=parser.parse_args()
def bodies(source,qualified):
    result=[]
    for match in re.finditer(r'^[^;{}]*\b'+re.escape(qualified)+r'\([^;{}]*\)(?:\s*const)?\s*\{',source,re.M):
        depth,end=1,match.end()
        while depth:
            depth+=(source[end]=='{')-(source[end]=='}');end+=1
        result.append(source[match.start():end])
    if not result:raise RuntimeError('missing production method: '+qualified)
    return '\n'.join(result)
manager=(args.source_root/'libultraship/src/ship/resource/ResourceManager.cpp').read_text()
rm_names=['ResourceFilter::ResourceFilter','ResourceIdentifier::ResourceIdentifier',
 'ResourceIdentifier::GetHash','ResourceIdentifier::CalculateHash','ResourceIdentifier::operator==',
 'ResourceIdentifierHash::operator()','ResourceManager::ResourceManager','ResourceManager::~ResourceManager',
 'ResourceManager::Init','ResourceManager::CreateResourceView','ResourceManager::IsLoaded',
 'ResourceManager::LoadFileProcess','ResourceManager::LoadResourceProcess','ResourceManager::LoadResourceAsync',
 'ResourceManager::LoadResource','ResourceManager::CheckCache','ResourceManager::GetCachedResource',
 'ResourceManager::OtrSignatureCheck','ResourceManager::IsAltAssetsEnabled','ResourceManager::SetAltAssetsEnabled',
 'ResourceManager::GetArchiveManager','ResourceManager::GetResourceLoader']
loader=(args.source_root/'libultraship/src/ship/resource/ResourceLoader.cpp').read_text()
rl_names=['ResourceLoader::ResourceLoader','ResourceLoader::~ResourceLoader',
 'ResourceLoader::RegisterResourceFactory','ResourceLoader::DecodeASCII','ResourceLoader::GetFactory',
 'ResourceLoader::GetResourceType','ResourceLoader::CreateDefaultResourceInitData',
 'ResourceLoader::ReadResourceInitData','ResourceLoader::ReadResourceInitDataBinary',
 'ResourceLoader::CreateBinaryReader','ResourceLoader::ResolveMetaAlias','SetBufferOffset','ResourceLoader::LoadResource']
source=(ROOT/'tests/sword_fallback/resource_view_test.cpp').read_text()
source=source.replace('/* RESOURCE_MANAGER */','\n'.join(bodies(manager,name) for name in rm_names))
source=source.replace('/* RESOURCE_LOADER */','\n'.join(bodies(loader,name) for name in rl_names))
registry=(args.source_root/'libultraship/src/ship/resource/CrossRMRegistry.cpp').read_text()
source=source.replace('/* RESOURCE_REGISTRY */',registry[registry.index('namespace Ship {'):])
helpers=(args.source_root/'soh/soh/ResourceManagerHelpers.cpp').read_text()
source=source.replace('/* BASE_OWNER */',bodies(helpers,'OOT_NeiEnsureGiBaseOwner'))
flags=['-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'libultraship/include'),'-I'+str(ROOT),'-pthread']
if args.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='sword-resource-view-') as tmp:
    header=(ROOT/'libultraship/include/ship/resource/ResourceManager.h').read_text()
    assert header.count('std::mutex mMutex;')==1
    instrumented=Path(tmp)/'include/ship/resource/ResourceManager.h'
    instrumented.parent.mkdir(parents=True)
    instrumented.write_text(header.replace('std::mutex mMutex;','ResourceLifetimeMutex mMutex;'))
    flags.insert(0,'-I'+str(Path(tmp)/'include'))
    cpp=Path(tmp)/'view.cpp';binary=Path(tmp)/'view'
    resource=(ROOT/'libultraship/src/ship/resource/Resource.cpp').read_text().replace('#include <spdlog/spdlog.h>','')
    cpp.write_text(source+'\n'+resource)
    extra=[ROOT/'libultraship/src/ship/utils'/name for name in
      ('Utils.cpp','binarytools/Stream.cpp','binarytools/MemoryStream.cpp','binarytools/BinaryReader.cpp')]
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),*map(str,extra),'-o',str(binary)],check=True)
    failed=[]
    for scenario in [args.case] if args.case else ('root','alias','lifetime','owner'):
        result=subprocess.run([str(binary),scenario],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        if result.returncode:failed.append(scenario)
    if failed:raise SystemExit('Failed production resource-view scenarios: '+', '.join(failed))
