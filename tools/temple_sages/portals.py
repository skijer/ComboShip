"""Five independently replaceable POC3 portal membranes, with owned samplers.

Each route has stable effect/material/texture_flow/texture_detail/geometry paths.
The ten texture entries initially contain identical grayscale art. Override any
canonical + Alt texture pair in a later .o2r without changing the executable.
The RGBA32 raw upload follows the proven native RAW1 replacement header, using a
512px source on two logical 32x32 RGBA16 tiles (exactly 4 KiB total TMEM).
"""
import math
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

from PIL import Image
import resource_geometry as G

NAMES = ('forest', 'fire', 'water', 'spirit', 'shadow')
SCROLLS = ((-1, 2), (2, -3))


def _texture():
    art = Image.open(Path(__file__).with_name('assets') / 'portal_flow.png')
    gray = art.convert('L').resize((512, 512), Image.Resampling.LANCZOS)
    # Vertex fade and native reward opacity own the default translucency.
    # Keep upload alpha full to avoid multiplying luminance into opacity too.
    # The material still samples texture alpha for later per-route overrides.
    pixels = Image.merge('RGBA', (gray, gray, gray, Image.new('L', gray.size, 255))).tobytes()
    return G.header(0x4f544558, 1) + struct.pack('<IIIIffI', 1, 512, 512, 1, 32., 16., len(pixels)) + pixels


def _row(x, y, z, alpha):
    # Full vertical interior has authored, nonconstant 10.5 fixed point UVs.
    return (x, y, z, 0, round((x + 51) / 102 * 1024), round((136 - y) / 120 * 1024),
            255, 255, 255, alpha)


def _geometry():
    faces = []
    for i in range(48):
        a = i * math.tau / 48
        b = (i + 1) * math.tau / 48
        def point(t, rx, ry, alpha):
            return _row(math.cos(t) * rx, 76 + math.sin(t) * ry, 0, alpha)
        # Preserve the original aperture outline, fading rim and ground sigil.
        p = [point(a, 42, 60, 240), point(a, 51, 70, 0),
             point(b, 51, 70, 0), point(b, 42, 60, 240)]
        faces.extend(((p[0], p[1], p[2]), (p[0], p[2], p[3])))
        p = [_row(math.cos(t) * r, 0, math.sin(t) * r, alpha)
             for t, r, alpha in ((a, 43, 170), (a, 60, 0), (b, 60, 0), (b, 43, 170))]
        faces.extend(((p[0], p[2], p[1]), (p[0], p[3], p[2])))
        # A visible membrane replaces the old nearly transparent center fan.
        # Its outer edge fades before the unchanged brighter aperture rim.
        p = [point(a, 0, 0, 245), point(a, 34, 48, 220), point(b, 34, 48, 220)]
        faces.append(tuple(p))
        p = [point(a, 34, 48, 220), point(a, 42, 60, 40),
             point(b, 42, 60, 40), point(b, 34, 48, 220)]
        faces.extend(((p[0], p[1], p[2]), (p[0], p[2], p[3])))
    for x, y, z in ((-37, 38, 4), (42, 68, -3), (-25, 127, -4),
                    (21, 145, 3), (-48, 93, 2), (38, 117, -2)):
        p = [_row(x-3, y, z, 0), _row(x, y-6, z, 190),
             _row(x+3, y, z, 0), _row(x, y+6, z, 190)]
        faces.extend(((p[0], p[1], p[2]), (p[0], p[2], p[3])))
    return faces


