#ifndef SWORD_AURA_HELD_FIXTURE_H
#define SWORD_AURA_HELD_FIXTURE_H
#include "mods/extended_equipment.h"
#include "../../combo/render/sword_element_aura_identity.h"
// Resource boundaries only. The runner includes the actual successful held
// selector and native compositor bodies below; the submitted aura is production.
static int auraHeldGraph;
static int auraNativePieces;
static Gfx auraPiece[1];
uint8_t ResourceMgr_FileExists(const char* path) {
    if (strstr(path, "nei_held_swords/") || strstr(path, "nei_gi_redesign/")) return auraHeldGraph;
    return AuraLegacyFileExists(path);
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    if (strstr(path, "LeftHandClosed") || strstr(path, "LeftHandHolding")) return hand;
    return AuraLegacyLoadGfx(path);
}
uint8_t ResourceMgr_FileAltExists(const char* path) { return 0; }
int ResourceMgr_IsModAsset(const char* path) { return 0; }
int ResourceMgr_IsModAssetForGame(const char* game, const char* path) { return 0; }
void* MmAssets_LoadResource(const char* path) { return auraNativePieces ? auraPiece : NULL; }
u8 WeaponUpgrade_HasGilded(void) {
#ifdef SWORD_AURA_TEST_MM
    return kokiriUpgrade == 2;
#else
    return renderKokiriUpgrade == 2;
#endif
}
#ifdef SWORD_AURA_TEST_MM
#include "mods/nei_oot_compat.h"
#define NEI_EQUIPMENT_MM
int NeiResource_Available(const char* path) { return auraHeldGraph; }
int NeiResource_EnsureGiBaseOwner(void) { return auraHeldGraph; }
const char* NeiResource_Route(const char* path) { return path; }
u8 Player_IsCustomLinkModel(Player* owner) { return 0; }
u8 FourSword_HeldSwordDLForFrame(void** blade, void** handle, int frame) { return 0; }
static int sPlayerLod;
u16 gEquipMasks[] = { 0x000F, 0x00F0, 0x0F00, 0xF000 };
u8 gEquipShifts[] = { 0, 4, 8, 12 };
void ResourceMgr_PatchGfxByName(const char* path, const char* patch, int index, Gfx instruction) {}
#include "objects/object_link_child/object_link_child.h"
#else
#define HAND_COUNTER_SCALE_Y_OFFSET 100.0f
bool ResourceMgr_IsAltAssetsEnabled(void) { return alt; }
int32_t OOT_NeiEnsureGiBaseOwner(void) { return auraHeldGraph; }
int32_t OOT_NeiResourceExists(const char* path) { return auraHeldGraph; }
u8 Player_IsCustomLinkModel(void) { return 0; }
u8 FourSword_HeldSwordDL(void** blade, void** handle) { return 0; }
size_t ResourceGetSizeByName(const char* path) { return 5 * sizeof(Gfx); }
Mtx* Matrix_MtxFToMtx(MtxF* source, Mtx* dest) { memset(dest, 0, sizeof(*dest)); return dest; }
#endif
#include "../../combo/NeiHeldSword.h"
#include "aura_held_production.inc"
#endif
