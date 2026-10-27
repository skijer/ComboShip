#!/usr/bin/env python3
"""Prepare the private MM room's actual native shop actor/camera commands.

The numeric C++ scene contract is shared with runtime validation. Only the MM
scene's camera/cutscene commands and its keeper's actor ID/params may change.
The ordinary shared preparation and every OoT resource remain byte-exact.
"""
from pathlib import Path
import re
import struct

import forest

ROOT = Path(__file__).resolve().parents[2]
SCENE = 'scenes/nonmq/skiejer_market/skiejer_market'
ROOM = SCENE + '_room_0'


def contract():
    source = (ROOT / 'combo/BlackMarketMmShopScene.h').read_text()
    result = {}
    for name in ('KeeperActor', 'KeeperParams', 'ItemParamBase', 'CameraSetting',
                 'CutsceneCount', 'FirstCutscene', 'FirstPriority', 'HudVisibility', 'LetterboxSize'):
        match = re.search(r'inline constexpr int16_t k' + name + r' = (-?\d+);', source)
        if not match:
            raise ValueError('inspect changed native MM shop scene contract: ' + name)
        result[name] = int(match[1])
    records = re.search(r'kCameraRecords\[4\]\[9\] = \{(.*?)\};', source, re.S)
    cameras = re.search(r'kCutsceneCameras\[4\] = \{([^{}]*)\};', source)
    targets = re.search(r'kSelectedTargets\[2\]\[3\] = \{(.*?)\};', source, re.S)
    if not records or not cameras or not targets:
        raise ValueError('inspect changed native MM shop camera contract')
    result['CameraRecords'] = [tuple(map(int, re.findall(r'-?\d+', row)))
                               for row in re.findall(r'\{([^{}]*)\}', records[1])]
    result['CutsceneCameras'] = tuple(map(int, re.findall(r'-?\d+', cameras[1])))
    result['SelectedTargets'] = [tuple(map(float, re.findall(r'-?\d+(?:\.\d+)?', row)))
                                 for row in re.findall(r'\{([^{}]*)\}', targets[1])]
    if (result['CutsceneCount'] != 4 or result['FirstCutscene'] != 0 or
            len(result['CameraRecords']) != 4 or any(len(row) != 9 for row in result['CameraRecords']) or
            sorted(result['CutsceneCameras']) != [0, 1, 2, 3] or
            len(result['SelectedTargets']) != 2 or any(len(row) != 3 for row in result['SelectedTargets'])):
        raise ValueError('native MM shop requires four linked camera records')
    return result


def native_commands(data):
    """Read native MM scene commands, narrowly adding command 2 to forest's reader.

    SetCsCameraFactory reads u32 count and u16 setting/u16 Vec3s count, followed
    by three s16 per vector. SetActorCutsceneListFactory reads u32 count and
    16 bytes per entry. Do not add these MM-only commands to the shared parser.
    """
    reader, result = forest.Reader(data), {}
    for _ in range(reader.take('I')):
        identity, start = reader.take('I'), reader.offset
        if identity in result:
            raise ValueError('duplicate native MM shop scene command')
        if identity in (0, 1, 14):
            count = reader.take('I')
            reader.offset += count * 16
        elif identity == 2:
            for _ in range(reader.take('I')):
                _, count = reader.take('HH')
                reader.offset += count * 6
        elif identity in (3, 26):
            reader.text()
        elif identity == 4:
            for _ in range(reader.take('I')):
                reader.text()
                reader.take('II')
        elif identity == 6:
            count = reader.take('I')
            reader.offset += count * 2
        elif identity == 7:
            reader.take('bh')
        elif identity == 8:
            reader.take('6b')
        elif identity == 10:
            if reader.take('4B') != (0, 0, 1, 0):
                raise ValueError('unexpected native MM shop mesh command')
            reader.text()
            reader.text()
        elif identity in (11, 19):
            count = reader.take('I')
            reader.offset += count * 2
        elif identity == 15:
            count = reader.take('I')
            reader.offset += count * 22
        elif identity in (16, 21):
            reader.take('3B')
        elif identity == 17:
            reader.take('4B')
        elif identity == 18:
            reader.take('2B')
        elif identity in (20, 25):
            pass
        elif identity == 22:
            reader.take('B')
        elif identity == 24:
            for _ in range(reader.take('I')):
                reader.text()
        elif identity == 27:
            count = reader.take('I')
            reader.offset += count * 16
        elif identity == 28:
            count = reader.take('I')
            reader.take('H')
            reader.offset += count * 10
        elif identity == 31:
            for _ in range(reader.take('B')):
                reader.text()
                reader.take('HBB')
        else:
            raise ValueError(f'unsupported native MM shop scene command {identity:#x}')
        if reader.offset > len(data):
            raise ValueError('truncated native MM shop scene command')
        result[identity] = bytes(data[start:reader.offset])
    if reader.offset != len(data):
        raise ValueError('native MM shop scene did not consume EOF')
    return result


def _scene(data, commands):
    return data[:64] + struct.pack('<I', len(commands)) + b''.join(
        struct.pack('<I', identity) + payload for identity, payload in commands.items())


def apply(resources):
    result, values = dict(resources), contract()
    cameras = struct.pack('<I', 4) + b''.join(
        struct.pack('<HH9h', values['CameraSetting'], 3, *row) for row in values['CameraRecords'])
    cutscenes = struct.pack('<I', 4) + b''.join(
        struct.pack('<5h2Bh2B', values['FirstPriority'] + index, -1, camera, -1,
                    index + 1 if index < 3 else -1, 0, 255, values['HudVisibility'],
                    0, values['LetterboxSize']) for index, camera in enumerate(values['CutsceneCameras']))
    commands = native_commands(result[SCENE])
    if 20 not in commands or list(commands)[-1] != 20:
        raise ValueError('native MM shop scene must end with its native end command')
    if 2 in commands and commands[2] != cameras:
        raise ValueError('unexpected existing native MM shop camera records')
    if commands.get(27, struct.pack('<I', 0)) not in (struct.pack('<I', 0), cutscenes):
        raise ValueError('unexpected existing native MM shop cutscene records')
    encoded = {}
    for identity, payload in commands.items():
        if identity == 2:
            continue
        if identity == 27 or (identity == 20 and 27 not in commands):
            encoded[2], encoded[27] = cameras, cutscenes
        if identity != 27:
            encoded[identity] = payload
    result[SCENE] = _scene(result[SCENE], encoded)

    keepers = 0
    for path in (SCENE, ROOM):
        commands = native_commands(result[path])
        if 1 not in commands:
            continue
        actors = bytearray(commands[1])
        count, = struct.unpack_from('<I', actors)
        for index in range(count):
            offset = 4 + index * 16
            actor = struct.unpack_from('<H6hH', actors, offset)
            identity = actor[0] & 0x1FFF
            if identity not in (0x1C1, values['KeeperActor']):
                continue
            if actor[5] & 0x7F != values['FirstCutscene']:
                raise ValueError('native MM shop keeper must begin the private cutscene chain')
            if (identity == 0x1C1 and actor[7] != 0) or (
                    identity == values['KeeperActor'] and actor[7] != values['KeeperParams']):
                raise ValueError('unexpected private MM shop keeper params')
            struct.pack_into('<H', actors, offset, (actor[0] & 0xE000) | values['KeeperActor'])
            struct.pack_into('<H', actors, offset + 14, values['KeeperParams'])
            keepers += 1
        commands[1] = bytes(actors)
        result[path] = _scene(result[path], commands)
    if keepers != 1:
        raise ValueError('native MM shop requires exactly one private keeper actor')
    return result
