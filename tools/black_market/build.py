#!/usr/bin/env python3
"""Build dependency-closed private MM and OoT shops from retained Djipi/Goth donors.

Inputs are never edited. Output is a standalone O2R plus an editable resource
directory, with donor hashes and the complete material/collision inventory.
"""
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as E
import zipfile
from keeper_lean import keeper_contract

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = "custom/skiejer_market/"
SCENE = "scenes/nonmq/skiejer_market/"
COSMETIC_CATEGORY = "Skiejer's Black Market"
PALETTE = [(165,148,230), (135,194,210), (204,170,215), (175,160,216)]
COSMETIC_LABELS = ["Black Market Violet Stone", "Black Market Teal Stone",
                   "Black Market Rose Stone", "Black Market Lavender Stone"]

def header(kind, version=0):
    return struct.pack("<B3xIIQ", 0, kind, version, 0xDEADBEEFDEADBEEF).ljust(64, b"\0")

def string(value):
    value = value.encode()
    return struct.pack("<I", len(value)) + value

def command(number, value=b""):
    return struct.pack("<I", number) + value

def actor(identity, x, y, z, yaw=0, params=0):
    return struct.pack("<HhhhhhhH", identity, x, y, z, 0, yaw, 0, params)

def xml(tree):
    E.indent(tree, space="  ")
    return E.tostring(tree, encoding="utf-8")

def archive(path, storm):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as pack:
            return {n: pack.read(n) for n in pack.namelist() if not n.endswith("/")}
    lib = C.CDLL(str(storm))
    lib.SFileOpenArchive.argtypes = [C.c_char_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_void_p)]
    lib.SFileOpenArchive.restype = C.c_bool
    lib.SFileOpenFileEx.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32, C.POINTER(C.c_void_p)]
    lib.SFileOpenFileEx.restype = C.c_bool
    lib.SFileGetFileSize.argtypes = [C.c_void_p, C.POINTER(C.c_uint32)]
    lib.SFileGetFileSize.restype = C.c_uint32
    lib.SFileReadFile.argtypes = [C.c_void_p, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32), C.c_void_p]
    lib.SFileReadFile.restype = C.c_bool
    lib.SFileCloseFile.argtypes = [C.c_void_p]
    lib.SFileCloseArchive.argtypes = [C.c_void_p]
    handle = C.c_void_p()
    if not lib.SFileOpenArchive(str(path).encode(), 0, 0, C.byref(handle)):
        raise ValueError(f"cannot open MPQ donor {path}")
    def read(name):
        file = C.c_void_p()
        if not lib.SFileOpenFileEx(handle, name.encode(), 0, C.byref(file)):
            raise ValueError(f"missing MPQ resource {name}")
        try:
            high, amount = C.c_uint32(), C.c_uint32()
            size = lib.SFileGetFileSize(file, C.byref(high))
            if high.value or size == 0xFFFFFFFF: raise ValueError("invalid donor file size")
            data = C.create_string_buffer(size)
            if not lib.SFileReadFile(file, data, size, C.byref(amount), None) or amount.value != size:
                raise ValueError(f"incomplete donor resource {name}")
            return data.raw
        finally:
            lib.SFileCloseFile(file)
    try:
        names = read("(listfile)").decode().splitlines()
        # Only the complete selected donor and magical reference are retained.
        return {n.replace("\\", "/"): read(n) for n in names
                if "mahouya_scene/" in n or "kenjyanoma_scene/" in n}
    finally:
        lib.SFileCloseArchive(handle)

def crc64(value):
    crc = 0xFFFFFFFFFFFFFFFF
    for byte in value.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ (0x42F0E1EBA9EA3693 if crc >> 63 else 0)) & 0xFFFFFFFFFFFFFFFF
    return crc

