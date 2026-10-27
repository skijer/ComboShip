#!/usr/bin/env python3
"""Execute the native draw functions' actual goto topology with real room attachment.

The parser keeps every label/goto/return, hook call and private cleanup call in
place. Unrelated drawing statements are removed and retained branch predicates
become exhaustive binary inputs. This overapproximates the actual paths, avoiding
GPU fixtures while detecting any end-hook bypass that retains the native shape.
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from pycparser import c_ast, c_generator, c_parser

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def draw_flow(source, function, begin, end, bridge):
    body = block_from(source, source.index('void ' + function + '('))
    body = re.sub(r'/\*.*?\*/|//[^\n]*|^\s*#[^\n]*', '', body, flags=re.S | re.M)
    types = 'PlayState FrameTimingSpan GraphicsContext Lights Vec3f u32 u16 u8 s32 s16 f32 bool Gfx View Mtx'
    parsed = c_parser.CParser().parse('typedef int ' + ', '.join(types.split()) + ';\n' + body)
    draw = parsed.ext[-1]
    calls = {begin, end, bridge, "Camera_Update"}
    choices, gotos = [], []
    generator = c_generator.CGenerator()

    def call(name, arg):
        return c_ast.FuncCall(c_ast.ID(name), c_ast.ExprList([c_ast.Constant('int', str(arg))]))

    def reduce(node):
        if node is None:
            return None
        if isinstance(node, c_ast.Compound):
            children = [changed for child in node.block_items or [] if (changed := reduce(child)) is not None]
            return c_ast.Compound(children) if children else None
        if isinstance(node, c_ast.If):
            yes, no = reduce(node.iftrue), reduce(node.iffalse)
            if yes is None and no is None:
                return None
            if isinstance(node.cond, c_ast.Constant):
                cond = node.cond
            else:
                index = len(choices)
                choices.append(generator.visit(node.cond))
                cond = call('FlowChoice', index)
            return c_ast.If(cond, yes or c_ast.EmptyStatement(), no)
        if isinstance(node, c_ast.Label):
            return c_ast.Label(node.name, reduce(node.stmt) or c_ast.EmptyStatement())
        if isinstance(node, c_ast.Goto):
            index = len(gotos)
            gotos.append(node.name)
            return c_ast.Compound([call('FlowRecordGoto', index), node])
        if isinstance(node, c_ast.Return):
            return node
        if isinstance(node, c_ast.FuncCall):
            return node if isinstance(node.name, c_ast.ID) and node.name.name in calls else None
        if isinstance(node, (c_ast.Switch, c_ast.For, c_ast.While, c_ast.DoWhile)):
            # Fail explicitly if a new nested transfer/hook needs modeling.
            text = generator.visit(node)
            assert not re.search(r'\b(?:goto|return)\b|' + '|'.join(calls), text), text
        return None

    draw.body = reduce(draw.body)
    output = generator.visit(draw)
    output = re.sub(r'\b' + function + r'\b', 'RunNativeDrawExitFlow', output)
    output = re.sub(r'\bthis\b', 'play', output)
    assert len(choices) < 16, 'Keep exhaustive flow coverage bounded'
    output += f'\nstatic constexpr unsigned kChoiceCount = {len(choices)}, kGotoCount = {len(gotos)};\n'
    print(function + ' retained predicates: ' + repr(choices), flush=True)
    print(function + ' goto targets: ' + repr(gotos), flush=True)
    return output


with tempfile.TemporaryDirectory(prefix='room-magic-draw-exits-') as temporary:
    folder = Path(temporary)
    for host in ('mm', 'soh'):
        controller = (ROOT / host / ('2s2h' if host == 'mm' else 'soh') / 'Enhancements/Gameplay' /
                      ('BlackMarket.cpp' if host == 'mm' else 'ComboBlackMarketNative.cpp')).read_text()
        start = controller.index('BlackMarketRoomMagic::Resources sRoomMagicResources;')
        stop = controller.index('void RestoreRoomMagic(', start)
        production = controller[start:stop] + '\n'.join(block_from(controller, controller.index(name)) for name in
            ('void RestoreRoomMagic(', 'void ClearRoomMagic(', 'void BeginRoomMagic('))
        resources = (ROOT / 'combo/BlackMarketRoomMagicResources.h').read_text()
        palette = block_from(resources, resources.index('inline Palette ReadPalette('))
        support = (ROOT / 'tests/black_market/room_magic_native_test.cpp').read_text().split('int main() {')[0]
        support = support.replace('/* PALETTE */', palette).replace('/* PRODUCTION */', production)
        bridge = 'ComboBlackMarket_RestoreRoomAfterDraw' if host == 'mm' else 'ComboBlackMarketOot_RestoreRoomAfterDraw'
        signature = 'extern "C" void ' + bridge + '('
        # Before the fix, no bridge is needed to reproduce the failing flow.
        implementation = (block_from(controller, controller.index(signature)) if signature in controller else
                          'void ' + bridge + '(PlayState*) {}')
        implementation += '\n#define RestoreAfterDraw ' + bridge + '\n'
        begin = 'GameInteractor_ExecuteOnPlayDrawWorldStart' if host == 'mm' else 'GameInteractor_ExecuteOnPlayDrawBegin'
        end = 'GameInteractor_ExecuteOnPlayDrawWorldEnd' if host == 'mm' else 'GameInteractor_ExecuteOnPlayDrawEnd'
        draw = (ROOT / host / 'src/code/z_play.c').read_text()
        flow = draw_flow(draw, 'Play_DrawMain' if host == 'mm' else 'Play_Draw', begin, end, bridge)
        fixture = (ROOT / 'tests/black_market/room_magic_draw_exit_test.cpp').read_text()
        fixture = fixture.replace('/* SUPPORT */', support).replace('/* BRIDGE */', implementation)
        fixture = fixture.replace('/* HOOK_ALIASES */', '#define ' + begin + ' FlowBegin\n#define ' + end + ' FlowEnd\n#define Camera_Update(...) FlowTailUse()')
        fixture = fixture.replace('/* DRAW_FLOW */', flow)
        cpp = folder / (host + '.cpp'); cpp.write_text(fixture)
        includes = [host, host + '/include', host + '/include/PR', host + '/src', host + '/assets',
                    host + ('/2s2h' if host == 'mm' else '/soh'), 'libultraship/include', 'combo']
        binary = folder / host
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2',
                        *(['-DOOT_ROOM_MAGIC_TEST'] if host == 'soh' else []), '-g',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', '-I' + str(ROOT),
                        *['-I' + str(ROOT / p) for p in includes], str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
