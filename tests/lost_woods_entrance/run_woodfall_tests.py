#!/usr/bin/env python3
"""Exercise Woodfall through the production paired boots and handoff hooks."""
from pathlib import Path
import hashlib
import os
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

oot = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8-sig')
mm = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()

def function(source, signature):
    return block_from(source, source.index(signature))

def fixture(name):
    return (ROOT / 'tests/lost_woods_entrance' / name).read_text().split('int main()')[0]

def build(host, source, name):
    if '--geometry-only' in sys.argv and name != 'geometry':
        return
    with tempfile.TemporaryDirectory(prefix='woodfall-' + name + '-') as directory:
        cpp, binary = Path(directory) / 'test.cpp', Path(directory) / 'test'
        cpp.write_text('#include "ComboGameMode.h"\nextern "C" { int gComboGameMode = COMBO_GAME_BOTH; }\n' + source)
        paths = [host, host + ('/2s2h' if host == 'mm' else '/soh'), host + '/include', host + '/include/PR',
                 host + '/src', host + '/assets', 'libultraship/include', 'combo']
        command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD',
                   '-DFMT_HEADER_ONLY', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                   *['-I' + str(ROOT / path) for path in paths]]
        if '--sanitize' in sys.argv:
            command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
        result = subprocess.run([*command, str(cpp), '-o', str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(result.stderr[:6500])
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})

source = fixture('mm_arrival_test.cpp')
source = source.replace('/* ARRIVAL_BODY */', function(mm, 'extern "C" void MM_ApplyComboPortalArrival('))
source = source.replace('/* ROUTE_BODY */', '\n'.join(function(mm, signature) for signature in (
    'static void Combo_CancelLostWoodsPassage(', 'static void Combo_ClearReturnRequests(',
    'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(', 'extern "C" COMBO_EXPORT void MM_SetComboPortalRoute(')))
source = source.replace('/* BOOT_BODY */', re.sub(r'\bthis\b', 'self', function(
    (ROOT / 'mm/src/code/title_setup.c').read_text(), 'void Setup_InitImpl(')))
source += r'''
int main() {
    SetupState state{};
    for (int slot = 0; slot < 3; ++slot) {
        gComboStartFileNum = slot;
        MM_SetComboEntryIsResume(0); MM_SetComboPortalRoute(4);
        Setup_InitImpl(&state);
        require(gSaveContext.save.entrance == ENTRANCE(WOODFALL, 0),
                "room 7 must reach the native Woodfall approach entrance after save loading");
        require(!sComboPortalArrivalPending && gSaveContext.respawnFlag == 0,
                "Woodfall arrival is one-shot and uses its native scene spawn");
        require(gSaveContext.fileNum == slot && gSaveContext.save.day == 2 && gSaveContext.save.time == 0x8000 &&
                gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 && queuedCameraCount == 0,
                "Woodfall must preserve paired slot, cycle, Farore and existing forest cameras");
    }
    for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
        gSaveContext.save.playerForm = form;
        const auto progress = gSaveContext.save.saveInfo;
        MM_SetComboPortalRoute(4); MM_ApplyComboPortalArrival();
        require(gSaveContext.save.playerForm == form &&
                !std::memcmp(&progress, &gSaveContext.save.saveInfo, sizeof(progress)),
                "Woodfall travel must not activate an owl, raise the temple, grant items or change forms");
    }
    for (int priority = 0; priority < 3; ++priority) {
        MM_SetComboEntryIsResume(0); MM_SetComboPortalRoute(4);
        if (priority == 0) crossArrival = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        if (priority == 1) bossRecovery = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        if (priority == 2) fwArrival = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        gComboStartFileNum = 0; Setup_InitImpl(&state);
        require(gSaveContext.save.entrance == ENTRANCE(SNOWHEAD_TEMPLE, 0) && !sComboPortalArrivalPending,
                "shuffled dungeon, boss recovery and Farore arrivals must supersede Woodfall");
    }
    MM_SetComboEntryIsResume(0); MM_SetComboPortalRoute(4);
    MM_SetComboEntryIsResume(1); sRememberLocation = true; gComboStartFileNum = 0; Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 3),
            "ordinary save resume must discard a stale Woodfall request");
    puts("PASS Woodfall MM boot: all slots/forms, progress preservation and explicit-arrival priority");
}
'''
build('mm', source, 'mm-boot')