def scope_keeper(resources):
    paths = {n.removeprefix("alt/"): PRIVATE + "keeper/" + n.rsplit("/", 1)[1] for n in resources}
    result = {}
    hashes = {crc64(prefix + old): crc64(new) for old, new in paths.items() for prefix in ("", "alt/")}
    for name, original in resources.items():
        data = original
        # Skeleton/limb paths are length-prefixed. Keep the resource reader's
        # binary framing correct when moving them to a longer private path.
        for old, new in paths.items():
            for prefix in ("", "__OTR__"):
                data = data.replace(string(prefix + old), string(prefix + new))
        # Packed display lists store the hash as high/low 32-bit words after
        # expanded resource commands. Never replace arbitrary vertex bytes.
        if data[4:8] == b"TLDO":
            value = bytearray(data)
            offset = 72
            expanded = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
            while offset + 8 <= len(value):
                w0, w1 = struct.unpack_from("<II", value, offset)
                opcode = w0 >> 24
                offset += 8
                if opcode in expanded:
                    high, low = struct.unpack_from("<II", value, offset)
                    old_hash = (high << 32) | low
                    if old_hash in hashes:
                        new_hash = hashes[old_hash]
                        struct.pack_into("<II", value, offset, new_hash >> 32, new_hash & 0xFFFFFFFF)
                    else:
                        raise ValueError(f"unresolved keeper dependency in {name}: {old_hash:016x}")
                    offset += 8
                if opcode in (0xDF, 0xB8): break
            data = bytes(value)
        if b"objects/object_bg/" in data:
            raise ValueError(f"unscoped keeper string in {name}")
        result[paths[name.removeprefix("alt/")]] = data
    return result

def triangles(resources):
    # Follow only reachable draw lists: an unused donor tri_27 contains
    # interior objects that must never become invisible collision obstacles.
    result, buffer, active = [], {}, set()

    def visit(path):
        if path in active:
            raise ValueError(f"recursive donor display list: {path}")
        active.add(path)
        tree = E.fromstring(resources["alt/" + path])
        index = int(path.rsplit("_tri_", 1)[1]) if "_tri_" in path else None
        for element in tree:
            if element.tag == "CallDisplayList":
                visit(element.get("Path"))
            elif element.tag == "LoadVertices":
                vertices = E.fromstring(resources["alt/" + element.get("Path")])
                offset = int(element.get("VertexOffset"))
                start = int(element.get("VertexBufferIndex"))
                for i, vertex in enumerate(list(vertices)[offset:offset + int(element.get("Count"))]):
                    buffer[start + i] = tuple(int(vertex.get(a)) for a in ("X", "Y", "Z"))
            elif element.tag == "Triangles2":
                for suffix in ("0", "1"):
                    result.append((index, tuple(buffer[int(element.get("V" + suffix + str(i)))] for i in range(3))))
            elif element.tag == "Triangle1":
                result.append((index, tuple(buffer[int(element.get("V0" + str(i)))] for i in range(3))))
            elif element.tag.startswith("Triangle"):
                raise ValueError(f"unsupported donor triangle command: {element.tag}")
        active.remove(path)

    visit("scenes/shared/mahouya_scene/mahouya_room_0DL_006548")
    return result

def collision(faces):
    vertices, indices, polygons = [], {}, []
    for _, points in faces:
        a, b, c = points
        ab = tuple(b[i] - a[i] for i in range(3))
        ac = tuple(c[i] - a[i] for i in range(3))
        normal = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        length = math.sqrt(sum(n*n for n in normal))
        if length < .1: continue
        normal = tuple(n / length for n in normal)
        row = []
        for point in points:
            if point not in indices:
                indices[point] = len(vertices); vertices.append(point)
            row.append(indices[point])
        n = tuple(max(-32767, min(32767, round(v * 32767))) for v in normal)
        distance = round(-sum(normal[i] * a[i] for i in range(3)))
        polygons.append(struct.pack("<HHHHhhhh", 0, *row, *n, distance))
    if len(vertices) >= 8192: raise ValueError("collision exceeds the native 13-bit vertex index")
    bounds = [min(v[i] for v in vertices) for i in range(3)] + [max(v[i] for v in vertices) for i in range(3)]
    data = header(0x4F434F4C) + struct.pack("<6hI", *bounds, len(vertices))
    data += b"".join(struct.pack("<3h", *p) for p in vertices)
    data += struct.pack("<I", len(polygons)) + b"".join(polygons)
    data += struct.pack("<III", 1, 0, 0)  # one ordinary surface, no exit index
    data += struct.pack("<I Hhi II", 1, 0, 0, -1, 0, 0) # normal camera, no positions/water
    return data, {"vertices": len(vertices), "triangles": len(polygons), "bounds": bounds}

