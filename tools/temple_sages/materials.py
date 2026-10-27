"""Private Temple materials using the unchanged, proven Black Market flow art.

All resources are supplied under both canonical and Alt paths. In particular,
canonical texture names must exist before a binary hash can reach Alt loading.
The XML materials use filepath references and own their complete draw state.
"""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET


THEMES = {
    'forest': (105, 255, 145), 'fire': (255, 105, 40),
    'water': (90, 180, 255), 'spirit': (255, 195, 85),
    'shadow': (190, 105, 255), 'light': (255, 242, 180),
}


def _texture():
    # This standard-library-only decoder verifies the existing bounded encoded
    # source parts and exact opaque pixels. Do not re-encode or resample the art.
    directory = str(Path(__file__).resolve().parents[1] / 'black_market')
    sys.path.insert(0, directory)
    try:
        import market_interior
        return market_interior.texture('arcane_stone', 64)
    finally:
        sys.path.remove(directory)


def _material(texture, tint, scroll):
    root = ET.Element('DisplayList', Version='0')
    ET.SubElement(root, 'PipeSync')
    ET.SubElement(root, 'Grayscale', Enabled='false')
    ET.SubElement(root, 'ClearGeometryMode', G_LIGHTING='1', G_FOG='1',
                  G_CULL_BOTH='1', G_TEXTURE_GEN='1', G_TEXTURE_GEN_LINEAR='1')
    ET.SubElement(root, 'SetGeometryMode', G_ZBUFFER='1', G_SHADE='1',
                  G_CULL_BACK='1', G_SHADING_SMOOTH='1')
    ET.SubElement(root, 'SetOtherMode', Cmd='G_SETOTHERMODE_H', Sft='4', Length='20',
                  G_AD_NOISE='1', G_CD_MAGICSQ='1', G_CK_NONE='1', G_TC_FILT='1',
                  G_TF_BILERP='1', G_TL_TILE='1', G_TD_CLAMP='1', G_TP_PERSP='1',
                  G_CYC_2CYCLE='1', G_PM_NPRIMITIVE='1')
    ET.SubElement(root, 'SetOtherMode', Cmd='G_SETOTHERMODE_L', Sft='0', Length='32',
                  G_AC_NONE='1', G_ZS_PIXEL='1', G_RM_PASS='1', G_RM_AA_ZB_OPA_SURF2='1')
    # Black Market shelves' MODULATERGB / MODULATERGB_PRIM2: keep authored
    # vertex shade on slab sides, then multiply the owned elemental tint.
    ET.SubElement(root, 'SetCombineLERP',
                  A0='G_CCMUX_TEXEL0', B0='G_CCMUX_0', C0='G_CCMUX_SHADE', D0='G_CCMUX_0',
                  Aa0='G_ACMUX_0', Ab0='G_ACMUX_0', Ac0='G_ACMUX_0', Ad0='G_ACMUX_1',
                  A1='G_CCMUX_COMBINED', B1='G_CCMUX_0', C1='G_CCMUX_PRIMITIVE', D1='G_CCMUX_0',
                  Aa1='G_ACMUX_0', Ab1='G_ACMUX_0', Ac1='G_ACMUX_0', Ad1='G_ACMUX_1')
    ET.SubElement(root, 'SetPrimColor', M='0', L='0',
                  R=str(tint[0]), G=str(tint[1]), B=str(tint[2]), A='255')
    ET.SubElement(root, 'SetTextureLUT', Mode='G_TT_NONE')
    ET.SubElement(root, 'Texture', S='65535', T='65535', Level='0', Tile='0', On='1')
    # Logical RGBA16/64x64 sampler with the exact corrected raw RGBA8 upload
    # used by BlackMarketShelvesNative and room_magic. The XML loader expands
    # this into SETTIMG, load/render tiles, LoadBlock and SetTileSize.
    ET.SubElement(root, 'LoadTextureBlock', Path=texture, Format='0', Size='2',
                  Width='64', Height='64', MaskS='6', MaskT='6', ShiftS='0', ShiftT='0',
                  CMS_TXWrap='1', CMT_TXWrap='1')
    ET.SubElement(root, 'ScrollTexture', Tile='0', SpeedX=str(scroll[0]), SpeedY=str(scroll[1]))
    ET.SubElement(root, 'EndDisplayList')
    ET.indent(root, space='  ')
    return ET.tostring(root, encoding='utf-8') + b'\n'


def bridge_resources(namespace='custom/temple_sages/'):
    """Return the closed canonical/Alt texture and seven opaque material lists.

    Materials are ``bridge_material_<theme>`` for all six medallion names and
    ``shell_material``. The shared texture is ``texture/arcane_stone``. This
    helper does not change donor archives or any Black Market resources.
    """
    texture = namespace + 'texture/arcane_stone'
    resources = {texture: _texture()}
    for name, tint in THEMES.items():
        resources[namespace + 'bridge_material_' + name] = _material(texture, tint, (-1, 1))
    # A restrained opaque enclosure; ceiling geometry may shade this further.
    resources[namespace + 'shell_material'] = _material(texture, (24, 26, 42), (0, 0))
    resources.update({'alt/' + name: data for name, data in list(resources.items())})
    return resources
