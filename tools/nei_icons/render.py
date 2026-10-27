"""Deterministic RGBA renderer for the repository's actual GI triangles.

Extends tools/nei_gi/SOURCE/preview.py's CPU raster approach. Geometry and
materials come from native GLB checkpoints or XML display lists, never from a
32px icon. Lighting is an offline approximation, not a game capture.
"""
from __future__ import annotations

import copy
import io
import json
import math
import struct
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def unit(v):
    v = np.asarray(v, dtype=float)
    return v / np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-12)


def rotation(axis, degrees):
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    return np.array({"x": [[1, 0, 0], [0, c, -s], [0, s, c]],
                     "y": [[c, 0, s], [0, 1, 0], [-s, 0, c]],
                     "z": [[c, -s, 0], [s, c, 0], [0, 0, 1]]}[axis])


@dataclass
class Material:
    color: list = field(default_factory=lambda: [1., 1., 1., 1.])
    texture: np.ndarray | None = None
    environment: list | None = None
    texgen: bool = False
    texgen_linear: bool = False
    tex_scale: tuple = (1., 1.)
    emissive: float = 0.
    metallic: float = 0.
    roughness: float = .6
    cull: bool = True


@dataclass
class Part:
    positions: np.ndarray
    normals: np.ndarray
    uv: np.ndarray
    triangles: np.ndarray
    material: Material


@dataclass
class Model:
    parts: list = field(default_factory=list)
    sources: set = field(default_factory=set)
    approximations: set = field(default_factory=set)


