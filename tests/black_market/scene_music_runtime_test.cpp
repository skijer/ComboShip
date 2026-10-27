// Execute the production routing with each host's real PlayState/SaveContext.
// Only the CVar store, preview stop and native command queue are test boundaries.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "global.h"
#include "AudioCollection.h"
#include "ship/utils/StringHelper.h"
#include "combo/audio/SceneMusic.h"
#include "combo/ComboGameMode.h"

PlayState* gPlayState;
SaveContext gSaveContext;
int gComboGameMode;
AudioCollection* AudioCollection::Instance;
static std::map<std::string,int> values;
int CVarGetInteger(const char* name,int fallback) {
    auto found=values.find(name);return found==values.end() ? fallback : found->second;
}
void CVarSetInteger(const char* name,int value) { values[name]=value; }
#ifndef CVAR_AUDIO
#define CVAR_AUDIO(name) "gAudioEditor." name
#endif
/* PRODUCTION */

static std::vector<uint16_t> queued;
extern "C" void Audio_InvalidateSceneSequence(void);
extern "C" void Audio_CancelPreviewRestore(void);
extern "C" void Audio_PreviewSceneSequence(uint16_t sequence);
static std::vector<uint8_t> ambienceRequests;
void Audio_SetEnvReverb(s8) {}
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#ifdef SCENE_MUSIC_MM
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#define COMBO_SCENE_MUSIC_MM
static uint16_t sComboSceneMusicPreviewRow;
u8 gSceneSeqState;
u16 sRequestedSceneSeqId=NA_BGM_DISABLED;
u16 sPrevMainBgmSeqId=NA_BGM_DISABLED;
static u16 activeMain=NA_BGM_DISABLED;
u16 AudioSeq_GetActiveSeqId(u8) { return activeMain; }
s32 Entrance_GetSceneId(u16) { return 0; }
namespace AudioPreview {
static u16 sPreviewRestoreSeqId=NA_BGM_DISABLED;
static bool sIsBgmPreviewing;
static uint8_t sGraceFrames;
/* NATIVE PREVIEW */
}
#undef SEQCMD_PLAY_SEQUENCE
#define SEQCMD_PLAY_SEQUENCE(player,fade,sequence) queued.push_back(sequence)
#undef SEQCMD_SET_SEQPLAYER_IO
#define SEQCMD_SET_SEQPLAYER_IO(...) ((void)0)
#define GameInteractor_Should(...) false
void Audio_StartSceneSequence(u16 sequence) { queued.push_back(sequence); }
void Audio_PlayAmbience(u8 ambience) { ambienceRequests.push_back(ambience); }
/* NATIVE CACHE */
void Audio_PlayMorningSceneSequence(u16 sequence,u8 day) { Audio_PlaySceneSequence(sequence,day); }
#else
void Audio_QueueSeqCmd(uint32_t command) { queued.push_back(command&0xffff); }
void Audio_PlaySceneSequence(u16 sequence) { queued.push_back(sequence); }
void Audio_PlayNatureAmbienceSequence(u8 ambience) { ambienceRequests.push_back(ambience); }
u16 sPrevMainBgmSeqId;
static u16 sPreviewNightBgmSeqId=NA_BGM_DISABLED;
extern "C" uint16_t Audio_GetActiveNightBgm(void) { return NA_BGM_DISABLED; }
extern "C" uint8_t Audio_RestoreNightBgm(uint16_t) { return false; }
static u16 activeMain=NA_BGM_DISABLED;
u16 func_800FA0B4(u8) { return activeMain; }
void Audio_SetSequenceMode(u8) {}
void Audio_QueuePreviewSeqCmd(u16 sequence) { queued.push_back(sequence); }
/* NATIVE EXACT PREVIEW */
#endif
/* NATIVE ENVIRONMENT */
#include "combo/audio/SceneMusicNative.h"
/* EDITOR CURRENT SELECTION */

