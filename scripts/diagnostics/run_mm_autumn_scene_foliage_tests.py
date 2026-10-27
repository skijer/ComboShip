"""Exercise reversible seasonal scene materials with real GBI/MM headers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import tempfile
import zipfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def production_function(source, name):
    match = re.search(r"^(?:static )?(?:bool|void\*?|int32_t|F3DGfx\*&?)\s+" +
                      re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m.group()), source, flags=re.S)
    pos, depth = match.end(), 1
    while depth:
        depth += (masked[pos] == "{") - (masked[pos] == "}")
        pos += 1
    return source[match.start():pos] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--materials", type=Path, help="Optional serialized Termina foliage material directory")
    parser.add_argument("--poc4", type=Path, help="Optional supplied POC4 archive for exact private brush coverage")
    parser.add_argument("--mm", type=Path, help="Optional native MM archive for both forest-layer controls")
    parser.add_argument("--woodfall-pack", type=Path,
                        help="Optional supplied Woodfall pack for actual HD gradient/detail pixel controls")
    parser.add_argument("--moon-tree-pack", type=Path,
                        help="Optional Prelude Termina export for actual Moon tree leaf/bark draw coverage")
    parser.add_argument("--all-scene-materials", action="store_true",
                        help="Check frozen grass/forest and negative material cases from the full --mm archive")
    parser.add_argument("--field-only", action="store_true",
                        help="Use --mm as an exact Termina native resource subset; verify its 19 frozen field cases")
    args = parser.parse_args()
    if args.field_only and (not args.mm or args.all_scene_materials or args.woodfall_pack):
        parser.error("--field-only requires --mm and cannot use --all-scene-materials or --woodfall-pack")
    if args.all_scene_materials and not args.mm:
        parser.error("--all-scene-materials requires --mm")
    if args.woodfall_pack and not args.mm:
        parser.error("--woodfall-pack requires --mm for the actual native owner display list")
    material_paths = sorted(args.materials.glob("*DL_*")) if args.materials else []
    if args.materials and not material_paths:
        parser.error("--materials must contain serialized *DL_* fixtures")
    coverage_cases = []
    scene_cases = []
    texture_paths = {}
    if args.poc4:
        with zipfile.ZipFile(args.poc4) as archive:
            for suffix, texture, triangles in (("0153A8", "leaves_rgba", 3576), ("015DF0", "leaves_rgba", 997),
                                                ("016598", "leaves_rgba", 2296), ("0158E8", "stems_rgba", 244),
                                                ("016180", "stems_rgba", 142), ("016A38", "stems_rgba", 200)):
                path = f"alt/scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_{suffix}"
                texture = f"scenes/nonmq/Z2_00KEIKOKU/foliage_poc3/{texture}"
                coverage_cases.append((path, texture, triangles, archive.read(path)))
    if args.mm:
        path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0241B0"
        with zipfile.ZipFile(args.mm) as archive:
            if args.all_scene_materials or args.field_only:
                # Use actual archive identities for the extracted texture-hash handlers.
                from_paths = [p for p in archive.namelist() if "Tex_" in p or "TLUT_" in p]
                # CRC64 is computed by the C++ fixture; resolve only the curated paths below.
                texture_paths = from_paths
            for suffix, triangles in (("034098", 14), ("037098", 8)):
                texture = f"scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_{suffix}"
                coverage_cases.append((path, texture, triangles, archive.read(path)))
            if args.all_scene_materials or args.field_only:
                reference = json.loads((ROOT / "mm/tests/autumn_scene_material_cases.json").read_text())
                for case in reference["cases"]:
                    if args.field_only and case["scene_id"] != "SCENE_00KEIKOKU":
                        continue
                    data = archive.read(case["path"])
                    if hashlib.sha256(data).hexdigest() != case["resource_sha256"]:
                        raise ValueError(f"Native scene fixture hash differs: {case['path']}")
                    winter_palette = (reference["termina_winter_palette"] if case["scene_id"] == "SCENE_00KEIKOKU"
                                      else reference["winter_palette"])
                    scene_cases.append((case, case.get("autumn_palette", reference["palette"]), winter_palette, data))
                if not scene_cases:
                    raise ValueError("The requested all-scene material reference has no cases")
    source = (ROOT / "mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp").read_text()
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    manager = (ROOT / "libultraship/src/ship/resource/ResourceManager.cpp").read_text()
    start = source.find("namespace {")
    if start < 0:
        start = source.index("void MMAutumnSceneFoliage_Update")
    flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h",
              "mm/assets", "mm/tests", "mm/2s2h/Enhancements/Graphics", "libultraship/include", "libultraship/src", "combo")]
    flags += shlex.split(os.environ.get("MM_FOLIAGE_TEST_CXXFLAGS", ""))
    with tempfile.TemporaryDirectory(prefix="mm-autumn-scene-foliage-") as temporary:
        build = Path(temporary)
        # Count parser-only preflights in the extracted fixture; steady cached
        # updates must not rescan/rebuild the large Termina/Moon display lists.
        fixture_source = source[start:]
        marker = "    std::set<uint64_t> covered;"
        assert fixture_source.count(marker) == 1
        fixture_source = fixture_source.replace(marker, marker + "\n    if (coverageOnly) ++sCoverageWalks;", 1)
        (build / "autumn_scene_foliage_production.inc").write_text(
            '#ifdef COMBO_BUILD\n#include "WoodsMysteryNativeBridge.h"\n#endif\n' + fixture_source)
        renderer_functions = ["GfxExecStack::start", "GfxExecStack::stop", "GfxExecStack::currCmd",
                              "GfxExecStack::branch", "GfxExecStack::call", "GfxExecStack::ret",
                              "Interpreter::SegAddr", "gfx_check_image_signature",
                              "ComboIsUnresolvedSegmentTarget", "ComboResolveDisplayListTarget",
                              "gfx_dl_handler_common", "gfx_end_dl_handler_common", "gfx_marker_handler_otr",
                              "gfx_vtx_hash_handler_custom", "gfx_set_timg_otr_hash_handler_custom",
                              "gfx_set_grayscale_handler_custom", "gfx_set_intensity_handler_custom",
                              "Interpreter::GfxDpSetGrayscaleColor", "gfx_quad_handler_f3dex2", "gfx_tri1_handler_f3dex2"]
        (build / "autumn_scene_renderer_production.inc").write_text(
            "\n".join(production_function(renderer, name) for name in renderer_functions))
        (build / "autumn_scene_renderer_signature.inc").write_text(
            production_function(manager, "ResourceManager::OtrSignatureCheck"))
        (build / "autumn_scene_palette_production.inc").write_text(
            production_function(renderer, "Interpreter::GfxDpLoadTlut"))
        (build / "autumn_road_vine_upload_production.inc").write_text(
            production_function(renderer, "Interpreter::GfxDpLoadBlock") + "\n" +
            production_function(renderer, "Interpreter::ImportTextureRaw"))
        fixtures = "static void CheckActualMaterials() {\n"
        if args.moon_tree_pack:
            with zipfile.ZipFile(args.moon_tree_pack) as archive:
                project = archive.read("prelude/project/edits.json")
                project_path = build / "moon-tree-project.json"
                project_path.write_bytes(project)
                shapes = [shape for edit in json.loads(project)["edits"]["Z2_00KEIKOKU"]
                          for shape in edit.get("data", {}).get("shapes", [])]
                if not shapes:
                    raise ValueError("The Moon tree export has no placed geometry")
                fixtures += f'CheckActualPreludeMoonTree({json.dumps(str(project_path))}, {{\n'
                leaf_count = 0
                for shape in shapes:
                    path = shape["newDlPath"]
                    labels = [texture.get("label") for texture in shape["stored"]["textures"]]
                    foliage = any(label in ("sou_ha_01", "sou_ha_02", "sou_ha_03", "sou_sida_01")
                                  for label in labels)
                    leaf_count += foliage
                    data = archive.read(path)
                    if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                        raise ValueError(f"Invalid Prelude Moon tree display list: {path}")
                    textures = ','.join(json.dumps(path) for path in shape["stored"]["texPaths"])
                    fixtures += '{' + json.dumps(path) + ', {'
                    fixtures += ','.join(f'Command(0x{a:08X}, 0x{b:08X})'
                                         for a, b in struct.iter_unpack('<II', data[72:]))
                    fixtures += '}, {' + textures + '}, ' + str(foliage).lower() + '},\n'
                if not leaf_count:
                    raise ValueError("The Prelude export contains no Moon leaf/fern material labels")
                fixtures += '});\n'
        # The actual bundled randomizer models split shrubbery/leaves from the
        # reward flower and brown stem. Keep that material ownership explicit.
        for folder, family in (("gameplay_field_keep", "gFieldBush"), ("gameplay_keep", "gRandoCuttableGrass")):
            for label in ("Random", "Minor", "Major", "Mask", "Token", "SmallKey", "Fairy", "BossKey", "Heart", "Junk"):
                asset = f"objects/{folder}/{family}{label}DL"
                root = ET.fromstring((ROOT / "mm/assets/custom" / asset).read_text())
                material = None
                for call in root.findall("CallDisplayList"):
                    path = call.attrib["Path"]
                    if "/mat_" in path:
                        material = path.rsplit("_f3dlite_", 1)[-1]
                        continue
                    if "_tri_" not in path:
                        continue
                    foliage = material in ("Grass", "shrubbery", "leaf")
                    expected = path.endswith(("_tri_0", "_tri_1"))
                    if foliage != expected:
                        raise ValueError(f"Randomizer material ownership changed: {path} ({material})")
                    tri_source = ET.fromstring((ROOT / "mm/assets/custom" / path).read_text())
                    fixtures += '{ const std::vector<Gfx> source = {\n'
                    count = 0
                    for command in tri_source:
                        attrs = command.attrib
                        if command.tag == "LoadVertices":
                            vertices, offset = int(attrs["Count"]), int(attrs["VertexOffset"]) * 16
                            word = (0x32 << 24) | (vertices << 12) | (vertices << 1)
                            resource = json.dumps(attrs["Path"])
                            fixtures += (f'Command(0x{word:08X}, {offset}), '
                                         f'Command(CRC64({resource}) >> 32, CRC64({resource}) & 0xFFFFFFFF),\n')
                        elif command.tag == "Triangles2":
                            values = [attrs[key] for key in ("V00", "V01", "V02", "Flag0", "V10", "V11", "V12", "Flag1")]
                            fixtures += 'gsSP2Triangles(' + ', '.join(values) + '),\n'
                            count += 2
                        elif command.tag == "Triangle1":
                            values = [attrs[key] for key in ("V00", "V01", "V02")] + [attrs.get("Flag", "0")]
                            fixtures += 'gsSP1Triangle(' + ', '.join(values) + '),\n'
                            count += 1
                        elif command.tag == "EndDisplayList":
                            fixtures += 'gsSPEndDisplayList(),\n'
                        else:
                            raise ValueError(f"Unsupported actual grass triangle command {command.tag}: {path}")
                    fixtures += f'}}; CheckRoadRandoGrassTarget({json.dumps(path)}, source, {count}, {str(foliage).lower()}); }}\n'
        fixtures += 'std::puts("PASS actual bundled randomizer grass/bush XML: all selected leaves and protected flower/stem children");\n'
        if args.materials:
            for path in material_paths:
                data = path.read_bytes()
                fixtures += '{ const std::vector<Gfx> source = {\n'
                fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
                fixtures += '}; const auto variant = BuildVariant(source); assert(!variant.empty());\n'
                fixtures += 'assert(Equal(RemoveTint(variant), source));\n'
                logical = "scenes/nonmq/Z2_00KEIKOKU/" + path.name
                fixtures += f'CheckActualGroundResource({json.dumps(logical)}, source); }}\n'
            fixtures += f'std::puts("PASS {len(material_paths)} actual serialized Termina bed materials and exact command preservation");\n'
        for path, texture, triangles, data in coverage_cases:
            if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                raise ValueError(f"Invalid serialized display list: {path}")
            fixtures += '{ const std::vector<Gfx> source = {\n'
            fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
            fixtures += f'}}; CheckActualMaterial({json.dumps(path)}, source, {json.dumps(texture)}, {triangles}); }}\n'
        for case, palette, winter_palette, data in scene_cases:
            if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                raise ValueError(f"Invalid serialized scene display list: {case['path']}")
            fixtures += '{ const std::vector<Gfx> source = {\n'
            fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
            expected = ', '.join('0x' + palette[i] for i in case['triangle_palettes'])
            winter_expected = ', '.join('0x' + winter_palette[i] for i in case['triangle_palettes'])
            fixtures += f'}}; CheckActualSceneMaterial({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{expected}}});\n'
            fixtures += f'CheckActualSceneMaterial({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{winter_expected}}}, SEASON_WINTER);\n'
            if case['path'] in (
                    'scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_005EC8',
                    'scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWN_room_00DL_004BE0',
                    'scenes/nonmq/Z2_ALLEY/Z2_ALLEY_room_00DL_003FF0',
                    'scenes/nonmq/Z2_ROMANYMAE/Z2_ROMANYMAE_room_00DL_001CB8'):
                # Include the scene's texture identities plus shared scene textures;
                # the native lists keep their exact reference hashes and offsets.
                folder = '/'.join(case['path'].split('/')[:3]) + '/'
                textures = ', '.join(json.dumps(p) for p in texture_paths
                                     if p.startswith(folder) or p.startswith('misc/scene_texture_'))
                fixtures += f'CheckActualSceneRenderer({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{expected}}}, {{{textures}}});\n'
                fixtures += f'CheckActualSceneRenderer({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{winter_expected}}}, {{{textures}}}, SEASON_WINTER);\n'
            fixtures += '}\n'
        if args.mm and not args.field_only:
            def texture_fixture(archive, path, label):
                data = archive.read(path)
                version = struct.unpack_from('<I', data, 8)[0]
                kind, width, height = struct.unpack_from('<III', data, 64)
                if version == 0:
                    flags, h_scale, v_scale = 0, 1.0, 1.0
                    size = struct.unpack_from('<I', data, 76)[0]
                    pixels = data[80:]
                elif version == 1:
                    flags, h_scale, v_scale, size = struct.unpack_from('<IffI', data, 76)
                    pixels = data[92:]
                else:
                    raise ValueError(f"Unsupported actual texture version {version}: {path}")
                if len(pixels) != size:
                    raise ValueError(f"Actual texture payload size differs: {path}")
                target = build / (label + '.pixels')
                target.write_bytes(pixels)
                return (f'ReadActualTexture({json.dumps(str(target))}, {kind}, {width}, {height}, '
                        f'{flags}, {h_scale}f, {v_scale}f)')

            surface_cases = (
                ("objects/object_mtoride/gWoodfallSceneryPurifiedWallsDL", "SCENE_21MITURINMAE",
                 "objects/object_mtoride/gWoodfallSceneryPurifiedWalls2Tex",
                 "objects/object_mtoride/gWoodfallSceneryPurifiedWalls1Tex", 79, True),
                ("scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAE_room_00DL_001C48", "SCENE_21MITURINMAE",
                 "scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_005190",
                 "scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_004990", 58, False),
                ("scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITI_room_00DL_004F78", "SCENE_24KEMONOMITI",
                 "misc/scene_texture_08/scene_texture_08_Tex_006000",
                 "misc/scene_texture_08/scene_texture_08_Tex_005000", 70, False),
            )
            with zipfile.ZipFile(args.mm) as archive:
                indexed_ground = {
                    "Z2_22DEKUCITY/Z2_22DEKUCITY_room_00Tex_0100F0": "Z2_22DEKUCITYTLUT_00D020",
                    "Z2_DANPEI2TEST/Z2_DANPEI2TEST_room_00Tex_014210": "Z2_DANPEI2TESTTLUT_005A50",
                    "Z2_DANPEI2TEST/Z2_DANPEI2TEST_room_00Tex_014710": "Z2_DANPEI2TESTTLUT_005A50",
                    "Z2_DEKUTES/Z2_DEKUTESTex_006490": "Z2_DEKUTESTLUT_008C90",
                    "Z2_KOEPONARACE/Z2_KOEPONARACETex_005D70": "Z2_KOEPONARACETLUT_00A270",
                    "Z2_LAST_DEKU/Z2_LAST_DEKU_room_00Tex_00A240": "Z2_LAST_DEKU_room_00TLUT_00DA60",
                    "Z2_OPENINGDAN/Z2_OPENINGDANTex_009C20": "Z2_OPENINGDANTLUT_00AC60",
                }
                for index, (image, palette_name) in enumerate(indexed_ground.items()):
                    image = 'scenes/nonmq/' + image
                    palette = image.rsplit('/', 1)[0] + '/' + palette_name
                    fixtures += ('CheckSpringIndexedGround(' + json.dumps(image) + ', ' + json.dumps(palette) + ', ' +
                                 texture_fixture(archive, image, f'spring-ground-{index}') + ', ' +
                                 texture_fixture(archive, palette, f'spring-ground-palette-{index}') + ');\n')
                vine = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITex_005730"
                vine_palette = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITITLUT_007950"
                fixtures += '{\n'
                fixtures += f'auto vines = {texture_fixture(archive, vine, "road-vines")};\n'
                fixtures += f'auto palette = {texture_fixture(archive, vine_palette, "road-vine-palette")};\n'
                fixtures += 'CheckRoadVines(vines, palette);\nconst std::vector<Gfx> source = {\n'
                vine_draw = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITI_room_00DL_003CE0"
                fixtures += ',\n'.join(f'Command(0x{a:08X}, 0x{b:08X})'
                                      for a, b in struct.iter_unpack('<II', archive.read(vine_draw)[72:]))
                fixtures += '}; CheckActualRoadVineDraw(vines, palette, source); }\n'
                trim = "scenes/nonmq/Z2_24KEMONOMITI/Z2_24KEMONOMITI_room_00DL_005FF0"
                fixtures += '{ const std::vector<Gfx> source = {\n'
                fixtures += ',\n'.join(f'Command(0x{a:08X}, 0x{b:08X})'
                                      for a, b in struct.iter_unpack('<II', archive.read(trim)[72:]))
                fixtures += '};\n'
                for selected, color in (("SEASON_AUTUMN", "F3974FFF"), ("SEASON_WINTER", "DCECF5FF"),
                                         ("SEASON_SPRING", "F6C1D9FF")):
                    expected = ','.join('0x' + color for _ in range(52))
                    fixtures += f'CheckActualSceneMaterial({json.dumps(trim)}, SCENE_24KEMONOMITI, source, {{{expected}}}, {selected});\n'
                fixtures += '}\n'
                branch = "objects/object_mtoride/gWoodfallSceneryBranchAndLeaves"
                fixtures += (f'CheckWoodfallMixedLeaves({texture_fixture(archive, branch + "TLUT", "branch-tlut")}, '
                             '{' + ','.join(str(byte) for byte in archive.read(branch + "Tex")[80:]) + '});\n')
                for index, (owner, scene, surface, companion, triangles, first) in enumerate(surface_cases):
                    data = archive.read(owner)
                    if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                        raise ValueError(f"Invalid actual surface owner: {owner}")
                    fixtures += '{ const std::vector<Gfx> source = {\n'
                    fixtures += ',\n'.join(f'Command(0x{a:08X}, 0x{b:08X})'
                                          for a, b in struct.iter_unpack('<II', data[72:]))
                    fixtures += '};\n'
                    fixtures += f'auto surface = {texture_fixture(archive, surface, str(index) + "-surface")};\n'
                    fixtures += f'auto companion = {texture_fixture(archive, companion, str(index) + "-companion")};\n'
                    fixtures += (f'CheckWoodfallSurface({json.dumps(surface)}, surface, {scene}, '
                                 f'{json.dumps(owner)}, &source);\n')
                    fixtures += (f'CheckActualSurfaceDraw({json.dumps(owner)}, {scene}, source, '
                                 f'{json.dumps(surface)}, surface, {json.dumps(companion)}, companion, '
                                 f'{triangles}, {str(first).lower()});\n')
                    if args.woodfall_pack and index == 1:
                        with zipfile.ZipFile(args.woodfall_pack) as pack:
                            actual_surface = 'alt/' + surface
                            actual_companion = 'alt/' + companion
                            fixtures += f'auto hdSurface = {texture_fixture(pack, actual_surface, "hd-surface")};\n'
                            fixtures += f'auto hdCompanion = {texture_fixture(pack, actual_companion, "hd-companion")};\n'
                        fixtures += (f'CheckWoodfallSurface({json.dumps(surface)}, hdSurface, {scene}, '
                                     f'{json.dumps(owner)}, &source);\n')
                        fixtures += (f'CheckActualSurfaceDraw({json.dumps(owner)}, {scene}, source, '
                                     f'{json.dumps(surface)}, hdSurface, {json.dumps(companion)}, hdCompanion, '
                                     f'{triangles}, false);\n')
                    fixtures += '}\n'
        if scene_cases:
            fixtures += f'std::puts("PASS {len(scene_cases)} frozen native scene materials in autumn and winter: exact triangle palettes, native/Alt, both namespaces and Off restoration");\n'
        fixtures += "}\n"
        (build / "autumn_scene_foliage_fixtures.inc").write_text(fixtures)
        binary = build / "test"
        result = subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, "-I" + str(build),
                                 str(ROOT / "mm/tests/autumn_scene_foliage_test.cpp"),
                                 str(ROOT / "libultraship/src/ship/utils/StrHash64.cpp"), "-o", str(binary)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
