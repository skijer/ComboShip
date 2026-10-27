"""
Pure-Python (numpy + PIL) software rasterizer for a front-facing preview of a
posed Zelda64 player model. Deterministic, headless, no GPU.

Input: a list of world-space textured triangles (produced by posing the
skeleton limbs and extracting geometry with gbi.walk). Output: an RGBA PIL image.
"""
from __future__ import annotations
import numpy as np
from PIL import Image
from typing import Dict, List, Optional, Tuple

from . import n64texture
from .gbi import Triangle, TileState


# A decoded texture ready for sampling
class Tex:
    __slots__ = ("rgba", "w", "h", "cms", "cmt", "scale_s", "scale_t")

    def __init__(self, rgba, cms, cmt):
        self.rgba = rgba
        self.h, self.w = rgba.shape[:2]
        self.cms = cms
        self.cmt = cmt
        self.scale_s = self.scale_t = 1.0


def decode_tile(seg: Dict[int, bytes], ts: TileState) -> Optional[Tex]:
    if not ts.timg_addr or ts.width <= 0 or ts.height <= 0:
        return None
    s = (ts.timg_addr >> 24) & 0xFF
    o = ts.timg_addr & 0xFFFFFF
    buf = seg.get(s)
    if buf is None:
        return None
    bits = ts.width * ts.height * n64texture.SIZ_BITS.get(ts.siz, 16)
    data = buf[o:o + (bits + 7) // 8]
    tlut = b""
    if ts.fmt == n64texture.G_IM_FMT_CI and ts.tlut_addr:
        ts_s = (ts.tlut_addr >> 24) & 0xFF
        ts_o = ts.tlut_addr & 0xFFFFFF
        tb = seg.get(ts_s)
        if tb is not None:
            tlut = tb[ts_o:ts_o + max(ts.tlut_count, 16) * 2]
    try:
        rgba = n64texture.decode(data, ts.fmt, ts.siz, ts.width, ts.height, tlut)
    except Exception:
        return None
    return Tex(rgba, ts.cms, ts.cmt)


def _wrap(coord: np.ndarray, n: int, mode: int) -> np.ndarray:
    # mode bit0 = mirror, bit1 = clamp
    if mode & 0x2:                      # clamp
        return np.clip(coord, 0, n - 1)
    if mode & 0x1:                      # mirror
        period = 2 * n
        c = np.mod(coord, period)
        c = np.where(c >= n, period - 1 - c, c)
        return c
    return np.mod(coord, n)             # wrap


def _tri_alpha(ts):
    """Constant (non-texture) surface alpha for a tile: PRIM/ENV alpha when the
    blender is on (FORCE_BL) and the combiner routes that alpha out, else opaque.
    < 255 => the material is translucent (ice, glass, spirit)."""
    if not (ts.othermode_l[1] & 0x4000):        # FORCE_BL off -> opaque surface
        return 255
    if getattr(ts, "combine_alpha_uses_prim", False):
        return ts.prim[3]
    if getattr(ts, "combine_alpha_uses_env", False):
        return ts.env[3]
    return 255


def _sample_bilinear(tex, tu, tv, cms, cmt):
    """2x2 bilinear tap - the N64's default G_TF_BILERP filter. tu/tv are float
    texel coordinates. Small textures tiled with WRAP (skin/cloth details) turn
    into smooth shading instead of the harsh checkerboard nearest-sampling gives.
    Colour is filtered; alpha is taken nearest so 1-bit cutouts stay crisp."""
    fu = tu - 0.5                       # texel centers sit at .5
    fv = tv - 0.5
    u0 = np.floor(fu); v0 = np.floor(fv)
    du = (fu - u0)[..., None]; dv = (fv - v0)[..., None]
    u0i = _wrap(u0.astype(np.int64), tex.w, cms); u1i = _wrap((u0 + 1).astype(np.int64), tex.w, cms)
    v0i = _wrap(v0.astype(np.int64), tex.h, cmt); v1i = _wrap((v0 + 1).astype(np.int64), tex.h, cmt)
    t00 = tex.rgba[v0i, u0i].astype(np.float64); t01 = tex.rgba[v0i, u1i].astype(np.float64)
    t10 = tex.rgba[v1i, u0i].astype(np.float64); t11 = tex.rgba[v1i, u1i].astype(np.float64)
    top = t00 * (1 - du) + t01 * du
    bot = t10 * (1 - du) + t11 * du
    out = top * (1 - dv) + bot * dv
    # nearest alpha (crisp cutouts): pick the tap the coordinate rounds to
    un = _wrap(np.floor(tu).astype(np.int64), tex.w, cms)
    vn = _wrap(np.floor(tv).astype(np.int64), tex.h, cmt)
    out[..., 3] = tex.rgba[vn, un, 3]
    return out


def _shift(coord, shift: int):
    """RDP tile UV shift (SoH fast-interpreter parity)."""
    if shift == 0:
        return coord
    if shift <= 10:
        return coord / (1 << shift)
    return coord * (1 << (16 - shift))


def render(tris_world, seg: Dict[int, bytes], size: int = 512,
           bg=(0, 0, 0, 0), supersample: int = 2, axis="auto", cull: int = 0, view_bounds=None) -> Image.Image:
    """
    tris_world: list of (Triangle, [(wx,wy,wz)*3]) - triangle + its posed verts.
    Projects a front orthographic view and rasterizes with a z-buffer.
    """
    S = size * supersample
    if not tris_world:
        return Image.new("RGBA", (size, size), bg)

    # gather all world positions to fit the frame
    P = np.array([wv for item in tris_world for wv in item[1]], dtype=np.float64)
    # Posed OoT world space is Y-up and Link faces -Z. Front view: camera in front
    # (+Z) looking toward -Z, so screen right = +X, screen up = +Y, and nearer
    # (larger Z) wins the depth test.
    right = P[:, 0]
    up = P[:, 1]

    umin, umax = right.min(), right.max()
    vmin, vmax = up.min(), up.max()
    if view_bounds is not None:
        umin, vmin, umax, vmax = view_bounds
    span = max(umax - umin, vmax - vmin) or 1.0
    pad = 0.10 * span
    span += 2 * pad
    ucen = 0.5 * (umin + umax)
    vcen = 0.5 * (vmin + vmax)
    ppu = S / span

    def project(wvs):
        xs, ys, zs = [], [], []
        for (wx, wy, wz) in wvs:
            u = wx
            v = wy
            sx = S * 0.5 + (u - ucen) * ppu
            sy = S * 0.5 - (v - vcen) * ppu
            xs.append(sx); ys.append(sy); zs.append(wz)
        return xs, ys, zs

    img = np.zeros((S, S, 4), np.float64)
    img[..., 0] = bg[0]; img[..., 1] = bg[1]; img[..., 2] = bg[2]; img[..., 3] = bg[3]
    zbuf = np.full((S, S), -1e18)

    texcache: Dict[tuple, Optional[Tex]] = {}
    light = np.array([0.35, 0.85, 0.35]); light /= np.linalg.norm(light)

    # Draw opaque first, then translucent surfaces back-to-front (larger world-z is
    # nearer in this view) so alpha-blended materials (ice, glass) composite over what
    # sits behind them instead of overwriting it.
    def _draw_key(item):
        translucent = _tri_alpha(item[0].tile) < 250
        wv = item[1]
        zc = (wv[0][2] + wv[1][2] + wv[2][2]) / 3.0
        return (1 if translucent else 0, zc)      # opaque group first; translucent far->near
    draw_list = sorted(tris_world, key=_draw_key)

    for item in draw_list:
        tri, wvs = item[0], item[1]
        cols = item[2] if len(item) > 2 else None           # per-vertex (r,g,b) 0..1
        tex_override = item[3] if len(item) > 3 else None
        # backface cull: drop triangles whose world normal faces away from the
        # +Z camera (keeps the front shell, removes see-through interior faces).
        if cull:
            (ax, ay, az), (bx, by, bz), (cx, cy, cz) = wvs
            e1 = (bx - ax, by - ay, bz - az)
            e2 = (cx - ax, cy - ay, cz - az)
            nz = e1[0] * e2[1] - e1[1] * e2[0]
            if nz * cull < 0:
                continue
        xs, ys, zs = project(wvs)
        (x0, x1, x2) = xs; (y0, y1, y2) = ys; (z0, z1, z2) = zs
        area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
        if area == 0:
            continue
        # bounding box
        minx = max(int(np.floor(min(xs))), 0); maxx = min(int(np.ceil(max(xs))), S - 1)
        miny = max(int(np.floor(min(ys))), 0); maxy = min(int(np.ceil(max(ys))), S - 1)
        if minx > maxx or miny > maxy:
            continue
        gx, gy = np.meshgrid(np.arange(minx, maxx + 1), np.arange(miny, maxy + 1))
        gx = gx + 0.5; gy = gy + 0.5
        w0 = ((x1 - gx) * (y2 - gy) - (x2 - gx) * (y1 - gy)) / area
        w1 = ((x2 - gx) * (y0 - gy) - (x0 - gx) * (y2 - gy)) / area
        w2 = 1.0 - w0 - w1
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not inside.any():
            continue
        z = w0 * z0 + w1 * z1 + w2 * z2

        ts = tri.tile
        tex = None
        if tex_override is not None:
            tex = tex_override
        elif ts.timg_addr:
            k = ts.key()
            if k not in texcache:
                texcache[k] = decode_tile(seg, ts)
            tex = texcache[k]

        va, vb, vc = tri.v
        # per-pixel vertex colour (shade x combiner-tint), interpolated
        if cols is not None:
            (r0, g0, b0), (r1, g1, b1), (r2, g2, b2) = cols
            vcr = w0 * r0 + w1 * r1 + w2 * r2
            vcg = w0 * g0 + w1 * g1 + w2 * g2
            vcb = w0 * b0 + w1 * b1 + w2 * b2
        else:
            vcr = vcg = vcb = np.ones_like(w0)

        if tex is not None:
            su = w0 * va.s + w1 * vb.s + w2 * vc.s
            sv = w0 * va.t + w1 * vb.t + w2 * vc.t
            tu = _shift(su / 32.0, ts.shift_s) - ts.uls * 0.25
            tv = _shift(sv / 32.0, ts.shift_t) - ts.ult * 0.25
            tu *= tex.scale_s
            tv *= tex.scale_t
            # wrap mode from the render tile (clamp/mirror/wrap); tex.cms may be a
            # stale default, so trust the tile state that produced this triangle.
            if tex.w > 1 and tex.h > 1:
                texel = _sample_bilinear(tex, tu, tv, ts.cms, ts.cmt)   # N64 default filter
            else:
                un = _wrap(np.floor(tu).astype(np.int64), tex.w, ts.cms)
                vn = _wrap(np.floor(tv).astype(np.int64), tex.h, ts.cmt)
                texel = tex.rgba[vn, un].astype(np.float64)
            col = np.empty(gx.shape + (3,), np.float64)
            col[..., 0] = texel[..., 0] * vcr
            col[..., 1] = texel[..., 1] * vcg
            col[..., 2] = texel[..., 2] * vcb
            # Only let the texture's alpha drive transparency when the combiner's
            # alpha actually samples it; otherwise alpha is shade/prim (opaque), so a
            # 0-alpha texture must NOT make a solid surface (shoes/shirt) disappear.
            if getattr(ts, "combine_alpha_uses_tex", True):
                alpha = texel[..., 3].copy()
            else:
                alpha = np.full(gx.shape, 255.0)
        else:
            # no texture (shade/tint only, or unresolved shared texture): the
            # shaded+tinted vertex colour IS the surface (skin, tunic base, ...).
            col = np.stack([vcr, vcg, vcb], -1) * 255.0
            alpha = np.full(gx.shape, 255.0)

        # Fold in constant PRIM/ENV translucency (ice, glass) when the blender is on.
        alpha *= _tri_alpha(ts) / 255.0

        colshaded = np.clip(col, 0, 255)
        srcA = np.clip(alpha / 255.0, 0.0, 1.0)[..., None]

        zsub = zbuf[miny:maxy + 1, minx:maxx + 1]
        write = inside & (z > zsub) & (alpha > 8)
        sub = img[miny:maxy + 1, minx:maxx + 1]
        # alpha-blend source over destination (opaque srcA=1 => plain overwrite)
        blended = srcA * colshaded + (1.0 - srcA) * sub[..., :3]
        for c in range(3):
            sub[..., c] = np.where(write, blended[..., c], sub[..., c])
        sub[..., 3] = np.where(write, np.maximum(sub[..., 3], alpha), sub[..., 3])
        img[miny:maxy + 1, minx:maxx + 1] = sub
        # Only (near-)opaque pixels claim the depth buffer, so translucent surfaces
        # don't occlude what's drawn behind them afterward.
        opaque_px = write & (srcA[..., 0] > 0.96)
        zbuf[miny:maxy + 1, minx:maxx + 1] = np.where(opaque_px, z, zsub)

    out = Image.fromarray(img.astype(np.uint8), "RGBA")
    if supersample > 1:
        out = out.resize((size, size), Image.LANCZOS)
    return out