def room_spawn_contract():
    source = (ROOT / 'combo/entrances/BlackMarketEntrance.h').read_text()
    values = {}
    for name in ('X', 'Y', 'Z', 'YAW'):
        match = re.search(r'^#define COMBO_BLACK_MARKET_SPAWN_' + name + r'\s+\(?(-?\d+)\)?$', source, re.M)
        assert match, 'Inspect the changed shared native room spawn contract'
        values[name] = int(match[1])
    return values


def scene_resources():
    keeper = keeper_contract()
    spawn = room_spawn_contract()
    assert all(value == round(value) for value in keeper["position"]), "Native room actors require integer positions"
    room = SCENE + "skiejer_market_room_0"
    light = bytes([65, 52, 90, 49, 49, 49, 160, 135, 200, 207, 207, 207, 65, 85, 110, 24, 17, 39])
    light += struct.pack("<hH", 980, 3500)
    commands = [command(0x15, bytes([0, 0xFF, 0xFF])),
                command(0x04, struct.pack("<I", 1) + string(room) + struct.pack("<II", 0, 0)),
                command(0x03, string(SCENE + "collision")),
                command(0x06, struct.pack("<Ibb", 1, 0, 0)),
                command(0x07, struct.pack("<bh", 0, 2)),
                command(0x00, struct.pack("<I", 1) + actor(0, spawn['X'], spawn['Y'], spawn['Z'], spawn['YAW'], 0x0EFF)),
                command(0x11, bytes([0, 0, 0, 1])),
                command(0x13, struct.pack("<I", 0)),
                command(0x0F, struct.pack("<I", 1) + light),
                command(0x1B, struct.pack("<I", 0)), command(0x14)]
    result = {SCENE + "skiejer_market": header(0x4F524F4D) + struct.pack("<I", len(commands)) + b"".join(commands)}
    commands = [command(0x08, bytes([0, 0, 0, 0, 1, 0])),
                command(0x0B, struct.pack("<IH", 1, 0x129)),
                command(0x01, struct.pack("<I", 1) + actor(0x1C1, *map(round, keeper["position"]), keeper["yaw"])),
                command(0x0A, bytes([0, 0, 1, 0]) + string(PRIVATE + "room") + string("")),
                command(0x10, bytes([0xFF, 0xFF, 0])),
                command(0x12, bytes([1, 1])), command(0x16, bytes([0])), command(0x14)]
    result[room] = header(0x4F524F4D) + struct.pack("<I", len(commands)) + b"".join(commands)
    return result

def oot_scene_resources():
    """Native SoH scene headers sharing only compatible room/model resources.

    SoH resolves this non-dungeon scene through scenes/shared, not nonmq.
    Its room behavior factory reads int8+int32; MM actor-cutscene command 1B
    is unsupported. The native controller owns the keeper carrier, so neither
    a vanilla shopkeeper nor MM actor IDs are embedded in this room.
    """
    spawn = room_spawn_contract()
    prefix = "scenes/shared/skiejer_market_oot/skiejer_market_oot"
    room = prefix + "_room_0"
    light = bytes([65,52,90,49,49,49,160,135,200,207,207,207,65,85,110,24,17,39])
    light += struct.pack("<hH", 980, 3500)
    commands = [command(0x15, bytes([0,0xFF,0xFF])),
                command(0x04, struct.pack("<I",1) + string(room) + struct.pack("<II",0,0)),
                command(0x03, string(SCENE + "collision")),
                command(0x06, struct.pack("<Ibb",1,0,0)),
                command(0x07, struct.pack("<bh",0,2)),
                command(0x00, struct.pack("<I",1) + actor(0,spawn['X'],spawn['Y'],spawn['Z'],spawn['YAW'],0x0DFF)),
                command(0x11, bytes([0,0,0,1])),
                command(0x13, struct.pack("<I",0)),
                command(0x0F, struct.pack("<I",1) + light), command(0x14)]
    result = {prefix:header(0x4F524F4D) + struct.pack("<I",len(commands)) + b"".join(commands)}
    commands = [command(0x08, struct.pack("<bi",0,0)),
                command(0x0B, struct.pack("<I",0)),
                command(0x01, struct.pack("<I",0)),
                command(0x0A, bytes([0,0,1,0]) + string(PRIVATE + "room") + string("")),
                command(0x10, bytes([0xFF,0xFF,0])),
                command(0x12, bytes([1,1])), command(0x16, bytes([0])), command(0x14)]
    result[room] = header(0x4F524F4D) + struct.pack("<I",len(commands)) + b"".join(commands)
    return result

