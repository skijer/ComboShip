#!/usr/bin/env python3
"""Execute each engine's production color-control arm and real-header syntax."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, marker):
    begin = source.index('{', source.index(marker))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


def main():
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    with tempfile.TemporaryDirectory(prefix='receipt-soul-color-') as temporary:
        out = Path(temporary)
        for game in (sys.argv[1:] or ('mm', 'soh')):
            filename = 'mm/src/code/z_message_nes.c' if game == 'mm' else 'soh/src/code/z_message_PAL.c'
            source = (ROOT / filename).read_text()
            start = source.index('void Message_DrawTextNES' if game == 'mm' else 'void Message_DrawText(')
            begin = source.index('            case MESSAGE_COLOR_RED:' if game == 'mm' else '            case MESSAGE_COLOR:', start)
            end = source.index("            case ' ':", begin)
            arms = source[begin:end]
            declarations = ''
            if game == 'mm':
                for table in ('D_801D07DC', 'D_801D086C', 'sColorsBombersNotebookNES', 'sColorsNormalNES'):
                    declarations += 'static Color_RGB16 ' + table + '[] = ' + block(source, table + '[]') + ';\n'
            declarations += 'static void RenderColor(PlayState* play,u16 character) {\n'
            declarations += 'MessageContext* msgCtx=&play->msgCtx; u16 i=0;\nswitch(character) {\n' + arms + '\n}\n}\n'
            fixture = (ROOT / 'tests/item_receipts/soul_color_test.c').read_text()
            fixture = fixture.replace('/* PRODUCTION_COLOR_ARMS */', declarations)
            test = out / (game + '_color.c')
            test.write_text(fixture)
            flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=0',
                     '-DNON_EQUIVALENT', '-DNON_MATCHING', '-Wno-incompatible-pointer-types', '-Wno-int-conversion']
            includes = (f'{game}/include', f'{game}/include/PR', f'{game}/src', game, f'{game}/assets',
                        f'{game}/2s2h' if game == 'mm' else 'soh/soh', 'libultraship/include', 'libultraship/src', 'combo', 'combo/menu')
            flags += ['-I' + str(ROOT / path) for path in includes]
            flags += ['-DMM_BUILD_DLL', '-DRECEIPT_MM'] if game == 'mm' else ['-DSOH_BUILD_DLL']
            flags += ['-DCVAR_PREFIX_' + prefix + '="' + value + '"' for prefix, value in
                      (('COSMETIC', 'gCosmetics'), ('ENHANCEMENT', 'gEnhancements'), ('SETTING', 'gSettings'),
                       ('REMOTE', 'gRemote'), ('RANDOMIZER_SETTING', 'gRandomizerSettings'),
                       ('CHEAT', 'gCheats'), ('DEVELOPER_TOOLS', 'gDeveloperTools'))]
            binary = out / (game + '_color')
            subprocess.run([*compiler, '-std=gnu11', '-Wall', '-Wextra', '-Werror=implicit-function-declaration',
                            *flags, str(test), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
            subprocess.run([*compiler, '-std=gnu11', *flags, '-Werror=implicit-function-declaration',
                            '-fsyntax-only', str(ROOT / filename)], check=True)
            print('PASS real-header textbox syntax:', filename)


if __name__ == '__main__':
    main()
