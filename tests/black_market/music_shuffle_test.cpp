// Run the actual collection, shuffle and event callbacks. Resource loading,
// GUI persistence and the audio worker are boundaries, not a running game.
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <libultraship/libultra/types.h>
#include "sequence.h"
#include "sfx.h"
#include "AudioCollection.h"
#include "combo/audio/SceneMusic.h"
#include "ship/utils/StringHelper.h"
#include "soh/soh/Enhancements/enhancementTypes.h"
#define CVAR_AUDIO(name) "gAudioEditor." name
/* AUDIO CONSTANTS */
#define GAMEMODE_NORMAL 0
#define GAMEMODE_END_CREDITS 3
#define IS_RANDO false
static std::map<std::string,int> values;
int CVarGetInteger(const char* key,int fallback) {
    auto it=values.find(key);return it==values.end() ? fallback : it->second;
}
void CVarSetInteger(const char* key,int value) { values[key]=value; }
void CVarClear(const char* key) { values.erase(key); }
namespace Ship {
struct Context {
    static Context* GetRawInstance() { static Context context;return &context; }
    Context* GetWindow() { return this; }
    Context* GetGui() { return this; }
    void SaveConsoleVariablesNextFrame() {}
};
}
namespace Rando {
struct Context {
    static Context* GetInstance() { static Context context;return &context; }
    uint32_t GetSeed() { return 0x12345678; }
};
}
struct { int gameMode=GAMEMODE_NORMAL;struct { struct { uint64_t fileCreatedAt=99; } stats; } ship; } gSaveContext;
class GameInteractor {
public:
    struct OnSceneInit { using Callback=std::function<void(int16_t)>; };
    struct OnGenerationCompletion { using Callback=std::function<void()>; };
    struct OnRandoSeedGeneration { using Callback=std::function<void()>; };
    struct OnGameStateUpdate { using Callback=std::function<void()>; };
    static GameInteractor* Instance;
    template<class Hook> static auto& Hooks() { static std::vector<typename Hook::Callback> hooks;return hooks; }
    template<class Hook,class F> void RegisterGameHook(F callback) { Hooks<Hook>().emplace_back(callback); }
};
GameInteractor interactor;
GameInteractor* GameInteractor::Instance=&interactor;
struct AudioEditor { void InitElement(); };
namespace AudioPreview { void Update() {} }
void ReplayCurrentBGM() {}
AudioCollection* AudioCollection::Instance;
namespace ShipUtils {
void RandInit(uint64_t seed,uint64_t* state=nullptr);
uint32_t next32(uint64_t* state=nullptr);
uint32_t Random(uint32_t min,uint32_t max,uint64_t* state=nullptr);
/* SHUFFLE TEMPLATE */
}
/* RNG */
/* COLLECTION */
/* EDITOR */