def title_texture():
    from PIL import Image, ImageDraw, ImageFont
    # Exact text is code-rendered into the engine's native IA8 title format.
    font_path = "/usr/share/fonts/opentype/urw-base35/NimbusRoman-Italic.otf"
    font = ImageFont.truetype(font_path, 17)
    image = Image.new("L", (192, 24), 0)
    draw = ImageDraw.Draw(image)
    text = "Skiejer's Black Market"
    box = draw.textbbox((0, 0), text, font=font)
    draw.text(((192 - (box[2]-box[0]))/2, (24-(box[3]-box[1]))/2-box[1]), text, font=font, fill=255)
    # IA8: upper four intensity bits and lower four alpha bits.
    data = bytes((0xF0 | (value >> 4)) if value else 0 for value in image.getdata())
    return header(0x4F544558) + struct.pack("<IIII", 8, 192, 24, len(data)) + data

def boundary_repairs():
    """Close three audited donor openings with their adjacent material/UVs.

    The upper side holes are missing z60 shelf end faces, not absent ceiling
    strips. A thin floor band is also missing. Existing geometry is never
    moved, deleted or covered broadly.
    """
    # Rows are X,Y,Z,S,T. The shelf returns copy the opposite z80 end UVs.
    right = [(183,189,60,25,1028),(183,160,60,25,1991),
             (158,160,60,301,1991),(158,189,60,301,1028)]
    left = [(-182,189,60,15,1049),(-159,189,60,414,1049),
            (-159,160,60,414,2012),(-182,160,60,15,2012)]
    floor = [(100,0,108,8258,10838),(160,0,99,10204,10552),
             (160,0,104,10204,10708),(128,0,109,9166,10867),
             (100,0,115,8258,11057)]
    quad_faces = ((0,1,2),(0,2,3))
    groups = [(19,6,right,quad_faces),(19,6,left,quad_faces),
              (24,20,floor,((0,2,1),(0,3,2),(0,4,3)))]
    vertex = E.Element("Vertex", {"Version":"0"})
    draw = E.Element("DisplayList", {"Version":"0"})
    E.SubElement(draw, "ClearGeometryMode", {"G_LIGHTING":"1"})
    vpath = PRIVATE + "geometry/room_boundary_repairs_vtx"
    offset, faces = 0, []
    for material, group, rows, triangles in groups:
        for x,y,z,s,t in rows:
            E.SubElement(vertex,"Vtx",dict(X=str(x),Y=str(y),Z=str(z),S=str(s),T=str(t),
                R="160",G="160",B="160",A="255"))
        E.SubElement(draw,"CallDisplayList",{"Path":PRIVATE + f"geometry/mat_mahouya_room_0DL_006548_GranmaShop{material:02}"})
        E.SubElement(draw,"LoadVertices",dict(Path=vpath,VertexBufferIndex="0",VertexOffset=str(offset),Count=str(len(rows))))
        for indices in triangles:
            E.SubElement(draw,"Triangle1",dict(zip(("V00","V01","V02"),map(str,indices)),Flag0="0"))
            faces.append((group,tuple(tuple(rows[i][:3]) for i in indices)))
        offset += len(rows)
    E.SubElement(draw, "PipeSync")
    E.SubElement(draw, "SetGeometryMode", {"G_LIGHTING":"1"})
    E.SubElement(draw, "Texture", dict(S="65535",T="65535",Level="0",Tile="0",On="0"))
    E.SubElement(draw, "EndDisplayList")
    return {vpath:xml(vertex),PRIVATE + "geometry/room_boundary_repairs":xml(draw)}, faces