def glb_model(path):
    path = Path(path)
    data = path.read_bytes()
    magic, version, length = struct.unpack_from("<4sII", data)
    assert (magic, version, length) == (b"glTF", 2, len(data))
    cursor = 12
    chunks = {}
    while cursor < len(data):
        size, kind = struct.unpack_from("<I4s", data, cursor)
        chunks[kind] = data[cursor + 8:cursor + 8 + size]
        cursor += 8 + size
    doc, binary = json.loads(chunks[b"JSON"]), chunks[b"BIN\0"]

    def accessor(index):
        a = doc["accessors"][index]
        view = doc["bufferViews"][a["bufferView"]]
        dtype = np.dtype({5126: "<f4", 5125: "<u4", 5123: "<u2", 5121: "u1"}[a["componentType"]])
        width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[a["type"]]
        stride = view.get("byteStride", dtype.itemsize * width)
        return np.ndarray((a["count"], width), dtype=dtype, buffer=binary,
                          offset=view.get("byteOffset", 0) + a.get("byteOffset", 0),
                          strides=(stride, dtype.itemsize)).copy()

    materials = []
    for mat in doc["materials"]:
        pbr = mat.get("pbrMetallicRoughness", {})
        tex = None
        if "baseColorTexture" in pbr:
            img = doc["images"][doc["textures"][pbr["baseColorTexture"]["index"]]["source"]]
            view = doc["bufferViews"][img["bufferView"]]
            offset = view.get("byteOffset", 0)
            tex = np.asarray(Image.open(io.BytesIO(binary[offset:offset + view["byteLength"]])).convert("RGBA"), dtype=float) / 255
        materials.append(Material(color=pbr.get("baseColorFactor", [1.] * 4), texture=tex,
                                  emissive=max(mat.get("emissiveFactor", [0.])),
                                  metallic=pbr.get("metallicFactor", 0.),
                                  roughness=pbr.get("roughnessFactor", .6),
                                  cull=not mat.get("doubleSided", False)))
    result = Model(sources={path})

    def visit(index, parent):
        node = doc["nodes"][index]
        local = np.eye(4)
        if "matrix" in node:
            local = np.array(node["matrix"]).reshape(4, 4).T
        else:
            if "rotation" in node:
                x, y, z, w = node["rotation"]
                local[:3, :3] = [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                                  [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                                  [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]
            local[:3, :3] = local[:3, :3] @ np.diag(node.get("scale", [1, 1, 1]))
            local[:3, 3] = node.get("translation", [0, 0, 0])
        transform = parent @ local
        if "mesh" in node:
            for primitive in doc["meshes"][node["mesh"]]["primitives"]:
                assert primitive.get("mode", 4) == 4
                a = primitive["attributes"]
                p = accessor(a["POSITION"])
                p = p @ transform[:3, :3].T + transform[:3, 3]
                n = unit(accessor(a["NORMAL"]) @ np.linalg.inv(transform[:3, :3]))
                uv = accessor(a["TEXCOORD_0"]) if "TEXCOORD_0" in a else np.zeros((len(p), 2))
                indices = accessor(primitive["indices"]).reshape(-1, 3)
                result.parts.append(Part(p, n, uv, indices, materials[primitive["material"]]))
        for child in node.get("children", []):
            visit(child, transform)
    for root in doc["scenes"][doc.get("scene", 0)]["nodes"]:
        visit(root, np.eye(4))
    return result


def native_model(entry):
    """Read nested XML display lists with the native vertex cache and material state.

    The six stat models use primitive/environment interpolation and native signed
    vertex normals. Unavailable ROM-only reflection maps use neutral illumination;
    their exact absence is recorded instead of inventing substitute texture art.
    """
    roots = [ROOT / host / "assets/custom" for host in ("soh", "mm")]
    result = Model()
    cache = {}
    state = Material()
    tex_path = None

    def resolve(path, required=True):
        for root in roots:
            found = root / path
            if found.is_file():
                result.sources.add(found)
                return found
        if required:
            raise FileNotFoundError(path)
        result.approximations.add("ROM-only texture unavailable; neutral reflection: " + path)
        return None

    def texture(path):
        source = resolve(path, False)
        if source is None:
            return None
        raw = source.read_bytes()
        typ, width, height, size = struct.unpack_from("<IIII", raw, 64)
        assert typ == 6 and size == width * height, source  # native I8
        intensity = np.frombuffer(raw, np.uint8, offset=80, count=size).reshape(height, width) / 255
        return np.stack([intensity] * 3 + [np.ones_like(intensity)], axis=-1)

    def visit(path, stack=()):
        nonlocal tex_path
        assert path not in stack, ("recursive display list", path)
        for command in ET.parse(resolve(path)).getroot():
            tag, a = command.tag, command.attrib
            if tag == "CallDisplayList":
                visit(a["Path"], stack + (path,))
            elif tag == "LoadVertices":
                vertices = list(ET.parse(resolve(a["Path"])).getroot())
                start, count, slot = int(a["VertexOffset"]), int(a["Count"]), int(a["VertexBufferIndex"])
                for i, vertex in enumerate(vertices[start:start + count]):
                    v = vertex.attrib
                    p = [int(v[k]) for k in ("X", "Y", "Z")]
                    normal = [((int(v[k]) + 128) % 256 - 128) for k in ("R", "G", "B")]
                    cache[slot + i] = (p, unit(normal), [int(v["S"]) / 1024, int(v["T"]) / 1024])
            elif tag in ("Triangle1", "Triangle2"):
                for prefix in (["V0"] if tag == "Triangle1" else ["V0", "V1"]):
                    v = [cache[int(a[prefix + str(i)])] for i in range(3)]
                    result.parts.append(Part(np.array([x[0] for x in v]), np.array([x[1] for x in v]),
                                             np.array([x[2] for x in v]), np.array([[0, 1, 2]]), copy.copy(state)))
            elif tag in ("SetPrimColor", "SetEnvColor"):
                color = [int(a[k]) / 255 for k in ("R", "G", "B", "A")]
                if tag == "SetPrimColor":
                    state.color = color
                else:
                    state.environment = color
            elif tag in ("SetGeometryMode", "ClearGeometryMode"):
                on = tag == "SetGeometryMode"
                if "G_TEXTURE_GEN" in a:
                    state.texgen = on
                if "G_TEXTURE_GEN_LINEAR" in a:
                    state.texgen_linear = on
                if "G_CULL_BACK" in a:
                    state.cull = on
            elif tag == "SetTextureImage":
                tex_path = a["Path"]
                state.texture = texture(tex_path)
            elif tag == "Texture":
                state.tex_scale = (int(a["S"]) / 2048, int(a["T"]) / 2048)
            elif tag == "Matrix":
                raise ValueError("Native stat importer has no matrix commands: " + path)
    visit(entry)
    assert result.parts, entry
    return result


def sample(texture, uv):
    h, w = texture.shape[:2]
    xy = np.mod(uv * [w, h] - .5, [w, h])
    lo = np.floor(xy).astype(int)
    fx, fy = (xy - lo).T[:, :, None]
    x, y = lo.T
    return ((texture[y, x] * (1-fx) + texture[y, (x+1) % w] * fx) * (1-fy)
            + (texture[(y+1) % h, x] * (1-fx) + texture[(y+1) % h, (x+1) % w] * fx) * fy)


def render(model, size=512, azimuth=20, elevation=10, roll=0, supersample=2, fill=.88):
    """Render straight-alpha RGBA with transparent margins and no background art."""
    width = size * supersample
    rot = rotation("z", roll) @ rotation("x", elevation) @ rotation("y", azimuth)
    points = np.concatenate([p.positions @ rot.T for p in model.parts])
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low + high) / 2
    scale = width * fill / max((high-low)[:2])
    pixels = np.zeros((width, width, 4), dtype=float)  # premultiplied RGBA
    depth_buffer = np.full((width, width), -np.inf)
    light = unit([-.42, .72, .72])
    half = unit(light + [0, 0, 1])
    work = []
    for part in model.parts:
        positions = part.positions @ rot.T - center
        normals = unit(part.normals @ rot.T)
        screen = np.c_[width/2 + positions[:, 0]*scale, width/2 - positions[:, 1]*scale]
        for face in part.triangles:
            # Transparent faces need depth ordering even inside one primitive.
            work.append((part.material.color[3] < .999, float(positions[face, 2].mean()), part, face, positions, normals, screen))
    work.sort(key=lambda x: (x[0], x[1] if x[0] else 0))
    for transparent, _, part, face, positions, normals, screen in work:
        mat = part.material
        face_normal = np.cross(positions[face[1]]-positions[face[0]], positions[face[2]]-positions[face[0]])
        if mat.cull and face_normal[2] <= 0:
            continue
        v = screen[face]
        x0, y0 = v[0]; x1, y1 = v[1]; x2, y2 = v[2]
        area = (x1-x0)*(y2-y0)-(x2-x0)*(y1-y0)
        if abs(area) < 1e-8:
            continue
        xmin, ymin = np.maximum(0, np.floor(v.min(axis=0)).astype(int))
        xmax, ymax = np.minimum(width-1, np.ceil(v.max(axis=0)).astype(int))
        if xmax < xmin or ymax < ymin:
            continue
        xx, yy = np.meshgrid(np.arange(xmin, xmax+1)+.5, np.arange(ymin, ymax+1)+.5)
        w0 = ((x1-xx)*(y2-yy)-(x2-xx)*(y1-yy))/area
        w1 = ((x2-xx)*(y0-yy)-(x0-xx)*(y2-yy))/area
        w2 = 1-w0-w1
        z = positions[face, 2]
        depth = w0*z[0] + w1*z[1] + w2*z[2]
        zb = depth_buffer[ymin:ymax+1, xmin:xmax+1]
        mask = (w0 >= -1e-8) & (w1 >= -1e-8) & (w2 >= -1e-8) & (depth > zb + 1e-9)
        if not mask.any():
            continue
        weights = np.stack([w0[mask], w1[mask], w2[mask]], axis=1)
        normal = unit(weights @ normals[face])
        diffuse = np.maximum(normal @ light, 0)
        shade = .46 + .54*diffuse
        base = np.broadcast_to(np.array(mat.color), (len(weights), 4)).copy()
        if mat.texture is not None:
            if mat.texgen:
                uv = ((np.arccos(np.clip(normal[:, :2], -1, 1)) / np.pi) if mat.texgen_linear
                      else (normal[:, :2] * .5 + .5)) * mat.tex_scale
            else:
                uv = weights @ part.uv[face]
            tex = sample(mat.texture, uv)
            if mat.environment is not None:
                base[:, :3] = np.array(mat.environment[:3]) + (base[:, :3] - mat.environment[:3]) * tex[:, :3]
            else:
                base *= tex
        elif mat.environment is not None and mat.texgen:
            reflection = (.62 + .28 * diffuse)[:, None]
            base[:, :3] = np.array(mat.environment[:3]) + (base[:, :3] - mat.environment[:3]) * reflection
        shade = shade * (1-mat.emissive) + mat.emissive
        # Black model cores remain black; highlights belong to their outer shell.
        specular = np.maximum(normal @ half, 0) ** (18 + 48*(1-mat.roughness))
        specular *= (.12 if mat.metallic else .025) * (float(max(mat.color[:3])) > 0)
        rgb = np.clip(base[:, :3]*shade[:, None] + specular[:, None], 0, 1)
        alpha = base[:, 3:4]
        target = pixels[ymin:ymax+1, xmin:xmax+1]
        target[mask, :3] = rgb*alpha + target[mask, :3]*(1-alpha)
        target[mask, 3:4] = alpha + target[mask, 3:4]*(1-alpha)
        if not transparent:
            zb[mask] = depth[mask]
    # Downsample premultiplied colors before unpremultiplying: no dark fringes.
    # Pillow's RGBA resize performs its own premultiplication. Resize the four
    # already-premultiplied channels separately so transparent edges stay exact.
    data = np.stack([np.asarray(Image.fromarray(pixels[:, :, c].astype(np.float32), "F")
                                .resize((size, size), Image.Resampling.LANCZOS))
                     for c in range(4)], axis=-1)
    data = np.clip(data, 0, 1)
    alpha = data[:, :, 3:4]
    data[:, :, :3] = np.divide(data[:, :, :3], alpha, out=np.zeros_like(data[:, :, :3]), where=alpha > 0)
    return Image.fromarray(np.uint8(np.clip(data*255+.5, 0, 255)), "RGBA")
