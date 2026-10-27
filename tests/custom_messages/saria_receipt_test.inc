// The complete production receipt builder and native decoder are linked by the
// runner. Only inventory/save services, donor availability and texture loading
// are boundaries. Saria's actual catalog row, icon selector and grant body are
// inserted below.
#include "mods/extended_inventory.h"
#include "rando/CrossForeign.h"
#include "ComboSongDrawMM.h"
#include "ComboItemIconOwnership.h"
#include "assets/archives/icon_item_static/icon_item_static_yar.h"

extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
u8 gItemSlots[77]{};
u32 gUpgradeMasks[8]{};
u8 gUpgradeShifts[8]{};
NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
const NeiItem* Nei_FindByItem(int32_t) { return nullptr; }
const NeiItem* Nei_FindByRg(int16_t) { return nullptr; }
u8 Nei_BulletBagLevel() { return 0; }
uint8_t Wand_RandoMode() { return 0; }
u16 Player_GetItemReceiptTextId(s16, s16) { assert(false && "Saria must use her song receipt"); return 0; }
int FcCombo_ItemForNative(int) { return -1; }
TexturePtr gItemIcons[131]{};
extern s16 D_801CFF94[250];
void Message_StageCustomItemIcon(void*, s16);
void Message_StageCustomItemIconTint(void*, s16, s16, u8, u8, u8, u8);
}

static unsigned receiptGrants, receiptSaves, receiptHints, receiptStarts;
namespace Rando {
void GiveItem(RandoItemId id, RandoCheckId) { assert(id == RI_SONG_SARIA); ++receiptGrants; }
namespace StaticData {
/* SARIA_CATALOG */
const std::string& GetCheckDisplayName(RandoCheckId) { static const std::string empty; return empty; }
const char* GetIconTexturePath(RandoItemId) { return nullptr; }
/* SARIA_ICON_SELECTOR */
}
namespace MiscBehavior {
void SariasSongHint() { ++receiptHints; }
uint64_t ComboRandoGen() { return 0; }
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) { return nullptr; }
}
}
namespace CustomMessage {
void SetActiveCustomMessage(std::string msg, Entry entry) { entry.msg = std::move(msg); shownReceipt = entry; }
void StartTextbox(std::string msg, Entry entry) {
    ++receiptStarts;
    SetActiveCustomMessage(std::move(msg), std::move(entry));
}
}
void SaveManager_SaveCurrentForCombo() { ++receiptSaves; }

/* SARIA_GRANT */

static void TestSariaRewardReceipt() {
    customText = sariaReceipt = true;
    // Both queued cutscene and in-air/climbing overhead delivery must finish
    // the same receipt. Test every supported Latin locale and solo MM too.
    for (int mode : { COMBO_GAME_BOTH, COMBO_GAME_MM }) {
        gComboGameMode = mode;
        for (u8 language : { u8(LANGUAGE_ENG), u8(LANGUAGE_FRE), u8(LANGUAGE_GER) }) {
            for (bool cutscene : { false, true }) {
                gSaveContext = {};
                gSaveContext.fileNum = 1;
                gSaveContext.options.language = language;
                gItemIcons[ITEM_SONG_SONATA] = (TexturePtr)gItemIconSongNoteTex;
                Actor item{};
                item.home.rot.z = 1;
                item.home.rot.x = cutscene ? CustomItem::GIVE_ITEM_CUTSCENE : CustomItem::GIVE_OVERHEAD;
                const auto grants = receiptGrants, saves = receiptSaves, hints = receiptHints, starts = receiptStarts;
                // A queued reward cannot grant to a different save slot.
                ++item.home.rot.z;
                GiveSariaSong(&item, &play);
                assert(receiptGrants == grants && receiptSaves == saves);
                --item.home.rot.z;
                GiveSariaSong(&item, &play);
                GiveSariaSong(&item, &play);
                assert(receiptGrants == grants + 1 && receiptSaves == saves + 1 && receiptHints == hints + 1);
                assert(receiptStarts == starts + !cutscene);
                assert(gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed == 1);
                assert(!shownReceipt.autoFormat && shownReceipt.textboxType == TEXTBOX_TYPE_2 && shownReceipt.icon == 0xF5);

                play.msgCtx = {};
                // A missing terminator must not let the decoder escape into
                // interfaceCtx, as it did in the supplied Windows minidump.
                std::memset(&play.interfaceCtx, 0xA5, sizeof(play.interfaceCtx));
                const auto hud = play.interfaceCtx;
                char* textboxSegments[TEXTBOX_SEG_MAX]{};
                play.msgCtx.textboxSegment = textboxSegments;
                Message_OpenText(&play, CUSTOM_MESSAGE_ID);
                std::fprintf(stderr, "Saria receipt: mode=%d language=%u cutscene=%d bytes=%zu end=0x%02X\n",
                             mode, language, cutscene, shownReceipt.msg.size(), u8(shownReceipt.msg.back()));
                u8 ending;
                unsigned pages = 0;
                do {
                    assert(++pages <= 4);
                    Message_Decode(&play);
                    assert(!play.msgCtx.textIsCredits);
                    assert(play.msgCtx.msgMode == MSGMODE_TEXT_DISPLAYING);
                    assert(play.msgCtx.decodedTextLen < sizeof(play.msgCtx.decodedBuffer.schar));
                    ending = u8(play.msgCtx.decodedBuffer.schar[play.msgCtx.decodedTextLen]);
                    assert(ending == 0xBF || ending == 0x10);
                    assert(std::memcmp(&hud, &play.interfaceCtx, sizeof(hud)) == 0);
                    assert(textboxSegments[TEXTBOX_SEG_ICON] == gItemIcons[ITEM_SONG_SONATA]);
                    assert(play.msgCtx.unk12014 == 16);
                    // Wrap may add a page in French with the real 240px icon
                    // layout. Match Message_Update's acknowledged-page step;
                    // no textbox rendering or frame-driven input runs here.
                    if (ending == 0x10) {
                        ++play.msgCtx.msgBufPos;
                        assert(play.msgCtx.msgBufPos < play.msgCtx.msgLength);
                    }
                } while (ending != 0xBF);
                assert(u8(shownReceipt.msg.back()) == 0xBF);
                Message_CloseTextbox(&play);
                play.msgCtx.stateTimer = 1;
                assert(Message_GetState(&play.msgCtx) == TEXT_STATE_CLOSING);
            }
        }
    }
    play.interfaceCtx = {};
    gComboGameMode = COMBO_GAME_BOTH;
    sariaReceipt = false;
    puts("PASS Saria actual grant + receipt builder + native decoder: 12 mode/locale/delivery paths, HUD preserved, one saved grant");
}
