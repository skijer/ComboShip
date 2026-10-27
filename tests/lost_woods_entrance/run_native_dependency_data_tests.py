#!/usr/bin/env python3
"""Check the real complete-graph dependency parser at its Blob boundary."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = r'''
#include "combo/entrances/NativeForestDependencies.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main() {
    // Version1, one private path and one stock path, with LE16 path lengths.
    std::vector<uint8_t> bytes={1,1,0,1,0,8,0,'c','u','s','t','o','m','/','a',
                               8,0,'s','c','e','n','e','s','/','b'};
    auto graph=NativeForestDependencies::Parse(bytes.data(),bytes.size());
    assert(graph.core.size()==1 && graph.core[0]=="custom/a");
    assert(graph.stock.size()==1 && graph.stock[0]=="scenes/b");
    bytes.resize(bytes.size()+16,0);
    graph=NativeForestDependencies::Parse(bytes.data(),bytes.size());
    assert(graph.core.size()==1 && graph.stock.size()==1);
    bytes.back()=1; assert(NativeForestDependencies::Parse(bytes.data(),bytes.size()).core.empty());
    bytes.resize(bytes.size()-16);
    for(size_t size=0;size<bytes.size();++size)
        assert(NativeForestDependencies::Parse(bytes.data(),size).core.empty());
    auto invalid=bytes; invalid[0]=2;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    invalid=bytes; invalid[1]=0;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    invalid=bytes; invalid[3]=0;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    invalid=bytes; invalid[2]=17;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    invalid=bytes; invalid[5]=0; invalid[6]=3;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    for(const char* path:{"/unsafe/","a//path/","a/../xyz","a/./xyz/","a\\path/x","a/path//"}) {
        invalid=bytes;
        for(int i=0;i<8;++i)invalid[7+i]=path[i];
        assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    }
    invalid=bytes; invalid[18]=0;
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    invalid=bytes;
    for(int i=0;i<8;++i)invalid[17+i]=bytes[7+i];
    assert(NativeForestDependencies::Parse(invalid.data(),invalid.size()).core.empty());
    std::puts("PASS complete forest dependency data: private/stock groups, exact counts/paths, Blob padding, truncation and invalid-path rejection");
}
'''
with tempfile.TemporaryDirectory(prefix="native-forest-deps-") as directory:
    cpp=Path(directory)/"test.cpp"
    binary=Path(directory)/"test"
    cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++20","-Wall","-Wextra","-Werror",
                    "-fsanitize=address,undefined","-fno-sanitize-recover=all","-no-pie",
                    "-I"+str(ROOT),str(cpp),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
