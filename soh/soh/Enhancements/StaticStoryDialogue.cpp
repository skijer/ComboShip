#include <soh/OTRGlobals.h>
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <vector>

extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_dialogue.h"
#include "variables.h"
}

static bool LoadStaticStoryDialogueIntoFont(const char* text, uint8_t language, void* context) {
    (void)language;
    (void)context;
    CustomMessage message(text, text, text);

    message.AutoFormat();
    message.LoadIntoFont();
    return true;
}

void StaticStoryDialogue_OnOpenText(uint16_t* textId, bool* loadFromMessageTable) {
    if (textId == nullptr) {
        return;
    }
    StaticStoryDialogue_HandleOpenText(*textId, gSaveContext.language, loadFromMessageTable,
                                       LoadStaticStoryDialogueIntoFont, nullptr);
}

void StaticStoryDialogue_Register() {
    static std::vector<HOOK_ID> hookIds;
    for (HOOK_ID hookId : hookIds) {
        GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnOpenText>(hookId);
    }
    hookIds.clear();
    // Register from the payload catalog so a new actor cannot fall through to
    // a native message-table ID which has no backing text.
    for (size_t index = 0;; ++index) {
        const uint16_t textId = StaticStoryDialogue_GetTextIdAt(index);
        if (textId == 0) {
            break;
        }
        hookIds.push_back(GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            textId, StaticStoryDialogue_OnOpenText));
    }
}

static RegisterShipInitFunc initFunc(StaticStoryDialogue_Register);
