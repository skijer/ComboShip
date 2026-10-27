"""
Write (and read back) SoH / 2Ship `.o2r` resources.

An `.o2r` is a ZIP (store) of libultraship resources. Each resource has a fixed
0x40-byte header then a body. We emit little-endian resources (ByteOrder=0):

  DisplayList (ODLT), Array-of-Vertex (OARR), Texture (OTEX).

Cross-resource references inside a DisplayList use the OTR "hash" opcodes: the
top opcode byte is remapped and the command is widened to 128 bits, the extra
64 bits carrying CRC64(referenced archive path).  Spec + byte layouts verified
against a real 2Ship model (see work/RESEARCH_o2r.md).
"""
from __future__ import annotations
import struct
import zipfile
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# ---- resource type fourccs (stored as u32 in the resource endianness) --------
T_ODLT = 0x4F444C54  # "ODLT" DisplayList
T_OARR = 0x4F415252  # "OARR" Array
T_OTEX = 0x4F544558  # "OTEX" Texture
T_OSKL = 0x4F534B4C  # "OSKL" Skeleton
T_OSLB = 0x4F534C42  # "OSLB" SkeletonLimb

# SkeletonType / LimbType enums (Shipwright resource/type/*.h)
SKEL_NORMAL, SKEL_FLEX, SKEL_CURVE = 0, 1, 2
LIMB_INVALID, LIMB_STANDARD, LIMB_LOD, LIMB_SKIN, LIMB_CURVE, LIMB_LEGACY = 0, 1, 2, 3, 4, 5

OTR_HEADER_SIZE = 0x40
DEADBEEF = 0xDEADBEEFDEADBEEF

ARRAY_TYPE_VERTEX = 0x19
UCODE_F3DEX2 = 0x04

# OTR expanded (128-bit) opcodes
G_MARKER = 0x33
G_VTX_OTR_HASH = 0x32
G_DL_OTR_HASH = 0x31
G_SETTIMG_OTR_HASH = 0x20

# plain F3DEX2 opcodes we emit inline
G_ENDDL = 0xDF
G_TRI1 = 0x05
G_TRI2 = 0x06
G_SETTILE = 0xF5
G_SETTILESIZE = 0xF2
G_LOADBLOCK = 0xF3
G_RDPLOADSYNC = 0xE6
G_RDPTILESYNC = 0xE8
G_RDPPIPESYNC = 0xE7
G_SETCOMBINE = 0xFC
G_TEXTURE = 0xD7
G_GEOMETRYMODE = 0xD9
G_SETOTHERMODE_H = 0xE3
G_SETOTHERMODE_L = 0xE2

# TextureType enum
TEX_RGBA16 = 2
TEX_RGBA32 = 1


# ------------------------------------------------------------------ CRC64
_POLY = 0x42F0E1EBA9EA3693


def _crc_table():
    t = []
    for n in range(256):
        c = n << 56
        for _ in range(8):
            c = ((c << 1) ^ _POLY) & 0xFFFFFFFFFFFFFFFF if (c >> 63) & 1 else (c << 1) & 0xFFFFFFFFFFFFFFFF
        t.append(c)
    return t


_CRC_TBL = _crc_table()


def crc64(path: str) -> int:
    """CRC-64/ECMA-182, init all-ones, NO final xor (libultraship StrHash64)."""
    crc = 0xFFFFFFFFFFFFFFFF
    for ch in path.encode("ascii"):
        crc = (_CRC_TBL[((crc >> 56) ^ ch) & 0xFF] ^ (crc << 8)) & 0xFFFFFFFFFFFFFFFF
    return crc


# ------------------------------------------------------------------ header
def _header(rtype: int, version: int = 0) -> bytes:
    b = bytearray(OTR_HEADER_SIZE)
    b[0] = 0                       # ByteOrder = Little
    b[1] = 1                       # IsCustom
    struct.pack_into("<I", b, 0x04, rtype)
    struct.pack_into("<I", b, 0x08, version)
    struct.pack_into("<Q", b, 0x0C, DEADBEEF)
    return bytes(b)


# ------------------------------------------------------------------ Vertex
def build_vertex_array(verts) -> bytes:
    """verts: iterable of (x,y,z,flag,s,t,cn0,cn1,cn2,cn3)."""
    out = bytearray(_header(T_OARR))
    verts = list(verts)
    out += struct.pack("<II", ARRAY_TYPE_VERTEX, len(verts))
    for (x, y, z, flag, s, t, c0, c1, c2, c3) in verts:
        out += struct.pack("<hhhHhhBBBB", x, y, z, flag, s, t,
                           c0 & 0xFF, c1 & 0xFF, c2 & 0xFF, c3 & 0xFF)
    return bytes(out)


