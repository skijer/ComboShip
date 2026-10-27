"""Compile the GI policy and production renderer against this checkout's real headers."""
import os
import json
import math
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import sys
import struct
import xml.etree.ElementTree as ET
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-I" + str(ROOT)]
# Both native CMake targets expose shared bridges, including their standalone stubs.
flags += ["-I" + str(ROOT / p) for p in
          ("soh", "soh/include", "soh/src", "soh/assets", "soh/mods", "libultraship/include", "combo", "combo/menu")]
for config in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / config).read_text()):
        flags.append(f'-D{key}="{value}"')
cc = os.environ.get("CXX", "c++")
if "--sword-toggle-only" in sys.argv:
    flags.append("-DSWORD_TOGGLE_REGRESSION_ONLY")
if "--sword-regressions-only" in sys.argv:
    flags.append("-DSWORD_REGRESSIONS_ONLY")
sanitize = ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if "--sanitize" in sys.argv else []
if "--fast-math" in sys.argv:
    sanitize.append("-ffast-math")
with tempfile.TemporaryDirectory(prefix="nei-gi-tests-") as tmp:
    graph = functions((ROOT / "soh/src/code/graph.c").read_text())
    (Path(tmp) / "nei_gi_graph.inc").write_text(graph["Graph_OpenDisps"] + "\n" + graph["Graph_CloseDisps"])
    draw = functions((ROOT / "soh/src/code/z_draw.c").read_text())
    player = functions((ROOT / "soh/src/code/z_player_lib.c").read_text())
    shop = functions((ROOT / "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c").read_text())
    custom = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text())
    (Path(tmp) / "nei_gi_dispatch.inc").write_text(draw["GetItemEntry_Draw"] + "\n" +
        re.sub(r"\bthis\b", "player", player["Player_DrawGetItemImpl"]) + "\n" +
        re.sub(r"\bthis\b", "shop", shop["EnGirlA_Draw"]) + "\n" +
        custom["Randomizer_DrawCaneSomariaUpgradeFlame"] + "\n" + custom["Randomizer_DrawTrueMasterSwordFlame"])
    fixtures = []
    bindings = (("ball_and_chain", "BallAndChain"), ("shovel", "Shovel"),
                           ("fire_rod", "FireRod"), ("ice_rod", "IceRod"), ("light_rod", "LightRod"),
                           ("hylia_grace", "HyliaGrace"), ("zonai_permafrost", "ZonaiPermafrost"),
                           ("demise_destruction", "DemiseDestruction"), ("time_gate", "TimeGate"),
                           ("switch_hook", "SwitchHook"), ("rocs_feather", "RocsFeatherSkijer"),
                           ("rocs_feather", "RocsFeather"), ("spinner", "Spinner"),
                           ("cane_of_somaria", "CaneOfSomaria"), ("cane_of_somaria", "CaneSomariaUpgrade"),
                           ("minish_cap", "MinishCap"), ("rocs_cape", "RocsCape"))
    bindings += (
        ("divine_shield", "ExtDivineShield"), ("sheikah_shield", "ExtSheikahShield"),
        ("shield_of_ikana", "ExtShieldOfIkana"), ("magic_cape", "ExtMagicCape"),
        ("spirit_breastplate", "ExtSpiritBreastplate"), ("sages_tunic", "ExtSagesTunic"),
        ("champions_tunic", "ExtChampionsTunic"), ("pegasus_anklet", "ExtPegasusAnklet"),
        ("trident", "ExtTrident"), ("climb_boots", "ExtClimbBoots"), ("roc_boots", "ExtRocBoots"),
        ("cane_of_byrna", "ExtCaneOfByrna"), ("four_sword", "ExtFourSword"),
        ("pendant_of_memories", "ExtPendantOfMemories"), ("elemental_wand", "ElementalWand"),
        ("room_key", "MmTradeQuest", "RG_MM_ROOM_KEY"),
        ("sand_rod", "ElementalWand", "RG_WAND_SAND_ROD"),
        ("tornado_rod", "ElementalWand", "RG_WAND_TORNADO_ROD"),
        ("water_rod", "ElementalWand", "RG_WAND_WATER_ROD"),
        ("meteor_rod", "ElementalWand", "RG_WAND_METEOR_ROD"),
        ("storm_rod", "ElementalWand", "RG_WAND_STORM_ROD"),
        ("shadow_scepter", "ElementalWand", "RG_WAND_SHADOW_SCEPTER"),
        ("sheikah_slate", "NeiSheikahSlate"), ("slate_bomb", "SlateRuneBomb"),
        ("slate_master_cycle", "SlateRuneMasterCycle"), ("slate_stasis", "SlateRuneStasis"),
        ("slate_cryonis", "SlateRuneCryonis"), ("slate_sensor", "SlateRuneSensor"),
        ("phantom_hourglass", "NeiPhantomHourglass"), ("shadow_crystal", "NeiShadowCrystal"),
        ("rod_of_seasons", "NeiRodOfSeasons"), ("kokiri_sword", "ProgressiveKokiriSword"),
        ("razor_sword", "RazorSword"), ("gilded_sword", "GildedSword"),
        ("master_sword", "MasterSword"), ("true_master_sword", "TrueMasterSword"),
        ("biggoron_sword", "ProgressiveBGS"), ("giants_knife", "GiantsKnife"), ("great_fairy_sword", "GreatFairySword"),
        ("iron_knuckle_axe", "IronKnuckleAxe"))
    for binding in bindings:
        slug, callback = binding[:2]
        identity = binding[2] if len(binding) > 2 else "0"
        root = ROOT / "soh/assets/custom/objects/nei_gi_redesign" / slug
        # Include the translucent shell: it is lower than the opaque spell core.
        vertices = [tuple(int(v.get(axis)) for axis in ("X", "Y", "Z"))
                    for path in root.glob("mesh_*_vtx") for v in ET.parse(path).getroot()]
        words = struct.unpack_from("<16I", (root / "scale_mtx").read_bytes(), 64)
        scale = ((words[0] >> 16) * 65536 + (words[8] >> 16)) / 65536
        radius = max(math.hypot(p[0], p[2]) for p in vertices) * scale
        meta = json.loads((ROOT / "tools/nei_gi/CHECKPOINTS" / slug / "checkpoint.json").read_text())
        low = ", ".join(f"{min(p[axis] for p in vertices)*scale}f" for axis in range(3))
        high = ", ".join(f"{max(p[axis] for p in vertices)*scale}f" for axis in range(3))
        fixtures.append(f'{{Randomizer_Draw{callback}, "{slug}", {json.dumps(meta["name"])}, "{callback}", '
                        f'{{{low}}}, {{{high}}}, {2*radius}f, {float(meta["draw_scale"])}f, '
                        f'{str((root / "gi_xlu_dl").exists()).lower()}, {identity}}},')
    (Path(tmp) / "nei_gi_bounds.inc").write_text("\n".join(fixtures))
    # Every bundled model is checked, including the eight formerly omitted
    # catalog entries and both real-engine owners of the Kokiri GI.
    all_frames = []
    pickup_vertices = []
    for asset in sorted((ROOT / "soh/assets/custom/objects/nei_gi_redesign").iterdir()):
        if not asset.is_dir():
            continue
        vertices = [tuple(int(v.get(axis)) for axis in ("X", "Y", "Z"))
                    for path in asset.glob("mesh_*_vtx") for v in ET.parse(path).getroot()]
        words = struct.unpack_from("<16I", (asset / "scale_mtx").read_bytes(), 64)
        scale = ((words[0] >> 16) * 65536 + (words[8] >> 16)) / 65536
        meta = json.loads((ROOT / "tools/nei_gi/CHECKPOINTS" / asset.name / "checkpoint.json").read_text())
        low = min(p[1] for p in vertices)*scale
        high = max(p[1] for p in vertices)*scale
        width = 2*max(math.hypot(p[0],p[2]) for p in vertices)*scale
        for point in sorted(set(vertices)):
            pickup_vertices.append(asset.name + ' ' + ' '.join(str(v*scale) for v in point))
        all_frames.append(f'{{"{asset.name}",{low}f,{high}f,{width}f,{float(meta["draw_scale"])}f,'
                          f'{str((asset/"gi_xlu_dl").exists()).lower()}}},')
    assert len(all_frames) == 63
    (Path(tmp) / "nei_all_frame_bounds.inc").write_text("\n".join(all_frames))
    pickup_vertex_path = Path(tmp) / "mm_pickup_vertices.txt"
    pickup_vertex_path.write_text("\n".join(pickup_vertices))
    names = ["nei_gi/effect_policy", "nei_gi/presentation"]
    if "--held" in sys.argv:
        names.append("nei_held/presentation")
        names.extend(("nei_gi/lantern_policy", "nei_gi/lantern_presentation"))
    for name in names:
        source = ROOT / "tests" / (name + "_test.cpp")
        out = str(Path(tmp) / name.replace("/", "_"))
        extra_flags = []
        if name == "nei_gi/presentation" and "--combo" in sys.argv:
            mm_source = (ROOT / "mm/2s2h/Rando/NeiGiPresentation.cpp").read_text()
            mm_functions = functions(mm_source)
            renderer = functions((ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text())["DrawSong"] + "\n" + mm_functions["HasMmLegacyGiMod"] + "\n" + mm_functions["GetSelectedOwnerGi"] + "\n" + mm_functions["MM_DrawNeiGi"] + "\n" + mm_functions["MM_DescribeNeiGi"] + "\n" + mm_functions["MM_TryDrawNeiGi"]
            # MM's item enum and binding table are copied verbatim so origin
            # selection is exercised without importing the unrelated MM engine.
            mm_types = (ROOT / "mm/2s2h/Rando/Types.h").read_text()
            item_enum = re.search(r"typedef enum \{\s*RI_UNKNOWN,.*?\} RandoItemId;", mm_types, re.S)[0]
            bindings = re.search(r"struct Binding \{.*?const Binding kBindings\[\] = \{.*?\n\};", mm_source, re.S)[0]
            fallback = mm_source[mm_source.index("MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer"):]
            fallback_class = (ROOT / "mm/2s2h/Rando/NeiGiPresentation.h").read_text()
            fallback_class = fallback_class[fallback_class.index("class MM_NeiGiFallbackShimmer"):]
            foreign_source = (ROOT / "combo/menu/ComboForeignDrawOOT.h").read_text()
            foreign_info = re.search(r"struct ComboForeignDrawInfo \{.*?\n\};", foreign_source, re.S)[0]
            foreign_draw = functions(foreign_source)["OOT_DrawComboForeign"]
            mirror_helper = functions((ROOT / "combo/menu/ComboMagicMirrorForeign.h").read_text())["ComboDrawMagicMirrorForeign"]
            foreign_draw = ('extern "C" void MagicMirror_DrawGI(PlayState*) { assert(false && "NEI dispatch selected Mirror"); }\n'
                            + mirror_helper + '\n' + foreign_draw)
            if "--baseline-gold-overlays" in sys.argv:
                foreign_draw=foreign_draw.replace("Kind::Gold","Kind::MarioMask")
            foreign_shop = functions(foreign_source.replace('extern "C" ', ''))["OOT_DrawComboForeignShop"]
            foreign_wrapper = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text()
                                        .replace('extern "C" ', ''))["Randomizer_DrawComboForeign"]
            routing_source = (ROOT / "mm/2s2h/Rando/NeiResourceRouting.cpp").read_text()
            route = routing_source[routing_source.index('extern "C" const char* NeiResource_Route'):]
            shim = """
#include <algorithm>
#include <unordered_set>
#include "ComboMaskShimmer.h"
#include "ComboCollectibleShimmerDraw.h"
extern "C" { PlayState* gPlayState = &Fixture::play; }
bool ownerAlt = false;
extern "C" int32_t OOT_NeiAltAssetsEnabled(void) { return ownerAlt; }
extern "C" int32_t OOT_NeiEnsureGiBaseOwner(void) { return 1; }
extern "C" int32_t OOT_NeiResourceExists(const char* path) {
    if(path && !std::strncmp(path,"__OTR__@oot-gi-base:",20))
        return ResourceMgr_FileExists((std::string("__OTR__")+(path+20)).c_str());
    return path && (ResourceMgr_FileExists(path) || (ownerAlt && ResourceMgr_FileAltExists(path)));
}
/* SELECTED_SWORD_PRODUCER */
int32_t SelectedSwordFixture(const char* name,CwItemDrawInfo* out) {
    const std::pair<const char*,RandomizerGet> names[] = {
      {"Kokiri Sword",RG_KOKIRI_SWORD},{"Razor Sword",RG_RAZOR_SWORD},
      {"Gilded Sword",RG_GILDED_SWORD},{"Master Sword",RG_MASTER_SWORD},
      {"True Master Sword",RG_TRUE_MASTER_SWORD},{"Biggoron's Sword",RG_BIGGORON_SWORD},
      {"Great Fairy's Sword",RG_GREAT_FAIRY_SWORD}};
    for(auto [title,item]:names)if(!std::strcmp(title,name))return CwAltSwordGi(item,out);
    if(!std::strcmp(name,"Lantern"))
        return CwCustomGi(out,"__OTR__objects/object_poh/gPoeLanternDL",.025f);
    return 0;
}
void* Combo_ResolveSym(const char* owner, const char* name) {
    assert(std::strcmp(owner, "soh") == 0);
    if (std::strcmp(name, "OOT_GetNeiGiDrawInfo") == 0)
        return reinterpret_cast<void*>(OOT_GetNeiGiDrawInfo);
    if (std::strcmp(name, "OOT_GetNeiGiDrawInfoForAssets") == 0)
        return reinterpret_cast<void*>(OOT_GetNeiGiDrawInfoForAssets);
    if (std::strcmp(name, "OOT_NeiResourceExists") == 0)
        return reinterpret_cast<void*>(OOT_NeiResourceExists);
    if (std::strcmp(name, "OOT_GetItemDrawInfo") == 0)
        return reinterpret_cast<void*>(SelectedSwordFixture);
    assert(false && "unexpected bridge symbol");
    return nullptr;
}
void DrawOotSlateRuneFlame(u8 r, u8 g, u8 b) { Fixture::flameColors.push_back({r,g,b}); }
void MM_DrawNeiGi(const CwItemDrawInfo&,bool shop=false,int mmPickup=0);
#define Gfx_SetupDL25_Opa Gfx_SetupDL_25Opa
#define Gfx_SetupDL25_Xlu Gfx_SetupDL_25Xlu
#define Matrix_RotateYF Matrix_RotateY
#define Matrix_RotateZF Matrix_RotateZ
#define Matrix_RotateXF Matrix_RotateX
#define MATRIX_FINALIZE_AND_LOAD(pkt, gfx) gSPMatrix(pkt, Matrix_NewMtx(gfx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)
"""
            owner_functions = functions((ROOT / "combo/menu/ComboItemDrawOOT.h").read_text())
            shim = shim.replace("/* SELECTED_SWORD_PRODUCER */", "\n".join(
                owner_functions[name] for name in ("CwSimple", "CwCustomGi", "CwAltSwordGi")))
            spell_producer = functions((ROOT / "combo/menu/ComboItemDrawMM.h").read_text())["MM_FillElementalSpellDrawInfo"]
            foreign_shim = """
const ComboForeignDrawInfo* selectedForeignInfo = nullptr;
int foreignFallbackCalls = 0;
RandomizerCheck OOT_GetQueuedDrawCheck() { return RC_UNKNOWN_CHECK; }
const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(RandomizerCheck) { return selectedForeignInfo; }
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*, const char*, PlayState*) { return false; }
#define FOREIGN_DRAW_STUB(name) \\
    void name(PlayState*, const ComboForeignDrawInfo*) { ++foreignFallbackCalls; Matrix_Scale(7,7,7,MTXMODE_APPLY); }
FOREIGN_DRAW_STUB(OOT_DrawForeignGoronSword)
void OOT_DrawForeignMasterSword(PlayState*, const ComboForeignDrawInfo*, bool=true) { ++foreignFallbackCalls; Matrix_Scale(7,7,7,MTXMODE_APPLY); }
void OOT_DrawForeignCustomGi(PlayState*, const ComboForeignDrawInfo*, bool, bool=true) { ++foreignFallbackCalls; Matrix_Scale(7,7,7,MTXMODE_APPLY); }
void OOT_DrawForeignWeaponFlame(PlayState*, const uint8_t color[4]) { Fixture::flameColors.push_back({color[0],color[1],color[2]}); }
FOREIGN_DRAW_STUB(OOT_DrawForeignDekuNuts)
FOREIGN_DRAW_STUB(OOT_DrawForeignRecoveryHeart)
FOREIGN_DRAW_STUB(OOT_DrawForeignFish)
FOREIGN_DRAW_STUB(OOT_DrawForeignPotion)
FOREIGN_DRAW_STUB(OOT_DrawForeignBlueFire)
FOREIGN_DRAW_STUB(OOT_DrawForeignPoes)
FOREIGN_DRAW_STUB(OOT_DrawForeignFairyBottle)
bool OOT_DrawForeignFairyContainer(PlayState*, const ComboForeignDrawInfo*) { ++foreignFallbackCalls; return true; }
FOREIGN_DRAW_STUB(OOT_DrawForeignSoulFlame)
FOREIGN_DRAW_STUB(OOT_DrawForeignOps)
#undef FOREIGN_DRAW_STUB
void ComboDrawSpinAttackGi(PlayState*, const char*, const char*, float, const uint8_t[4], const char*) {
    ++foreignFallbackCalls;
}
"""
            # Song tests submit the actual portable note, while unrelated
            # fallback families retain this fixture's original boundary stub.
            oot_song_simple = functions(foreign_source)["OOT_DrawForeignSimple"].replace(
                "OOT_DrawForeignSimple(", "Fixture_DrawForeignSongSimple(", 1)
            foreign_shim += "\n" + foreign_source[foreign_source.index("#define COMBO_FOREIGN_MTX("):
                                                    foreign_source.index("// Biggoron's Sword:")] + """
constexpr int kMaxMatEntries=16;
bool ComboForeignTexAnim_Run(PlayState*,const char*,const char*,bool,int32_t*,int32_t*) {assert(false);return false;}
void ComboForeignTexAnim_Restore(PlayState*,const int32_t*,int32_t,bool) {assert(false);}
bool spellFallbackAllowed=false;
extern "C" Gfx* Gfx_TwoTexScrollEx(GraphicsContext* gfx,s32,u32,u32,s32,s32,s32,u32,u32,s32,s32,s32,s32,s32,s32) {
    assert(spellFallbackAllowed);return static_cast<Gfx*>(Graph_Alloc(gfx,12*sizeof(Gfx)));
}
extern "C" void gSPSegment(void* dst,int segment,uintptr_t address) {
    assert(spellFallbackAllowed);auto* cmd=static_cast<Gfx*>(dst);
    cmd->words.w0=(G_MOVEWORD<<24)|(G_MW_SEGMENT<<16)|segment*4;cmd->words.w1=address;
}
""" + functions(foreign_source)["OOT_RestoreForeignSegs"] + "\n" + functions(foreign_source)["OOT_DrawForeignMagicSpell"] + "\n" + oot_song_simple + """
void OOT_DrawForeignSimple(PlayState* play,const ComboForeignDrawInfo* info) {
    if(info->drawKind==CW_DRAW_KIND_SONG_GI || info->drawKind==CW_DRAW_KIND_ELEMENTAL_ARROW) Fixture_DrawForeignSongSimple(play,info);
    else {++foreignFallbackCalls;Matrix_Scale(7,7,7,MTXMODE_APPLY);}
}
"""
            mm_foreign_source = (ROOT / "combo/menu/ComboForeignDrawMM.h").read_text()
            mm_foreign_info = re.search(r"struct ComboForeignDrawInfoOOT \{.*?\n\};",mm_foreign_source,re.S)[0]
            mm_foreign_draw = functions(mm_foreign_source)["MM_DrawComboForeign"]
            if "--baseline-gold-overlays" in sys.argv:
                mm_foreign_draw=mm_foreign_draw.replace("Kind::Gold","Kind::MarioMask")
            mm_handlers = sorted(set(re.findall(r"\b(MM_DrawForeign\w+)\(", mm_foreign_draw)))
            mm_shop_support = """
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
struct FixtureNeiSaveData { uint8_t comboObtained[FC_COMBO_OBTAINED_SIZE] = {}; };
FixtureNeiSaveData fixtureNeiSaveData;
FixtureNeiSaveData* Nei_Save() { return &fixtureNeiSaveData; }
using RandoCheckId=int;
constexpr RandoCheckId RC_UNKNOWN=0;
struct MmShopEnGirlA { Actor actor; s16 rotY; };
struct MmShopSaveCheck { RandoItemId randoItemId=RI_NONE; };
MmShopSaveCheck mmShopChecks[8];
#define RANDO_SAVE_CHECKS mmShopChecks
void Matrix_RotateYS(s16, u8) {}
void func_800B8118(Actor*,PlayState*,int) {}
void func_800B8050(Actor*,PlayState*,int) {}
int DungeonItem_GetOwner(RandoItemId) {return -1;}
bool GetItem_DrawDungeonItem(PlayState*,s16,int) {assert(false);return false;}
int mmShopLegacyDraws;
namespace Rando {
void DrawItem(RandoItemId,RandoCheckId,Actor*);
void DrawResolvedItem(RandoItemId,RandoCheckId,Actor*);
RandoItemId ConvertItem(RandoItemId item,RandoCheckId) {return item;}
namespace StaticData {struct FixtureItem {s16 drawId;};FixtureItem Items[RI_MAX];}
}
void DrawOotNeiUltrahand() {assert(false);}
void DrawOotNeiCaneOfSomaria(RandoItemId) {assert(false);}
""" + mm_foreign_info + """
const ComboForeignDrawInfoOOT* selectedForeignInfoMM=nullptr;
const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(RandoCheckId) {return selectedForeignInfoMM;}
""" + "\n".join(("bool " if name == "MM_DrawForeignSilverRupee" else "void ")+name+"(const ComboForeignDrawInfoOOT*" +
                    (", bool" if name in {"MM_DrawForeignCustomGi", "MM_DrawForeignMasterSword"} else "") +
                    ") {assert(false);" + ("return false;" if name == "MM_DrawForeignSilverRupee" else "") + "}" for name in mm_handlers
                    if name not in {"MM_DrawForeignMusicNote", "MM_DrawForeignSimple", "MM_DrawForeignCustomGi"})
            mm_pin = mm_foreign_source[mm_foreign_source.index("#define MM_FOREIGN_PIN_OPA()"):
                                       mm_foreign_source.index("// Restore the segments a handler bound")]
            mm_shop_support += "\n" + mm_pin + "\n" + """
Gfx* MM_DrawForeignMagicJarDList(Gfx*,const char*,const uint8_t*) {assert(false);return nullptr;}
""" + functions(mm_foreign_source)["MM_DrawForeignSimple"] + "\n" + functions(mm_foreign_source)["MM_DrawForeignMusicNote"] + "\n" + functions(mm_foreign_source)["MM_DrawForeignCustomGi"]
            mm_draw_source=(ROOT/"mm/2s2h/Rando/DrawItem.cpp").read_text()
            # The receipt is a CustomItem actor, not Player_DrawGetItemImpl.
            # Execute its actual visible pose statements before the queue's
            # draw callback scale so camera framing probes use the active path.
            custom_source=(ROOT/"mm/2s2h/CustomItem/CustomItem.cpp").read_text()
            pickup_start=custom_source.index('                actor->shape.yOffset = 900.0f;')
            pickup_end=custom_source.index('                actor->world.pos.y += height;',pickup_start)+len('                actor->world.pos.y += height;')
            pose_body=custom_source[pickup_start:pickup_end].replace('GET_PLAYER_FORM','form').replace('Actor_SetScale','FixtureActorScale')
            form_enum=re.search(r'typedef enum PlayerTransformation \{.*?\} PlayerTransformation;',
                                (ROOT/'mm/include/z64player.h').read_text(),re.S)[0]
            queue_source=(ROOT/'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
            queue_scale=re.search(r'Matrix_Scale\(30\.0f, 30\.0f, 30\.0f, MTXMODE_APPLY\);',queue_source)[0]
            camera_source=(ROOT/'mm/src/code/z_camera_data.inc').read_text()
            # Use Camera_KeepOn4's actual transformation selector and Item0
            # table, including the forward target offsets omitted by the old
            # human-only test. Player_GetHeight supplies the upright heights.
            camera_function=functions((ROOT/'mm/src/code/z_camera.c').read_text())['Camera_KeepOn4']
            item_modes=re.search(r'CameraMode sCamSetItem0Modes\[\] = \{(.*?)\n\};',camera_source,re.S)[1]
            mode_rows=dict((mode,row) for row,mode in re.findall(
                r'CAM_SETTING_MODE_ENTRY\(CAM_FUNC_KEEPON4, (\w+)\),\s*// (\w+)',item_modes))
            height_function=re.search(r'f32 Player_GetHeight\(Player\* player\) \{.*?\n\}',
                                      (ROOT/'mm/src/code/z_actor.c').read_text(),re.S)[0]
            camera_rows=[]
            forms=re.findall(r'/\* \d+ \*/ (PLAYER_FORM_\w+),',form_enum)
            for form in forms:
                selected=re.search(r'case '+form+r':\s*camMode = (\w+);',camera_function)
                mode=selected[1] if selected else 'CAM_MODE_NORMAL'
                values=re.search(r'CameraModeValue '+mode_rows[mode]+r'\[\] = \{\s*CAM_FUNCDATA_KEEP4\(([^\n]+)',camera_source)[1].split(',')
                def number(value):
                    return float(int(value.strip(),0)) if '0x' in value else float(value)
                camera_values=[number(values[i]) for i in (0,1,2,5,4)]
                height_case=re.search(r'case '+form+r':.*?return ([^;]+);',height_function,re.S)
                height_values=re.findall(r'(\d+)\.0f',height_case[0])
                camera_rows.append('{'+','.join(str(v)+'f' for v in [float(height_values[-1]),*camera_values])+'}')
            pickup_support='''\nvoid FixtureActorScale(Actor* actor,float s) {actor->scale.x=actor->scale.y=actor->scale.z=s;}
std::pair<float,float> FixtureMmReceiptPose(int form=PLAYER_FORM_HUMAN) {
 fixtureMmForm=form;
 Actor storage{};Actor* actor=&storage;
'''+pose_body+'''\n Fixture::matrix=actor->scale.x;
 Fixture::matrixY=actor->world.pos.y+actor->shape.yOffset*actor->scale.y;
'''+queue_scale+'''\n return {Fixture::matrix,Fixture::matrixY};
}\n'''+f'constexpr float kMmPickupCamera[PLAYER_FORM_MAX][6]={{{",".join(camera_rows)}}};\n'+'''
std::map<std::string,std::vector<std::array<float,3>>> FixtureMmPickupVertices() {
 std::ifstream input('''+json.dumps(str(pickup_vertex_path))+''');
 assert(input);
 std::map<std::string,std::vector<std::array<float,3>>> vertices;
 std::string slug;std::array<float,3> point{};
 while(input>>slug>>point[0]>>point[1]>>point[2])vertices[slug].push_back(point);
 assert(input.eof() && vertices.size()==63);
 return vertices;
}
'''
            # Keep the exact outer item conversion/context/early dispatcher and
            # actual foreign branch; unrelated switch bodies have own fixtures.
            resolved = mm_draw_source[mm_draw_source.index("void Rando::DrawResolvedItem("):]
            prefix = resolved[:resolved.index("    switch (randoItemId) {")]
            foreign_case = re.search(r"        case RI_COMBO_FOREIGN:.*?            break;", resolved,re.S)[0]
            resolved = prefix + "    switch(randoItemId) {\n" + foreign_case + "\n        default: ++mmShopLegacyDraws;break;\n    }\n}"
            draw_item = mm_draw_source[mm_draw_source.index("void Rando::DrawItem("):mm_draw_source.index("void Rando::DrawResolvedItem(")]
            callback = functions((ROOT/"mm/2s2h/Rando/ActorBehavior/EnGirlA.cpp").read_text())["EnGirlA_RandoDrawFunc"].replace("EnGirlA*","MmShopEnGirlA*")
            mm_shop_support += "\n" + mm_foreign_draw + "\n" + resolved + "\n" + draw_item + "\n" + callback + "\n#undef RANDO_SAVE_CHECKS\n"
            flags_source=(ROOT/'mm/2s2h/CustomItem/CustomItem.h').read_text()
            custom_flags='namespace CustomItem {\n'+re.search(r'enum CustomItemFlags.*?\n\};',flags_source,re.S)[0]+'\n}\n'
            tested_bridge = (form_enum + '\nint fixtureMmForm=PLAYER_FORM_HUMAN;\n#define GET_PLAYER_FORM fixtureMmForm\n' + custom_flags + shim + route + "\n" + item_enum + "\n" + spell_producer + "\nbool MM_TryDrawNeiGi(RandoItemId,bool shop=false,int mmPickup=0);\n#include \"ComboSongDrawMM.h\"\n" + bindings + "\n" + fallback_class + "\n" +
                             renderer + "\n" + fallback + "\n" + foreign_info + "\n" +
                             foreign_shim + foreign_draw + "\n" + foreign_wrapper + "\n" + foreign_shop + "\n" + mm_shop_support)
            tested_bridge = ('#include "ComboBottleContents.h"\n'
                             'int ComboBottleContents_Draw(PlayState*,int) {assert(false);return 0;}\n' + tested_bridge)
            # The production renderer now includes the real runtime resolver.
            # Keep this fixture's donor simulator distinct from that inline API.
            tested_bridge = tested_bridge.replace("Combo_ResolveSym(", "Fixture_ResolveSym(")
            candidate = source.read_text().replace("int main() {", tested_bridge + "\n" + pickup_support + "\nint main() {", 1)
            checks = (ROOT / "tests/mm_presentation/gi_bridge_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/shop_dispatch_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/foreign_sword_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/gold_shimmer_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/legacy_mod_checks.inc").read_text()
            checks += (ROOT / "tests/song_gi/foreign_dispatch_checks.inc").read_text()
            checks += (ROOT / "tests/elemental_arrow_gi/foreign_dispatch_checks.inc").read_text()
            checks += (ROOT / "tests/mm_presentation/selected_sword_checks.inc").read_text()
            candidate = candidate.replace("  using namespace Fixture;\n", "  using namespace Fixture;\n" + checks, 1)
            source = Path(tmp) / "combo_gi_presentation.cpp"
            source.write_text("#define COMBO_BUILD 1\n" + candidate)
            extra_flags = ["-I" + str(ROOT), "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "combo/menu")]
        subprocess.run([cc, *flags, *sanitize, *extra_flags, "-I" + tmp, str(source), "-o", out], check=True)
        subprocess.run([out], check=True, env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})

# Check the actual C dispatch boundary, using the same CVar definitions as CMake.
cflags = ["-std=gnu2x", "-fsyntax-only", "-Werror=implicit-function-declaration",
          "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-pointer-to-int-cast"]
cflags += flags[1:]
sources = ["soh/src/code/z_draw.c", "soh/src/code/z_player_lib.c",
           "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c"]
if "--held" in sys.argv:
    sources += ["soh/src/overlays/actors/ovl_player_actor/z_player.c", "soh/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c"]
    for source in ("NeiHeldPresentation.cpp", "NeiLanternPresentation.cpp"):
        subprocess.run([cc, *flags, "-fsyntax-only", str(ROOT / "soh/soh/Enhancements/randomizer" / source)], check=True)
for source in sources:
    subprocess.run([os.environ.get("CC", "cc"), *cflags, str(ROOT / source)], check=True)
print("PASS: real-header common, overhead and shop GI C translation units")
if "--held" in sys.argv:
    print("PASS: held C++ renderers, custom-items unity and hook actor C translation units")

if preview_path := os.environ.get("NEI_SHOP_PREVIEW_EXPORT"):
    path = Path(preview_path)
    preview = json.loads(path.read_text())
    sources = ("soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp",
               "soh/soh/Enhancements/randomizer/NeiGiRender.h",
               "soh/soh/Enhancements/randomizer/NeiGiShopFit.h",
               "soh/soh/Enhancements/randomizer/draw.cpp",
               "soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c",
               "tests/nei_gi/presentation_test.cpp",
               "scripts/diagnostics/run_nei_gi_tests.py")
    preview["metadata"]["source_sha256"] = {
        source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in sources
    }
    for item in preview["items"]:
        asset = ROOT / "soh/assets/custom/objects/nei_gi_redesign" / item["slug"]
        item["asset_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in sorted(asset.iterdir()) if p.is_file()}
        checkpoint = ROOT / "tools/nei_gi/CHECKPOINTS" / item["slug"]
        item["checkpoint_sha256"] = {
            name: hashlib.sha256((checkpoint / name).read_bytes()).hexdigest()
            for name in ("checkpoint.json", item["slug"] + ".glb")
        }
    path.write_text(json.dumps(preview, indent=2) + "\n")
    print(f"PASS: production shop poses exported to {path}")
