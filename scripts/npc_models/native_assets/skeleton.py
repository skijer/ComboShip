"""
Parse a Zelda64 (Flex)SkeletonHeader out of a zobj and compute a rest/bind pose.

SkeletonHeader:   { u32 limbArraySeg; u8 limbCount; pad3; (FlexSkel adds) u8 dListCount; }
StandardLimb:     { s16 x,y,z; u8 child; u8 sibling; u32 dListSeg; }

In the rest pose every joint uses identity rotation, so a limb's world
translation is just the sum of joint translations from the root along the
child/sibling tree. That is exactly the neutral pose we want for a front
portrait, and it lets us place each rigid limb DL without a matrix stack.
"""
from __future__ import annotations
import math
import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

import numpy as np

# Frame 0 of Link's idle animation (gPlayerAnim_link_normal_wait, link_animetion
# @0x1C3030), flattened to [rootPos, limb0rot … limb20rot] = 22 Vec3s in binang
# units. Link's animations live in the external link_animetion file, not the
# object, so without these rotations the skeleton renders as the meaningless
# bind-pose tangle (all bones extend along +X). Shared by adult & child Link
# (link animations are age-independent). Source: OoT decomp / MegatonHammer.
PLAYER_IDLE_POSE = [
    -57, 3377, 0,   0, 0, 0,   -16384, 730, -16384,   0, 0, 0,
    2332, 3070, -4374,   0, 0, 8198,   2115, 1887, -20292,   -1632, -2986, -4648,
    0, 0, 8327,   -1967, -1551, -20002,   16384, -2295, 16384,   -24, -431, 13532,
    0, 0, 19902,   0, 0, 0,   2353, -4705, 30867,   0, 0, -6675,
    -36, -503, -16171,   -793, 4973, -30158,   0, 0, -22217,   3940, 2803, -16884,
    -16384, 27240, -730,   0, 0, 0,
]

_BINANG = 2.0 * math.pi / 65536.0


@dataclass
class Limb:
    index: int
    x: int; y: int; z: int
    child: int
    sibling: int
    dlist: int              # segmented address (0 = none)
    world: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    # full posed affine: world_v = M @ v + T
    M: Optional[np.ndarray] = None
    T: Optional[np.ndarray] = None

    def apply(self, x, y, z):
        v = np.array([x, y, z], np.float64)
        if self.M is None:
            return (v[0] + self.world[0], v[1] + self.world[1], v[2] + self.world[2])
        w = self.M @ v + self.T
        return (float(w[0]), float(w[1]), float(w[2]))


@dataclass
class Skeleton:
    limbs: List[Limb] = field(default_factory=list)
    limb_count: int = 0
    dlist_count: int = 0
    slot_to_limb: List[int] = field(default_factory=list)  # flex slot -> limb index

    def with_dlists(self):
        return [l for l in self.limbs if l.dlist]


def _u32(d: bytes, o: int) -> int:
    return struct.unpack_from(">I", d, o)[0]


def parse(d: bytes, header_seg_addr: int) -> Skeleton:
    so = header_seg_addr & 0xFFFFFF
    limb_arr = _u32(d, so) & 0xFFFFFF
    limb_count = d[so + 4]
    dlist_count = d[so + 8] if so + 8 < len(d) else 0

    sk = Skeleton(limb_count=limb_count, dlist_count=dlist_count)
    for i in range(limb_count):
        lp = _u32(d, limb_arr + i * 4) & 0xFFFFFF
        x, y, z = struct.unpack_from(">hhh", d, lp)
        child = d[lp + 6]
        sib = d[lp + 7]
        dl = _u32(d, lp + 8)
        sk.limbs.append(Limb(i, x, y, z, child, sib, dl))

    _pose(sk, PLAYER_IDLE_POSE)
    return sk