static void CheckRoll(int event,int selected,bool useCategories) {
    values.clear();
    AudioCollection collection;
    AudioCollection::Instance=&collection;
    char custom[]="music/Only Included_bgm";
    char excluded[]="music/Excluded Track_bgm";
    char fanfare[]="music/Excluded Fanfare_fanfare";
    collection.AddToCollection(custom,selected);
    collection.AddToCollection(excluded,0x1237);
    collection.AddToCollection(fanfare,0x1238);
    auto map=collection.GetAllSequences();
    std::vector<uint16_t> targets={ComboSceneMusic::kForest,ComboSceneMusic::kMarket};
#ifndef SCENE_MUSIC_MM
    uint16_t night=0;
    for (const auto& [id,row]:map) if (row.label=="Hyrule Field (Night)") night=id;
    assert(night && "Night music must join the actual scene/generation shuffle");
    targets.push_back(NA_BGM_FIELD_LOGIC);
    targets.push_back(night);
#endif
    for (const auto& [id,row]:map)
        if (id!=selected) values["gAudioEditor.Excluded."+row.sfxKey]=1;
    for (auto key:targets) values[collection.GetCvarKey(map.at(key).sfxKey)]=0x1237;
    values["gAudioEditor.UseSongCategories"]=useCategories;
    // Disabled event options leave all selections alone.
#ifdef SCENE_MUSIC_MM
    AudioEditor_RandomizeOnSceneLoad();
    for (auto& callback:GameInteractor::Hooks<GameInteractor::OnRandoSeedGeneration>()) callback();
#else
    for (auto& callback:GameInteractor::Hooks<GameInteractor::OnSceneInit>()) callback(0x51);
    for (auto& callback:GameInteractor::Hooks<GameInteractor::OnGenerationCompletion>()) callback();
#endif
    for (auto key:targets) assert(collection.GetReplacementSequence(key)==0x1237);
#ifdef SCENE_MUSIC_MM
    if (event==0) {
        values["gAudioEditor.RandomizeAllOnNewScene"]=1;
        AudioEditor_RandomizeOnSceneLoad();
    } else {
        values["gAudioEditor.RandomizeAllOnRandoGen"]=1;
        for (auto& callback:GameInteractor::Hooks<GameInteractor::OnRandoSeedGeneration>()) callback();
    }
#else
    values["gAudioEditor.RandomizeAudioGenModes"]=event==0 ? RANDOMIZE_ON_NEW_SCENE : RANDOMIZE_ON_RANDO_GEN_ONLY;
    if (event==0)
        for (auto& callback:GameInteractor::Hooks<GameInteractor::OnSceneInit>()) callback(0x51);
    else
        for (auto& callback:GameInteractor::Hooks<GameInteractor::OnGenerationCompletion>()) callback();
#endif
    for (auto key:targets) assert(collection.GetReplacementSequence(key)==selected);
    for (auto* row:collection.GetIncludedSequences()) assert(row->canBeUsedAsReplacement);
    // Locks and reset must preserve a deliberately
    // retained choice. Group controls must apply to metadata scene rows too.
    const auto target=targets.back();
    const auto valueKey=collection.GetCvarKey(map.at(target).sfxKey);
    const auto lockKey=collection.GetCvarLockKey(map.at(target).sfxKey);
    values[valueKey]=0x1237;
    values[lockKey]=1;
    RandomizeGroup(SEQ_BGM_WORLD);
    ResetGroup(map,SEQ_BGM_WORLD);
    assert(values.at(valueKey)==0x1237);
    UnlockGroup(map,SEQ_BGM_WORLD);
    RandomizeGroup(SEQ_BGM_WORLD);
    assert(collection.GetReplacementSequence(target)==selected);
    LockGroup(map,SEQ_BGM_WORLD);
    for (auto key:targets) assert(values.at(collection.GetCvarLockKey(map.at(key).sfxKey))==1);
}
static void CheckEmptyPool() {
    values.clear();
    AudioCollection collection;
    AudioCollection::Instance=&collection;
    const auto map=collection.GetAllSequences();
    for (const auto& [id,row]:map) values["gAudioEditor.Excluded."+row.sfxKey]=1;
    const auto key=collection.GetCvarKey(map.at(ComboSceneMusic::kMarket).sfxKey);
    values[key]=0x3e;
    RandomizeGroup(SEQ_BGM_WORLD);
    assert(values.at(key)==0x3e);
}
int main() {
#ifdef SCENE_MUSIC_MM
    AudioEditor editor;editor.InitElement();
#else
    AudioEditorRegisterOnSceneInitHook();
    AudioEditorRegisterOnGenerationCompletionHook();
#endif
    for (int event:{0,1}) for (bool categories:{false,true}) {
        CheckRoll(event,0x1234,categories);
        CheckRoll(event,0x1236,categories);
    }
    CheckEmptyPool();
    std::puts("PASS scene/generation shuffle: imported music, exclusions, scene rows, night selector, locks and reset");
}
