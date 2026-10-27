"""Preview-only GLB stock loading and native shelf-pose composition."""
import ctypes as C
from functools import lru_cache
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile

import numpy as np
from PIL import Image
from native_preview import Reader, rotate_xyz, translation

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def upload(gpu, pixels, width, height, *, wrap_s=0x2901, wrap_t=0x2901):
    pixels = np.ascontiguousarray(pixels, dtype=np.uint8)
    tid = gpu.uint()
    gpu.GenTextures(1, C.byref(tid))
    gpu.BindTexture(0x0DE1, tid.value)
    for prop, value in ((0x2801, 0x2601), (0x2800, 0x2601), (0x2802, wrap_s), (0x2803, wrap_t)):
        gpu.TexParameteri(0x0DE1, prop, value)
    gpu.TexImage(0x0DE1, 0, 0x1908, width, height, 0, 0x1908, 0x1401, pixels.ctypes.data)
    return tid.value


def keeper_placement(archive):
    """Read actual room actor-list bytes; do not assume a keeper spawn."""
    reader = Reader(archive.read("scenes/nonmq/skiejer_market/skiejer_market_room_0"))
    for _ in range(reader.read("I")):
        command = reader.read("I")
        if command == 0x08:
            reader.read("6B")
        elif command == 0x0B:
            for _ in range(reader.read("I")):
                reader.read("H")
        elif command == 0x01:
            for _ in range(reader.read("I")):
                actor = reader.read("HhhhhhhH")
                if actor[0] == 0x1C1:
                    return {"position": list(actor[1:4]), "yaw": actor[5], "scale": .01}
        else:
            raise ValueError(f"Keeper actor list not found before room command {command:02x}")
    raise ValueError("Keeper actor is missing")


def shelf_positions():
    hosts = []
    for path in ("mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp",
                 "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp"):
        source = (ROOT / path).read_text()
        block = re.search(r"constexpr Vec3f kShelves\[8\] = \{(.*?)\};", source, re.S)
        assert block, "Inspect changed DrawMarket shelf declaration before previewing"
        shelves = [[float(n.strip().removesuffix("f")) for n in row.split(",")] for row in re.findall(r"\{([^{}]+)\}", block[1])]
        assert len(shelves) == 8
        assert "const float scale = sTalking && slot == sSelected ? .32f : .25f;" in source
        hosts.append(shelves)
    assert hosts[0] == hosts[1], "Both native shops must share the previewed display positions"
    return hosts[0]

@lru_cache(maxsize=2)
def shelf_fixture(granny=False):
    """Use the same source-authored shelf mesh/grain as both native engines."""
    with tempfile.TemporaryDirectory(prefix='market-shelf-preview-') as temporary:
        binary=Path(temporary)/'shelves'
        subprocess.run(['c++','-std=c++20','-I'+str(ROOT),str(HERE/'shelves_preview.cpp'),'-o',str(binary)],check=True)
        return json.loads(subprocess.check_output([str(binary),'granny' if granny else 'magical'],text=True))

@lru_cache(maxsize=2)
def shelf_draw_data(granny=False):
    import preview_mm_lost_woods as gpu
    fixture=shelf_fixture(granny)
    quads=np.array(fixture['vertices'],dtype=np.float32).reshape(-1,4,9)
    groups={}
    for material,quad in zip(fixture['materials'],quads):
        triangles=quad[[0,1,2,0,2,3]].copy()
        size=[32,64] if material==1 else [64,64]
        triangles[:,3:5]/=np.array(size)*32;triangles[:,5:9]/=255
        groups.setdefault(material,[]).append(triangles)
    return {m:np.ascontiguousarray(np.concatenate(rows)) for m,rows in groups.items()}

def granny_shelf_textures(gpu,resources):
    # Read exact installed resources just as the production Granny adapter does.
    from granny_preview import native_texture
    class Pack:
        def read(self,name):return resources[name]
    fixture=shelf_fixture(True)
    textures=[]
    for name in (fixture['wood_texture'],fixture['atlas_texture']):
        pixels,w,h=native_texture(Pack(),name);textures.append(upload(gpu,pixels,w,h))
    return textures

def draw_shelves(gpu,tick=0,granny=False,textures=(),tint=None,room_textures=None):
    groups=shelf_draw_data(granny)
    tint=np.array(shelf_fixture()['tint'] if tint is None else tint)/255
    if granny and len(textures)!=2:raise ValueError('Granny shelf preview requires its exact external textures')
    for material,points in groups.items():
        array=points.copy()
        if not granny:
            array[:,3:5]-=np.array([1,0] if material==4 else [-1,1])*tick/(64*4)
            array[:,5:8]*=tint[:3]
        texture=room_textures[shelf_fixture()['flow_texture']] if not granny else textures[0 if material==1 else 1]
        gpu.draw(array,texture)