def build(args):
    maps = archive(args.maps, args.stormlib)
    maps.update(archive(args.textures, args.stormlib))
    keeper = archive(args.keeper, args.stormlib)
    resources = scope_keeper(keeper)
    donor_prefix = "alt/scenes/shared/mahouya_scene/"
    sage_prefix = "alt/scenes/shared/kenjyanoma_scene/"
    textures = ["rm_kja_kabe_01__UDIM_", "rm_kja_kabe_02_4000", "rm_kja_kabe_03_8000",
                "rm_kja_mark_01_C000", "rm_kja_water_02_13C00", "rm_kja_water_04_16400", "rm_kja_yuka_01_16C00"]
    from PIL import Image
    image = Image.open(args.surface).convert("RGBA").resize((512,512), Image.Resampling.LANCZOS)
    pixels = image.tobytes()
    # Engine packaging: raw RGBA pixels, logical 64x64 repeating tile.
    resources[PRIVATE + "texture/arcane_stone"] = header(0x4F544558, 1) + struct.pack("<IIIIffI", 2, 512, 512, 1, 16.0, 8.0, len(pixels)) + pixels
    mapping = {name.removeprefix("alt/"): PRIVATE + "geometry/" + name.rsplit("/",1)[1]
               for name in maps if name.startswith(donor_prefix) and maps[name].lstrip().startswith(b"<")}
    materials = []
    cosmetics = E.Element("CosmeticEntries")
    for name in mapping:
        tree = E.fromstring(maps["alt/" + name])
        if tree.tag == "Vertex":
            for vertex in tree:
                for channel in ("R","G","B"):
                    vertex.set(channel,str(max(160,int(vertex.get(channel)))))
        for element in tree.iter():
            path = element.get("Path")
            if path in mapping: element.set("Path", mapping[path])
        if "/mat_" in name:
            index = int(name.rsplit("Shop", 1)[1])
            chosen = "arcane_stone"
            texture = resources[PRIVATE + "texture/" + chosen]
            kind, width, height, flags, hscale, vscale, _ = struct.unpack_from("<IIIIffI", texture, 64)
            logical_w = round(width * (2 if flags & 1 else 1) / hscale)
            logical_h = round(height / vscale)
            image = tree.find("SetTextureImage")
            image.set("Path", PRIVATE + "texture/" + chosen)
            for tile in tree.findall("SetTile"):
                tile.set("MaskS", str(round(math.log2(logical_w))))
                tile.set("MaskT", str(round(math.log2(logical_h))))
                tile.set("Cms0", "G_TX_WRAP"); tile.set("Cmt0", "G_TX_WRAP")
                if tile.get("Tile") == "0": tile.set("Line", str(logical_w // 4))
            block = tree.find("LoadBlock")
            block.set("Lrs", str(logical_w * logical_h - 1)); block.set("Dxt", str(2048 // (logical_w // 4)))
            size = tree.find("SetTileSize")
            size.set("Lrs", str((logical_w - 1)*4)); size.set("Lrt", str((logical_h - 1)*4))
            color = tree.find("SetPrimColor")
            palette_index = index % len(PALETTE)
            for key, value in zip(("R","G","B"), PALETTE[palette_index]): color.set(key,str(value))
            color.set("CosmeticEntry", COSMETIC_LABELS[palette_index])
            color.set("CosmeticCategory", COSMETIC_CATEGORY)
            E.SubElement(cosmetics, "Entry", dict(MaterialPath=mapping[name],CosmeticType="Prim",
                CosmeticEntry=COSMETIC_LABELS[palette_index],CosmeticCategory=COSMETIC_CATEGORY))
            # One quarter-texel per native tick, with direction variation. The
            # engine opcode uses its game tick and supports interpolation.
            scroll = E.Element("ScrollTexture", {"Tile":"0", "SpeedX":str(-1 if index % 3 == 0 else 1), "SpeedY":str(0 if index % 2 else 1)})
            tree.insert(list(tree).index(size)+1, scroll)
            materials.append({"path": mapping[name], "texture": chosen, "logicalSize":[logical_w,logical_h],
                              "scroll":[int(scroll.get("SpeedX")),int(scroll.get("SpeedY"))]})
        resources[mapping[name]] = xml(tree)
    room = E.Element("DisplayList", {"Version":"0"})
    E.SubElement(room, "CallDisplayList", {"Path": mapping["scenes/shared/mahouya_scene/mahouya_room_0DL_006548"]})
    boundary_resources, boundary_faces = boundary_repairs()
    resources.update(boundary_resources)
    E.SubElement(room, "CallDisplayList", {"Path":PRIVATE + "geometry/room_boundary_repairs"})
    E.SubElement(room, "EndDisplayList")
    resources[PRIVATE + "room"] = xml(room)
    resources["CosmeticEntries"] = xml(cosmetics)
    faces = triangles(maps)
    resources[SCENE + "collision"], collision_info = collision(faces + boundary_faces)
    resources.update(scene_resources())
    resources.update(oot_scene_resources())
    resources[PRIVATE + "title"] = title_texture()

    # A cropped Djipi entrance shell makes the concealed threshold. The room
    # and the untouched POC9 forest keep their own collision and resources.
    shell = [(material, points) for material, points in faces
             if material in (19,21,22) and all(p[2] < -130 for p in points)]
    entrance = E.Element("DisplayList", {"Version":"0"})
    for index, (material, points) in enumerate(shell):
        vertex = E.Element("Vertex", {"Version":"0"})
        for x,y,z in points:
            E.SubElement(vertex,"Vtx",dict(X=str(round(x*.55)),Y=str(round(y*.65)),Z=str(round(-(z+291)*.55)),
                S=str(x*12),T=str(y*-12),R="170",G="155",B="210",A="255"))
        path = PRIVATE + f"entrance_vtx_{index}"
        resources[path] = xml(vertex)
        E.SubElement(entrance,"CallDisplayList",{"Path": PRIVATE + f"geometry/mat_mahouya_room_0DL_006548_GranmaShop{material:02}"})
        E.SubElement(entrance,"LoadVertices",{"Path":path,"VertexBufferIndex":"0","VertexOffset":"0","Count":"3"})
        E.SubElement(entrance,"Triangle1",{"V00":"0","V01":"1","V02":"2","Flag0":"0"})
    E.SubElement(entrance,"EndDisplayList")
    resources[PRIVATE + "entrance"] = xml(entrance)
    manifest = {"version":1,"scene":2,"entrance":0x1200,"donor":"Djipi Granny Potion Shop",
                "materials":sorted(materials,key=lambda m:m["path"]),"collision":collision_info,
                "cosmeticCategory":COSMETIC_CATEGORY,
                "boundaryRepairTriangles":len(boundary_faces),
                "ootScene":"scenes/shared/skiejer_market_oot/skiejer_market_oot", "keeper":keeper_contract(),
                "exteriorTriangles":len(shell),"titleStyle":"centered OoT place card, 192x24 native IA8",
                "sources":{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (args.maps,args.textures,args.keeper,args.surface)}}
    manifest["resources"] = sorted([*resources, PRIVATE + "manifest.json"])
    resources[PRIVATE + "manifest.json"] = json.dumps(manifest,indent=2).encode()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    directory = args.output.with_suffix("")
    if directory.exists(): raise ValueError(f"refusing to overwrite an existing resource directory: {directory}")
    for name, data in resources.items():
        target = directory / name; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(data)
    with zipfile.ZipFile(args.output,"w",zipfile.ZIP_DEFLATED) as output:
        for name,data in sorted(resources.items()):
            info=zipfile.ZipInfo(name,(2026,10,17,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            output.writestr(info,data)
    if args.contract_header:
        rows = "\n".join('    "' + name + '",' for name in sorted(resources))
        args.contract_header.write_text('#pragma once\n// Generated by tools/black_market/build.py.\n'
            'inline constexpr const char* kBlackMarketResources[] = {\n' + rows + '\n};\n')
    print(json.dumps({"pack":str(args.output),"resources":len(resources),"materials":len(materials),
                      "collision":collision_info,"exteriorTriangles":len(shell)},indent=2))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("maps","textures","keeper","stormlib","output"):
        parser.add_argument("--"+name,type=Path,required=True)
    parser.add_argument("--surface",type=Path,default=Path(__file__).parent/"assets/arcane_stone.png")
    parser.add_argument("--contract-header",type=Path)
    build(parser.parse_args())
