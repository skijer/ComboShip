#include "ChestContents.h"
#include "src/overlays/actors/ovl_En_Box/z_en_box.h"
#include "2s2h/BenPort.h"
#include "2s2h/Rando/Types.h"
#include <libultraship/bridge/consolevariablebridge.h>

#define MM_CHEST_MODEL(name)                                                 \
    static const ALIGN_ASSET(2) char sBody##name[] =                         \
        "__OTR__objects/object_box/cor_3ds_chests_mm/gChestBody" #name "DL"; \
    static const ALIGN_ASSET(2) char sLid##name[] = "__OTR__objects/object_box/cor_3ds_chests_mm/gChestLid" #name "DL"

MM_CHEST_MODEL(Major);
MM_CHEST_MODEL(Minor);
MM_CHEST_MODEL(Heart);
MM_CHEST_MODEL(SmallKey);
MM_CHEST_MODEL(Token);
MM_CHEST_MODEL(Junk);

static const struct {
    const char* body;
    const char* lid;
} sModels[RITYPE_MAX] = {
    [RITYPE_MAJOR] = { sBodyMajor, sLidMajor },           [RITYPE_LESSER] = { sBodyMinor, sLidMinor },
    [RITYPE_HEALTH] = { sBodyHeart, sLidHeart },          [RITYPE_SMALL_KEY] = { sBodySmallKey, sLidSmallKey },
    [RITYPE_SKULLTULA_TOKEN] = { sBodyToken, sLidToken }, [RITYPE_JUNK] = { sBodyJunk, sLidJunk },
};

static int MMChest_GetNativeItemType(s32 getItemId) {
    if (getItemId >= GI_MASK_DEKU && getItemId <= GI_MASK_KAFEIS_MASK) {
        return RITYPE_MASK;
    }
    switch (getItemId) {
        case GI_KEY_BOSS:
            return RITYPE_BOSS_KEY;
        case GI_KEY_SMALL:
            return RITYPE_SMALL_KEY;
        case GI_SKULL_TOKEN:
            return RITYPE_SKULLTULA_TOKEN;
        case GI_STRAY_FAIRY:
            return RITYPE_STRAY_FAIRY;
        case GI_HEART_PIECE:
        case GI_HEART_CONTAINER:
            return RITYPE_HEALTH;
        case GI_MAP:
        case GI_COMPASS:
        case GI_WALLET_GIANT:
        case GI_BOMB_BAG_30:
        case GI_BOMB_BAG_40:
        case GI_QUIVER_40:
        case GI_QUIVER_50:
        case GI_SWORD_RAZOR:
        case GI_SWORD_GILDED:
        case GI_SWORD_GREAT_FAIRY:
        case GI_MAGIC_BEANS:
        case GI_BOMBERS_NOTEBOOK:
        case GI_GOLD_DUST_2:
            return RITYPE_LESSER;
        case GI_BOMB_BAG_20:
        case GI_WALLET_ADULT:
        case GI_POWDER_KEG:
        case GI_QUIVER_30:
        case GI_ARROW_FIRE:
        case GI_ARROW_ICE:
        case GI_ARROW_LIGHT:
        case GI_SHIELD_HERO:
        case GI_SHIELD_MIRROR:
        case GI_SWORD_KOKIRI:
        case GI_HOOKSHOT:
        case GI_LENS_OF_TRUTH:
        case GI_PICTOGRAPH_BOX:
        case GI_OCARINA_OF_TIME:
        case GI_REMAINS_ODOLWA:
        case GI_REMAINS_GOHT:
        case GI_REMAINS_GYORG:
        case GI_REMAINS_TWINMOLD:
        case GI_POTION_RED_BOTTLE: // ComboShip/NEI's native Longshot slot is still a major reward.
        case GI_BOTTLE:
        case GI_MILK_BOTTLE:
        case GI_GOLD_DUST:
        case GI_CHATEAU_BOTTLE:
        case GI_MOONS_TEAR:
        case GI_DEED_LAND:
        case GI_DEED_SWAMP:
        case GI_DEED_MOUNTAIN:
        case GI_DEED_OCEAN:
        case GI_ROOM_KEY:
        case GI_LETTER_TO_MAMA:
        case GI_LETTER_TO_KAFEI:
        case GI_PENDANT_OF_MEMORIES:
            return RITYPE_MAJOR;
        case GI_RUPEE_GREEN:
        case GI_RUPEE_BLUE:
        case GI_RUPEE_10:
        case GI_RUPEE_RED:
        case GI_RUPEE_PURPLE:
        case GI_RUPEE_SILVER:
        case GI_RUPEE_HUGE:
        case GI_RECOVERY_HEART:
        case GI_MAGIC_JAR_SMALL:
        case GI_MAGIC_JAR_BIG:
        case GI_BOMBS_1:
        case GI_BOMBS_5:
        case GI_BOMBS_10:
        case GI_BOMBS_20:
        case GI_BOMBS_30:
        case GI_DEKU_STICKS_1:
        case GI_DEKU_NUTS_1:
        case GI_DEKU_NUTS_5:
        case GI_DEKU_NUTS_10:
        case GI_BOMBCHUS_1:
        case GI_BOMBCHUS_5:
        case GI_BOMBCHUS_10:
        case GI_BOMBCHUS_20:
        case GI_ARROWS_10:
        case GI_ARROWS_30:
        case GI_ARROWS_40:
        case GI_ARROWS_50:
        case GI_POTION_RED:
        case GI_POTION_GREEN:
        case GI_POTION_BLUE:
        case GI_FAIRY:
        case GI_MILK_HALF:
        case GI_FISH:
        case GI_BUG:
        case GI_BLUE_FIRE:
        case GI_POE:
        case GI_BIG_POE:
        case GI_SPRING_WATER:
        case GI_HOT_SPRING_WATER:
        case GI_CHATEAU:
        case GI_MILK:
            return RITYPE_JUNK;
        default:
            // Unknown GI slots and undisguised native ice traps keep their authored appearance.
            return RITYPE_MAX;
    }
}