// Execute the actual graph-thread sequence starts through the native command
// macros. Capture only delivery to the audio worker, without loading a bank.
static std::vector<uint16_t> started;
ActiveSequence gActiveSeqs[4];
u16 AudioEditor_GetReplacementSeq(u16 sequence) {
    return AudioCollection::Instance->GetReplacementSequence(sequence);
}
#ifdef SCENE_MUSIC_MM
AudioContext gAudioCtx;
u8 sStartSeqDisabled;
void AudioThread_QueueCmdS32(u32 command,s32 data) {
    if ((command>>24)==0x82 || (command>>24)==0x85) started.push_back(uint32_t(data)>>16);
}
void AudioThread_QueueCmdF32(u32,f32) {}
#else
AudioContext gAudioContext;
u8 D_80133408;
static u16 sResolvedMainBgm;
void Audio_QueueCmdS32(u32 command,s32) {
    if ((command>>24)==0x82 || (command>>24)==0x85) started.push_back(command&0xffff);
}
void Audio_QueueCmdF32(u32,f32) {}
#endif
/* NATIVE START */

static PlayState play;
static void Reset(int mode,int scene) {
    play={};gSaveContext={};values.clear();queued.clear();ambienceRequests.clear();sPreviousSceneMusicRow=0;
    gPlayState=&play;gComboGameMode=mode;gSaveContext.gameMode=GAMEMODE_NORMAL;gSaveContext.fileNum=0;
#ifdef SCENE_MUSIC_MM
    play.sceneId=scene;gSaveContext.save.time=CLOCK_TIME(12,0);
    sComboSceneMusicPreviewRow=0;gSceneSeqState=SCENESEQ_DEFAULT;
    sRequestedSceneSeqId=NA_BGM_DISABLED;activeMain=NA_BGM_DISABLED;
    play.envCtx.timeSeqState=TIMESEQ_DISABLED;
#else
    play.sceneNum=scene;gSaveContext.dayTime=0x8000;
    activeMain=NA_BGM_DISABLED;
#endif
}
static void Selection(uint16_t row,int selected) {
    values[AudioCollection::Instance->GetCvarKey(
        AudioCollection::Instance->GetAllSequences().at(row).sfxKey)]=selected;
}
static void CheckForest() {
    assert(AudioEditor_GetSceneMusicRow(0x3e)==ComboSceneMusic::kForest);
    assert(AudioEditor_GetSceneMusicRow(0x803e)==ComboSceneMusic::kForest);
    assert(AudioEditor_GetSceneMusicRow(0x123e)==0);
    Selection(ComboSceneMusic::kForest,0x1234);
    assert(AudioEditor_ResolveSceneSequence(0x3e,0x7777)==0x1234);
    assert(AudioEditor_ResolveSceneSequence(0x123e,0x7777)==0x7777);
    assert(AudioEditor_ResolveSceneSequence(NA_BGM_SHOP,0x7777)==0x7777);
    AudioEditor_ConfigureSceneMusic(&play);
    assert(queued.empty());Environment_PlaySceneSequence(&play);
    assert(queued.size()==1 && queued.back()==0x3e);
#ifdef SCENE_MUSIC_MM
    assert(play.sceneSequences.seqId==0x3e);
    const auto ambience=play.sceneSequences.ambienceId;
    gSaveContext.save.time=CLOCK_TIME(23,0);
#else
    assert(play.sequenceCtx.seqId==0x3e);
    const auto ambience=play.sequenceCtx.natureAmbienceId;
    gSaveContext.dayTime=0;
#endif
    queued.clear();AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
    assert(queued.empty());
#ifdef SCENE_MUSIC_MM
    assert(play.sceneSequences.ambienceId==ambience);
    queued.clear();gSaveContext.save.time=CLOCK_TIME(6,0);
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);assert(queued.empty());
    gSaveContext.save.time=CLOCK_TIME(6,1);
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
    assert(queued.size()==1 && queued[0]==0x3e);
    queued.clear();play.envCtx.timeSeqState=TIMESEQ_REQUEST;
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);assert(queued.empty());
#else
    assert(play.sequenceCtx.natureAmbienceId==ambience);
