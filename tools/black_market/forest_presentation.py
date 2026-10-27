"""Bounded, deterministic polish over the preserved native forest source.

The immutable split ZIP remains the recovery baseline. Build preparation checks
its complete inventory first, applies this recipe, and checks the separately
pinned result before writing either port's assets. No collision, texture or geometry changes; only the ramp draw owner moves to XLU.
"""
import struct
import xml.etree.ElementTree as ET

import native_forest as native

CORE = native.PRIVATE
ART = native.AUTHORED
PURPLE = 0x581880


def ramp_material():
    root = ET.Element('DisplayList', Version='0')
    ET.SubElement(root, 'PipeSync')
    # Room culling changes draw order. Own every relevant pipeline setting,
    # especially one-cycle/tile LOD: a second inherited tile caused the old
    # log to sample unrelated room textures from some camera angles.
    ET.SubElement(root, 'SetOtherMode', Cmd='G_SETOTHERMODE_H', Sft='4', Length='20',
                  G_AD_NOISE='1', G_CD_MAGICSQ='1', G_CK_NONE='1', G_TC_FILT='1',
                  G_TF_BILERP='1', G_TL_TILE='1', G_TD_CLAMP='1', G_TP_PERSP='1',
                  G_CYC_1CYCLE='1', G_PM_NPRIMITIVE='1')
    ET.SubElement(root, 'SetOtherMode', Cmd='G_SETOTHERMODE_L', Sft='0', Length='32',
                  G_AC_NONE='1', G_ZS_PIXEL='1', G_RM_AA_ZB_XLU_SURF='1', G_RM_AA_ZB_XLU_SURF2='1')
    ET.SubElement(root, 'ClearGeometryMode', G_LIGHTING='1', G_CULL_BOTH='1',
                  G_TEXTURE_GEN='1', G_TEXTURE_GEN_LINEAR='1', G_FOG='1')
    ET.SubElement(root, 'SetGeometryMode', G_ZBUFFER='1', G_SHADE='1', G_CULL_BACK='1', G_SHADING_SMOOTH='1')
    # Reuse the wall/bench track's scrolling second layer. Tile zero in that
    # track is stationary and 32x32; the authored mask is 32x64 on tile one.
    ET.SubElement(root, 'Texture', S='65535', T='65535', Level='0', Tile='1', On='1')
    ET.SubElement(root, 'SetTextureLUT', Mode='G_TT_NONE')
    ET.SubElement(root, 'SetPrimColor', M='0', L='0', R='184', G='112', B='248', A='112')
    ET.SubElement(root, 'SetEnvColor', R='88', G='24', B='128', A='255')
    combine = {}
    for cycle in ('0', '1'):
        for key, value in (('A', 'G_CCMUX_PRIMITIVE'), ('B', 'G_CCMUX_ENVIRONMENT'),
                           ('C', 'G_CCMUX_TEXEL0'), ('D', 'G_CCMUX_ENVIRONMENT'),
                           ('Aa', 'G_ACMUX_0'), ('Ab', 'G_ACMUX_0'), ('Ac', 'G_ACMUX_0'),
                           ('Ad', 'G_ACMUX_PRIMITIVE')):
            combine[key + cycle] = value
    ET.SubElement(root, 'SetCombineLERP', combine)
    ET.SubElement(root, 'LoadTextureBlock', Path=ART + 'prelude/black_market_wall_magic_mask',
                  Format='4', Size='1', Width='32', Height='64', MaskS='5', MaskT='6',
                  ShiftS='0', ShiftT='0', CMS_TXWrap='1', CMT_TXWrap='1')
    ET.SubElement(root, 'SetTile', Size='G_IM_SIZ_8b', Line='4', Tile='1', Format='G_IM_FMT_I',
                  TMem='0', Palette='0', Cms0='G_TX_WRAP', Cms1='G_TX_NOMIRROR',
                  Cmt0='G_TX_WRAP', Cmt1='G_TX_NOMIRROR', MaskS='5', MaskT='6', ShiftS='0', ShiftT='0')
    ET.SubElement(root, 'CallDisplayList', Path='>0x08000000')
    ET.SubElement(root, 'EndDisplayList')
    return native._xml(root)


def route_ramp_xlu(resources):
    """Move only the retained ramp entry to each host's native XLU stream."""
    changed = {}
    root = CORE + 'mirror_access_root'
    for mode, prefix in native.SCENES.items():
        path = prefix + '_room_0'
        commands = native.read_scene(resources[path], oot=mode == 'oot')
        mesh = native.read_mesh(commands[10])
        if mesh['type'] != 2:
            raise ValueError('Translucent Mirror ramp requires type-two room culling')
        entries = [entry for entry in mesh['entries'] if root in (entry['opaque'], entry['translucent'])]
        if len(entries) != 1:
            raise ValueError('Translucent Mirror ramp requires exactly one draw owner')
        entry = entries[0]
        if (entry['opaque'], entry['translucent']) not in ((root, ''), ('', root)):
            raise ValueError('Mirror ramp shares an unexpected room draw entry')
        entry.update(opaque='', translucent=root)
        commands[10] = native.write_mesh(mesh)
        changed[path] = native.write_scene(commands)
    return changed


