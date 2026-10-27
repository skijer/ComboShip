"""Rotate the exact sword GLBs with both production C++ particle passes.

The current GLBs retain their authored materials and normal-generated UV
metadata. The six corrected sword meshes are loaded from their checkpoints.
Only camera, material shading, and lighting are offline approximations. Opaque
geometry writes depth; actual quantized native effect triangles blend over it
without depth writes. No extra particles, bloom, or generated art are added.
All swords share one camera fit, measured across every sampled rotation and
both production effect meshes. A guard-aligned, effects-off scale sheet is
always saved; --before-root adds common-scale pairs from earlier checkpoints.

Requires Python 3.11+ with NumPy/Pillow, Mesa EGL/OpenGL libraries, the DejaVu
Sans font, and a C++20 compiler. From the repository root:
    python3 -m pip install -r tools/nei_gi/SOURCE/forged_swords/requirements.txt
    python3 tools/nei_gi/runtime_preview/render_swords.py /tmp/nei-gi-previews \
        --stem swords_current --groups
"""
import argparse
import ctypes as C
import io
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")
from gl_context import context, gl, integer, ptr, uint

REPO = Path(__file__).resolve().parents[3]
ROOT = REPO / "tools" / "nei_gi"
PROFILES = {
    "kokiri_sword": ("KokiriSword", "Kokiri Sword · OoT", "Green leaves", "#93dc88"),
    "mm_kokiri_sword": ("MmKokiriSword", "Kokiri Sword · MM", "Purple wisps", "#bb99ef"),
    "razor_sword": ("RazorSword", "Razor Sword", "Silver sparks", "#d2d9e5"),
    "gilded_sword": ("GildedSword", "Gilded Sword", "Golden motes", "#ebcc78"),
    "master_sword": ("MasterSword", "Master Sword", "Blue wisps", "#82b9fa"),
    "true_master_sword": ("SwordAura", "True Master Sword", "Ivory + gold aura", "#f3dfae"),
    "biggoron_sword": ("BiggoronSword", "Biggoron's Sword", "Forge embers", "#f8ae79"),
    "great_fairy_sword": ("GreatFairySword", "Great Fairy Sword", "Green + violet petals", "#ccabdf"),
    "four_sword": ("FourSword", "Four Sword", "Green, red, blue + violet trails", "#aad0e4"),
}
DTYPE = np.dtype([("p", "<f4", 3), ("rgba", "u1", 4)])
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
BACKGROUND = "#121a24"
F, D = C.c_float, C.c_double
W, H, HEADER, CELL = 1200, 1340, 100, 400
GROUP_CELL = 640
BUFFER_H = HEADER + 3 * GROUP_CELL + 40


def samples(frames, elevation, directory):
    """Compile enum selectors against the current policy rather than fixed IDs."""
    source = directory / "kinds.cpp"
    source.write_text('#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"\n'
                      '#include <iostream>\nint main(){\n' + "\n".join(
                          'std::cout << int(NeiGi::Kind::' + item[0] + ') << "\\n";'
                          for item in PROFILES.values()) + '\n}\n')
    compiler = os.environ.get("CXX", "c++")
    flags = ["-std=c++20", "-O2", "-I" + str(REPO / "soh")]
    selector, exporter = directory / "kinds", directory / "export_swords"
    subprocess.run([compiler, *flags, str(source), "-o", str(selector)], check=True)
    kinds = subprocess.check_output([str(selector)], text=True).splitlines()
    subprocess.run([compiler, *flags, str(Path(__file__).with_name("export_swords.cpp")),
                    "-o", str(exporter)], check=True)
    result = {}
    for slug, kind in zip(PROFILES, kinds, strict=True):
        path = directory / (slug + ".bin")
        subprocess.run([str(exporter), str(path), str(frames), str(elevation), kind], check=True)
        raw = path.read_bytes()
        assert raw[:4] == b"SWFX"
        version, count = struct.unpack_from("<II", raw, 4)
        assert version == 1 and count == frames
        cursor, rows = 12, []
        for _ in range(frames):
            frame, yaw, intrinsic_count, shimmer_count = struct.unpack_from("<IfII", raw, cursor)
            cursor += 16
            meshes = []
            for vertices in (intrinsic_count, shimmer_count):
                assert 0 < vertices <= 1536 and vertices % 3 == 0
                mesh = np.frombuffer(raw, dtype=DTYPE, count=vertices, offset=cursor)
                cursor += vertices * DTYPE.itemsize
                assert np.isfinite(mesh["p"]).all()
                assert np.array_equal(mesh["p"] * 16, np.round(mesh["p"] * 16))
                meshes.append(mesh)
            rows.append((frame, math.degrees(yaw), *meshes))
        assert cursor == len(raw)
        result[slug] = rows
    return result