#endif
    values["gAudioEditor.Playing"]=0x3e;
    assert(AudioEditor_ResolveSceneSequence(0x3e,0x7777)==0x7777);
}
static void CheckMarket() {
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==ComboSceneMusic::kMarket);
    Selection(ComboSceneMusic::kMarket,0x1234);
    Selection(NA_BGM_SHOP,0x3e);
    assert(AudioEditor_ResolveSceneSequence(NA_BGM_SHOP,0x3e)==0x1234);
    // Both appearances use this same scene row, even at night.
#ifdef SCENE_MUSIC_MM
    gSaveContext.save.time=CLOCK_TIME(23,0);
#else
    gSaveContext.dayTime=0;
#endif
    gSaveContext.seqId=NA_BGM_SHOP;
    AudioEditor_ConfigureSceneMusic(&play);
    assert(queued.empty());Environment_PlaySceneSequence(&play);
    gSaveContext.seqId=NA_BGM_SHOP;
    AudioEditor_ConfigureSceneMusic(&play);
    Environment_PlaySceneSequence(&play);
    assert(queued.size()==2 && queued[0]==NA_BGM_SHOP && queued[1]==NA_BGM_SHOP);
#ifdef SCENE_MUSIC_MM
    assert(play.sceneSequences.seqId==NA_BGM_SHOP && play.sceneSequences.ambienceId==AMBIENCE_ID_13);
#else
    assert(play.sequenceCtx.seqId==NA_BGM_SHOP && play.sequenceCtx.natureAmbienceId==NATURE_ID_NONE);
#endif
    Selection(ComboSceneMusic::kMarket,0xeeee);
    assert(AudioEditor_ResolveSceneSequence(NA_BGM_SHOP,0x3e)==NA_BGM_SHOP);
    Selection(ComboSceneMusic::kMarket,0x1234);
    assert(CurrentBGMReplacement(NA_BGM_SHOP)==0x1234);
    Selection(ComboSceneMusic::kMarket,NA_BGM_SHOP);
    assert(CurrentBGMReplacement(NA_BGM_SHOP)==NA_BGM_SHOP);
    queued.clear();
#ifdef SCENE_MUSIC_MM
    gSaveContext.forcedSeqId=NA_BGM_CLOCK_TOWER;
#else
    gSaveContext.forcedSeqId=NA_BGM_DISABLED;
    values["gAudioEditor.Playing"]=0x1234;sPrevMainBgmSeqId=NA_BGM_SHOP;
#endif
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
#ifdef SCENE_MUSIC_MM
    // MM queues its native forced request; Configure must add no scene start.
    assert(queued.size()==1 && queued[0]==(NA_BGM_CLOCK_TOWER|SEQ_FLAG_ASYNC));
    queued.clear();play.envCtx.timeSeqState=TIMESEQ_DISABLED;
    gSaveContext.save.day=3;gSaveContext.save.time=CLOCK_TIME(2,0);
    activeMain=NA_BGM_FINAL_HOURS;
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
    assert(queued.empty());
#else
    assert(queued.empty());
#endif
    gSaveContext.fileNum=3;
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    gSaveContext.fileNum=0;gSaveContext.gameMode=1;
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    gSaveContext.gameMode=GAMEMODE_NORMAL;gSaveContext.sceneLayer=1;
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    gSaveContext.sceneLayer=0;
#ifdef SCENE_MUSIC_MM
    gSaveContext.save.cutsceneIndex=1;
#else
    gSaveContext.cutsceneIndex=1;