def ramp_root(resources):
    """Preserve all retained ramp geometry and restore the XLU stream state."""
    ramp = ET.fromstring(resources[CORE + 'mirror_access_root'])
    if ramp[-1].tag != 'EndDisplayList':
        raise ValueError('Unexpected Mirror ramp draw terminator')
    ramp.remove(ramp[-1])
    # Draw ownership and render mode are distinct: native room SETUPDL_25
    # uses this two-cycle/fog OPA2 baseline even for its XLU output stream.
    # Restore that baseline only after the last ramp triangle has blended.
    ET.SubElement(ramp, 'PipeSync')
    ET.SubElement(ramp, 'SetOtherMode', Cmd='G_SETOTHERMODE_H', Sft='4', Length='20',
                  G_AD_PATTERN='1', G_CD_MAGICSQ='1', G_CK_NONE='1', G_TC_FILT='1',
                  G_TF_BILERP='1', G_TL_TILE='1', G_TD_CLAMP='1', G_TP_PERSP='1',
                  G_CYC_2CYCLE='1', G_PM_NPRIMITIVE='1')
    ET.SubElement(ramp, 'SetOtherMode', Cmd='G_SETOTHERMODE_L', Sft='0', Length='32',
                  G_AC_NONE='1', G_ZS_PIXEL='1', G_RM_FOG_SHADE_A='1', G_RM_AA_ZB_OPA_SURF2='1')
    ET.SubElement(ramp, 'SetTextureLUT', Mode='G_TT_NONE')
    ET.SubElement(ramp, 'Texture', S='65535', T='65535', Level='0', Tile='0', On='1')
    # SETUPDL_25's MODULATEIDECALA / MODULATEIA_PRIM2 combiner.
    ET.SubElement(ramp, 'SetCombineLERP',
                  A0='G_CCMUX_TEXEL0', B0='G_CCMUX_0', C0='G_CCMUX_SHADE', D0='G_CCMUX_0',
                  Aa0='G_ACMUX_0', Ab0='G_ACMUX_0', Ac0='G_ACMUX_0', Ad0='G_ACMUX_TEXEL0',
                  A1='G_CCMUX_COMBINED', B1='G_CCMUX_0', C1='G_CCMUX_PRIMITIVE', D1='G_CCMUX_0',
                  Aa1='G_ACMUX_COMBINED', Ab1='G_ACMUX_0', Ac1='G_ACMUX_PRIMITIVE', Ad1='G_ACMUX_0')
    ET.SubElement(ramp, 'SetGeometryMode', G_ZBUFFER='1', G_SHADE='1', G_CULL_BACK='1',
                  G_FOG='1', G_LIGHTING='1', G_SHADING_SMOOTH='1')
    ET.SubElement(ramp, 'EndDisplayList')
    return native._xml(ramp)


def apply(resources):
    """Return a new mapping; reject unexpected baselines before any mutation."""
    result = dict(resources)
    mouth = ART + 'room_0/ReturnTunnelMouth'
    data = bytearray(resources[mouth])
    prims = [offset for offset, _, word, op in native.display_commands(data, mouth) if op == 0xFA]
    if len(prims) != 1 or struct.unpack_from('<I', data, prims[0] + 4)[0] != 0x44FF7460:
        raise ValueError('Forest polish requires the preserved green portal baseline')
    struct.pack_into('<I', data, prims[0] + 4, (PURPLE << 8) | 96)
    result[mouth] = bytes(data)

    glow_root = CORE + 'portal_xlu'
    calls = ET.fromstring(resources[glow_root])
    if [node.get('Path') for node in calls if node.get('Path')] != [mouth, ART + 'room_0/ReturnTunnelPinkLight']:
        raise ValueError('Unexpected forest portal glow ownership')
    # The old vertex-colored rim bloom is the detached purple spot on the
    # left bark. Keep its source available, remove its active draw call.
    result[glow_root] = native._calls([mouth])

    signs = CORE + 'building/recipe7624751371212635_vtx'
    vertices = bytearray(resources[signs])
    if vertices[4:8] != b'RRAO' or struct.unpack_from('<I', vertices, 68)[0] != 52:
        raise ValueError('Unexpected hanging-sign vertex array')
    # First two atlas cells: front sign currently moon/key, rear Black Market.
    # Move only S; all positions, T, vertex colors and the four avatar vertices
    # at indices 48..51 remain byte-identical.
    for index in range(48):
        offset = 72 + index * 16
        z, = struct.unpack_from('<h', vertices, offset + 4)
        s, = struct.unpack_from('<h', vertices, offset + 8)
        if z > 480 and 6 <= s <= 375:
            s += 379
        elif z < 0 and 385 <= s <= 754:
            s -= 379
        else:
            raise ValueError('Unexpected front/rear hanging-sign atlas coordinates')
        struct.pack_into('<h', vertices, offset + 8, s)
    result[signs] = bytes(vertices)
    result[CORE + 'mirror_access_material'] = ramp_material()
    result[CORE + 'mirror_access_root'] = ramp_root(resources)
    result.update(route_ramp_xlu(resources))
    return result