def node_matrix(node):
    if "matrix" in node:
        return np.array(node["matrix"], dtype=np.float64).reshape(4, 4, order="F")
    x, y, z, w = node.get("rotation", (0, 0, 0, 1))
    rotation = np.array([[1 - 2 * (y*y + z*z), 2 * (x*y - z*w), 2 * (x*z + y*w)],
                         [2 * (x*y + z*w), 1 - 2 * (x*x + z*z), 2 * (y*z - x*w)],
                         [2 * (x*z - y*w), 2 * (y*z + x*w), 1 - 2 * (x*x + y*y)]])
    matrix = np.eye(4)
    matrix[:3, :3] = rotation @ np.diag(node.get("scale", (1, 1, 1)))
    matrix[:3, 3] = node.get("translation", (0, 0, 0))
    return matrix


class Model:
    """Read GLB scene instances, packed accessors, exact textures, and extras."""
    def __init__(self, slug, renderer, checkpoint_root=None):
        checkpoint = (checkpoint_root or ROOT / "CHECKPOINTS") / slug
        self.slug = slug
        path = checkpoint / (slug + ".glb")
        raw = path.read_bytes()
        magic, version, length = struct.unpack_from("<4sII", raw)
        assert magic == b"glTF" and version == 2 and length == len(raw)
        cursor, chunks = 12, {}
        while cursor < len(raw):
            size, kind = struct.unpack_from("<I4s", raw, cursor)
            cursor += 8
            chunks[kind] = raw[cursor:cursor + size]
            cursor += size
        self.doc, self.binary = json.loads(chunks[b"JSON"]), chunks[b"BIN\0"]
        self.meta = json.loads((checkpoint / "checkpoint.json").read_text())
        self.materials = []
        for material in self.doc.get("materials", []):
            pbr, extra = material.get("pbrMetallicRoughness", {}), material.get("extras", {})
            texture = 0
            if "baseColorTexture" in pbr:
                item = self.doc["textures"][pbr["baseColorTexture"]["index"]]
                image = self.doc["images"][item["source"]]
                view = self.doc["bufferViews"][image["bufferView"]]
                offset = view.get("byteOffset", 0)
                pixels = Image.open(io.BytesIO(self.binary[offset:offset + view["byteLength"]])).convert("RGBA")
                sampler = self.doc.get("samplers", [{}])[item.get("sampler", 0)]
                texture = renderer.texture(pixels, sampler)
            self.materials.append(dict(texture=texture, base=pbr.get("baseColorFactor", [1, 1, 1, 1]),
                                       metal=pbr.get("metallicFactor", 1), rough=pbr.get("roughnessFactor", 1),
                                       emission=max(material.get("emissiveFactor", [0, 0, 0])),
                                       transparent=material.get("alphaMode") == "BLEND",
                                       double_sided=material.get("doubleSided", False), extras=extra))
        self.parts = []
        self.part_names = []

        def visit(index, parent):
            node = self.doc["nodes"][index]
            transform = parent @ node_matrix(node)
            if "mesh" in node:
                for primitive in self.doc["meshes"][node["mesh"]]["primitives"]:
                    assert primitive.get("mode", 4) == 4
                    attributes = primitive["attributes"]
                    position = self.accessor(attributes["POSITION"])
                    positions = (position @ transform[:3, :3].T + transform[:3, 3]) * self.meta["draw_scale"]
                    native = "_NEI_NATIVE_NORMAL" in attributes
                    normals = self.accessor(attributes.get("_NEI_NATIVE_NORMAL", attributes["NORMAL"]))
                    # Scene scale changes positions, but only rotation affects native normals.
                    normal_matrix = np.linalg.inv(transform[:3, :3]).T
                    normal_matrix /= np.linalg.norm(normal_matrix[:, 0])
                    normals = normals @ normal_matrix.T
                    if not native:
                        normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-9)
                    uv = self.accessor(attributes["TEXCOORD_0"]) if "TEXCOORD_0" in attributes else np.zeros((len(positions), 2))
                    indices = self.accessor(primitive["indices"]).ravel().astype(int) if "indices" in primitive else np.arange(len(positions))
                    data = np.ascontiguousarray(np.column_stack((positions, normals, uv))[indices], dtype=np.float32)
                    self.parts.append((data, self.materials[primitive.get("material", 0)], native))
                    self.part_names.append(self.doc["meshes"][node["mesh"]].get("name", node.get("name", "")))
            for child in node.get("children", []):
                visit(child, transform)

        scene = self.doc["scenes"][self.doc.get("scene", 0)]
        for index in scene["nodes"]:
            visit(index, np.eye(4))
        self.positions = np.concatenate([part[0][:, :3] for part in self.parts])
        self.bounds = (self.positions.min(axis=0), self.positions.max(axis=0))
        self.guard = self.guard_baseline()

    def guard_baseline(self):
        markers = self.meta.get("markers", {})
        if "guard_baseline" in markers:
            return np.asarray(markers["guard_baseline"], dtype=float)
        if "guard_baseline_y" in markers:
            y = float(markers["guard_baseline_y"])
            lean = math.radians(markers.get("authored_lean_degrees", 0))
            # A baked Z lean also moves the central guard point along X.
            x = markers.get("guard_baseline_x", -math.tan(lean) * y)
            return np.array([x, y, markers.get("guard_baseline_z", 0)], dtype=float)
        legacy_y = {"razor_sword": -11.04833984375,
                    "biggoron_sword": -12.388763427734375}.get(self.slug)
        if legacy_y is not None and markers.get("authored_lean_degrees") == -14:
            # Exact world guard origins of the retained pre-correction models.
            return np.array([-math.tan(math.radians(-14)) * legacy_y, legacy_y, 0])
        # The forged models were authored with their blade/guard joint at zero.
        # Stable older legacy checkpoints have named central guard geometry.
        legacy_name = {"razor_sword": "Plum Razor central shield",
                       "biggoron_sword": "Goron guard dark central mark"}.get(self.slug)
        matches = [part[0][:, :3] for name, part in zip(self.part_names, self.parts)
                   if name == legacy_name]
        if matches:
            points = np.concatenate(matches)
            return (points.min(axis=0) + points.max(axis=0)) / 2
        return np.zeros(3)

    def accessor(self, index):
        accessor = self.doc["accessors"][index]
        view = self.doc["bufferViews"][accessor["bufferView"]]
        columns = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[accessor["type"]]
        dtype = np.dtype({5120: "i1", 5121: "u1", 5122: "<i2", 5123: "<u2", 5125: "<u4", 5126: "<f4"}[accessor["componentType"]])
        result = np.ndarray((accessor["count"], columns), dtype=dtype, buffer=self.binary,
                            offset=view.get("byteOffset", 0) + accessor.get("byteOffset", 0),
                            strides=(view.get("byteStride", dtype.itemsize * columns), dtype.itemsize))
        if accessor.get("normalized", False) and dtype.kind in "ui":
            result = result.astype(np.float32) / np.iinfo(dtype).max
            if dtype.kind == "i":
                result = np.maximum(result, -1)
        return result