source = fixture('oot_arrival_test.cpp')
source = source.replace('/* ARRIVAL_BODY */', function(oot, 'extern "C" void SOH_ApplyComboPortalArrival('))
source = source.replace('/* BOOT_BODY */', function((ROOT / 'soh/src/code/title_setup.c').read_text(), 'void TitleSetup_InitImpl('))
source += r'''
int main() {
    GameState state{};
    for (int slot = 0; slot < 3; ++slot) {
        gComboReturnFileNum = slot; sComboReturnPortalRoute = 4; TitleSetup_InitImpl(&state);
        const auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
        require(gSaveContext.entranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT && arrival.roomIndex == 7 &&
                arrival.pos.x == 1780 && arrival.pos.z == -2400 && arrival.yaw == -16384,
                "Woodfall passage must return outside room 7's east tunnel, facing into the room");
        require(gSaveContext.respawnFlag == 2 && sComboReturnPortalRoute == 0 &&
                gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 && gSaveContext.health == 0x50,
                "Woodfall return must consume its request and preserve paired progress");
    }
    for (int priority = 0; priority < 3; ++priority) {
        sComboReturnPortalRoute = 4; gComboReturnFileNum = 0;
        if (priority == 0) crossArrival = ENTR_DEKU_TREE_ENTRANCE;
        if (priority == 1) bossRecovery = ENTR_DEKU_TREE_ENTRANCE;
        if (priority == 2) fwArrival = ENTR_DEKU_TREE_ENTRANCE;
        TitleSetup_InitImpl(&state);
        require(gSaveContext.entranceIndex == ENTR_DEKU_TREE_ENTRANCE && sComboReturnPortalRoute == 0,
                "explicit OoT arrivals must consume and supersede Woodfall return");
    }
    puts("PASS Woodfall OoT boot: fixed room 7 endpoint, all slots and explicit-arrival priority");
}
'''
build('soh', source, 'oot-boot')

source = fixture('oot_handoff_test.cpp')
source = source.replace('/* MIRROR_QUEUE */', re.search(r'static int sComboMirrorSwitchSlot = [^;]+;', oot)[0] + '\n' +
                        function(oot, 'extern "C" int SOH_QueueMagicMirrorHandoff('))
begin = oot.index('    // Flag set when we want to switch to MM.')
source = source.replace('/* HANDOFF_HOOKS */', oot[begin:oot.index('    // ComboShip release gate:', begin)])
source += r'''
void Woodfall(int room = 7, Vec3f position = {1980, 0, -2400}) {
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    play = {}; player = {}; gSaveContext = {}; gPlayState = &play; gGameState = &play.state;
    play.state.running = true; play.sceneNum = SCENE_LOST_WOODS; play.roomCtx.curRoom.num = room;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.actor.world.pos = position; gSaveContext.fileNum = 0; gSaveContext.gameMode = GAMEMODE_NORMAL;
    GameInteractor::Fire<GameInteractor::OnPlayerUpdate>();
}
int main() {
    RegisterCandidate(); Woodfall(); Frames();
    require(saves == 1 && switches == 1 && recordedRoute == 4,
            "room 7's right tunnel must save and hand off through the fixed Woodfall route");
    Woodfall(8); Frames(); Woodfall(7, {1600, 0, -2780}); Frames(); Woodfall(7, {1220, 0, -2400}); Frames();
    require(saves == 1 && switches == 1, "room 8, the straight fountain and the left tunnel must remain separate");
    Woodfall(); GameInteractor::Fire<GameInteractor::OnPlayDestroy>(); Frames();
    require(saves == 1 && switches == 1, "Play destruction must cancel Woodfall before saving");
    Woodfall(); gSaveContext.fileNum = 2; Frames();
    require(saves == 1 && switches == 1, "slot changes must cancel Woodfall before saving");
    Woodfall(); ComboCrossOotRuntime::pending = true; Frames();
    require(recordedRoute == 0, "entrance shuffle must retain priority over an armed Woodfall fade");
    puts("PASS Woodfall OoT handoff: right tunnel only, existing exits, cancellation and shuffle priority");
}
'''
build('soh', source, 'oot-handoff')

