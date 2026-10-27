"""Run the real MM spawn bodies, then check their native floor footprints."""
import importlib.util
import io
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

from test_generator import ROOT

sys.path.insert(0, str(ROOT / 'tools/black_market'))
from native_forest_bundle import load_bundle


def body(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class Placements(unittest.TestCase):
    def test_real_mm_spawns_flank_the_authored_rim_and_face_its_forward_path(self):
        source = (ROOT / 'mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp').read_text()
        fixture = r'''
#include "combo/entrances/ForestActorPlacements.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
using s16=int16_t; using s32=int32_t;
struct Vec3f { float x,y,z; };
struct Actor { int id{}, params{}; struct { Vec3f pos; } world; Actor* next{}; };
struct ActorContext { struct List {Actor* first{};}; std::array<List, 2> actorLists; };
struct CollisionPoly {};
struct PlayState {
 struct { bool running=true; } state;
 int sceneId{}, transitionTrigger{}, transitionMode{}, soaringCsOrSoTCsPlaying{};
 struct {int state{};} pauseCtx,csCtx;
 struct {struct {int num{};void* segment=reinterpret_cast<void*>(1);}curRoom;int status{};}roomCtx;
 ActorContext actorCtx{}; int colCtx{};
};
struct {int gameMode{}, fileNum{}, cutsceneTrigger{};struct {int cutsceneIndex{};}save;}gSaveContext;
PlayState storage, *gPlayState=&storage;
void* gGameState=&storage.state;
bool sSpawnAttempted=false, sHorseSpawnAttempted=false;
struct {bool Resolve(){return true;}}sBridge;
constexpr int SCENE_00KEIKOKU=28,GAMEMODE_NORMAL=0,TRANS_TRIGGER_OFF=0,TRANS_MODE_OFF=0;
constexpr int PAUSE_STATE_OFF=0,CS_STATE_IDLE=0,ACTOR_EN_VIEWER=1,ACTOR_EN_HORSE=13,BGCHECK_SCENE=0;
constexpr float BGCHECK_Y_MIN=-32000;
constexpr s16 kSongGiftParams=0x7E33,kWaitingParams=0x7F13,COMBO_FOREST_EPONA_PARAMS=0x7E40;
bool ComboForestActors_IsMmGame(){return true;}
bool ComboLostWoods_IsPlayableSlot(int){return true;}
bool ComboForestActors_IsExploration(PlayState* play){return play->sceneId==0;}
bool Player_InCsMode(PlayState*){return false;}
bool IsSongGiftParam(int p){return p==kSongGiftParams;}
bool ComboSaria_IsParam(int p){return p==kWaitingParams;}
Vec3f ComboOpeningForest_ReturnAnchor(PlayState*){return {-695,0,-313};}
s16 ComboOpeningForest_ReturnYaw(PlayState*){return 12288;}
int DEG_TO_BINANG(int degrees){return int(degrees*65536.f/360.f);}
float BgCheck_EntityRaycastFloor5(int*,CollisionPoly** poly,s32*,void*,Vec3f*){
 static CollisionPoly floor;*poly=&floor;return 0;
}
std::array<Actor,2> spawned{};int count{};
Actor* Actor_Spawn(ActorContext* ctx,PlayState*,int id,float x,float y,float z,int,int yaw,int,int params){
 auto* actor=&spawned[count++];actor->id=id;actor->params=params;actor->world.pos={x,y,z};
 actor->next=ctx->actorLists[0].first;ctx->actorLists[0].first=actor;
 std::printf("%d %.6f %.6f %.6f %d\n",id,x,y,z,yaw);return actor;
}
/* SPAWN BODIES */
int main(){
 SpawnForestSaria();SpawnForestEpona();assert(count==2);
 SpawnForestSaria();SpawnForestEpona();assert(count==2);
}
'''
        # Only renderer/scene access, floor raycast and allocation are boundaries;
        # the source's placement, orientation, duplicate checks and actor params run.
        fixture = fixture.replace('/* SPAWN BODIES */',
                                  body(source, 'void SpawnForestSaria()') + '\n' +
                                  body(source, 'void SpawnForestEpona()'))
        with tempfile.TemporaryDirectory(prefix='native-forest-placements-') as folder:
            cpp, binary = Path(folder) / 'spawn.cpp', Path(folder) / 'spawn'
            cpp.write_text(fixture)
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                            '-I' + str(ROOT), str(cpp), '-o', str(binary)], check=True)
            output = subprocess.check_output([str(binary)], text=True,
                                             env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        spawns = [[float(value) for value in line.split()] for line in output.splitlines()]
        self.assertEqual(len(spawns), 2)
        angle = 12288 * math.tau / 65536
        sine, cosine = math.sin(angle), math.cos(angle)
        for row, lateral in zip(spawns, (-120, 120)):
            _, x, y, z, yaw = row
            dx, dz = x + 695, z + 313
            self.assertAlmostEqual(dx * cosine - dz * sine, lateral, delta=.001)
            self.assertAlmostEqual(dx * sine + dz * cosine, 120, delta=.001)
            self.assertEqual(y, 0)
            self.assertEqual(int(yaw) & 65535, 12288,
                             'Both companions must face out from the portal along the approach')
        # These authored offsets are chosen from the radius-68 rim with a clear
        # walking lane and the real horse's two 22-unit collision cylinders.
        spec = importlib.util.spec_from_file_location('forest_placement_collision',
                                                      ROOT / 'tests/magic_mirror/check_scene.py')
        geometry = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(geometry)
        with zipfile.ZipFile(io.BytesIO(load_bundle(ROOT / 'tools/native_forest'))) as archive:
            polys = geometry.collision(archive.read('scenes/nonmq/comboship_forest/comboship_forest_collision'))
        for _, x, y, z, _ in spawns:
            for dx, dz in ((0, 0), (25, 0), (-25, 0), (0, 25), (0, -25)):
                self.assertEqual(geometry.floor_at(polys, x + dx, z + dz, 120), 0)
                self.assertFalse(geometry.blocked(polys, (x, 5, z), (x + dx, 75, z + dz)))


if __name__ == '__main__':
    unittest.main()
