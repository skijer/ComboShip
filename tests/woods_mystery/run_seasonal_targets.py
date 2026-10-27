"""Exercise Woods seasonal image bindings and the production XML FILEPATH handler."""
from pathlib import Path
import argparse
import json
import os
import shlex
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_autumn_scene_foliage_tests import production_function
from run_time_pedestal_tests import block_from

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('candidate', type=Path)
p.add_argument('--native', type=Path, help='Native room resources for the bridge canopy vertex-path integration')
a = p.parse_args()
source = (ROOT / 'mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp').read_text()
renderer = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
manager = (ROOT / 'libultraship/src/ship/resource/ResourceManager.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='woods-seasonal-') as directory:
    build = Path(directory)
    (build / 'autumn_scene_foliage_production.inc').write_text(
        '#ifdef COMBO_BUILD\n#include "WoodsMysteryNativeBridge.h"\n#endif\n' + source[source.index('namespace {'):])
    names = ['GfxExecStack::start', 'GfxExecStack::stop', 'GfxExecStack::currCmd',
             'GfxExecStack::branch', 'GfxExecStack::call', 'GfxExecStack::ret',
             'Interpreter::SegAddr', 'gfx_check_image_signature',
             'ComboIsUnresolvedSegmentTarget', 'ComboResolveDisplayListTarget',
             'gfx_dl_handler_common', 'gfx_end_dl_handler_common', 'gfx_marker_handler_otr',
             'gfx_vtx_hash_handler_custom', 'gfx_set_timg_otr_hash_handler_custom',
             'gfx_set_grayscale_handler_custom', 'gfx_set_intensity_handler_custom',
             'Interpreter::GfxDpSetGrayscaleColor', 'gfx_quad_handler_f3dex2', 'gfx_tri1_handler_f3dex2']
    (build / 'autumn_scene_renderer_production.inc').write_text(
        '\n'.join(production_function(renderer, name) for name in names))
    (build / 'autumn_scene_renderer_signature.inc').write_text(
        production_function(manager, 'ResourceManager::OtrSignatureCheck'))
    (build / 'autumn_scene_palette_production.inc').write_text(
        production_function(renderer, 'Interpreter::GfxDpLoadTlut'))
    (build / 'autumn_road_vine_upload_production.inc').write_text(
        production_function(renderer, 'Interpreter::GfxDpLoadBlock') + '\n' +
        production_function(renderer, 'Interpreter::ImportTextureRaw'))
    (build / 'autumn_scene_foliage_fixtures.inc').write_text('static void CheckActualMaterials() {}\n')
    (build / 'woods_filepath_production.inc').write_text(
        production_function(renderer, 'gfx_set_timg_otr_filepath_handler_custom'))
    fixtures = ['static auto ActualWoodsTextures() {',
                'std::map<std::string,std::shared_ptr<Fast::Texture>> result;']
    with zipfile.ZipFile(a.candidate) as archive:
        for name in archive.namelist():
            if '/textures/' not in name:
                continue
            data = archive.read(name)
            typ, width, height, flags, hs, vs, size = struct.unpack_from('<IIIIffI', data, 64)
            assert len(data) == size + 92
            if '/cor_woods_mystery/poc1/textures/' in name:
                # These are native RGBA16 sampler wrappers carrying raw RGBA32,
                # not ordinary PNGs or unscaled two-byte RGBA16 payloads.
                leaf = name.rsplit('/', 1)[1]
                if leaf in {'grass', 'rock', 'vines', 'ruins', 'henriko_canopy', 'henriko_forest_edge'}:
                    assert struct.unpack_from('<II', data, 4) == (0x4F544558, 1)
                    expected_height = 512 if leaf == 'henriko_forest_edge' else 2048 if leaf == 'ruins' else 1024
                    assert (typ, width, height, flags, hs, vs, size) == (
                        2, 1024, expected_height, 1, 2 * 1024 / 32, 32, 1024 * expected_height * 4)
            payload = build / ('texture-' + str(len(fixtures)) + '.rgba')
            payload.write_bytes(data[92:])
            path = name.removeprefix('alt/')
            fixtures.append(f'result[{json.dumps(path)}] = ReadActualTexture({json.dumps(str(payload))}, '
                            f'{typ},{width},{height},{flags},{hs}f,{vs}f);')
            fixtures.append(f'result[{json.dumps(path)}]->initData=std::make_shared<Ship::ResourceInitData>();')
            fixtures.append(f'result[{json.dumps(path)}]->initData->Path={json.dumps(path)};')
    fixtures += ['return result;', '}']
    (build / 'woods_seasonal_pixels.inc').write_text('\n'.join(fixtures))
    passage = (ROOT / 'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
    hook = passage.index('OnPlayDrawWorldStart>')
    hook = passage.index('{', hook)
    (build / 'woods_native_bridge_world_hook.inc').write_text(
        'static void ActualNativeBridgeWorldStart() ' + block_from(passage, hook))
    canopy = []
    if a.native:
        with zipfile.ZipFile(a.native) as archive:
            data = archive.read('scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01DL_002968')
        canopy += ['static std::vector<Gfx> ActualNativeCanopy() { return {',
                   ','.join(f'Command({w0}u,{w1}u)' for w0, w1 in struct.iter_unpack('<II', data[72:])), '}; }']
        canopy += ['using WoodsMysteryNativeBridge::CanopyVertexPath;',
                   'using WoodsMysteryNativeBridge::CanopyVertexRange;',
                   'static void CheckPatchedNativeCanopy() {',
                   'auto source=ActualNativeCanopy();',
                   'source[28]=source[49]=CanopyVertexPath();',
                   'source[29]=CanopyVertexRange(12,0);source[50]=CanopyVertexRange(18,12);',
                   'for(int selected : {SEASON_AUTUMN,SEASON_WINTER,SEASON_SPRING}) {',
                   'auto tinted=BuildVariant(source,selected,SCENE_26SARUNOMORI);',
                   'assert(!tinted.empty());assert(Equal(RemoveTint(tinted),source));}',
                   'ClearCache();auto rm=std::make_shared<Ship::ResourceManager>();',
                   'Ship::Context::GetRawInstance()->rm=rm;',
                   'const std::string path="scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01DL_002968";',
                   'auto list=std::make_shared<Fast::DisplayList>();list->Instructions=ActualNativeCanopy();',
                   'rm->resources[path]=list;rm->archive->files[path]=list;',
                   'PlayState play{};play.sceneId=SCENE_26SARUNOMORI;season=SEASON_AUTUMN;',
                   'MMAutumnSceneFoliage_Update(&play);assert((list->Instructions[0].words.w0>>24)==G_DL);',
                   'const auto firstDraw=list->Instructions[0].words.w1;',
                   'for(int phase=0;phase<6;++phase) {',
                   'bool patched=phase%2==0;auto original=ActualNativeCanopy();',
                   'list->Instructions[28]=patched?CanopyVertexPath():original[28];',
                   'list->Instructions[29]=patched?CanopyVertexRange(12,0):original[29];',
                   'list->Instructions[49]=patched?CanopyVertexPath():original[49];',
                   'list->Instructions[50]=patched?CanopyVertexRange(18,12):original[50];',
                   'MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)==G_DL);',
                   'const auto& variant=CurrentVariants().at(list.get());',
                   'assert((variant.original[28].words.w0>>24)==(patched?G_VTX_OTR_FILEPATH:G_VTX_OTR_HASH));',
                   'assert((reinterpret_cast<const Gfx*>(firstDraw)[0].words.w0>>24)!=0);',
                   'season=SEASON_OFF;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)!=G_DL);',
                   'season=SEASON_AUTUMN;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)==G_DL);}',
                   'assert(sCanopyLayouts.size()==1);',
                   'list->Instructions[28]=list->Instructions[49]=CanopyVertexPath();',
                   'list->Instructions[29]=CanopyVertexRange(12,0);list->Instructions[50]=CanopyVertexRange(18,12);',
                   'list->Instructions[17].words.w1^=1;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)!=G_DL);',
                   'list->Instructions[17].words.w1^=1;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)==G_DL);',
                   'list->Instructions[50].words.w0=19;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)!=G_DL);',
                   'list->Instructions[50]=CanopyVertexRange(18,12);MMAutumnSceneFoliage_Update(&play);',
                   'list->Instructions[28].words.w1=0x123456u;MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)!=G_DL);',
                   'list->Instructions[28]=CanopyVertexPath();MMAutumnSceneFoliage_Update(&play);',
                   'assert((list->Instructions[0].words.w0>>24)==G_DL);ClearCache();',
                   'WoodsMysteryNativeBridge::Reset();NativeBridgeFixture::sizes.clear();',
                   'rm=std::make_shared<Ship::ResourceManager>();Ship::Context::GetRawInstance()->rm=rm;',
                   'for(const char* asset : { WoodsMysteryNativeBridge::kNativeWall, WoodsMysteryNativeBridge::kBakedWall,',
                   'WoodsMysteryNativeBridge::kBridge }) {auto dl=std::make_shared<Fast::DisplayList>();',
                   'dl->Instructions={gsSPEndDisplayList()};rm->resources[NativeBridgeFixture::Path(asset)]=dl;}',
                   'for(const char* asset : {WoodsMysteryNativeBridge::kBakedWallVertices,',
                   'WoodsMysteryNativeBridge::kBridgeVertices,WoodsMysteryNativeBridge::kBark,',
                   'WoodsMysteryNativeBridge::kPlanks,WoodsMysteryNativeBridge::kBlack})',
                   'NativeBridgeFixture::sizes[NativeBridgeFixture::Path(asset)]=1;',
                   'NativeBridgeFixture::sizes[NativeBridgeFixture::Path(WoodsMysteryNativeBridge::kCanopyVertices)]=38*sizeof(Vtx);',
                   'list=std::make_shared<Fast::DisplayList>();list->Instructions=ActualNativeCanopy();',
                   'rm->resources[path]=list;rm->archive->files[path]=list;',
                   'RoomShapeCullableEntry entry{};entry.opa=reinterpret_cast<Gfx*>(const_cast<char*>(WoodsMysteryNativeBridge::kNativeWall));',
                   'RoomShape shape{};shape.cullable.base.type=ROOM_SHAPE_TYPE_CULLABLE;',
                   'shape.cullable.numEntries=1;shape.cullable.entries=&entry;',
                   'play={};play.sceneId=SCENE_26SARUNOMORI;play.roomCtx.curRoom.num=1;play.roomCtx.curRoom.roomShape=&shape;',
                   'gPlayState=&play;gSaveContext.gameMode=GAMEMODE_NORMAL;season=SEASON_AUTUMN;',
                   'MMAutumnSceneFoliage_Update(&play);',
                   'assert((CurrentVariants().at(list.get()).original[28].words.w0>>24)==G_VTX_OTR_HASH);',
                   'ActualNativeBridgeWorldStart();assert(WoodsMysteryNativeBridge::Active(&play));',
                   'assert((CurrentVariants().at(list.get()).original[28].words.w0>>24)==G_VTX_OTR_FILEPATH);',
                   'for(int selected : {SEASON_AUTUMN,SEASON_WINTER,SEASON_SPRING}) {season=selected;',
                   'for(int room : {0,1,0,1}) {play.roomCtx.curRoom.num=room;',
                   'MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert((CurrentVariants().at(list.get()).original[28].words.w0>>24)==',
                   '(room==1?G_VTX_OTR_FILEPATH:G_VTX_OTR_HASH));',
                   'const auto branch=list->Instructions[0];ActualNativeBridgeWorldStart();',
                   'assert(SameCommand(branch,list->Instructions[0]));}}',
                   'season=SEASON_OFF;MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert((list->Instructions[0].words.w0>>24)!=G_DL);',
                   'season=SEASON_AUTUMN;MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'auto alternate=std::make_shared<Fast::DisplayList>();alternate->Instructions=ActualNativeCanopy();',
                   'rm->resources["alt/"+path]=alternate;rm->archive->files["alt/"+path]=alternate;rm->alt=true;',
                   'MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert((CurrentVariants().at(alternate.get()).original[28].words.w0>>24)==G_VTX_OTR_FILEPATH);',
                   'play.sceneId=SCENE_21MITURINMAE;MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert(!WoodsMysteryNativeBridge::sActive);',
                   'assert((alternate->Instructions[28].words.w0>>24)==G_VTX_OTR_HASH);',
                   'play.sceneId=SCENE_26SARUNOMORI;MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert((CurrentVariants().at(alternate.get()).original[28].words.w0>>24)==G_VTX_OTR_FILEPATH);',
                   'alternate->Instructions[17].words.w1^=1;MMAutumnSceneFoliage_Update(&play);ActualNativeBridgeWorldStart();',
                   'assert((alternate->Instructions[0].words.w0>>24)!=G_DL);',
                   'gPlayState=nullptr;ActualNativeBridgeWorldStart();ClearCache();NativeBridgeFixture::sizes.clear();',
                   'std::puts("PASS native canopy vertex-path patch: known two-word commands preserve seasonal materials and exact geometry payloads");}']
    else:
        canopy += ['static void CheckPatchedNativeCanopy() {}']
    (build / 'woods_native_canopy_case.inc').write_text('\n'.join(canopy))
    flags = ['-std=c++20', '-O1', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
             '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING']
    flags += ['-I' + str(ROOT / r) for r in ('mm/include', 'mm/include/PR', 'mm/src', 'mm', 'mm/2s2h',
              'mm/assets', 'mm/tests', 'mm/2s2h/Enhancements/Graphics', 'libultraship/include',
              'libultraship/src', 'combo')]
    flags += shlex.split(os.environ.get('MM_FOLIAGE_TEST_CXXFLAGS', ''))
    binary = build / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, '-I' + str(build),
                    str(Path(__file__).with_name('seasonal_targets_test.cpp')),
                    str(ROOT / 'libultraship/src/ship/utils/StrHash64.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