#endif
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    gPlayState=nullptr;assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
}
int main() {
    AudioCollection collection;AudioCollection::Instance=&collection;
    char custom[]="custom/music/Scene Test_bgm";collection.AddToCollection(custom,0x1234);
#ifdef SCENE_MUSIC_MM
    Reset(COMBO_GAME_MM,COMBO_BLACK_MARKET_SCENE);
#else
    Reset(COMBO_GAME_OOT,SCENE_SKIEJER_MARKET);
#endif
    Selection(ComboSceneMusic::kMarket,0x1234);Selection(NA_BGM_SHOP,0x3e);
#ifdef SCENE_MUSIC_MM
    AudioSeq_StartSequence(SEQ_PLAYER_BGM_MAIN,NA_BGM_SHOP,0,0);
    assert(started.size()==1 && started.back()==0x1234);
    AudioSeq_StartSequence(SEQ_PLAYER_FANFARE,NA_BGM_SHOP,0,0);
    assert(started.size()==2 && started.back()==0x3e);
#else
    Audio_StartSequenceInternal(SEQ_PLAYER_BGM_MAIN,NA_BGM_SHOP,0,0,false,0);
    assert(started.size()==1 && started.back()==0x1234);
    Audio_StartSequenceInternal(SEQ_PLAYER_FANFARE,NA_BGM_SHOP,0,0,false,0);
    assert(started.size()==2 && started.back()==0x3e);
    gAudioContext.seqReplaced[SEQ_PLAYER_BGM_MAIN]=1;
    gAudioContext.seqToPlay[SEQ_PLAYER_BGM_MAIN]=0x2222;
    Audio_StartSequenceInternal(SEQ_PLAYER_BGM_MAIN,NA_BGM_SHOP,0,0,true,0x3333);
    assert(started.back()==0x3333 && gAudioContext.seqReplaced[SEQ_PLAYER_BGM_MAIN]==1);
    Audio_StartSequenceInternal(SEQ_PLAYER_BGM_MAIN,NA_BGM_SHOP,0,0,false,0);
    assert(started.back()==0x2222 && gAudioContext.seqReplaced[SEQ_PLAYER_BGM_MAIN]==0);
#endif
    assert(gActiveSeqs[SEQ_PLAYER_BGM_MAIN].seqId==NA_BGM_SHOP);
#ifdef SCENE_MUSIC_MM
    Reset(COMBO_GAME_BOTH,SCENE_LOST_WOODS);
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);CheckForest();
    values.clear();gSaveContext.save.cutsceneIndex=1;
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    gSaveContext.save.cutsceneIndex=0;gSaveContext.sceneLayer=1;
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    gSaveContext.sceneLayer=0;gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,1);
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    Reset(COMBO_GAME_MM,COMBO_SOLO_FOREST_MM_SCENE);CheckForest();
    Reset(COMBO_GAME_OOT,COMBO_SOLO_FOREST_MM_SCENE);
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    for(int mode:{COMBO_GAME_MM,COMBO_GAME_BOTH}) {
        Reset(mode,COMBO_BLACK_MARKET_SCENE);CheckMarket();
    }
    Reset(COMBO_GAME_OOT,COMBO_BLACK_MARKET_SCENE);
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    Reset(COMBO_GAME_MM,0x20);
    Selection(ComboSceneMusic::kMarket,0x1234);
    sComboSceneMusicPreviewRow=ComboSceneMusic::kMarket;values["gAudioEditor.Playing"]=NA_BGM_SHOP;
    assert(AudioEditor_ResolveSceneSequence(NA_BGM_SHOP,0x7777)==0x1234);
    AudioEditor_ConfigureSceneMusic(&play);
    assert(sComboSceneMusicPreviewRow==0 && queued.empty());
    assert(gSaveContext.seqId==NA_BGM_DISABLED);
    sComboSceneMusicPreviewRow=ComboSceneMusic::kMarket;values["gAudioEditor.Playing"]=NA_BGM_SHOP;
    AudioPreview::sPreviewRestoreSeqId=NA_BGM_CLOCK_TOWER;
    AudioPreview::sGraceFrames=0;AudioPreview::Update();
    assert(sComboSceneMusicPreviewRow==0 && AudioPreview::sPreviewRestoreSeqId==NA_BGM_DISABLED);
    for(int mode:{COMBO_GAME_BOTH,COMBO_GAME_MM}) {
        Reset(mode,mode==COMBO_GAME_BOTH ? SCENE_LOST_WOODS : COMBO_SOLO_FOREST_MM_SCENE);
        gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
        Selection(ComboSceneMusic::kForest,0x1234);
        AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
        assert(AudioEditor_ResolveSceneSequence(0x3e,0x7777)==0x1234);
        if(mode==COMBO_GAME_BOTH) {
            gSaveContext.save.entrance=ENTRANCE(OPENING_DUNGEON,0);
            ComboOpeningForest_NormalizeEntry();
            assert(gSaveContext.save.entrance==ENTRANCE(CLOCK_TOWER_INTERIOR,0));
        } else gSaveContext.save.entrance=ENTRANCE(CLOCK_TOWER_INTERIOR,0);
        play.sceneId=SCENE_INSIDETOWER;play.sceneSequences.seqId=NA_BGM_CLOCK_TOWER;
        queued.clear();gSaveContext.seqId=0x3e;
        AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
        assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
        assert(AudioEditor_ResolveSceneSequence(NA_BGM_CLOCK_TOWER,0x7777)==0x7777);
        assert(queued.size()==1 && queued[0]==NA_BGM_CLOCK_TOWER);
    }
