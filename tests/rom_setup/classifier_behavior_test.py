#!/usr/bin/env python3
"""Run the real extractor header reader and exported quest classifier (no ROM assets)."""
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
extractor=(ROOT/'soh/soh/Extractor/Extract.cpp').read_text()
otr=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
source=r'''
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include "combo/ComboExtract.h"
#include "combo/ComboExport.h"
#include "soh/soh/Extractor/Extract.h"
#include "libultraship/include/ship/utils/binarytools/BitConverter.h"
#define UNREACHABLE __builtin_unreachable()
'''
source+=extractor[extractor.index('static constexpr uint32_t OOT_PAL_GC'):extractor.index('enum class ButtonId')]
for signature in ['void Extractor::SetRomInfo','uint32_t Extractor::GetRomVerCrc','size_t Extractor::GetCurRomSize',
                  'bool Extractor::ValidateRomSize','bool Extractor::ValidateNotCompressed',
                  'bool Extractor::ClassifyRom','bool Extractor::IsMasterQuest']:
    source+=block_from(extractor,extractor.index(signature))+'\n'
source+=block_from(otr,otr.index('extern "C" COMBO_EXPORT int SOH_ClassifyRomKind'))
source+=r'''
void WriteHeader(const std::string& path,uint32_t crc,int order){
    std::array<unsigned char,4096> header{};
    header[0]=0x80;header[1]=0x37;header[2]=0x12;header[3]=0x40;
    for(int i=0;i<4;++i)header[0x10+i]=(crc>>(24-8*i))&0xff;
    if(order==1)for(size_t i=0;i<header.size();i+=2)std::swap(header[i],header[i+1]);
    if(order==2)for(size_t i=0;i<header.size();i+=4){std::swap(header[i],header[i+3]);std::swap(header[i+1],header[i+2]);}
    std::ofstream file(path,std::ios::binary);file.write((const char*)header.data(),header.size());file.close();
    std::filesystem::resize_file(path,32*1024*1024);
}
int main(){
    struct Case{uint32_t crc;int want;};
    const Case cases[]={{0xEC7011B7,1},{0xF034001A,2},{0xF43B45BA,2},{0x1D4136F3,2},
                        {0x917D18F6,2},{0x871E1C92,1},{0x87121EFE,1},{0x09465AC3,1},{0xdeadbeef,0}};
    for(const auto& c:cases)for(int order=0;order<3;++order){
        const std::string path=order==0?"header.z64":order==1?"header.v64":"header.n64";
        WriteHeader(path,c.crc,order);
        assert(SOH_ClassifyRomKind(path.c_str())==c.want && "supported OoT headers must be classified by quest in all byte orders");
        std::filesystem::remove(path);
    }
    assert(SOH_ClassifyRomKind(nullptr)==0);
    assert(SOH_ClassifyRomKind("missing.z64")==0);
    assert(SOH_ClassifyRomKind(".")==0);
    std::ofstream("short.z64").put('x');
    assert(SOH_ClassifyRomKind("short.z64")==0);
}
'''
with tempfile.TemporaryDirectory(prefix='rom-setup-classifier-') as tmp:
    cpp=Path(tmp)/'classifier.cpp';binary=Path(tmp)/'classifier';cpp.write_text(source)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],cwd=tmp,check=True)
print('OoT classifier: 27 real header/byte-order cases, invalid paths and truncated files PASS')
