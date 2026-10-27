#!/usr/bin/env python3
"""Trace the Lake render fault using a minidump and its matching Windows DLLs.

Read-only evidence tool. Original game archives and dumps are never included in
the repository. Requires the shader-fault dump, not just a truncated game log.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def unpack(fmt, data, offset=0):
    return struct.unpack_from('<' + fmt, data, offset)


class Dump:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        assert self.data[:4] == b'MDMP'
        count, directory = unpack('II', self.data, 8)
        self.streams = {kind: (size, at) for kind, size, at in
                        (unpack('III', self.data, directory + 12*i) for i in range(count))}
        self.ranges = []
        if 5 in self.streams:
            _, at = self.streams[5]
            for i in range(unpack('I', self.data, at)[0]):
                self.ranges.append(unpack('QII', self.data, at + 4 + 16*i))
        if 9 in self.streams:
            _, at = self.streams[9]
            count, payload = unpack('QQ', self.data, at)
            for i in range(count):
                base, extent = unpack('QQ', self.data, at + 16 + 16*i)
                self.ranges.append((base, extent, payload))
                payload += extent
        self.modules = {}
        _, at = self.streams[4]
        for i in range(unpack('I', self.data, at)[0]):
            start = at + 4 + 108*i
            base, size, _, timestamp, name_at = unpack('QIIII', self.data, start)
            length = unpack('I', self.data, name_at)[0]
            name = self.data[name_at+4:name_at+4+length].decode('utf-16le').replace('\\', '/').split('/')[-1]
            self.modules[name.lower()] = dict(base=base, size=size, timestamp=timestamp)

    def read(self, address, size):
        for base, extent, at in self.ranges:
            if base <= address and address + size <= base + extent:
                return self.data[at + address-base:at + address-base + size]
        raise ValueError(f'Memory not captured: {address:#x}+{size:#x}')


class PE:
    def __init__(self, path, module):
        self.data = Path(path).read_bytes()
        self.base = module['base']
        p = unpack('I', self.data, 0x3c)[0]
        assert self.data[p:p+4] == b'PE\0\0'
        n, timestamp = unpack('HI', self.data, p+6)
        size = unpack('H', self.data, p+20)[0]
        optional = p+24
        assert unpack('H', self.data, optional)[0] == 0x20b
        self.preferred = unpack('Q', self.data, optional+24)[0]
        assert timestamp == module['timestamp'], 'DLL timestamp differs from dump'
        assert unpack('I', self.data, optional+56)[0] == module['size'], 'DLL extent differs from dump'
        self.sections = [unpack('IIII', self.data, optional+size+40*i+8) for i in range(n)]
        self.relocations = set()
        rva, extent = unpack('II', self.data, optional+112+5*8)
        end = rva+extent
        while rva < end:
            page, block_size = unpack('II', self.data, self.offset(rva))
            assert block_size >= 8
            for i in range((block_size-8)//2):
                entry = unpack('H', self.data, self.offset(rva)+8+2*i)[0]
                if entry >> 12 == 10:
                    self.relocations.add(page+(entry & 4095))
            rva += block_size
        self.symbols = {}
        rva, _ = unpack('II', self.data, optional+112)
        if rva:
            e = self.offset(rva)
            _, _, names_count, functions, names, ordinals = unpack('IIIIII', self.data, e+16)
            for i in range(names_count):
                name_at = self.offset(unpack('I', self.data, self.offset(names)+4*i)[0])
                name = self.data[name_at:self.data.index(0, name_at)].decode()
                ordinal = unpack('H', self.data, self.offset(ordinals)+2*i)[0]
                self.symbols[name] = unpack('I', self.data, self.offset(functions)+4*ordinal)[0]

    def offset(self, rva):
        for virtual_size, va, raw_size, raw in self.sections:
            if va <= rva < va + max(virtual_size, raw_size):
                return raw+rva-va
        raise ValueError(f'No PE section at RVA {rva:#x}')

    def words(self, address):
        rva = address-self.base
        a, b = unpack('QQ', self.data, self.offset(rva))
        return tuple(v + (self.base-self.preferred if rva+8*i in self.relocations else 0)
                     for i, v in enumerate((a, b)))


def trace(dump_path, soh_path, lus_path, mm_path):
    dump = Dump(dump_path)
    soh = PE(soh_path, dump.modules['soh.dll'])
    lus = PE(lus_path, dump.modules['libultraship.dll'])
    _, exception = dump.streams[6]
    code = unpack('I', dump.data, exception+8)[0]
    fault_address = unpack('Q', dump.data, exception+48)[0]
    context_size, context_at = unpack('II', dump.data, exception+160)
    assert code == 0xc0000005 and context_size >= 256
    rcx, rdx = unpack('QQ', dump.data, context_at+128)
    rip = unpack('Q', dump.data, context_at+248)[0]
    shader = next(rva for name, rva in lus.symbols.items() if name.startswith('?gfx_push_shader@'))
    assert shader <= rip-lus.base < shader+0x100, 'Fault is outside shader dispatch'
    segaddr = next(rva for name, rva in lus.symbols.items() if name.startswith('?SegAddr@'))
    native = lus.data[lus.offset(segaddr):lus.offset(segaddr)+64]
    pattern = b'\x48\x8b\x8c\xc1'
    table_offset = unpack('I', native, native.index(pattern)+4)[0]
    segment_c = unpack('Q', dump.read(rcx+table_offset+12*8, 8))[0]
    command = unpack('Q', dump.read(rdx, 8))[0]
    assert soh.words(segment_c)[0] & 0xffffffff == 0xd9ffffff
    assert soh.words(segment_c)[1] == 0x400, 'Expected inherited front-cull list'
    assert soh.words(segment_c+16) == (0xdf000000, 0)
    pc = segment_c+2*16
    bad_opcodes = []
    shaders = []
    while pc <= command:
        w0, w1 = soh.words(pc)
        opcode = (w0 >> 24) & 255
        if opcode == 0xa9:
            bad_opcodes.append(pc)
        if opcode == 0x43:
            shaders.append(dict(pc=hex(pc), operand=hex(w1)))
        # The stray float is interpreted as the expanded hash-movemem opcode;
        # its handler consumes the following pointer pair as hash data.
        pc += 32 if opcode == 0x42 else 16
    assert len(bad_opcodes) == 9, bad_opcodes
    w0, operand = soh.words(command)
    assert (w0 >> 24) & 255 == 0x43 and operand & 1
    invalid_segment = (operand >> 24) & 0xffffffff
    computed_fault = rcx+table_offset+invalid_segment*8
    assert computed_fault == fault_address, (hex(computed_fault), hex(fault_address))
    with zipfile.ZipFile(mm_path) as archive:
        calls = 0
        for name in archive.namelist():
            if not name.startswith('objects/object_zog/') or '_DL_' not in name:
                continue
            data = archive.read(name)
            at = 72
            while at+8 <= len(data):
                w0, w1 = unpack('II', data, at)
                opcode = w0 >> 24
                if opcode == 0x3d:
                    assert w1 == 0x0c000002
                    calls += 1
                at += 16 if opcode in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42) else 8
                if opcode == 0xdf:
                    break
        assert calls > 0
    return dict(status='exact shader-fault reconstruction', segment_c=hex(segment_c),
                inherited_list='gCullFrontDList: two commands; index 2 is one past the end',
                native_mikau_segment_c_index_2_calls=calls, unhandled_a9=len(bad_opcodes),
                shader_commands=shaders, faulting_command=hex(command),
                computed_invalid_read=hex(computed_fault), dump_invalid_read=hex(fault_address),
                dump_sha256=hashlib.sha256(Path(dump_path).read_bytes()).hexdigest(),
                limitation='Exact second-crash trace; no GPU runtime acceptance or first-dump reconstruction')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dump', 'soh_dll', 'lus_dll', 'mm_archive'):
        parser.add_argument('--'+name.replace('_', '-'), required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(trace(args.dump, args.soh_dll, args.lus_dll, args.mm_archive), indent=2))