# ------------------------------------------------------------------ Texture
def build_texture_rgba16(rgba16: bytes, w: int, h: int) -> bytes:
    out = bytearray(_header(T_OTEX, 0))
    out += struct.pack("<IIII", TEX_RGBA16, w, h, len(rgba16))
    out += rgba16
    return bytes(out)


# ------------------------------------------------------------------ Skeleton / Limb
def _str(s: str) -> bytes:
    b = s.encode("ascii")
    return struct.pack("<I", len(b)) + b


def build_skeleton(limb_paths, limb_type=LIMB_LOD, skel_type=SKEL_FLEX,
                   dlist_count=0) -> bytes:
    """OSKL: Flex skeleton referencing its limb resources by path."""
    out = bytearray(_header(T_OSKL))
    out += struct.pack("<bbII", skel_type, limb_type, len(limb_paths), dlist_count)
    out += struct.pack("<bI", limb_type, len(limb_paths))   # limbTableType, limbTableCount
    for p in limb_paths:
        out += _str(p)
    return bytes(out)


def build_limb(joint_pos, child_index, sibling_index,
               near_dl_path="", far_dl_path="", limb_type=LIMB_LOD) -> bytes:
    """OSLB: one LOD/standard skeleton limb (matches base game field order)."""
    out = bytearray(_header(T_OSLB))
    out.append(limb_type & 0xFF)        # limbType (int8)
    out.append(0)                       # skinSegmentType (int8)
    out += _str("")                     # skinDList
    out += struct.pack("<H", 0)         # skinVtxCnt
    out += struct.pack("<I", 0)         # skinLimbModifCount
    out += _str("")                     # skinDList2
    out += struct.pack("<fff", 0.0, 0.0, 0.0)   # legTrans
    out += struct.pack("<HHH", 0, 0, 0)         # rot
    out += _str("")                     # childPtr (base leaves empty; uses index)
    out += _str("")                     # siblingPtr
    out += _str(near_dl_path)           # dListPtr
    out += _str(far_dl_path)            # dList2Ptr
    out += struct.pack("<hhh", int(joint_pos[0]), int(joint_pos[1]), int(joint_pos[2]))
    out.append(child_index & 0xFF)
    out.append(sibling_index & 0xFF)
    return bytes(out)


# ------------------------------------------------------------------ DisplayList
class DLBuilder:
    """Accumulate F3DEX2 gfx words (8 or 16 byte) for one ODLT resource."""

    def __init__(self):
        self.words: List[int] = []      # flat list of u32 words

    def _w(self, *ws):
        self.words.extend(ws)

    def marker(self, self_path: str):
        h = crc64(self_path)
        self._w(G_MARKER << 24, 0xBEEFBEEF, h >> 32, h & 0xFFFFFFFF)

    def settimg_hash(self, fmt: int, siz: int, width: int, tex_path: str):
        # w0 low 24 bits carry fmt/siz per gsDPSetTextureImage encoding
        w0 = (G_SETTIMG_OTR_HASH << 24) | ((fmt & 7) << 21) | ((siz & 3) << 19) | ((width - 1) & 0xFFF)
        h = crc64(tex_path)
        self._w(w0, 0, h >> 32, h & 0xFFFFFFFF)

    def vtx_hash(self, numv: int, dst: int, byte_off: int, vtx_path: str):
        w0 = (G_VTX_OTR_HASH << 24) | ((numv & 0xFF) << 12) | (((dst + numv) & 0x7F) << 1)
        h = crc64(vtx_path)
        self._w(w0, byte_off, h >> 32, h & 0xFFFFFFFF)

    def dl_hash(self, dl_path: str, push: bool = True):
        w0 = (G_DL_OTR_HASH << 24) | (0 if push else 0x00010000)
        h = crc64(dl_path)
        self._w(w0, 0, h >> 32, h & 0xFFFFFFFF)

    def raw(self, w0: int, w1: int):
        self._w(w0 & 0xFFFFFFFF, w1 & 0xFFFFFFFF)

    def tri1(self, a: int, b: int, c: int):
        self._w((G_TRI1 << 24) | ((a * 2) << 16) | ((b * 2) << 8) | (c * 2), 0)

    def end(self):
        self._w(G_ENDDL << 24, 0)

    def to_resource(self) -> bytes:
        out = bytearray(_header(T_ODLT))
        out.append(UCODE_F3DEX2)
        while len(out) % 8 != 0:
            out.append(0xFF)
        for w in self.words:
            out += struct.pack("<I", w & 0xFFFFFFFF)
        return bytes(out)