def seal_style():
    lock = native_lock()
    return {"kind": "native model-fitted wrapping X", "locked_only": True,
            "texture_dimensions": list(lock.texture().shape[1::-1]), "s_wrap_t_clamp": True,
            "selected_rgba": np.rint(lock.vertices([-3,-4,-2], [3,4,2], 0, True)[0,5:] * 255).astype(int).tolist(),
            "locked_rgba": np.rint(lock.vertices([-3,-4,-2], [3,4,2], 0, False)[0,5:] * 255).astype(int).tolist()}


class NativeLock:
    """Call the same quantized BuildLock and IA8 texture as both engines."""
    def __init__(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="market-preview-lock-")
        library = Path(self.temporary.name) / "lock.so"
        subprocess.run(["c++", "-std=c++20", "-O2", "-shared", "-fPIC", "-I" + str(ROOT),
                        str(HERE / "lock_preview.cpp"), "-o", str(library)], check=True)
        self.native = C.CDLL(str(library))
        self.native.BlackMarketLock_Preview.argtypes = [C.POINTER(C.c_float), C.POINTER(C.c_float),
                                                       C.c_uint, C.c_int, C.c_float, C.POINTER(C.c_float)]
        self.native.BlackMarketLock_Preview.restype = C.c_int
        self.native.BlackMarketLock_Texture.restype = C.POINTER(C.c_uint8)

    def vertices(self, minimum, maximum, tick, selected, padding_scale=1):
        low = np.ascontiguousarray(minimum, dtype=np.float32)
        high = np.ascontiguousarray(maximum, dtype=np.float32)
        rows = np.empty((96, 9), dtype=np.float32)
        count = self.native.BlackMarketLock_Preview(low.ctypes.data_as(C.POINTER(C.c_float)),
                    high.ctypes.data_as(C.POINTER(C.c_float)), tick, selected, padding_scale,
                    rows.ctypes.data_as(C.POINTER(C.c_float)))
        return np.ascontiguousarray(rows[:count])

    def texture(self):
        packed = np.ctypeslib.as_array(self.native.BlackMarketLock_Texture(), shape=(8,32))
        intensity, alpha = (packed >> 4) * 17, (packed & 15) * 17
        return np.ascontiguousarray(np.stack((intensity, intensity, intensity, alpha), axis=2))


@lru_cache(maxsize=1)
def native_lock():
    return NativeLock()


def production_stock(json_include):
    with tempfile.TemporaryDirectory(prefix="black-market-preview-stock-") as folder:
        executable = Path(folder) / "stock"
        command = ["g++", "-std=c++20", "-O2", "-I" + str(ROOT)]
        if json_include:
            command.append("-I" + str(json_include))
        command += [str(HERE / "sample_stock.cpp"), "-o", str(executable)]
        subprocess.run(command, check=True, capture_output=True, text=True)
        return json.loads(subprocess.check_output([str(executable)], text=True))


def checkpoint(gpu, slug):
    """Exact retained checkpoint geometry/materials at its GI caller scale."""
    folder = ROOT / "tools/nei_gi/CHECKPOINTS" / slug
    data = (folder / (slug + ".glb")).read_bytes()
    length = struct.unpack_from("<I", data, 12)[0]
    doc, binary = json.loads(data[20:20 + length]), data[28 + length:]
    meta = json.loads((folder / "checkpoint.json").read_text())
    factor = doc["nodes"][-1]["scale"][0] * meta["draw_scale"]

    def acc(index):
        a = doc["accessors"][index]
        view = doc["bufferViews"][a["bufferView"]]
        cols = {"VEC3": 3, "VEC2": 2, "VEC4": 4, "SCALAR": 1}[a["type"]]
        dtype = {5126: "<f4", 5125: "<u4", 5123: "<u2", 5121: "u1"}[a["componentType"]]
        return np.frombuffer(binary, dtype=dtype, count=a["count"] * cols,
                             offset=view.get("byteOffset", 0) + a.get("byteOffset", 0)).reshape(-1, cols)

    mats = []
    for mat in doc["materials"]:
        pbr, tex = mat["pbrMetallicRoughness"], None
        if "baseColorTexture" in pbr:
            image = doc["images"][doc["textures"][pbr["baseColorTexture"]["index"]]["source"]]
            view = doc["bufferViews"][image["bufferView"]]
            start = view.get("byteOffset", 0)
            im = Image.open(io.BytesIO(binary[start:start + view["byteLength"]])).convert("RGBA")
            tex = upload(gpu, np.asarray(im), im.width, im.height)
        mats.append((tex, np.array(pbr.get("baseColorFactor", [1, 1, 1, 1])),
                     bool(mat.get("emissiveFactor", [0])[0]), mat.get("alphaMode") == "BLEND"))
    rows = []
    for mesh in doc["meshes"]:
        for primitive in mesh["primitives"]:
            tex, base, emissive, transparent = mats[primitive["material"]]
            attrs = primitive["attributes"]
            positions, normals = acc(attrs["POSITION"]) * factor, acc(attrs["NORMAL"])
            shade = np.ones(len(positions)) if emissive else .62 + .38 * np.maximum(normals @ np.array([-.35, .8, -.48]), 0)
            colors = np.tile(base, (len(positions), 1))
            colors[:, :3] *= shade[:, None]
            array = np.column_stack((positions, acc(attrs["TEXCOORD_0"]), colors))[acc(primitive["indices"]).ravel()]
            rows.append((tex, np.ascontiguousarray(array, dtype=np.float32), transparent))
    return rows, meta, hashlib.sha256(data).hexdigest()