def rotation(elevation, yaw):
    x, y = np.deg2rad([elevation, yaw])
    sx, cx, sy, cy = np.sin(x), np.cos(x), np.sin(y), np.cos(y)
    return np.array([[cy, 0, sy], [sx * sy, cx, -sx * cy], [-cx * sy, sx, cx * cy]])


def fit_camera(models, sampled, elevation, before_models=None):
    """One fit for all cells, groups, aligned scale views, and optional history."""
    lower, upper = np.full(2, np.inf), np.full(2, -np.inf)
    coverage = {}
    for version, collection in (("after", models), ("before", before_models or {})):
        for slug, model in collection.items():
            model_low, model_high = np.full(2, np.inf), np.full(2, -np.inf)
            for _, yaw, intrinsic, shimmer in sampled[slug]:
                camera = rotation(elevation, yaw)
                vertices = [model.positions, model.positions - model.guard]
                if version == "after":
                    vertices += [intrinsic["p"], shimmer["p"]]
                for points in vertices:
                    projected = points @ camera.T
                    model_low = np.minimum(model_low, projected[:, :2].min(axis=0))
                    model_high = np.maximum(model_high, projected[:, :2].max(axis=0))
            lower, upper = np.minimum(lower, model_low), np.maximum(upper, model_high)
            coverage[version + "/" + slug] = [model_low.tolist(), model_high.tolist()]
    assert np.isfinite(lower).all() and np.isfinite(upper).all()
    # Taller group/scale cells have the narrowest aspect ratio. Include their
    # horizontal coverage when choosing the common vertical world range.
    narrowest_aspect = 400 / (GROUP_CELL - 78)
    span = max(upper[1] - lower[1], 2 * max(abs(lower[0]), abs(upper[0])) / narrowest_aspect) * 1.12
    center = (lower[1] + upper[1]) / 2
    fit = dict(low=float(center - span / 2), high=float(center + span / 2), span=float(span),
               measured_xy=[lower.tolist(), upper.tolist()], coverage=coverage)
    half = span * narrowest_aspect / 2
    assert lower[0] > -half and upper[0] < half and lower[1] > fit["low"] and upper[1] < fit["high"]
    print(f"Shared camera: Y [{fit['low']:.3f}, {fit['high']:.3f}], span {span:.3f}; "
          f"all {len(sampled) * len(next(iter(sampled.values())))} sword/rotation samples fit", flush=True)
    return fit