# ------------------------------------------------------------------ archive
NATIVE_NPC_PREFIXES = tuple("objects/" + name + "/NativeNpcV1/" for name in
                            ("object_sa", "object_kw1", "object_ma1", "object_ma2"))
_HASH_EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def repair_native_npc_hashes(records, reference_names=()):
    """Bind alt-only private NPC references to literal indexed archive names.

    The four supported families are confirmed to use this private contract.
    Native resource hashes and unrelated model namespaces remain untouched.
    Return a copy: a writer must not mutate its source model's resource map.
    """
    names = set(records) | set(reference_names)
    literal = {crc64(name) for name in names}
    replacements = {crc64(name[4:]): name for name in names
                    if name.startswith("alt/") and name[4:].startswith(NATIVE_NPC_PREFIXES)
                    and name[4:] not in names}
    repaired, changes = dict(records), []
    for name, raw in records.items():
        if not name.startswith("alt/") or not name[4:].startswith(NATIVE_NPC_PREFIXES) or raw[4:8] != b"TLDO":
            continue
        if raw[0] != 0 or len(raw) < 72 or raw[64] != UCODE_F3DEX2:
            raise ValueError("Expected little-endian F3DEX2 display list: " + name)
        data, offset = bytearray(raw), 72
        while offset + 8 <= len(data):
            opcode = struct.unpack_from("<I", data, offset)[0] >> 24
            size = 16 if opcode in _HASH_EXPANDED else 8
            if offset + size > len(data):
                raise ValueError("Truncated display list: " + name)
            if opcode in _HASH_EXPANDED and opcode != G_MARKER:
                high, low = struct.unpack_from("<II", data, offset + 8)
                hashed = high << 32 | low
                target = replacements.get(hashed) if hashed not in literal else None
                if target:
                    actual = crc64(target)
                    struct.pack_into("<II", data, offset + 8, actual >> 32, actual & 0xffffffff)
                    changes.append(dict(resource=name, offset=offset + 8, target=target,
                                        previous_hash=f"{hashed:016x}", actual_hash=f"{actual:016x}"))
            offset += size
            if opcode == G_ENDDL:
                break
        repaired[name] = bytes(data)
    return repaired, changes


@dataclass
class O2R:
    root: str = "objects"
    files: Dict[str, bytes] = field(default_factory=dict)

    def add(self, path: str, data: bytes):
        self.files[path] = data

    def write(self, out_path: str, compression=zipfile.ZIP_STORED, timestamp=None):
        records, _ = repair_native_npc_hashes(self.files)
        with zipfile.ZipFile(out_path, "w", compression, compresslevel=6 if compression == zipfile.ZIP_DEFLATED else None) as z:
            for path, data in sorted(records.items()) if timestamp else records.items():
                entry = zipfile.ZipInfo(path, timestamp) if timestamp else path
                if timestamp:
                    entry.compress_type = compression
                z.writestr(entry, data)


# ------------------------------------------------------------------ reader (round-trip / QA)
def _read_header(data: bytes):
    bo = data[0]
    endt = "<" if bo == 0 else ">"
    rtype = struct.unpack_from(endt + "I", data, 0x04)[0]
    ver = struct.unpack_from(endt + "I", data, 0x08)[0]
    return bo, endt, rtype, ver


def read_texture(data: bytes) -> Tuple[int, int, int, bytes]:
    bo, e, rtype, ver = _read_header(data)
    ttype, w, h = struct.unpack_from(e + "III", data, 0x40)
    if ver == 0:
        size = struct.unpack_from(e + "I", data, 0x4C)[0]
        body = data[0x50:0x50 + size]
    else:
        size = struct.unpack_from(e + "I", data, 0x58)[0]
        body = data[0x5C:0x5C + size]
    return ttype, w, h, body


def read_vertex_array(data: bytes):
    bo, e, rtype, ver = _read_header(data)
    atype, count = struct.unpack_from(e + "II", data, 0x40)
    verts = []
    off = 0x48
    for _ in range(count):
        rec = struct.unpack_from(e + "hhhHhhBBBB", data, off)
        verts.append(rec)
        off += 16
    return verts