#else
    for(int layer=0;layer<4;++layer) {
        Reset(COMBO_GAME_OOT,SCENE_SKIEJER_FOREST);gSaveContext.sceneLayer=layer;CheckForest();
    }
    Reset(COMBO_GAME_BOTH,SCENE_SKIEJER_FOREST);
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    Reset(COMBO_GAME_OOT,SCENE_LOST_WOODS);
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    Reset(COMBO_GAME_OOT,SCENE_SKIEJER_MARKET);CheckMarket();
    Reset(COMBO_GAME_MM,SCENE_SKIEJER_MARKET);
    assert(AudioEditor_GetSceneMusicRow(NA_BGM_SHOP)==0);
    Reset(COMBO_GAME_OOT,SCENE_SKIEJER_FOREST);
    Selection(ComboSceneMusic::kForest,0x1234);
    AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
    // Includes entrances which retain BGM: the same native Saria ID must be
    // resolved again when the portal returns to ordinary OoT Lost Woods.
    play.sceneNum=SCENE_LOST_WOODS;play.sequenceCtx.seqId=0x3e;gSaveContext.seqId=0x3e;
    queued.clear();AudioEditor_ConfigureSceneMusic(&play);Environment_PlaySceneSequence(&play);
    assert(AudioEditor_GetSceneMusicRow(0x3e)==0);
    assert(AudioEditor_ResolveSceneSequence(0x3e,0x7777)==0x7777);
    assert(queued.size()==1 && queued[0]==0x3e);
    values["gAudioEditor.Playing"]=0x1234;sPrevMainBgmSeqId=0x3e;
    AudioEditor_ConfigureSceneMusic(&play);
    assert(values["gAudioEditor.Playing"]==0 && sPrevMainBgmSeqId==NA_BGM_DISABLED);
    queued.clear();activeMain=NA_BGM_SHOP;
    PreviewSequence(NA_BGM_SHOP);assert(queued.empty());
    Audio_PreviewSceneSequence(NA_BGM_SHOP);
    assert(queued.size()==1 && queued[0]==NA_BGM_SHOP && sPrevMainBgmSeqId==NA_BGM_SHOP);
#endif
    std::puts("PASS native scene music: solo/combined exploration guards, exact custom BGM, scene-entry restart, night ambience, indoor shop, preview isolation and intro/cutscene exclusions");
}