def _rot_matrix(binang_xyz) -> np.ndarray:
    """R with columns rc0,rc1,rc2 = Matrix_TranslateRotateZYX rotation (ZYX)."""
    x, y, z = (v * _BINANG for v in binang_xyz)
    sx, cx = math.sin(x), math.cos(x)
    sy, cy = math.sin(y), math.cos(y)
    sz, cz = math.sin(z), math.cos(z)
    rc0 = (cz * cy, sz * cy, -sy)
    rc1 = (cz * sy * sx - sz * cx, sz * sy * sx + cz * cx, cy * sx)
    rc2 = (cz * sy * cx + sz * sx, sz * sy * cx - cz * sx, cy * cx)
    # columns are rc0,rc1,rc2
    return np.array([[rc0[0], rc1[0], rc2[0]],
                     [rc0[1], rc1[1], rc2[1]],
                     [rc0[2], rc1[2], rc2[2]]], np.float64)


def _pose(sk: Skeleton, joints: Optional[List[int]]):
    """Compute each limb's posed affine (SkelAnime_DrawFlex order).

    childM = parentM @ R(rot);  childT = parentM @ pos + parentT.
    Root uses jointTable[0] as translation; each limb's rotation = joints[limb+1].
    Falls back to translation-only bind pose when `joints` is None.
    """
    if not sk.limbs:
        return
    have = joints is not None and len(joints) >= (len(sk.limbs) + 1) * 3

    def rot_for(i):
        if not have:
            return (0, 0, 0)
        j = 3 * (i + 1)
        return (joints[j], joints[j + 1], joints[j + 2])

    sk.slot_to_limb = []

    def recurse(idx, pM, pT, visited):
        while idx != 0xFF and idx < len(sk.limbs) and idx not in visited:
            visited.add(idx)
            lb = sk.limbs[idx]
            if idx == 0 and have:
                pos = np.array([joints[0], joints[1], joints[2]], np.float64)
            else:
                pos = np.array([lb.x, lb.y, lb.z], np.float64)
            R = _rot_matrix(rot_for(idx))
            M = pM @ R
            T = pM @ pos + pT
            lb.M, lb.T = M, T
            lb.world = (float(T[0]), float(T[1]), float(T[2]))
            # each limb with a display list consumes the next seg-0x0D flex slot
            # (draw order, child before sibling) - mirrors SkelAnime_DrawFlex.
            if lb.dlist:
                sk.slot_to_limb.append(idx)
            if lb.child != 0xFF:
                recurse(lb.child, M, T, visited)
            idx = lb.sibling

    I = np.eye(3, dtype=np.float64)
    Z = np.zeros(3, np.float64)
    recurse(0, I, Z, set())


def find_skeleton_header(d: bytes) -> Optional[int]:
    """Locate the skeleton header via the MODLOADER64 block pointer, else scan."""
    m = d.find(b"MODLOADER64")
    if m >= 0 and m + 16 <= len(d):
        ptr = _u32(d, m + 12)
        if (ptr >> 24) == 0x06 and (ptr & 0xFFFFFF) < len(d):
            return ptr
    # Newer/output zzplayas zobjs leave the header pointer as 0xFFFFFFFF; scan
    # for a FlexSkeletonHeader instead: a seg-6 pointer to a run of `limbCount`
    # seg-6 limb pointers, with limbCount right after (SkeletonHeader layout).
    return _scan_skeleton_header(d)


def _scan_skeleton_header(d: bytes) -> Optional[int]:
    n = len(d)
    for x in range(0x8, n - 8, 4):
        p = _u32(d, x)
        if (p >> 24) != 0x06:
            continue
        arr = p & 0xFFFFFF
        limb_count = d[x + 4]
        if not (16 <= limb_count <= 24) or arr + limb_count * 4 > n:
            continue
        # every entry in the limb array must itself be a valid seg-6 pointer
        ok = True
        for i in range(limb_count):
            lp = _u32(d, arr + i * 4)
            if (lp >> 24) != 0x06 or (lp & 0xFFFFFF) + 12 > n:
                ok = False
                break
        if ok:
            return 0x06000000 | x
    return None