VERTEX_SHADER = """
#version 120
varying vec3 viewNormal;
varying vec2 modelUV;
void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    viewNormal = gl_NormalMatrix * gl_Normal;
    modelUV = gl_MultiTexCoord0.xy;
}
"""
FRAGMENT_SHADER = """
#version 120
varying vec3 viewNormal;
varying vec2 modelUV;
uniform sampler2D pigment;
uniform vec4 baseColor;
uniform float metal, roughness, emission;
uniform int textured, reflected, linearGen, nativeNormal, decal;
uniform vec2 texgenFactor;
void main() {
    vec3 normal = nativeNormal != 0 ? viewNormal : normalize(viewNormal);
    vec2 uv = modelUV;
    if (reflected != 0) {
        vec2 mapped = linearGen != 0 ? acos(-clamp(normal.xy, -1.0, 1.0)) / 6.28318530718
                                    : (normal.xy + 1.0) * 0.25;
        uv = mapped * texgenFactor;
    }
    vec4 color = baseColor;
    if (textured != 0) color *= texture2D(pigment, uv);
    vec3 light = normalize(vec3(-0.45, 0.70, 0.62));
    float shade = mix(0.32 + 0.68 * max(dot(normal, light), 0.0), 1.0, emission);
    vec3 halfDirection = normalize(light + vec3(0.0, 0.0, 1.0));
    float specular = pow(max(dot(normal, halfDirection), 0.0), 8.0 + roughness * 40.0)
                     * (decal != 0 ? 0.0 : (metal > 0.0 ? 32.0 : 9.0)) / 255.0;
    gl_FragColor = vec4(color.rgb * shade + vec3(specular), color.a);
}
"""