def _material(path):
    tree = ET.Element('DisplayList', Version='0')
    def add(tag, **attrs):
        return ET.SubElement(tree, tag, {k: str(v) for k, v in attrs.items()})
    add('PipeSync')
    add('Grayscale', Enabled='false')
    add('ClearGeometryMode', G_LIGHTING=1, G_FOG=1, G_CULL_BOTH=1, G_TEXTURE_GEN=1, G_TEXTURE_GEN_LINEAR=1)
    add('SetGeometryMode', G_ZBUFFER=1, G_SHADE=1, G_SHADING_SMOOTH=1)
    add('SetOtherMode', Cmd='G_SETOTHERMODE_H', Sft=4, Length=20,
        G_AD_NOISE=1, G_CD_MAGICSQ=1, G_CK_NONE=1, G_TC_FILT=1, G_TF_BILERP=1,
        G_TL_TILE=1, G_TD_CLAMP=1, G_TP_PERSP=1, G_CYC_2CYCLE=1, G_PM_NPRIMITIVE=1)
    add('SetOtherMode', Cmd='G_SETOTHERMODE_L', Sft=0, Length=32,
        G_AC_NONE=1, G_ZS_PIXEL=1, G_RM_PASS=1, G_RM_AA_ZB_XLU_SURF2=1)
    add('SetEnvColor', R=255, G=255, B=255, A=128)
    # RGB = average(flow, detail) * native destination tint. Alpha is flow alpha
    # * authored vertex fade * native reward opacity. No borrowed scene segment.
    add('SetCombineLERP', A0='G_CCMUX_TEXEL1', B0='G_CCMUX_TEXEL0', C0='G_CCMUX_ENV_ALPHA', D0='G_CCMUX_TEXEL0',
        Aa0='G_ACMUX_TEXEL0', Ab0='G_ACMUX_0', Ac0='G_ACMUX_SHADE', Ad0='G_ACMUX_0',
        A1='G_CCMUX_COMBINED', B1='G_CCMUX_0', C1='G_CCMUX_PRIMITIVE', D1='G_CCMUX_0',
        Aa1='G_ACMUX_COMBINED', Ab1='G_ACMUX_0', Ac1='G_ACMUX_PRIMITIVE', Ad1='G_ACMUX_0')
    add('Texture', S=65535, T=65535, Level=0, Tile=0, On=1)
    add('SetTextureLUT', Mode='G_TT_NONE')
    for tile, (suffix, speed) in enumerate(zip(('texture_flow', 'texture_detail'), SCROLLS)):
        add('TileSync')
        add('SetTextureImage', Path=path + suffix, Format='G_IM_FMT_RGBA', Size='G_IM_SIZ_16b_LOAD_BLOCK', Width=1)
        desc = dict(Format='G_IM_FMT_RGBA', Size='G_IM_SIZ_16b_LOAD_BLOCK', Line=0, TMem=tile*256,
                    Tile=7, Palette=0, Cms0='G_TX_WRAP', Cms1='G_TX_NOMIRROR',
                    Cmt0='G_TX_WRAP', Cmt1='G_TX_NOMIRROR', MaskS=5, MaskT=5, ShiftS=0, ShiftT=0)
        add('SetTile', **desc)
        add('LoadSync')
        add('LoadBlock', Tile=7, Uls=0, Ult=0, Lrs=1023, Dxt=256)
        add('PipeSync')
        desc.update(Size='G_IM_SIZ_16b', Line=8, Tile=tile,
                    ShiftS=15 if tile else 0, ShiftT=15 if tile else 0)
        add('SetTile', **desc)
        add('SetTileSize', T=tile, Uls=0, Ult=0, Lrs=124, Lrt=124)
        add('ScrollTexture', Tile=tile, SpeedX=speed[0], SpeedY=speed[1])
    add('EndDisplayList')
    return G.xml(tree)


def resources(namespace='custom/temple_sages/'):
    """Return a closed canonical + Alt set with one binding family per route."""
    result = {}
    texture = _texture()
    faces = _geometry()
    for name in NAMES:
        path = namespace + 'portals/' + name + '/'
        result.update(G.geometry(faces, path + 'geometry'))
        result[path + 'texture_flow'] = texture
        result[path + 'texture_detail'] = texture
        result[path + 'material'] = _material(path)
        tree = ET.Element('DisplayList', Version='0')
        ET.SubElement(tree, 'CallDisplayList', Path=path + 'material')
        ET.SubElement(tree, 'CallDisplayList', Path=path + 'geometry')
        ET.SubElement(tree, 'EndDisplayList')
        result[path + 'effect'] = G.xml(tree)
    return result | {'alt/' + name: data for name, data in result.items()}
