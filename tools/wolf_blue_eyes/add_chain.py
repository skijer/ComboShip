#!/usr/bin/env python3
"""Attach the supplied TP chain to the HD Wolf cuff in native NEIWOLF1 v2.

Six copies of the original link mesh use additional bones and baked drape
tracks. The original 40 bone tracks, body mesh, weights and sounds survive
exactly. This adds no native gameplay physics or extra rendering pass.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

import numpy as np
from PIL import Image

NS = {"c": "http://www.collada.org/2005/11/COLLADASchema"}
RESOURCE = "objects/forms/wolf_link/gWolfLinkData"
HD_RESOURCE = "objects/forms/wolf_link/gWolfLinkHDData"
SOFT_SHA = "3f9d052d6542ff46e0c1b4e6f60a96dcfc821aeb32e42d871016c9250f152f5a"
SOFT_GLOW_SHA = "8e1cfe015918952253568cdf5ef9e127faf7b78162d33f692dc4963dda47c49e"
VERTEX = struct.Struct("<3f3bhhB")
LINKS, CUFF_BONE, CHAIN_PARENT, SPACING = 6, 17, 17, 8.5
ANCHOR = np.array([33.0, 18.8, 38.0, 1.0])
TILE = (820, 4)


def read_link(source):
    root = ET.parse(source / "chain.DAE").getroot()
    skin = root.find(".//c:controller/c:skin", NS)
    if skin is None:
        raise ValueError("expected the supplied skinned TP chain")
    bind_shape = np.fromstring(skin.find("c:bind_shape_matrix", NS).text, sep=" ").reshape(4, 4)
    if not np.allclose(bind_shape, np.eye(4)):
        raise ValueError("unexpected chain bind shape")
    src = {e.get("id"): e for e in skin.findall("c:source", NS)}
    joints = {e.get("semantic"): e.get("source")[1:] for e in skin.find("c:joints", NS)}
    inv = np.fromstring(src[joints["INV_BIND_MATRIX"]].find("c:float_array", NS).text, sep=" ").reshape(-1, 4, 4)
    vw = skin.find("c:vertex_weights", NS)
    inputs = {e.get("semantic"): e for e in vw.findall("c:input", NS)}
    stride = max(int(e.get("offset")) for e in inputs.values()) + 1
    if set(vw.find("c:vcount", NS).text.split()) != {"1"}:
        raise ValueError("chain link must have rigid source weights")
    weights = np.fromstring(vw.find("c:v", NS).text, sep=" ", dtype=int).reshape(-1, stride)
    mesh = next(g for g in root.findall(".//c:geometry", NS) if "#" + g.get("id") == skin.get("source")).find("c:mesh", NS)
    arrays = {e.get("id"): np.fromstring(e.find("c:float_array", NS).text, sep=" ").reshape(
        -1, int(e.find("c:technique_common/c:accessor", NS).get("stride"))) for e in mesh.findall("c:source", NS)}
    positions = {e.get("id"): e.find("c:input", NS).get("source")[1:] for e in mesh.findall("c:vertices", NS)}
    group = mesh.find("c:triangles", NS)
    inputs = {e.get("semantic"): e for e in group.findall("c:input", NS)}
    stride = max(int(e.get("offset")) for e in inputs.values()) + 1
    indices = np.fromstring(group.find("c:p", NS).text, sep=" ", dtype=int).reshape(-1, stride)
    vertex_ids = indices[:, int(inputs["VERTEX"].get("offset"))]
    p = arrays[positions[inputs["VERTEX"].get("source")[1:]]][vertex_ids]
    n = arrays[inputs["NORMAL"].get("source")[1:]][indices[:, int(inputs["NORMAL"].get("offset"))]]
    uv = arrays[inputs["TEXCOORD"].get("source")[1:]][indices[:, int(inputs["TEXCOORD"].get("offset"))]][:, :2]
    # This authored link occupies one small repeated UV tile (negative V).
    # Move the whole tile together so its original texture taps stay intact.
    uv -= np.floor(uv.min(0))
    if np.any(uv < 0) or np.any(uv > 1):
        raise ValueError("chain link crosses a UV tile; split it before atlasing")
    # Use the actual authored link, normalized to an upright long axis; the
    # cutscene's distant placement and randomly posed source bones are removed.
    centered = p - (p.min(0) + p.max(0)) / 2
    _, axes = np.linalg.eigh(np.cov(centered.T))
    basis = axes[:, [1, 2, 0]]
    if np.linalg.det(basis) < 0:
        basis[:, 2] *= -1
    p, n = centered @ basis, n @ basis
    n /= np.linalg.norm(n, axis=1)[:, None]
    assert len(p) == 144 and np.isfinite(p).all() and np.isfinite(n).all()
    assert np.ptp(p[:, 1]) > np.ptp(p[:, 0]) > np.ptp(p[:, 2])
    return p, n, uv


def matrices(tracks):
    t = tracks.astype(float)
    x, y, z = np.deg2rad(t[..., 3:6]).transpose(2, 0, 1)
    sx, cx, sy, cy, sz, cz = np.sin(x), np.cos(x), np.sin(y), np.cos(y), np.sin(z), np.cos(z)
    m = np.zeros(t.shape[:-1] + (4, 4))
    m[..., 0, 0], m[..., 1, 0], m[..., 2, 0] = cy * cz, cy * sz, -sy
    m[..., 0, 1], m[..., 1, 1], m[..., 2, 1] = sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy
    m[..., 0, 2], m[..., 1, 2], m[..., 2, 2] = cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy
    m[..., :3, :3] *= t[..., None, 6:9]
    m[..., :3, 3], m[..., 3, 3] = t[..., :3], 1
    return m


def decompose(m):
    scale = np.linalg.norm(m[..., :3, :3], axis=-2)
    r = m[..., :3, :3] / scale[..., None, :]
    # Original leg scales can introduce shear when cancelling their rotation.
    # Use the closest supported TRS for the metal orientation. Translation is
    # exact; the first link's head remains a constant child of the cuff bone.
    u, _, vt = np.linalg.svd(r)
    r = u @ vt
    if np.any(np.linalg.det(r) < 0):
        raise ValueError("chain orientation is reflected")
    scale = np.sum(r * m[..., :3, :3], axis=-2)
    y = np.arcsin(np.clip(-r[..., 2, 0], -1, 1))
    x = np.arctan2(r[..., 2, 1], r[..., 2, 2])
    z = np.arctan2(r[..., 1, 0], r[..., 0, 0])
    singular = np.abs(np.cos(y)) < 1e-7
    x = np.where(singular, np.arctan2(-r[..., 1, 2], r[..., 1, 1]), x)
    z = np.where(singular, 0, z)
    tracks = np.concatenate([m[..., :3, 3], np.rad2deg(np.stack([x, y, z], -1)), scale], -1)
    # Keep consecutive Euler samples continuous for native half-frame blending.
    tracks[..., 3:6] = np.rad2deg(np.unwrap(np.deg2rad(tracks[..., 3:6]), axis=0))
    assert np.isfinite(tracks).all()
    assert np.allclose(matrices(tracks)[..., :3, 3], m[..., :3, 3], atol=2e-5)
    return tracks.astype("<f4")


def drape(anchors, velocity=None):
    frames = len(anchors)
    nodes = np.zeros((frames, LINKS + 1, 3))
    nodes[:, 0] = anchors
    floor = np.minimum(5.5, anchors[:, 1])
    direction = np.tile([0.0, -1.0], (frames, 1))
    if velocity is not None:
        direction[:, 0] -= np.clip(velocity[:, 0] * 0.12, -0.6, 0.6)
        direction[:, 1] -= np.clip(velocity[:, 2] * 0.08, -0.3, 0.3)
    direction /= np.linalg.norm(direction, axis=1)[:, None]
    for i in range(LINKS):
        drop = np.clip(nodes[:, i, 1] - floor, 0, SPACING * 0.8)
        horizontal = np.sqrt(SPACING ** 2 - drop ** 2)
        nodes[:, i + 1] = nodes[:, i] + np.column_stack([direction[:, 0] * horizontal, -drop, direction[:, 1] * horizontal])
    m = np.tile(np.eye(4), (frames, LINKS, 1, 1))
    for i in range(LINKS):
        y = (nodes[:, i] - nodes[:, i + 1]) / SPACING
        x = np.tile([1.0, 0.0, 0.0], (frames, 1))
        x -= np.sum(x * y, axis=1)[:, None] * y
        x /= np.linalg.norm(x, axis=1)[:, None]
        z = np.cross(x, y)
        if i % 2:
            x, z = z, -x
        m[:, i, :3, :3] = np.stack([x, y, z], axis=-1)
        m[:, i, :3, 3] = (nodes[:, i] + nodes[:, i + 1]) / 2
    assert np.allclose(np.linalg.norm(np.diff(nodes, axis=1), axis=-1), SPACING)
    return m


def read_soft_wolf(wolf):
    with zipfile.ZipFile(wolf) as archive:
        resource = archive.read(RESOURCE)
    base = resource[68:]
    if hashlib.sha256(base).hexdigest() not in (SOFT_SHA, SOFT_GLOW_SHA):
        raise ValueError("expected the softer-eye TP candidate")
    return resource


def build(source, wolf, output):
    resource = read_soft_wolf(wolf)
    base = resource[68:]
    h = list(struct.unpack_from("<20I", base, 12))
    assert h[2:4] == [40, 144] and h[18] == len(base)
    p, n, uv = read_link(source)
    inv = np.frombuffer(base, "<f4", h[2] * 16, h[9]).reshape(-1, 4, 4).transpose(0, 2, 1).astype(float)
    bind = np.linalg.inv(inv)
    mesh_bind = drape(ANCHOR[None, :3])[0]
    chain_bind = mesh_bind.copy()
    # First bone's origin is the attachment, rather than the link center. Its
    # local translation can then remain exact during native half-frame blends.
    chain_bind[0, :3, 3] = ANCHOR[:3]
    vb, wb = bytearray(base[h[6]:h[6] + h[0] * 20]), bytearray(base[h[7]:h[7] + h[0] * 8])
    image = np.array(Image.open(source / "kusari.png").convert("RGB"))
    height, width = image.shape[:2]
    if width > 192 or height > 192 or min(width, height) < 8:
        raise ValueError("unexpected chain texture size")
    for link in range(LINKS):
        world_p = p @ mesh_bind[link, :3, :3].T + mesh_bind[link, :3, 3]
        world_n = n @ mesh_bind[link, :3, :3].T
        for position, normal, tex in zip(world_p, world_n, uv):
            sn = np.rint(normal * 127).clip(-127, 127).astype(int)
            st = np.rint([32 * (TILE[0] + tex[0] * width), 32 * (TILE[1] + (1 - tex[1]) * height)]).astype(int)
            vb.extend(VERTEX.pack(*position, *sn, *st, 255))
            wb.extend(bytes([h[2] + link, 0, 0, 0, 255, 0, 0, 0]))
    atlas = np.frombuffer(base, "<u2", h[4] * h[5], h[16]).reshape(h[5], h[4]).copy()
    rgb = image.astype(np.uint16)
    texture = ((rgb[..., 0] >> 3) << 11) | ((rgb[..., 1] >> 3) << 6) | ((rgb[..., 2] >> 3) << 1) | 1
    for y in range(-4, height + 4):
        for x in range(-4, width + 4):
            atlas[TILE[1] + y, TILE[0] + x] = texture[y % height, x % width]
    parents = np.frombuffer(base, "<i2", h[2], h[8])
    frame_chunk, entries = bytearray(), []
    max_attachment_error = 0.0
    max_trs_error = 0.0
    for i in range(h[3]):
        name, count, bones, rate, offset = struct.unpack_from("<IHHfI", base, h[11] + i * 16)
        tracks = np.frombuffer(base, "<f4", count * bones * 9, offset).reshape(count, bones, 9)
        world = matrices(tracks)
        for bone, parent in enumerate(parents):
            if parent >= 0:
                assert parent < bone
                world[:, bone] = world[:, parent] @ world[:, bone]
        anchors = (world[:, CUFF_BONE] @ inv[CUFF_BONE] @ ANCHOR)[:, :3]
        velocity = np.diff(anchors, axis=0, prepend=anchors[:1])
        chain_world = drape(anchors, velocity)
        chain_world[:, 0, :3, 3] = anchors
        local = np.linalg.inv(world[:, CHAIN_PARENT])[:, None] @ chain_world
        extra = decompose(local)
        extra[:, 0, :3] = (inv[CUFF_BONE] @ ANCHOR)[:3]
        max_trs_error = max(max_trs_error, float(np.abs(matrices(extra)[..., :3, :3] - local[..., :3, :3]).max()))
        restored = world[:, CHAIN_PARENT, None] @ matrices(extra)
        attachment = restored[:, 0, :3, 3]
        error = np.linalg.norm(attachment - anchors, axis=1).max()
        max_attachment_error = max(max_attachment_error, float(error))
        assert error < 0.001
        start = len(frame_chunk)
        for frame in range(count):
            # Copy original bytes, never reserialize the wolf's own float tracks.
            frame_chunk.extend(base[offset + frame * bones * 36:offset + (frame + 1) * bones * 36])
            frame_chunk.extend(extra[frame].tobytes())
        entries.append((name, count, bones + LINKS, rate, start))
    out = bytearray(b"NEIWOLF1" + struct.pack("<I", 2) + bytes(80))
    new = h.copy()
    new[0], new[1], new[2] = len(vb) // 20, len(vb) // 60, h[2] + LINKS
    def append(data):
        out.extend(bytes((-len(out)) % 4))
        offset = len(out)
        out.extend(data)
        return offset
    new[6], new[7] = append(vb), append(wb)
    new[8] = append(base[h[8]:h[8] + h[2] * 2] + np.full(LINKS, CHAIN_PARENT, "<i2").tobytes())
    new[9] = append(base[h[9]:h[9] + h[2] * 64] + np.linalg.inv(chain_bind).transpose(0, 2, 1).astype("<f4").tobytes())
    local_bind = inv[CHAIN_PARENT] @ chain_bind
    new[10] = append(base[h[10]:h[10] + h[2] * 12] + local_bind[:, :3, 3].astype("<f4").tobytes())
    new[11] = append(bytes(h[3] * 16))
    new[12] = append(base[h[12]:h[12] + h[13]])
    new[14], new[15] = append(frame_chunk), len(frame_chunk)
    for i, (name, count, bones, rate, frames) in enumerate(entries):
        struct.pack_into("<IHHfI", out, new[11] + i * 16, name - h[12] + new[12], count, bones, rate, frames + new[14])
    new[16] = append(atlas.astype("<u2").tobytes())
    new[19] = append(base[h[19]:])
    new[18] = len(out)
    struct.pack_into("<20I", out, 12, *new)
    output.mkdir(parents=True, exist_ok=True)
    (output / "wolf_link_hd.bin").write_bytes(out)
    serialized = resource[:64] + struct.pack("<I", len(out)) + out
    (output / "gWolfLinkHDData").write_bytes(serialized)
    report = {"input_payload_sha256": hashlib.sha256(base).hexdigest(), "payload_sha256": hashlib.sha256(out).hexdigest(),
              "source_dae_sha256": hashlib.sha256((source / "chain.DAE").read_bytes()).hexdigest(),
              "source_texture_sha256": hashlib.sha256((source / "kusari.png").read_bytes()).hexdigest(),
              "resource_path": HD_RESOURCE, "chain_links": LINKS, "attachment_bone": CUFF_BONE,
              "attachment_bind_position": ANCHOR[:3].tolist(), "vertices": new[0], "bones": new[2],
              "animation_clips": h[3], "original_wolf_tracks_mesh_weights_audio": "byte-preserved",
              "max_cuff_attachment_error_tp_units": max_attachment_error,
              "max_chain_trs_matrix_approximation": max_trs_error,
              "attachment_strategy": "constant cuff-child head offset, including native frame interpolation",
              "chain_motion": "baked drape and paw-motion lag; no world collision simulation",
              "runtime_status": "untested"}
    (output / "chain_conversion.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--wolf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build(args.source, args.wolf, args.output)
