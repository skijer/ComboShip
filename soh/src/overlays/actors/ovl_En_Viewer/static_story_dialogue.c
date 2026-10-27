#include "static_story_dialogue.h"

#include <stddef.h>

typedef struct {
    uint16_t textId;
    const char* english;
} StaticStoryDialogueEntry;

static const StaticStoryDialogueEntry sDialogue[] = {
    { STATIC_STORY_TEXT_TREASURE_CHEST_SHOP_GAL, "Finding out what's inside is half the fun!" },
    { STATIC_STORY_TEXT_SKULL_KID, "That won't do you any good. Hee, hee." },
    { STATIC_STORY_TEXT_KEATON, "We Keatons can recognize our own by the sheen of our tails." },
    { STATIC_STORY_TEXT_HAPPY_MASK_SALESMAN, "You've met with a terrible fate, haven't you?" },
    { STATIC_STORY_TEXT_CHILD_KAFEI, "I've made a promise to Anju." },
    { STATIC_STORY_TEXT_LULU, "Pleased to meet you. I'm Lulu." },
    { STATIC_STORY_TEXT_ANJU, "...Kafei... I promised I'd wait for you." },
    { STATIC_STORY_TEXT_MONKEY, "Ook! Ook! The Woods of Mystery are just ahead!" },
    { STATIC_STORY_TEXT_CHAMBER_CHILD_ZELDA,
      "Do you hear it? The stones are humming... as if they remember your footsteps." },
    { STATIC_STORY_TEXT_CHAMBER_ADULT_ZELDA,
      "Every path leaves an echo in this chamber. Some wake to a song... others to a promise fulfilled." },
    { STATIC_STORY_TEXT_CHILD_ZELDA,
      "A fairy boy... I thought the forest kept its secrets. Did your fairy guide you here?" },
};

uint16_t StaticStoryDialogue_GetTextIdAt(size_t index) {
    return index < sizeof(sDialogue) / sizeof(sDialogue[0]) ? sDialogue[index].textId : 0;
}

const char* StaticStoryDialogue_GetText(uint16_t textId, uint8_t language) {
    /* All approved translations currently fall back to English. */
    (void)language;
    for (size_t i = 0; i < sizeof(sDialogue) / sizeof(sDialogue[0]); ++i) {
        if (sDialogue[i].textId == textId) {
            return sDialogue[i].english;
        }
    }
    return NULL;
}

bool StaticStoryDialogue_HandleOpenText(uint16_t textId, uint8_t language, bool* loadFromMessageTable,
                                        StaticStoryDialogueLoadFunc load, void* context) {
    const char* text;

    if (loadFromMessageTable == NULL || load == NULL) {
        return false;
    }
    text = StaticStoryDialogue_GetText(textId, language);
    if (text == NULL || !load(text, language, context)) {
        return false;
    }
    *loadFromMessageTable = false;
    return true;
}