source = fixture('mm_handoff_test.cpp')
begin = mm.index('static bool sComboReturnPending = false;')
source = source.replace('/* RETURN_STATE */', mm[begin:mm.index("// MM's own ResourceManager", begin)])
begin = mm.index('    // Reverse MM->OOT trigger:')
source = source.replace('/* HANDOFF_HOOKS */', mm[begin:mm.index('    } // Combo-only handoff hooks:', begin)])
source += r'''
int main() {
    RegisterCandidate(); EnterPassage(0);
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    play.sceneId = SCENE_21MITURINMAE; play.roomCtx.curRoom.num = 0; woodfallPassage = true;
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor); Frames();
    require(saves == 1 && switches == 1 && recordedRoute == 4,
            "the dedicated Woodfall passage must save and return through route 4");
    Combo_ClearReturnRequests();
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor); MM_QueueFwHandoff(); Frames();
    require(recordedRoute == 0 && kind == 3, "Farore must supersede a Woodfall passage return");
    Combo_ClearReturnRequests();
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor); MM_RequestComboReturn(); Frames();
    require(recordedRoute == 0 && kind == 1, "reset must discard a Woodfall return route");
    woodfallPassage = false;
    puts("PASS Woodfall MM handoff: dedicated endpoint and Farore/reset priority");
}
'''
build('mm', source, 'mm-handoff')

endpoint = ROOT / 'mm/2s2h/Enhancements/Gameplay/ComboWoodfallPassage.cpp'
production = endpoint.read_text() if endpoint.exists() else '''
extern "C" void ComboWoodfallPassage_ReadSpawns(PlayState*, const ActorEntry*, unsigned) {}
extern "C" void ComboWoodfallPassage_Reset(void) {}
extern "C" int ComboWoodfallPassage_Get(PlayState*, Vec3f*, s16*) { return 0; }
extern "C" int ComboWoodfallPassage_AtReturn(PlayState*, const Vec3f*) { return 0; }
'''
production = re.sub(r'^#include[^\n]*\n', '', production, flags=re.M)
source = (ROOT / 'tests/lost_woods_entrance/woodfall_geometry_test.cpp').read_text()
source = source.replace('extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }', '')
if '--mm-archive' in sys.argv:
    # Read the user's existing native resources; never copy them into the repo.
    sys.path.insert(0, str(ROOT / 'tools/black_market'))
    from native_forest import Reader, read_scene
    archive = Path(sys.argv[sys.argv.index('--mm-archive') + 1])
    with zipfile.ZipFile(archive) as native:
        scene_path = next(p for p in native.namelist() if p.endswith('/Z2_21MITURINMAE'))
        scene = read_scene(native.read(scene_path))
        spawns = Reader(scene[0])
        assert spawns.take('I') >= 1
        spawn = spawns.take('HhhhhhhH')
        assert spawn[1:4] == (-41, 12, -1353) and spawn[5] == 0 and spawn[7] == 0xD04, spawn
        collision_path = Reader(scene[3]).text()
        collision = native.read(collision_path)
    reader = Reader(collision, 64)
    reader.take('6h')
    vertices = [reader.take('3h') for _ in range(reader.take('I'))]
    polygons = [reader.take('4H4h') for _ in range(reader.take('I'))]
    surfaces = [reader.take('2I') for _ in range(reader.take('I'))]
    reader.skip(reader.take('I') * 8)  # camera records
    reader.skip(reader.take('I') * 6)  # camera positions
    water = [reader.take('5hI') for _ in range(reader.take('I'))]
    reader.finish()
    assert water == [(-2400, 1, -2400, 4800, 4800, 7938)], water
    rows = []
    for polygon in polygons:
        points = [vertices[index & 0x1FFF] for index in polygon[1:4]]
        relative = [(x - spawn[1], y - spawn[2] + 80, z - spawn[3]) for x, y, z in points]
        points_text = ','.join('{' + ','.join(map(str, p)) + '}' for p in relative)
        normal_text = ','.join(map(str, polygon[4:7]))
        exit_index = (surfaces[polygon[0]][1] >> 8) & 0x1F
        rows.append('    {{' + points_text + '},{' + normal_text + '},' +
                    str(bool(polygon[1] & 0x4000)).lower() + ',' + str(exit_index) + '},')
    source = re.sub(r'/\* NATIVE_GEOMETRY \*/.*?/\* END_NATIVE_GEOMETRY \*/',
                    '\n'.join(rows), source, flags=re.S)
    print(f'Native Woodfall collision: {len(vertices)} vertices, {len(polygons)} polygons; '
          f'archive SHA256 {hashlib.sha256(archive.read_bytes()).hexdigest()}', flush=True)
build('mm', source.replace('/* ENDPOINT_PRODUCTION */', production), 'geometry')
