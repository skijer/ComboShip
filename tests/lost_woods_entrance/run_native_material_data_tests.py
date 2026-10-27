#!/usr/bin/env python3
"""Validate stock material data before it can change GPU segment bindings."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#if __has_include("combo/entrances/NativeForestMaterials.h")
#include "combo/entrances/NativeForestMaterials.h"
#else
namespace NativeForestMaterials {
struct Layer { int8_t x,y; uint8_t width,height; };
struct Binding { uint8_t segment,count; Layer layers[2]; };
std::vector<Binding> Parse(const uint8_t*,size_t) { return {}; }
}
#endif
int main() {
 const uint8_t empty[]={1,0};
 assert(NativeForestMaterials::Parse(empty,sizeof(empty)).empty());
 std::vector<uint8_t> bytes={1,2,8,2,0,0,32,32,255,1,32,64,9,1,1,254,64,32};
 auto result=NativeForestMaterials::Parse(bytes.data(),bytes.size());
 assert(result.size()==2 && result[0].segment==8 && result[0].count==2);
 assert(result[0].layers[1].x==-1 && result[0].layers[1].y==1 && result[0].layers[1].height==64);
 assert(result[1].segment==9 && result[1].count==1 && result[1].layers[0].x==1 && result[1].layers[0].y==-2);
 bytes.resize(bytes.size()+16,0);
 assert(NativeForestMaterials::Parse(bytes.data(),bytes.size()).size()==2);
 bytes.back()=1;
 assert(NativeForestMaterials::Parse(bytes.data(),bytes.size()).empty());
 bytes.resize(18);bytes.push_back(0);
 assert(NativeForestMaterials::Parse(bytes.data(),bytes.size()).empty());
 bytes.resize(18);
 for(size_t offset: {size_t(0),size_t(1),size_t(2),size_t(3),size_t(6),size_t(12)}) {
  auto bad=bytes;
  if(offset==0)bad[offset]=2;
  else if(offset==1)bad[offset]=0;
  else if(offset==2)bad[offset]=7;
  else if(offset==3)bad[offset]=3;
  else if(offset==6)bad[offset]=0;
  else bad[offset]=8;
  assert(NativeForestMaterials::Parse(bad.data(),bad.size()).empty());
 }
 for(size_t length=0;length<bytes.size();++length)
  assert(NativeForestMaterials::Parse(bytes.data(),length).empty());
 assert(NativeForestMaterials::Parse(nullptr,0).empty());
 puts("PASS stock material data: exact signed rates/dimensions, factory padding, invalid/truncated/duplicate segment rejection");
}
'''
with tempfile.TemporaryDirectory(prefix="native-material-data-") as directory:
    source, binary = Path(directory)/"test.cpp", Path(directory)/"test"
    source.write_text(fixture)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I"+str(ROOT), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