float MMChest_PrepareDraw(EnBox* chest, PlayState* play) {
    chest->contentsBodyDL = NULL;
    chest->contentsLidDL = NULL;
    bool matchSize = CVarGetInteger("gEnhancements.ChestSizeMatchesContentsMM", 0);
    bool matchStyle = CVarGetInteger("gEnhancements.ChestStyleMatchesContentsMM", 0) ||
                      (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO && CVarGetInteger("gRando.CSMC", 0));
    if ((!matchSize && !matchStyle) || play->sceneId == SCENE_TAKARAYA) {
        return 1.0f;
    }

    int category = MMChest_GetRandoItemType(chest);
    if (category < 0) {
        category = MMChest_GetNativeItemType(chest->getItemId);
    }
    if (category < 0 || category >= RITYPE_MAX) {
        return 1.0f;
    }

    float scale = 1.0f;
    if (matchSize) {
        bool small = category == RITYPE_JUNK || category == RITYPE_SMALL_KEY || category == RITYPE_SKULLTULA_TOKEN;
        scale = (small ? 0.0075f : 0.01f) / (Actor_IsSmallChest(chest) ? 0.0075f : 0.01f);
    }

    // The approved materials are opaque. Native materials retain the fade/Lens render modes.
    bool lensPass = (chest->type == ENBOX_TYPE_BIG_INVISIBLE || chest->type == ENBOX_TYPE_SMALL_INVISIBLE) &&
                    (chest->dyna.actor.flags & ACTOR_FLAG_REACT_TO_LENS);
    const char* body = sModels[category].body;
    const char* lid = sModels[category].lid;
    bool supportedPass = chest->alpha == 255 && !lensPass;
    if (category == RITYPE_BOSS_KEY) {
        // Boss keys use the active native pack (including Djipi), even in an authored small chest.
        // Native ornate materials already support the actor's fade and Lens render-mode segment.
        body = gBoxChestBaseOrnateDL;
        lid = gBoxChestLidOrnateDL;
        supportedPass = true;
    }
    if (matchStyle && supportedPass && body != NULL && lid != NULL && ResourceMgr_FileExists(body) &&
        ResourceMgr_FileExists(lid) && ResourceMgr_LoadGfxByName(body) != NULL &&
        ResourceMgr_LoadGfxByName(lid) != NULL) {
        // Keep stable resource names only; archive reloads must never reuse a retained raw display-list pointer.
        chest->contentsBodyDL = body;
        chest->contentsLidDL = lid;
    }
    return scale;
}
