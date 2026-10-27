"""Read-only decoder for the native keeper resources used by the asset preview.

Binary layouts follow SkeletonFactory, SkeletonLimbFactory, AnimationFactory,
ArrayFactory and Fast's DisplayListFactory. Poses follow SkelAnime_GetFrameData
and SkelAnime_DrawFlexOpa; this is intentionally independent of OpenGL.
"""
from dataclasses import dataclass
import math
import struct

import numpy as np
from keeper_lean import keeper_contract

KEEPER = "custom/skiejer_market/keeper/"
EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


class Reader:
    def __init__(self, data):
        self.data, self.at = data, 64

    def read(self, fmt):
        value = struct.unpack_from("<" + fmt, self.data, self.at)
        self.at += struct.calcsize("<" + fmt)
        return value[0] if len(value) == 1 else value

    def string(self):
        length = self.read("I")
        value = self.data[self.at:self.at + length].decode()
        self.at += length
        return value.removeprefix("__OTR__")

    def finish(self):
        assert self.at == len(self.data), (self.at, len(self.data))


def crc64(value):
    crc = 0xFFFFFFFFFFFFFFFF
    for byte in value.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ (0x42F0E1EBA9EA3693 if crc >> 63 else 0)) & 0xFFFFFFFFFFFFFFFF
    return crc


def translation(position):
    matrix = np.eye(4)
    matrix[:3, 3] = position
    return matrix


def rotate_xyz(rotation):
    """Matrix_TranslateRotateZYX postmultiplies Rz, Ry, Rx."""
    x, y, z = np.asarray(rotation) * math.pi / 32768
    sx, sy, sz, cx, cy, cz = math.sin(x), math.sin(y), math.sin(z), math.cos(x), math.cos(y), math.cos(z)
    rx = np.array([[1, 0, 0], [0, cx, -sx], [0, sx, cx]])
    ry = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    rz = np.array([[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]])
    result = np.eye(4)
    result[:3, :3] = rz @ ry @ rx
    return result


@dataclass
class Limb:
    name: str
    display: str
    position: tuple
    child: int
    sibling: int


