#!/usr/bin/env python3
"""Completion must mean the OoT worker can accept the complementary ROM next."""
from pathlib import Path
import argparse
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
otr=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
start=otr.index('static std::atomic<size_t> gComboExtractCount')
end=otr.index('// Returns nonzero if romPath',start)
source=r'''
#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <string>
#include "combo/ComboExport.h"
'''+otr[start:end]+block_from(otr,otr.index('extern "C" COMBO_EXPORT void SOH_GetExtractionProgress'))+r'''
int main(){
    std::promise<void> published,release;
    auto publishedFuture=published.get_future(),releaseFuture=release.get_future();
    gComboExtractFuture=std::async(std::launch::async,[&]{
        gComboExtractSuccess=true;gComboExtractDone=true;
        published.set_value();releaseFuture.wait();
    });
    publishedFuture.wait();
    int done=-1,success=-1;
    SOH_GetExtractionProgress(nullptr,nullptr,&done,&success);
    assert(done==0 && "the UI must not start another OoT extraction while the previous future is not ready");
    release.set_value();gComboExtractFuture.wait();
    SOH_GetExtractionProgress(nullptr,nullptr,&done,&success);
    assert(done==1&&success==1);
}
'''
with tempfile.TemporaryDirectory(prefix='rom-setup-progress-') as tmp:
    cpp=Path(tmp)/'progress.cpp';binary=Path(tmp)/'progress';cpp.write_text(source)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-pthread','-I',str(ROOT)]
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],cwd=tmp,check=True)
print('OoT extraction progress: worker readiness before complementary extraction PASS')