def item_pose(item, position, slot, tick, selected=False):
    caller = .32 if selected else .25
    position = np.array(position) + [0, 2 * math.sin((tick + slot * 7) * .04), 0]
    # DrawMarket, FrameFit, then GI renderer spin. Checkpoint already includes
    # the GI draw scale and serialized resource matrix; no vanilla +24 origin.
    result = translation(position) @ rotate_xyz((0, tick * 100 + slot * 4096, 0))
    result = result @ np.diag([caller] * 3 + [1]) @ translation((0, item["fit_lift"], 0))
    result = result @ np.diag([item["fit_scale"]] * 3 + [1])
    return result @ rotate_xyz((0, tick * .02 * 32768 / math.pi, 0))


def transform(array, matrix):
    result = array.copy()
    result[:, :3] = np.column_stack((array[:, :3], np.ones(len(array)))) @ matrix[:3, :].T
    return np.ascontiguousarray(result)


def triangle_box_intersects(triangle, minimum, maximum):
    """13-axis SAT, including nine edge×box axes; touching counts as overlap."""
    points = np.asarray(triangle) - (minimum + maximum) / 2
    half = (maximum - minimum) / 2
    edges = np.roll(points, -1, axis=0) - points
    axes = list(np.eye(3)) + [np.cross(edges[0], edges[1])]
    axes += [np.cross(edge, axis) for edge in edges for axis in np.eye(3)]
    for axis in axes:
        if axis @ axis < 1e-12:
            continue
        projection = points @ axis
        radius = half @ np.abs(axis)
        if projection.min() > radius + 1e-6 or projection.max() < -radius - 1e-6:
            return False
    return True


def clearance_report(room, models, stock, shelves):
    """Conservatively bound continuous 360° spin, full bob, both caller scales.

    No mesh triangle can intersect room geometry if its enclosing swept box
    has no triangle-box intersections. This proves model/room clearance only,
    not camera visibility, player collision or procedural effect clearance.
    """
    triangles = np.concatenate([row[3][:, :3] for row in room]).reshape(-1, 3, 3)
    tri_minimum, tri_maximum = triangles.min(1), triangles.max(1)
    report = []
    for slot, (item, position) in enumerate(zip(stock, shelves)):
        points = np.concatenate([array[:, :3] for _, array, _ in models[item["slug"]]])
        horizontal_radius = np.linalg.norm(points[:, [0, 2]], axis=1).max()
        low, high = [], []
        for caller in (.25, .32):
            factor = caller * item["fit_scale"]
            radius = horizontal_radius * factor
            lift = caller * item["fit_lift"]
            low.append(np.array(position) + [-radius, -2 + lift + points[:, 1].min() * factor, -radius])
            high.append(np.array(position) + [radius, 2 + lift + points[:, 1].max() * factor, radius])
        minimum, maximum = np.min(low, axis=0), np.max(high, axis=0)
        candidates = np.all(tri_maximum >= minimum, axis=1) & np.all(tri_minimum <= maximum, axis=1)
        hits = sum(triangle_box_intersects(triangle, minimum, maximum) for triangle in triangles[candidates])
        report.append({"slot": slot + 1, "slug": item["slug"], "swept_minimum": minimum.tolist(),
                       "swept_maximum": maximum.tolist(), "room_aabb_candidates": int(candidates.sum()),
                       "room_triangle_box_intersections": hits, "model_room_clearance_proven": hits == 0})
    return {"method": "Conservative continuous 360-degree model sweep plus full +/-2 bob; caller .25 and .32; 13-axis triangle-box SAT",
            "procedural_effects_included": False, "items": report}