class NativeKeeper:
    def __init__(self, archive, animation=None):
        self.archive = archive
        self.animation_path = (animation or keeper_contract()["animation"]).removeprefix("__OTR__")
        self.hashes = {crc64(n): n for n in archive.namelist()}
        reader = Reader(archive.read(KEEPER + "gTreasureChestShopGalSkel"))
        skeleton_type, limb_type = reader.read("bb")
        self.limb_count, self.display_count = reader.read("II")
        table_type, table_count = reader.read("bI")
        assert (skeleton_type, limb_type, table_type) == (1, 1, 1), "Expected native flex/standard keeper"
        assert table_count == self.limb_count
        names = [reader.string() for _ in range(table_count)]
        reader.finish()
        self.limbs = [self.read_limb(n) for n in names]
        reader = Reader(archive.read(self.animation_path))
        assert reader.read("I") == 0, "Expected normal animation"
        self.frame_count = reader.read("h")
        count = reader.read("I")
        self.values = np.array([reader.read("h") for _ in range(count)], dtype=np.int16)
        count = reader.read("I")
        self.indices = np.array([reader.read("HHH") for _ in range(count)], dtype=np.int32)
        self.static_max = reader.read("h")
        reader.finish()
        assert len(self.indices) == self.limb_count + 1
        assert np.max(self.indices) + self.frame_count <= len(self.values)
        self.vertices = {}
        self.display_lists = {}
        self.position_meshes = {}
        self.draw_order = []
        self._order(0)
        assert len(self.draw_order) == self.display_count
        assert len(set(self.draw_order)) == self.display_count

    def read_limb(self, name):
        reader = Reader(self.archive.read(name))
        kind, skin_type = reader.read("bb")
        assert kind == 1 and skin_type == 0
        assert reader.string() == ""
        assert reader.read("HI") == (0, 0)
        assert reader.string() == ""
        reader.read("fffHHH")
        assert reader.string() == reader.string() == ""
        display, far_display = reader.string(), reader.string()
        assert not far_display
        position = reader.read("hhh")
        child, sibling = reader.read("BB")
        reader.finish()
        return Limb(name, display, position, child, sibling)

    def _order(self, index):
        limb = self.limbs[index]
        if limb.display:
            self.draw_order.append(index)
        if limb.child != 255:
            self._order(limb.child)
        if limb.sibling != 255:
            self._order(limb.sibling)

    def joints(self, frame):
        # Native animation samples integer frames; fractional interpolation omitted.
        frame = int(frame) % self.frame_count
        return self.values[self.indices + np.where(self.indices >= self.static_max, frame, 0)]

    def matrices(self, frame, actor_position=None, yaw=None, scale=None):
        if actor_position is None or yaw is None or scale is None:
            contract = keeper_contract()
            actor_position = contract["position"] if actor_position is None else actor_position
            yaw = contract["yaw"] if yaw is None else yaw
            scale = contract["scale"] if scale is None else scale
        joints = self.joints(frame)
        actor = translation(actor_position) @ rotate_xyz((0, yaw, 0)) @ np.diag([scale, scale, scale, 1])
        matrices = {}

        def visit(index, parent):
            limb = self.limbs[index]
            position = joints[0] if index == 0 else limb.position
            matrix = parent @ translation(position) @ rotate_xyz(joints[index + 1])
            matrices[index] = matrix
            if limb.child != 255:
                visit(limb.child, matrix)
            if limb.sibling != 255:
                visit(limb.sibling, parent)

        visit(0, actor)
        assert len(matrices) == self.limb_count
        return [matrices[index] for index in self.draw_order]

    def vertex_array(self, name):
        if name not in self.vertices:
            data = self.archive.read(name)
            array_type, count = struct.unpack_from("<II", data, 64)
            assert array_type == 25 and len(data) == 72 + count * 16
            dtype = np.dtype([("xyz", "<i2", 3), ("flag", "<u2"), ("uv", "<i2", 2), ("rgba", "u1", 4)])
            self.vertices[name] = np.frombuffer(data, dtype=dtype, count=count, offset=72)
        return self.vertices[name]

    def commands(self, name):
        if name not in self.display_lists:
            data = self.archive.read(name)
            assert data[64] == 4, "Expected F3DEX2"
            at, commands = 72, []
            while at < len(data):
                w0, w1 = struct.unpack_from("<II", data, at)
                at += 8
                opcode, resource = w0 >> 24, None
                if opcode in EXPANDED:
                    high, low = struct.unpack_from("<II", data, at)
                    at += 8
                    resource = self.hashes[high << 32 | low]
                commands.append((opcode, w0, w1, resource))
                if opcode == 0xDF:
                    break
            assert at == len(data)
            self.display_lists[name] = commands
        return self.display_lists[name]

    def position_triangles(self, frame, actor_position=None, yaw=None, scale=None, blink=0):
        """Exact native triangle positions without repeating per-frame materials.

        A vertex keeps the matrix active when it entered the native cache;
        flex triangles may refer to vertices loaded by several limb matrices.
        """
        if blink not in self.position_meshes:
            cache, points, owners = {}, [], []
            for matrix_index, limb_index in enumerate(self.draw_order):
                limb = self.limbs[limb_index]
                current = matrix_index
                name = (KEEPER + f"ShopGalMMDBlinkHead{blink}DL") if limb.name.endswith("HeadLimb") else limb.display
                for opcode, w0, w1, resource in self.commands(name):
                    if opcode == 0xDA:
                        assert w1 >> 24 == 13, hex(w1)
                        current = (w1 & 0xFFFFFF) // 64
                    elif opcode == 0x32:
                        count = (w0 >> 12) & 255
                        start = ((w0 >> 1) & 127) - count
                        vertices = self.vertex_array(resource)[w1 // 16:w1 // 16 + count]
                        assert len(vertices) == count
                        for index, vertex in enumerate(vertices):
                            cache[start + index] = (vertex["xyz"], current)
                    elif opcode in (0x05, 0x06):
                        indices = [((w0 >> shift) & 255) // 2 for shift in (16, 8, 0)]
                        if opcode == 0x06:
                            indices += [((w1 >> shift) & 255) // 2 for shift in (16, 8, 0)]
                        for index in indices:
                            point, owner = cache[index]
                            points.append(point)
                            owners.append(owner)
                    elif opcode not in {0x20, 0x33, 0xF2, 0xFA, 0xE7, 0xE6, 0xE3, 0xE2, 0xD7, 0xD9, 0xFC, 0xF5, 0xF3, 0xDF}:
                        raise ValueError(f"Unsupported keeper opcode {opcode:02x} in {name}")
            self.position_meshes[blink] = (np.column_stack((points, np.ones(len(points)))), np.array(owners))
        points, owners = self.position_meshes[blink]
        matrices = np.array(self.matrices(frame, actor_position, yaw, scale))
        return np.einsum("nij,nj->ni", matrices[owners], points)[:, :3].reshape(-1, 3, 3)

    def geometry(self, frame, actor_position=None, yaw=None, scale=None, blink=0):
        matrices = self.matrices(frame, actor_position, yaw, scale)
        texture, size, prim, current = None, (32, 32), np.ones(4), matrices[0]
        cache, result = {}, []
        for index, limb_index in enumerate(self.draw_order):
            limb = self.limbs[limb_index]
            current = matrices[index]
            name = (KEEPER + f"ShopGalMMDBlinkHead{blink}DL") if limb.name.endswith("HeadLimb") else limb.display
            for opcode, w0, w1, resource in self.commands(name):
                if opcode == 0x20:
                    texture = resource
                elif opcode == 0xF2:
                    size = (((w1 >> 12) & 4095) / 4 + 1, (w1 & 4095) / 4 + 1)
                elif opcode == 0xFA:
                    prim = np.array([(w1 >> s) & 255 for s in (24, 16, 8, 0)]) / 255
                elif opcode == 0xDA:
                    assert w1 >> 24 == 13, hex(w1)
                    # SegAddr reserves bit zero as an address tag.
                    current = matrices[(w1 & 0xFFFFFF) // 64]
                elif opcode == 0x32:
                    count = (w0 >> 12) & 255
                    start = ((w0 >> 1) & 127) - count
                    vertices = self.vertex_array(resource)[w1 // 16:w1 // 16 + count]
                    assert len(vertices) == count
                    for i, vertex in enumerate(vertices):
                        point = current @ np.r_[vertex["xyz"], 1.0]
                        normal = vertex["rgba"][:3].view(np.int8).astype(float)
                        normal = current[:3, :3] @ normal
                        normal /= max(np.linalg.norm(normal), 1e-12)
                        shade = .62 + .38 * max(float(normal @ np.array([-.35, .8, -.48])), 0)
                        cache[start + i] = np.r_[point[:3], vertex["uv"], prim[:3] * shade, vertex["rgba"][3] / 255 * prim[3]]
                elif opcode in (0x05, 0x06):
                    indices = [((w0 >> s) & 255) // 2 for s in (16, 8, 0)]
                    if opcode == 0x06:
                        indices += [((w1 >> s) & 255) // 2 for s in (16, 8, 0)]
                    array = np.array([cache[i] for i in indices], dtype=np.float32)
                    array[:, 3:5] /= np.array(size) * 32
                    result.append((texture, np.ascontiguousarray(array)))
                elif opcode not in {0x33, 0xE7, 0xE6, 0xE3, 0xE2, 0xD7, 0xD9, 0xFC, 0xF5, 0xF3, 0xDF}:
                    raise ValueError(f"Unsupported keeper opcode {opcode:02x} in {name}")
        return result