class Renderer:
    def __init__(self):
        context(W, BUFFER_H)
        self.fit = None
        signatures = {
            "ClearColor": (None, F, F, F, F), "Clear": (None, uint), "Enable": (None, uint), "Disable": (None, uint),
            "BlendFunc": (None, uint, uint), "DepthMask": (None, C.c_ubyte), "DepthFunc": (None, uint),
            "Viewport": (None, integer, integer, integer, integer), "MatrixMode": (None, uint),
            "LoadIdentity": (None,), "Ortho": (None, D, D, D, D, D, D), "Rotatef": (None, F, F, F, F),
            "Translatef": (None, F, F, F),
            "ReadPixels": (None, integer, integer, integer, integer, uint, uint, ptr),
            "EnableClientState": (None, uint), "DisableClientState": (None, uint),
            "VertexPointer": (None, integer, uint, integer, ptr), "NormalPointer": (None, uint, integer, ptr),
            "ColorPointer": (None, integer, uint, integer, ptr), "TexCoordPointer": (None, integer, uint, integer, ptr),
            "DrawArrays": (None, uint, integer, integer), "BindTexture": (None, uint, uint),
            "GenTextures": (None, integer, C.POINTER(uint)), "TexParameteri": (None, uint, uint, integer),
            "TexImage2D": (None, uint, integer, integer, integer, integer, integer, uint, uint, ptr),
            "CreateShader": (uint, uint), "ShaderSource": (None, uint, integer, C.POINTER(C.c_char_p), C.POINTER(integer)),
            "CompileShader": (None, uint), "GetShaderiv": (None, uint, uint, C.POINTER(integer)),
            "GetShaderInfoLog": (None, uint, integer, C.POINTER(integer), ptr),
            "CreateProgram": (uint,), "AttachShader": (None, uint, uint), "LinkProgram": (None, uint),
            "GetProgramiv": (None, uint, uint, C.POINTER(integer)), "GetProgramInfoLog": (None, uint, integer, C.POINTER(integer), ptr),
            "UseProgram": (None, uint), "GetUniformLocation": (integer, uint, C.c_char_p),
            "Uniform1i": (None, integer, integer), "Uniform1f": (None, integer, F),
            "Uniform2f": (None, integer, F, F), "Uniform4f": (None, integer, F, F, F, F),
            "GetError": (uint,), "GetString": (C.c_char_p, uint),
        }
        for name, (result, *arguments) in signatures.items():
            setattr(self, name, gl("gl" + name, result, *arguments))
        self.Enable(0x0B71)
        self.Enable(0x0BE2)
        self.DepthFunc(0x0203)  # LEQUAL also permits exact coplanar material decals.
        self.BlendFunc(0x0302, 0x0303)
        self.ClearColor(18 / 255, 26 / 255, 36 / 255, 1)
        self.program = self.CreateProgram()
        for shader_type, source in ((0x8B31, VERTEX_SHADER), (0x8B30, FRAGMENT_SHADER)):
            shader = self.CreateShader(shader_type)
            string = C.c_char_p(source.encode())
            self.ShaderSource(shader, 1, C.byref(string), None)
            self.CompileShader(shader)
            status = integer()
            self.GetShaderiv(shader, 0x8B81, C.byref(status))
            if not status.value:
                log = C.create_string_buffer(4096)
                self.GetShaderInfoLog(shader, 4096, None, log)
                raise RuntimeError(log.value.decode())
            self.AttachShader(self.program, shader)
        self.LinkProgram(self.program)
        status = integer()
        self.GetProgramiv(self.program, 0x8B82, C.byref(status))
        if not status.value:
            log = C.create_string_buffer(4096)
            self.GetProgramInfoLog(self.program, 4096, None, log)
            raise RuntimeError(log.value.decode())
        self.uniforms = {name: self.GetUniformLocation(self.program, name.encode()) for name in
                         ("pigment", "baseColor", "metal", "roughness", "emission", "textured", "reflected",
                          "linearGen", "nativeNormal", "decal", "texgenFactor")}
        print("Offline renderer:", self.GetString(0x1F01).decode(), flush=True)

    def texture(self, image, sampler):
        texture = uint()
        self.GenTextures(1, C.byref(texture))
        self.BindTexture(0x0DE1, texture.value)
        for name, value in ((0x2800, 0x2601), (0x2801, 0x2601),
                            (0x2802, sampler.get("wrapS", 10497)), (0x2803, sampler.get("wrapT", 10497))):
            self.TexParameteri(0x0DE1, name, value)
        pixels = np.ascontiguousarray(image)
        self.TexImage2D(0x0DE1, 0, 0x1908, image.width, image.height, 0, 0x1908, 0x1401, pixels.ctypes.data)
        return texture.value

    def pose(self, index, elevation, yaw, cell_height=CELL, guard=None):
        assert self.fit is not None, "Fit all models and effect frames before rendering"
        top = HEADER + (index // 3) * cell_height
        self.Viewport((index % 3) * 400, BUFFER_H - (top + cell_height - 8), 400, cell_height - 78)
        self.MatrixMode(0x1701)
        self.LoadIdentity()
        half = self.fit["span"] * 400 / (cell_height - 78) / 2
        self.Ortho(-half, half, self.fit["low"], self.fit["high"], -300, 300)
        self.MatrixMode(0x1700)
        self.LoadIdentity()
        self.Rotatef(elevation, 1, 0, 0)
        self.Rotatef(yaw, 0, 1, 0)
        if guard is not None:
            self.Translatef(*(-guard))

    def draw_model(self, model, transparent=False):
        self.UseProgram(self.program)
        self.Uniform1i(self.uniforms["pigment"], 0)
        for data, material, native in model.parts:
            if material["transparent"] != transparent:
                continue
            (self.Disable if material["double_sided"] else self.Enable)(0x0B44)
            extra = material["extras"]
            reflected = extra.get("neiTextureGen") == "spherical"
            self.Uniform4f(self.uniforms["baseColor"], *material["base"])
            for name, key in (("metal", "metal"), ("roughness", "rough"), ("emission", "emission")):
                self.Uniform1f(self.uniforms[name], material[key])
            for name, value in (("textured", bool(material["texture"])), ("reflected", reflected),
                                ("linearGen", extra.get("linear", False)), ("nativeNormal", native),
                                ("decal", extra.get("neiDecal", False))):
                self.Uniform1i(self.uniforms[name], int(value))
            shift = np.asarray(extra.get("shift", (0, 0)), dtype=float)
            divisor = np.where(shift <= 10, 2 ** shift, 2 ** (shift - 16))
            factor = np.asarray(extra.get("scale", (1984, 1984))) / 32 / np.asarray(extra.get("neiTextureTile", (32, 32))) / divisor
            self.Uniform2f(self.uniforms["texgenFactor"], *factor)
            self.BindTexture(0x0DE1, material["texture"])
            for array in (0x8074, 0x8075, 0x8078):
                self.EnableClientState(array)
            self.VertexPointer(3, 0x1406, 32, data.ctypes.data)
            self.NormalPointer(0x1406, 32, data.ctypes.data + 12)
            self.TexCoordPointer(2, 0x1406, 32, data.ctypes.data + 24)
            self.DrawArrays(0x0004, 0, len(data))
            for array in (0x8074, 0x8075, 0x8078):
                self.DisableClientState(array)
        self.UseProgram(0)

    def draw_effect(self, mesh):
        self.Disable(0x0B44)
        self.Disable(0x0DE1)
        self.EnableClientState(0x8074)
        self.EnableClientState(0x8076)
        self.VertexPointer(3, 0x1406, 16, mesh.ctypes.data)
        self.ColorPointer(4, 0x1401, 16, mesh.ctypes.data + 12)
        self.DrawArrays(0x0004, 0, len(mesh))
        self.DisableClientState(0x8074)
        self.DisableClientState(0x8076)

    def frame(self, models, sampled, index, elevation, effects=True, row_only=None,
              cell_height=None, align_guards=False):
        self.DepthMask(1)
        self.Clear(0x4000 | 0x0100)
        selected = list(models.items()) if row_only is None else list(models.items())[row_only * 3:row_only * 3 + 3]
        cell_height = cell_height or (CELL if row_only is None else GROUP_CELL)
        for cell, (slug, model) in enumerate(selected):
            _, yaw, intrinsic, shimmer = sampled[slug][index]
            self.pose(cell, elevation, yaw, cell_height, model.guard if align_guards else None)
            self.DepthMask(1)
            self.draw_model(model)
            self.DepthMask(0)
            if effects:
                self.draw_effect(intrinsic)
                self.draw_effect(shimmer)
            self.draw_model(model, transparent=True)
        self.DepthMask(1)
        height = HEADER + math.ceil(len(selected) / 3) * cell_height + 40
        pixels = np.zeros((height, W, 4), dtype=np.uint8)
        self.ReadPixels(0, BUFFER_H - height, W, height, 0x1908, 0x1401, pixels.ctypes.data)
        error = self.GetError()
        assert error == 0, f"OpenGL error {error:#x}"
        image = Image.fromarray(pixels[::-1].copy()).convert("RGB")
        return annotate(image, effects, [PROFILES[slug] for slug, _ in selected], cell_height,
                        [model for _, model in selected], self.fit if align_guards else None)


def annotate(image, effects=True, profiles=None, cell_height=CELL, models=None, aligned_fit=None):
    draw = ImageDraw.Draw(image)
    font = lambda size: ImageFont.truetype(FONT, size)
    title = "Sword collection · measured model scale" if aligned_fit else "Sword collection · rotating preview"
    draw.text((25, 17), title, font=font(30), fill="#eff3f9")
    subtitle = "Guard-aligned · common scale · effects off" if aligned_fit else (
        "Colored effects + shimmer" if effects else "Model reference · effects off")
    draw.text((26, 58), subtitle, font=font(20), fill="#b6c5d7")
    for index, (_, name, caption, color) in enumerate(profiles or PROFILES.values()):
        x, y = (index % 3) * CELL, HEADER + (index // 3) * cell_height
        draw.line((x + 16, y + 2, x + CELL - 16, y + 2), fill="#2b394c")
        draw.text((x + 20, y + 15), name, font=font(23), fill="#edf2f8")
        note = caption if effects else "Exact sword model"
        if aligned_fit:
            note = f"Height {np.ptp(models[index].positions[:, 1]):.1f} model units"
            baseline = y + 70 + aligned_fit["high"] / aligned_fit["span"] * (cell_height - 78)
            # Short outer marks show the aligned guard height without covering it.
            for left, right in ((x + 18, x + 70), (x + 330, x + 382)):
                draw.line((left, baseline, right, baseline), fill="#46576b", width=1)
        draw.text((x + 20, y + 47), note, font=font(17), fill=color if effects else "#b6c5d7")
    draw.text((25, image.height - 28), "Offline exact-model preview · reflections and lighting approximate · not gameplay",
              font=font(16), fill="#91a4bc")
    return image


def save_movie(images, destination):
    # Learn a shared palette from multiple rotations, including all nine hues.
    selected = images[::max(1, len(images) // 12)]
    thumb_height = round(400 * images[0].height / images[0].width)
    thumb = Image.new("RGB", (400 * 4, thumb_height * math.ceil(len(selected) / 4)), BACKGROUND)
    for index, image in enumerate(selected):
        thumb.paste(image.resize((400, thumb_height)), ((index % 4) * 400, (index // 4) * thumb_height))
    palette = thumb.quantize(colors=256)
    frames = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in images]
    frames[0].save(destination, save_all=True, append_images=frames[1:], duration=100,
                   loop=0, optimize=False, disposal=2)
    with Image.open(destination) as check:
        assert check.n_frames == len(images) and check.info.get("loop") == 0
    print(f"Saved {destination} ({destination.stat().st_size / 1024**2:.2f} MiB)", flush=True)


def scale_pairs(before, after, before_models, after_models, destination):
    """Pair equally sized cell crops; neither model is resized or separately fit."""
    slugs = list(before_models)
    rows = math.ceil(len(slugs) / 3)
    sheet = Image.new("RGB", (2400, HEADER + rows * GROUP_CELL + 40), BACKGROUND)
    draw = ImageDraw.Draw(sheet)
    font = lambda size: ImageFont.truetype(FONT, size)
    draw.text((25, 17), f"{len(slugs)} swords · before and after", font=font(30), fill="#eff3f9")
    draw.text((26, 58), "Guard-aligned · common scale · effects off", font=font(20), fill="#b6c5d7")
    for index, slug in enumerate(slugs):
        x, y = (index % 3) * 800, HEADER + (index // 3) * GROUP_CELL
        source_x, source_y = (index % 3) * 400, HEADER + (index // 3) * GROUP_CELL
        for column, image, model, label in ((0, before, before_models[slug], "Before"),
                                          (1, after, after_models[slug], "After")):
            sheet.paste(image.crop((source_x, source_y + 70, source_x + 400, source_y + GROUP_CELL)),
                        (x + column * 400, y + 70))
            draw.text((x + column * 400 + 20, y + 47),
                      f"{label} · height {np.ptp(model.positions[:, 1]):.1f} model units",
                      font=font(17), fill="#b6c5d7" if column == 0 else "#c8e6bd")
        draw.line((x + 16, y + 2, x + 784, y + 2), fill="#2b394c")
        draw.text((x + 20, y + 15), PROFILES[slug][1], font=font(23), fill="#edf2f8")
    draw.text((25, sheet.height - 28),
              "Offline exact-model preview · reflections and lighting approximate · not gameplay",
              font=font(16), fill="#91a4bc")
    sheet.save(destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--frames", type=int, default=72)
    parser.add_argument("--elevation", type=float, default=12)
    parser.add_argument("--stem", default="swords_rotation")
    parser.add_argument("--groups", action="store_true", help="Also save three larger row GIFs")
    parser.add_argument("--before-root", type=Path,
                        help="Optional directory containing earlier slug/slug.glb and checkpoint.json pairs")
    args = parser.parse_args()
    assert 8 <= args.frames <= 720 and math.isfinite(args.elevation)
    assert args.stem.startswith("swords"), "Keep generated sword previews under swords*"
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="nei-swords-preview-") as temporary:
        sampled = samples(args.frames, args.elevation, Path(temporary))
        renderer = Renderer()
        models = {slug: Model(slug, renderer) for slug in PROFILES}
        before_models = {slug: Model(slug, renderer, args.before_root) for slug in PROFILES
                         if args.before_root and (args.before_root / slug / (slug + ".glb")).is_file()}
        renderer.fit = fit_camera(models, sampled, args.elevation, before_models)
        for slug, model in models.items():
            print(slug, "exact GLB bounds", [bound.round(3).tolist() for bound in model.bounds], flush=True)
        images = []
        for index in range(args.frames):
            images.append(renderer.frame(models, sampled, index, args.elevation))
            if index % 12 == 0:
                print(f"Rendered frame {index + 1}/{args.frames}", flush=True)
        # A front-ish angle reveals material detail while the GIF includes a full turn.
        front_index = round(args.frames * 16 / 360) % args.frames
        reference = renderer.frame(models, sampled, front_index, args.elevation, effects=False)
        measured = renderer.frame(models, sampled, front_index, args.elevation, effects=False,
                                  cell_height=GROUP_CELL, align_guards=True)
        if before_models:
            after_subset = {slug: models[slug] for slug in before_models}
            before_sheet = renderer.frame(before_models, sampled, front_index, args.elevation, effects=False,
                                          cell_height=GROUP_CELL, align_guards=True)
            after_sheet = renderer.frame(after_subset, sampled, front_index, args.elevation, effects=False,
                                         cell_height=GROUP_CELL, align_guards=True)
            scale_pairs(before_sheet, after_sheet, before_models, after_subset,
                        args.output / (args.stem + "_before_after_scale.png"))
    save_movie(images, args.output / (args.stem + ".gif"))
    if args.groups:
        for row in range(3):
            grouped = [renderer.frame(models, sampled, index, args.elevation, row_only=row)
                       for index in range(args.frames)]
            save_movie(grouped, args.output / (args.stem + f"_group_{row + 1}.gif"))
            grouped[front_index].save(args.output / (args.stem + f"_group_{row + 1}_front.png"))
    images[front_index].save(args.output / (args.stem + "_front.png"))
    reference.save(args.output / (args.stem + "_models_only.png"))
    measured.save(args.output / (args.stem + "_measured_scale.png"))
    camera = dict(renderer.fit)
    camera["guard_baselines"] = {slug: model.guard.tolist() for slug, model in models.items()}
    camera["model_heights"] = {slug: float(np.ptp(model.positions[:, 1])) for slug, model in models.items()}
    camera["sampled_frames"] = args.frames
    camera["views"] = {"overview": {"viewport": [400, CELL - 78]},
                       "groups_and_measured_scale": {"viewport": [400, GROUP_CELL - 78]}}
    (args.output / (args.stem + "_camera.json")).write_text(json.dumps(camera, indent=2) + "\n")
    contact = Image.new("RGB", (W * 2, H * 2), BACKGROUND)
    for cell, fraction in enumerate((0, .25, .5, .75)):
        contact.paste(images[round(fraction * args.frames) % args.frames], ((cell % 2) * W, (cell // 2) * H))
    contact.save(args.output / (args.stem + "_contact.png"))
    print(f"Rendered {args.frames} frames; intrinsic + shimmer ON, opaque depth ON", flush=True)


if __name__ == "__main__":
    main()
