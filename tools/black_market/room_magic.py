#!/usr/bin/env python3
"""Optional original room materials; preserve the final per-host mesh verbatim.

Room and shelf art use the same neutral luminance from the verified original
bounded zlib/base64 source parts. The room retains its own runtime texture and
material paths so overrides and cosmetic colors remain separate.
"""
import hashlib
import re
import xml.etree.ElementTree as ET

import market_interior
import mm_shop_presentation

PRIVATE = 'custom/skiejer_market/'
PREFIX = PRIVATE + 'room_magic_v1/'
ROOM = PREFIX + 'room'
TEXTURE = PREFIX + 'texture/arcane_stone'
PALETTE = ((165,148,230), (135,194,210), (204,170,215), (175,160,216))
PIXELS_SHA256 = '3e2bac601d1a9c3e0317d159aa6b9f5354abaac982a24848edfec5e97dbb3231'
MATERIAL = re.compile(r'geometry/mat_mahouya_room_0DL_006548_GranmaShop(\d\d)$')


def texture():
    pixels = market_interior.shelf_pixels()
    if hashlib.sha256(pixels).hexdigest() != PIXELS_SHA256:
        raise ValueError('Original magical room pixel identity changed')
    # Existing primitive palette controls now supply hue without baked chroma.
    return market_interior.grayscale_magic_texture()


def _material(tree, index):
    required = ('SetTextureImage', 'SetTextureLUT', 'LoadBlock', 'SetTileSize', 'SetPrimColor')
    if any(len(tree.findall(tag)) != 1 for tag in required) or len(tree.findall('SetTile')) != 2:
        raise ValueError('Unexpected magical room material framing')
    tree.find('SetTextureImage').set('Path', TEXTURE)
    tree.find('SetTextureLUT').set('Mode', 'G_TT_NONE')
    for tile in tree.findall('SetTile'):
        tile.set('Line', '16' if tile.get('Tile') == '0' else '0')
        for axis in ('S', 'T'):
            tile.set('Mask' + axis, '6')
        tile.set('Cms0', 'G_TX_WRAP')
        tile.set('Cmt0', 'G_TX_WRAP')
    block = tree.find('LoadBlock')
    block.set('Lrs', '4095')
    block.set('Dxt', '128')
    size = tree.find('SetTileSize')
    size.set('Lrs', '252')
    size.set('Lrt', '252')
    for channel, value in zip('RGB', PALETTE[index % 4]):
        tree.find('SetPrimColor').set(channel, str(value))
    for node in list(tree):
        if node.tag == 'ScrollTexture':
            tree.remove(node)
        node.attrib.pop('CosmeticEntry', None)
        node.attrib.pop('CosmeticCategory', None)
    tree.insert(list(tree).index(size) + 1, ET.Element('ScrollTexture', {
        'Tile': '0', 'SpeedX': str(-1 if index % 3 == 0 else 1),
        'SpeedY': str(0 if index % 2 else 1)}))


def apply(resources, host):
    if host not in ('oot', 'mm'):
        raise ValueError('Magical room host must be oot or mm')
    source_root = (mm_shop_presentation.CLONES[mm_shop_presentation.ROOM_ROOT]
                   if host == 'mm' else PRIVATE + 'room')
    trees, visiting = {}, set()

    def visit(path):
        if path in visiting:
            raise ValueError('Recursive magical room display-list graph')
        if path in trees:
            return
        if not path.startswith(PRIVATE) or path not in resources:
            raise ValueError('Missing or external magical room display list: ' + path)
        tree = ET.fromstring(resources[path])
        if tree.tag != 'DisplayList':
            raise ValueError('Magical room graph contains a non-display-list resource')
        visiting.add(path)
        for node in tree:
            if node.tag in ('CallDisplayList', 'JumpToDisplayList'):
                visit(node.get('Path', ''))
        visiting.remove(path)
        trees[path] = tree

    visit(source_root)
    # Material 17 is retained but has no active draw owner. Keep the complete
    # original 28-slot palette contract for host resource validation.
    for index in range(28):
        visit(PRIVATE + f'geometry/mat_mahouya_room_0DL_006548_GranmaShop{index:02}')
    def clone_path(path):
        relative = path.removeprefix(PRIVATE).removeprefix('mm_shop_presentation_v1/')
        return PREFIX + relative
    clones = {path: clone_path(path) for path in trees}
    if len(set(clones.values())) != len(clones):
        raise ValueError('Ambiguous magical room display-list owners')
    result = dict(resources)
    result[TEXTURE] = texture()
    for path, tree in trees.items():
        match = MATERIAL.search(path)
        if match:
            _material(tree, int(match[1]))
        for node in tree:
            if node.tag in ('CallDisplayList', 'JumpToDisplayList'):
                node.set('Path', clones[node.get('Path')])
        ET.indent(tree, space='  ')
        result[clones[path]] = ET.tostring(tree, encoding='utf-8') + b'\n'
    return result
