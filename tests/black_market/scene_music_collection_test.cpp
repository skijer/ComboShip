#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "sequence.h"
#include "sfx.h"
#include "AudioCollection.h"
#include "combo/audio/SceneMusic.h"
#include "ship/utils/StringHelper.h"
#define CVAR_AUDIO(name) "gAudioEditor." name
static std::map<std::string,int> values;
int CVarGetInteger(const char* name,int fallback) {
    auto found=values.find(name);return found==values.end() ? fallback : found->second;
}
/* PRODUCTION */
int main() {
    AudioCollection collection;
    const auto map=collection.GetAllSequences();
#ifndef SCENE_MUSIC_MM
    // The legacy night selector must be an independent shuffle target, even
    // when it currently selects an imported track with no native replacement row.
    uint16_t night=0;
    for (const auto& [id,entry]:map)
        if (entry.label=="Hyrule Field (Night)") night=id;
    assert(night && "Hyrule Field night music is missing from World BGM shuffle targets");
    const auto& nightEntry=map.at(night);
    assert(nightEntry.canBeReplaced && !nightEntry.canBeUsedAsReplacement);
    assert(collection.GetCvarKey(nightEntry.sfxKey)=="gAudioEditor.HyruleFieldNightSequence");
    assert(collection.GetReplacementSequence(night)==NA_BGM_KAKARIKO_ADULT);
    values["gAudioEditor.HyruleFieldNightSequence"]=0x3e;
    assert(collection.GetReplacementSequence(night)==0x3e);
    values["gAudioEditor.HyruleFieldNightSequence"]=night;
    assert(collection.GetReplacementSequence(night)==NA_BGM_KAKARIKO_ADULT);
    values.erase("gAudioEditor.HyruleFieldNightSequence");
#endif
    uint16_t forest=0,market=0;
    for(const auto& [id,entry]:map) {
        if(entry.label=="Lost Woods (2Ship)")forest=id;
        if(entry.label=="Skiejer's Black Market")market=id;
    }
    assert(forest && market && forest!=market && "Both locations need independent audio editor entries");
    assert(!map.at(forest).canBeUsedAsReplacement && !map.at(market).canBeUsedAsReplacement);
    assert(map.at(forest).canBeReplaced && map.at(market).canBeReplaced);
    assert(collection.GetReplacementSequence(forest)==0x3e);
    assert(collection.GetReplacementSequence(market)==NA_BGM_SHOP);
    const auto forestKey=collection.GetCvarKey(map.at(forest).sfxKey);
    const auto marketKey=collection.GetCvarKey(map.at(market).sfxKey);
    assert(forestKey!=marketKey);
    values[collection.GetCvarKey(map.at(NA_BGM_SHOP).sfxKey)]=0x3e;
    assert(collection.GetReplacementSequence(NA_BGM_SHOP)==0x3e);
    assert(collection.GetReplacementSequence(market)==NA_BGM_SHOP);
    values[marketKey]=0x3e;
    values[forestKey]=NA_BGM_SHOP;
    assert(collection.GetReplacementSequence(market)==0x3e);
    assert(collection.GetReplacementSequence(forest)==NA_BGM_SHOP);
    for(int bad:{-1,0xeeee,int(forest),int(market),0}) {
        values[marketKey]=bad;
        assert(collection.GetReplacementSequence(market)==NA_BGM_SHOP);
    }
    values.erase(marketKey);assert(collection.GetReplacementSequence(market)==NA_BGM_SHOP);
    for(auto alias:{forest,market}) {
        values[collection.GetCvarKey(map.at(NA_BGM_SHOP).sfxKey)]=alias;
        assert(collection.GetReplacementSequence(NA_BGM_SHOP)==NA_BGM_SHOP);
    }
    char custom[]="custom/music/Scene Test_bgm";
    collection.AddToCollection(custom,0x1234);
#ifndef SCENE_MUSIC_MM
    values["gAudioEditor.HyruleFieldNightSequence"]=0x1234;
    assert(collection.GetReplacementSequence(night)==0x1234);
    values[collection.GetCvarKey(map.at(NA_BGM_SHOP).sfxKey)]=night;
    assert(collection.GetReplacementSequence(NA_BGM_SHOP)==NA_BGM_SHOP);
#endif
    values[forestKey]=0x1234;
    assert(collection.GetReplacementSequence(forest)==0x1234);
#ifdef SCENE_MUSIC_MM
    assert(collection.GetOriginalSequence(0x1234)==0x3e);
#endif
    char fanfare[]="custom/music/Scene Fanfare_fanfare";
    collection.AddToCollection(fanfare,0x1235);
#ifndef SCENE_MUSIC_MM
    for (int bad:{-1,0x1235,0xeeee,int(night)}) {
        values["gAudioEditor.HyruleFieldNightSequence"]=bad;
        assert(collection.GetReplacementSequence(night)==NA_BGM_KAKARIKO_ADULT);
    }
#endif
    values[marketKey]=0x1235;
    assert(collection.GetReplacementSequence(market)==NA_BGM_SHOP);
    std::puts("PASS native audio collection: independent scene rows, valid native defaults, isolated settings, invalid-value fallback and non-playable metadata keys");
}
