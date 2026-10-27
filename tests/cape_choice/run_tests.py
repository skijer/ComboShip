#!/usr/bin/env python3
"""Execute cape receipt, confirmation and icon drawing with actual engine/save headers."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
sys.path.insert(0, str(ROOT / 'tests/mm_presentation'))
from run_foreign_scale_tests import function


def block(source, marker):
    """Copy an actual production function or draw block, including its braces."""
    start = source.index(marker)
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


def icon_fixture(game):
    equipment = (ROOT / game / 'mods/extended_equipment.c').read_text()
    folder = ('mm/src/overlays/kaleido_scope/ovl_kaleido_scope' if game == 'mm'
              else 'soh/src/overlays/misc/ovl_kaleido_scope')
    pause = (ROOT / folder / 'z_kaleido_equipment.c').read_text()
    accessors = '\n'.join(block(equipment, signature) for signature in (
        'u8 ExtEquip_CapeOwned(', 'u8 ExtEquip_CapeVisible(', 'void ExtEquip_GivePendant(',
        'u8 ExtEquip_PendantOwned(', 'u8 ExtEquip_PendantActive('))
    accessors = (re.search(r'^#define TRADE_ADULT_PENDANT[^\n]*', equipment, re.M).group(0) +
                 '\n' + accessors)
    draw = function(pause, 'KaleidoScope_DrawEquipment')
    if game == 'mm':
        accessors += '\n' + block(pause, 'static s16 KaleidoEquip_UpgradeValue(')
        accessors += '\n' + block(pause, 'static void* KaleidoEquip_UpgradeIcon(')
        passive = block(draw, '        for (row = 0; row < 4; row++)')
        passive = 's16 row;\n' + passive
    else:
        # Keep the real row loop, ownership gate, texture dispatch and primitive-alpha reset.
        start = draw.index('    for (rowStart = 0, j = 0, temp = 0, i = 0;')
        passive = draw[start:draw.index('        // Draw inventory screen icons', start)] + '\n    }\n'
        passive = 's16 rowStart, j, temp, i;\n' + passive
    fixture = (ROOT / 'tests/cape_choice/native_icon_test.c').read_text()
    return fixture.replace('/* PRODUCTION_ACCESSORS */', accessors).replace('/* PRODUCTION_DRAW */', passive)


def flags(game):
    paths = (game, f'{game}/include', f'{game}/include/PR', f'{game}/src', f'{game}/assets',
             f'{game}/2s2h' if game == 'mm' else 'soh/soh', 'libultraship/include',
             'combo', 'combo/menu')
    return ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DCONTROLLERBUTTONS_T=uint32_t',
            '-DLOG_LEVEL_GAME_PRINTS=0', '-DNON_EQUIVALENT', '-DNON_MATCHING',
            *['-I' + str(ROOT / path) for path in paths]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--queued-fallback', type=Path, action='append', default=[])
    args = parser.parse_args()
    sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g', '-fno-pie', '-no-pie'] \
        if args.sanitize else []
    with tempfile.TemporaryDirectory(prefix='cape-choice-') as directory:
        build = Path(directory)
        source = (ROOT / 'mm/2s2h/CustomMessage/CustomMessage.cpp').read_text()
        names = ('CustomMessage::Replace', 'CustomMessage::ReplaceColorChars', 'CustomMessage::AddLineBreaks',
                 'CustomMessage::EnsureMessageEnd', 'CustomMessage::LoadCustomMessageIntoFont')
        loader = (ROOT / 'tests/cape_choice/loader_test.cpp').read_text()
        loader = loader.replace('/* PRODUCTION_FORMATTERS */', '\n'.join(function(source, name) for name in names))
        test = build / 'loader.cpp'
        test.write_text(loader)
        binary = build / 'loader'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags('mm'), *sanitize,
                        str(test), '-o', str(binary)], check=True, capture_output=True, text=True)
        subprocess.run([str(binary), *map(str, args.queued_fallback)], check=True)
        for game, path in (('mm', 'mm/src/code/z_message.c'), ('soh', 'soh/src/code/z_message_PAL.c')):
            engine = (ROOT / path).read_text()
            if 'Message_HandleCapeVisibilityChoice' in engine:
                production = ('static u8 sCapeVisibilityChoice;\n' +
                              function(engine, 'Message_SetCapeVisibilityChoice') + '\n' +
                              function(engine, 'Message_HandleCapeVisibilityChoice'))
            else:
                production = ('void Message_SetCapeVisibilityChoice(int enabled) { (void)enabled; }\n'
                              'static int Message_HandleCapeVisibilityChoice(PlayState* play) { (void)play; return 0; }')
            if game == 'soh':
                # Execute the actual registered message-state serializer, with
                # its original field declarations and the production macros.
                serializer = engine[engine.index('#define MESSAGE_PAL_SHIP_SAVESTATE_FIELDS'):]
                declarations = []
                for field in re.findall(r'F\((\w+)\)', serializer):
                    if field == 'sCapeVisibilityChoice':
                        continue
                    declarations.append(re.search(
                        r'^(?:static\s+)?(?:u8|s16|u16|u32)\s+' + field + r'\b[^;]*;',
                        engine, re.M).group(0))
                production += ('\n#include "soh/Enhancements/savestate_serialize.h"\n' +
                               '\n'.join(declarations) + '\n' + serializer)
            fixture = (ROOT / 'tests/cape_choice/native_choice_test.c').read_text()
            update_prefix = ''
            if game == 'mm':
                # Execute the actual pre-switch dispatch, including whether it calls
                # the Cape handler before reveal/intro modes process their input.
                update = function(engine, 'Message_Update')
                update_prefix = update[:update.index('    msgCtx->stickAdjX')]
                update_prefix = update_prefix.replace('Message_Update(', 'Message_UpdateCapePrefix(', 1) + '}\n'
            test = build / (game + '.c')
            test.write_text(fixture.replace('/* PRODUCTION_CHOICE */', production)
                            .replace('/* PRODUCTION_UPDATE_PREFIX */', update_prefix))
            binary = build / game
            extra = ['-DCAPE_TEST_MM'] if game == 'mm' else []
            subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', *flags(game), *extra, *sanitize,
                            str(test), '-o', str(binary)], check=True, capture_output=True, text=True)
            subprocess.run([str(binary)], check=True)
        icon_result = 0
        for game in ('mm', 'soh'):
            test = build / (game + '_icon.c')
            test.write_text(icon_fixture(game))
            binary = build / (game + '_icon')
            extra = ['-DCAPE_TEST_MM'] if game == 'mm' else []
            subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', *flags(game), *extra, *sanitize,
                            str(test), '-o', str(binary)], check=True, capture_output=True, text=True)
            icon_result |= subprocess.run([str(binary)]).returncode
        if icon_result:
            raise subprocess.CalledProcessError(icon_result, 'native cape icon tests')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        if error.stdout:
            print(error.stdout)
        if error.stderr:
            print(error.stderr)
        raise
