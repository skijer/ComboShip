#!/usr/bin/env python3
"""Compile MM's production boundary bodies and exercise native solo versus Combo sessions."""
from pathlib import Path
import re
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DEPS = Path(os.environ.get('COMBO_TEST_DEPS', str(ROOT / 'build-deps'))) / 'include'

def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

COMMON = r'''
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#define COMBO_BUILD
#define COMBO_EXPORT
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_MM; }
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
'''

save = (ROOT/'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
if 'SaveManager_GetSavesFolderPath() {' in save:
    path_impl = block(save, 'static std::filesystem::path SaveManager_GetSavesFolderPath()')
else:
    path_impl = re.search(r'const std::filesystem::path savesFolderPath[^;]+;', save).group(0)
save_program = COMMON + r'''
#include <nlohmann/json.hpp>
#include <ctime>
const std::string appShortName="2ship";
namespace Ship { struct Context {
static std::string GetPathRelativeToAppDirectory(const std::string& s,const std::string& = "") {return s;}
}; struct Config {std::string GetString(const char*,const std::string&){return "stale-oot.sav";}}; }
namespace ComboRando { constexpr int GAME_MM=1; }
int writes=0,reads=0;
int marketWrites=0;
void ComboBlackMarket_SaveState(nlohmann::json&,int){++marketWrites;}
int forestWrites=0,forestSlot=-1;
void ComboSoloForestMm_SaveState(nlohmann::json&,int slot){++forestWrites;forestSlot=slot;}
void WriteCombined(int,int,const char*) {++writes;}
const char* ReadCombined(int,int) {++reads;return "{\"combo\":true}";}
auto gComboWriteGameSave=WriteCombined;
auto gComboReadGameSave=ReadCombined;
int Combo_MM_SaveCrossDungeonCheckpoint() {return -1;}
namespace Notification { struct Notice {std::string message;}; void Emit(Notice) {} }
struct SaveOptions {int optionId=0,audioSetting=0,zTargetSetting=0,language=0,languageSetting=0;};
void to_json(nlohmann::json& j,const SaveOptions& o){j={{"optionId",o.optionId},{"audioSetting",o.audioSetting},{"zTargetSetting",o.zTargetSetting}};}
void from_json(const nlohmann::json& j,SaveOptions& o){j.at("optionId").get_to(o.optionId);j.at("audioSetting").get_to(o.audioSetting);j.at("zTargetSetting").get_to(o.zTargetSetting);}
#define SAVE_OPTIONS_VALID_ID 0xA51D
#define CVAR_AUDIO_SETTING "gSettings.AudioSetting"
#define CVAR_ZTARGET_SETTING "gSettings.ZTargetSetting"
constexpr int LANGUAGE_ENG=0,SAVE_AUDIO_STEREO=0;
int cvarWrites=0;
void CVarSetInteger(const char*,int){++cvarWrites;}
void CVarSave(){}
void* CVarGet(const char*){return nullptr;}
int CVarGetInteger(const char*,int def){return def;}
''' + path_impl + '\n' + block(save, 'static int SaveManager_ClassifySaveFile(') + '\n'
for marker in ['void SaveManager_WriteSaveFile(', 'void SaveManager_DeleteSaveFile(',
               'int SaveManager_ReadSaveFile(const std::filesystem::path& fileName, nlohmann::json& j) {',
               'void SaveManager_MoveInvalidSaveFile(', 'int SaveManager_GetOpenFileSlot()',
               'void SaveManager_WriteGlobalOptions(', 'bool SaveManager_ReadGlobalOptions(']:
    save_program += block(save, marker) + '\n'
save_program += block((ROOT/'mm/2s2h/Enhancements/Items/ColorPictograph.cpp').read_text(), 'static std::filesystem::path PictoDir()') + '\n'
save_program += block((ROOT/'mm/2s2h/BenPort.cpp').read_text(), 'std::filesystem::path GetSaveFile(std::shared_ptr<Ship::Config> Conf)') + '\n'
save_program += r'''
int main(int argc,char** argv) {
    std::filesystem::current_path(argv[1]);
    assert(PictoDir()=="saves/mm");
    auto config=std::make_shared<Ship::Config>();
    assert(GetSaveFile(config)==std::filesystem::absolute("saves/mm/mm_save.sav"));
    const nlohmann::json source={{"native",true}};
    // Stale callbacks must not divert solo writes, reads, backups, copy or erasure.
    SaveManager_WriteSaveFile("file1.json",source);
    SaveManager_WriteSaveFile("file1backup.json",source);
    SaveManager_WriteSaveFile("global.json",source);
    assert(writes==0);
    assert(marketWrites==2); // native slot + backup; global options are unrelated
    assert(forestWrites==2 && forestSlot==0);
    for (const char* name:{"file1.json","file1backup.json","global.json"})
        assert(std::filesystem::exists(std::filesystem::path("saves/mm")/name));
    assert(!std::filesystem::exists("saves/file1.json"));
    nlohmann::json loaded;
    assert(SaveManager_ReadSaveFile("file1.json",loaded)==0 && loaded==source && reads==0);
    assert(SaveManager_GetOpenFileSlot()==2);
    SaveManager_WriteSaveFile("file2.json",loaded); // native copy uses these same primitives
    assert(forestWrites==3 && forestSlot==1);
    SaveManager_DeleteSaveFile("file1.json");
    SaveManager_DeleteSaveFile("file1backup.json");
    assert(SaveManager_GetOpenFileSlot()==1);
    assert(SaveManager_ReadSaveFile("file2.json",loaded)==0 && loaded==source);
    SaveManager_MoveInvalidSaveFile("file2.json","bad save");
    assert(!std::filesystem::exists("saves/mm/file2.json"));
    bool moved=false;for(const auto& e:std::filesystem::directory_iterator("saves/mm"))
        moved|=e.path().filename().string().starts_with("file2_invalid_");
    assert(moved);
    SaveOptions options;options.optionId=SAVE_OPTIONS_VALID_ID;options.audioSetting=2;options.zTargetSetting=1;
    SaveManager_WriteGlobalOptions(options);assert(cvarWrites==0);
    SaveOptions reread;assert(SaveManager_ReadGlobalOptions(reread));
    assert(reread.audioSetting==2 && reread.zTargetSetting==1);
    // Combined slots retain the container route; backups stay suppressed.
    gComboGameMode=COMBO_GAME_BOTH;
    assert(PictoDir()=="Save");
    assert(GetSaveFile(config)==std::filesystem::absolute("stale-oot.sav"));
    SaveManager_WriteSaveFile("file1.json",source);
    SaveManager_WriteSaveFile("file1backup.json",source);
    assert(writes==1);
    assert(forestWrites==3); // the private native forest never writes into a combined slot
    assert(SaveManager_ReadSaveFile("file1.json",loaded)==0 && loaded["combo"]==true && reads==1);
    SaveManager_DeleteSaveFile("file1.json");
    assert(writes==2);
    SaveManager_WriteGlobalOptions(options);assert(cvarWrites==2);
    std::cout<<"MM native save boundaries passed\n";
}
'''

gui = (ROOT/'mm/2s2h/BenGui/BenGui.cpp').read_text()
menu_program = COMMON + r'''
struct Menu {};
std::shared_ptr<Menu> mBenMenu=std::make_shared<Menu>();
struct Gui {int assignments=0;void SetMenu(std::shared_ptr<Menu>) {++assignments;}} gui;
struct Window {Gui* GetGui() {return &gui;}} window;
namespace Ship {struct Context {static Context* GetRawInstance(){static Context c;return &c;}
Window* GetWindow(){return &window;}};}
void SetupMenu(){mBenMenu=std::make_shared<Menu>();}
''' + block(gui, 'void ActivateMenu()') + r'''
int main() {
    ActivateMenu();assert(gui.assignments==1);
    gComboGameMode=COMBO_GAME_BOTH;ActivateMenu();assert(gui.assignments==1);
    std::cout<<"MM native menu activation passed\n";
}
'''

goal = (ROOT/'mm/2s2h/FleetShipCombo/FleetComboGoal.cpp').read_text()
goal_program = COMMON + r'''
int CVarGetInteger(const char*,int) {return 0;}
#define CVAR_COMBO 1
''' + block(goal, 'static bool BeatBothBosses()') + r'''
int main() {
    assert(!BeatBothBosses());
    gComboGameMode=COMBO_GAME_BOTH;assert(BeatBothBosses());
    std::cout<<"MM native boss goal passed\n";
}
'''
pools = (ROOT/'mm/2s2h/Rando/Logic/GeneratePools.cpp').read_text()
pool_prefix = pools[pools.index('void GeneratePools('):pools.index('    std::vector<RandoItemId> startingItems')]
pool_program = COMMON + r'''
#include <vector>
using RandoCheckId=int;using RandoItemId=int;
constexpr int RO_SHUFFLE_TRIFORCE_PIECES=0,RO_TRIFORCE_PIECES_REQUIRED=1,RO_TRIFORCE_PIECES_MAX=2;
constexpr int RO_GENERIC_YES=1,RO_GENERIC_NO=0,RO_GENERIC_ON=1,RO_SHUFFLE_MAGIC_MIRROR=3;
struct RandoSaveInfo {uint32_t randoSaveOptions[4]={1,7,9,0};};
int mirrorRequested=0,mirrorReads=0;
int CVarGetInteger(const char* name,int fallback) {
    assert(std::string(name)=="gRando.Generation.ShuffleMagicMirror" && fallback==0);
    ++mirrorReads;return mirrorRequested;
}
int gMMComboGoalHunt=0,gMMComboGoalRequired=99,gMMComboGoalPieces=100;
''' + pool_prefix + '}\n' + r'''
int main() {
    RandoSaveInfo info;std::vector<int> checks,items;
    GeneratePools(info,checks,items);
    assert(info.randoSaveOptions[0]==1 && info.randoSaveOptions[1]==7 && info.randoSaveOptions[2]==9);
    assert(mirrorReads==0 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
    gComboGameMode=COMBO_GAME_BOTH;GeneratePools(info,checks,items);assert(info.randoSaveOptions[0]==0);
    assert(mirrorReads==1 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
    mirrorRequested=1;gComboGameMode=COMBO_GAME_MM;GeneratePools(info,checks,items);
    assert(mirrorReads==1 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==0);
    gComboGameMode=COMBO_GAME_BOTH;GeneratePools(info,checks,items);
    assert(mirrorReads==2 && info.randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR]==RO_GENERIC_ON);
    gMMComboGoalHunt=1;GeneratePools(info,checks,items);
    assert(info.randoSaveOptions[1]==99 && info.randoSaveOptions[2]==100);
    std::cout<<"MM native randomizer goals passed\n";
}
'''

port=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
entry_program = COMMON + r'''
bool sComboTransitionActive=false;
struct OTRGlobals {inline static void* Instance=nullptr;};
int gComboStartFileNum=2,runs=0;
void MM_RunMain(){assert(sComboTransitionActive && gComboStartFileNum==-1);OTRGlobals::Instance=&runs;++runs;}
'''+block(port,'extern "C" COMBO_EXPORT void MM_SetGameMode(')+'\n'+block(port,'extern "C" COMBO_EXPORT void MM_RunSolo(')+r'''
int main(){
    MM_SetGameMode(999);assert(Combo_IsCombinedGame());
    MM_SetGameMode(COMBO_GAME_MM);assert(!Combo_IsCombinedGame());MM_RunSolo();assert(runs==1);
    MM_SetGameMode(COMBO_GAME_BOTH);assert(!Combo_IsCombinedGame());
    MM_SetGameMode(999);assert(!Combo_IsCombinedGame());
    std::cout<<"MM solo entry boundary passed\n";
}
'''
opening=(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboOpeningForest.h').read_text()
opening_program=COMMON+r'''
constexpr int GAMEMODE_NORMAL=0,SCENE_LOST_WOODS=1,ENTR_SCENE_OPENING_DUNGEON=2;
#define ENTRANCE(scene,spawn) (0x600+(spawn))
struct PlayState {int sceneId=SCENE_LOST_WOODS;};
struct {int gameMode=GAMEMODE_NORMAL,fileNum=0,sceneLayer=0,nextCutsceneIndex=17,cutsceneTrigger=1,respawnFlag=2;
struct {int entrance=ENTRANCE(LOST_WOODS,0),cutsceneIndex=0;} save;} gSaveContext;
bool ComboLostWoods_IsPlayableSlot(int slot){return slot>=0&&slot<3;}
'''+block(opening,'static inline int ComboOpeningForest_IsExploration(')+'\n'+block(opening,'static inline void ComboOpeningForest_NormalizeEntry(')+r'''
int main(){
    PlayState play;assert(!ComboOpeningForest_IsExploration(&play));
    gSaveContext.save.entrance=ENTR_SCENE_OPENING_DUNGEON<<9;ComboOpeningForest_NormalizeEntry();
    assert(gSaveContext.save.entrance==(ENTR_SCENE_OPENING_DUNGEON<<9) && gSaveContext.cutsceneTrigger==1);
    gComboGameMode=COMBO_GAME_BOTH;ComboOpeningForest_NormalizeEntry();
    assert(gSaveContext.save.entrance==ENTRANCE(CLOCK_TOWER_INTERIOR,0) && gSaveContext.cutsceneTrigger==0);
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);assert(ComboOpeningForest_IsExploration(&play));
    std::cout<<"MM native prologue preservation passed\n";
}
'''

give=(ROOT/'mm/2s2h/Rando/GiveItem.cpp').read_text()
triforce_case=give[give.index('        case RI_TRIFORCE_PIECE:'):give.index('        // Technically these should never be used')]
triforce_program=COMMON+r'''
#include <vector>
constexpr int RI_TRIFORCE_PIECE=1,RI_TRIFORCE_PIECE_PREVIOUS=2,RI_SOUL_BOSS_MAJORA=3;
constexpr int RO_TRIFORCE_PIECES_REQUIRED=0,RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA=0;
constexpr int TRANS_TRIGGER_START=1,TRANS_TYPE_FADE_BLACK=0;
#define ENTRANCE(scene,spawn) (0x1234+(spawn))
struct {int fileNum=0;struct {struct {struct {int foundTriforcePieces=0;int options[1]={2};} rando;} shipSaveInfo;} save;} gSaveContext;
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.options
struct GIEventTransition {int entrance,cutsceneIndex,transitionTrigger,transitionType;};
struct GameInteractor {static GameInteractor* Instance;std::vector<GIEventTransition> events;} gi;
GameInteractor* GameInteractor::Instance=&gi;
int completions=0,majoraSouls=0,comboProgress=0;
void GameInteractor_ExecuteOnGameCompletion(){++completions;}
bool Flags_GetRandoInf(int){return false;}
namespace Rando {void GiveItem(int item){assert(item==RI_SOUL_BOSS_MAJORA);++majoraSouls;}}
void Progress(int,int){++comboProgress;}
void (*gMMComboTriforceProgress)(int,int)=Progress;
void Collect(int item){switch(item){
'''+triforce_case+r'''
}}
int main(){
    Collect(RI_TRIFORCE_PIECE);assert(!completions && !comboProgress);
    Collect(RI_TRIFORCE_PIECE);assert(completions==1 && majoraSouls==1 && !comboProgress);
    assert(gi.events.size()==1 && gi.events[0].cutsceneIndex==0xFFF7);
    gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces=1;gComboGameMode=COMBO_GAME_BOTH;
    Collect(RI_TRIFORCE_PIECE);assert(completions==1 && comboProgress==1);
    std::cout<<"MM native Triforce completion passed\n";
}
'''

failed=[]
with tempfile.TemporaryDirectory(prefix='mm-solo-behavior-') as d:
    tmp=Path(d)
    for name,program in [('save',save_program),('menu',menu_program),('boss',goal_program),('pool',pool_program),('entry',entry_program),('opening',opening_program),('triforce',triforce_program)]:
        cpp=tmp/(name+'.cpp');exe=tmp/name;cpp.write_text(program)
        compiled=subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Wno-unused-parameter','-I',str(ROOT),'-I',str(DEPS),str(cpp),'-o',str(exe)])
        if compiled.returncode:
            failed.append(name+' (compile)');continue
        run_dir=tmp/(name+'-run');run_dir.mkdir()
        ran=subprocess.run([str(exe),str(run_dir)])
        if ran.returncode:failed.append(name)
if failed:
    raise SystemExit('MM behavior failures: '+', '.join(failed))