def background_regions(image, minimum_pixels=8):
    """Record connected viewport pixels that expose the clear background."""
    pixels = np.asarray(image)
    mask = np.all(pixels[:, :, :3] == [7, 5, 12], axis=2)
    seen, regions = set(), []
    for y, x in zip(*np.where(mask)):
        if (y, x) in seen:
            continue
        todo, points = [(y, x)], []
        seen.add((y, x))
        while todo:
            yy, xx = todo.pop()
            points.append((yy, xx))
            for a, b in ((yy - 1, xx), (yy + 1, xx), (yy, xx - 1), (yy, xx + 1)):
                if 0 <= a < mask.shape[0] and 0 <= b < mask.shape[1] and mask[a, b] and (a, b) not in seen:
                    seen.add((a, b))
                    todo.append((a, b))
        if len(points) >= minimum_pixels:
            array = np.array(points)
            regions.append({"pixels": len(points), "minimum": array.min(0)[::-1].tolist(), "maximum": array.max(0)[::-1].tolist()})
    return sorted(regions, key=lambda region: -region["pixels"])


def bind_equivalent_pack(evidence_path, candidate_path):
    """Retain render origin while rebinding a collision-only archive revision.

    Every other resource, including native room actors, must be byte-identical.
    A geometry/material/animation change requires an actual rerender.
    """
    evidence_path, candidate_path = Path(evidence_path), Path(candidate_path)
    evidence = json.loads(evidence_path.read_text())
    original_path = Path(evidence["pack"])
    assert hashlib.sha256(original_path.read_bytes()).hexdigest() == evidence["pack_sha256"], "Original rendered archive changed"
    excluded = {"custom/skiejer_market/manifest.json", "scenes/nonmq/skiejer_market/collision"}
    with zipfile.ZipFile(original_path) as original, zipfile.ZipFile(candidate_path) as candidate:
        names = set(original.namelist())
        assert names == set(candidate.namelist()), "Archive resource membership changed; rerender required"
        changed = {name for name in names if original.read(name) != candidate.read(name)}
        assert changed <= excluded, f"Rendered or scene-placement resources changed: {sorted(changed - excluded)}"
        checked = sorted(names - excluded)
        digest = hashlib.sha256()
        for name in checked:
            digest.update(name.encode() + b"\0" + hashlib.sha256(original.read(name)).digest())
    evidence["rendered_from_pack"] = {"path": str(original_path.resolve()), "sha256": evidence["pack_sha256"]}
    evidence["pack"] = str(candidate_path.resolve())
    evidence["pack_sha256"] = hashlib.sha256(candidate_path.read_bytes()).hexdigest()
    evidence["visual_equivalence"] = {"byte_identical_resource_count": len(checked),
                                     "resource_set_sha256": digest.hexdigest(), "changed_nonvisual_resources": sorted(changed),
                                     "note": "All other resources, including room XML, vertices, textures, skeleton, idle, blink and native scene/spawn data, are byte-identical."}
    evidence_path.write_text(json.dumps(evidence, indent=2) + "\n")
    return evidence["visual_equivalence"]


def draw_keeper(gpu, keeper, textures, tick, placement):
    # Initial 90-frame blink timer from InitKeeper; first cycle is exact.
    blink = 1 if tick in (90, 91, 94, 95) else 2 if tick in (92, 93) else 0
    key = (tick, tuple(placement["position"]), placement["yaw"], placement["scale"], blink)
    if getattr(keeper, "last_draw_key", None) != key:
        rows = keeper.geometry(tick, placement["position"], placement["yaw"], placement["scale"], blink)
        groups = {}
        for texture, array in rows:
            groups.setdefault(texture, []).append(array)
        keeper.last_draw_rows = [(texture, np.ascontiguousarray(np.concatenate(arrays))) for texture, arrays in groups.items()]
        keeper.last_draw_key = key
    for texture, array in keeper.last_draw_rows:
        gpu.draw(array, textures[texture])


def seal_arrays(stock, shelves, tick, selected=None, clears=0, models=None):
    rows = []
    for slot, (item, position) in enumerate(zip(stock, shelves)):
        locked, chosen = clears < item["clears"], slot == selected
        if not locked:
            continue
        matrix = item_pose(item, position, slot, tick, chosen)
        points = np.concatenate([transform(array, matrix)[:, :3] for _, array, _ in models[item["slug"]]])
        vertices = native_lock().vertices(points.min(0), points.max(0), tick, chosen,
                                          shelf_fixture().get('seal_padding_scale', 1))
        if len(vertices): rows.append(vertices)
    return rows
