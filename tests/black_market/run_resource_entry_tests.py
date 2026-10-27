#!/usr/bin/env python3
"""Execute both production entry routes against immutable native scene resources.

This intentionally does not claim a game session: it exercises production door
hooks/controllers, structural readiness, scene tables, return and re-entry in
sanitized executables. Archive membership and binary parsing have test adapters.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "scripts/diagnostics"), str(ROOT / "scripts/assets"),
               str(ROOT / "tools/black_market")]
from run_time_pedestal_tests import block_from
import final_forest
import native_forest
import exit_door
import native_market
import prepare_native_forest
import prepare_native_market


def replace_once(text, old, new):
    assert text.count(old) == 1, (old, text.count(old))
    return text.replace(old, new)


def file_hash(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def read_archive(path, expected_hash):
    actual = file_hash(path)
    assert actual == expected_hash, (path.name, actual, expected_hash)
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        assert len(names) == len(set(names)) and archive.testzip() is None
        return {name: archive.read(name) for name in names if not name.endswith("/")}


def write_resources(root, resources):
    result = {}
    for name, data in resources.items():
        assert not name.startswith("/") and ".." not in Path(name).parts
        target = root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        result[name] = str(target)
        # Reproduce the known raw-json OTRExtScanner key, not a manufactured
        # manifest. Its raw bytes remain available at the written filename.
        if name.endswith(".json"):
            result[name[:-5]] = result.pop(name)
    return result


def floor_data(data):
    native_forest.check_header(data, b"LOCO")
    reader = native_forest.Reader(data, 64)
    bounds = reader.take("6h")
    vertices = [reader.take("3h") for _ in range(reader.take("I"))]
    polygons = [reader.take("4H4h") for _ in range(reader.take("I"))]
    assert vertices and polygons
    return {"bounds": bounds, "vertices": vertices, "polygons": polygons}


def forest_data(scene, collision, oot=False):
    commands = native_forest.read_scene(scene, oot=oot)
    reader = native_forest.Reader(commands[14])
    transitions = [reader.take("4bhhhhhH") for _ in range(reader.take("I"))]
    reader.finish()
    assert len(transitions) == 2 and tuple(transitions[1][5:8]) == (-680, 0, 72)
    return {"transitions": transitions, "scene_sha256": native_forest.sha(scene),
            "collision_sha256": native_forest.sha(collision), **floor_data(collision)}


def source_destination_audit(market):
    entrances = (ROOT / "soh/include/tables/entrance_table.h").read_text()
    scenes = (ROOT / "soh/include/tables/scene_table.h").read_text()
    assert re.search(r"/\* 0x614 \*/ DEFINE_ENTRANCE\(ENTR_SKIEJER_MARKET, SCENE_SKIEJER_MARKET, 0,", entrances)
    assert re.search(r"/\* 0x6E \*/ DEFINE_SCENE\(skiejer_market_oot, none, SCENE_SKIEJER_MARKET,", scenes)
    result = {}
    for host, scene in native_market.SCENES.items():
        commands = native_forest.read_scene(market[scene], oot=host == "oot")
        reader = native_forest.Reader(commands[4]); assert reader.take("I") == 1
        room = reader.text(); reader.take("II"); reader.finish()
        collision = native_forest.read_string(commands[3])
        assert room == scene + "_room_0" and collision == native_market.COLLISION
        room_commands = native_forest.read_scene(market[room], oot=host == "oot")
        mesh = native_forest.read_mesh(room_commands[10])
        assert mesh["entries"][0]["opaque"] == "custom/skiejer_market/room"
        spawn = native_forest.read_actor_list(commands[0])[0]
        assert tuple(spawn[1:4]) == (-11, 0, 381)
        result[host] = {"scene": scene, "room": room, "collision": collision,
                        "spawn": spawn, "scene_sha256": native_forest.sha(market[scene]),
                        "room_sha256": native_forest.sha(market[room]),
                        "collision_sha256": native_forest.sha(market[collision])}
    return result


def build_input(args, work):
    manifest = json.loads((ROOT / "tools/native_forest/manifest.json").read_text())
    full = not args.bundled_only
    base = read_archive(args.mm_base, manifest["inputs"]["native_mm"]["sha256"]) if full else {}
    authored = read_archive(args.authored_forest, native_forest.FINAL_SHA) if full else {}
    art = read_archive(args.market_pack, native_market.SOURCE_SHA) if full else {}
    forest = prepare_native_forest.verified_resources(ROOT / "tools/native_forest/manifest.json")
    market = prepare_native_market.prepared_resources(ROOT / "tools/native_market/bundled")
    # The explicitly requested native exit door is the only scene-header
    # adaptation; collision, room and player-start payloads stay protected.
    framing_source = art if full else native_market.read_bundle(ROOT / "tools/native_market/bundled")
    expected_destinations = exit_door.apply(framing_source)
    for path in (native_market.COLLISION, *(scene + suffix for scene in native_market.SCENES.values()
                                          for suffix in ("", "_room_0"))):
        assert market[path] == expected_destinations[path], ("authored/native destination framing differs", path)
    # Exercise the ordinary preparation stage and consume its files, rather
    # than generate a list of expected names and call that mounted content.
    prepared = work / "prepared"
    prepare_native_forest.prepare(prepared)
    prepare_native_market.prepare(prepared)
    built = {host: {p.relative_to(prepared / host / "assets/custom").as_posix(): str(p)
                    for p in (prepared / host / "assets/custom").rglob("*") if p.is_file()}
             for host in ("mm", "soh")}
    mm_market = prepare_native_market.prepared_resources(ROOT / "tools/native_market/bundled", host='mm')
    assert set(built["mm"]) == set(forest) | set(mm_market)
    assert set(built["soh"]) == set(forest) | set(market)
    # Keeper model/texture/vertex bytes come from the actual base owner. Other
    # stock forest resources remain indexed as real base files as well.
    base_needed = {name: data for name, data in base.items()
                   if name.startswith("objects/object_bg/") or name in manifest["external_resources"] or
                   name == native_forest.NATIVE + "_room_00"}
    base_index = write_resources(work / "mm-base", base_needed)
    art_index = write_resources(work / "mods/2ship/market", art)
    # This is the actual POC14 visual payload with scene/room framing excluded;
    # it is a test view of the supplied archive, not a new user-facing addon.
    visual_index = {name: path for name, path in art_index.items() if not name.startswith("scenes/")}
    scenarios = {}
    for host, port in (("mm", "mm"), ("oot", "soh")):
        native = {**built[port], **(base_index if host == "mm" else {})}
        scenarios[host + "_native"] = {"files": native, "alt": False}
        if full:
            scenarios[host + "_mods_2ship_visuals"] = {"files": {**native, **visual_index}, "alt": True}
            scenarios[host + "_authored_market"] = {"files": {**native, **art_index}, "alt": True}
    forests = {}
    for mode, path in manifest["scene_paths"].items():
        commands = native_forest.read_scene(forest[path], oot=mode == "oot")
        forests[mode] = forest_data(forest[path], forest[native_forest.read_string(commands[3])], mode == "oot")
    if full:
        authored_scene = authored[final_forest.SOURCE_SCENE]
        commands = native_forest.read_scene(authored_scene)
        collision = authored["alt/" + native_forest.read_string(commands[3])]
        forests["authored_mm"] = forest_data(authored_scene, collision)
        # Consume the exact authored transition payload through the production
        # OoT converter. This is conversion proof; the old solo archive is absent.
        commands[14] = final_forest.transition_rows(commands[14], True)
        converted = native_forest.write_scene(commands)
        forests["authored_oot"] = forest_data(converted, collision, True)
    data = {"boundary": "production-controller executable; no game loop", "full_real_inputs": full, "scenarios": scenarios,
            "base_files": base_index,
            "base_hashes": [{"hash": native_forest.crc64(name), "path": name} for name in base_index],
            "forests": forests, "destinations": source_destination_audit(market)}
    path = work / "resource-input.json"
    path.write_text(json.dumps(data))
    receipt = {"source_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
               "boundary": data["boundary"], "full_real_inputs": full,
               "base_sha256": file_hash(args.mm_base) if full else None,
               "authored_forest_sha256": file_hash(args.authored_forest) if full else None,
               "market_pack_sha256": file_hash(args.market_pack) if full else None,
               "external_resource_status": "actual supplied base owner" if full else "absent; no declared inventory or placeholder resources",
               "native_forest_bundle_sha256": manifest["bundle"]["sha256"],
               "native_market_decoded_sha256": json.loads((ROOT / "tools/native_market/bundled/manifest.json").read_text())["decoded_sha256"],
               "native_market_surfaces": json.loads((ROOT / "tools/black_market/assets/interior/encoded.json").read_text()),
               "native_resources": len(forest) + len(market), "base_resources_consumed": len(base_index),
               "optional_visual_resources": len(visual_index), "destinations": data["destinations"],
               "forests": {key: {k: v for k, v in value.items() if k not in ("vertices", "polygons")}
                           for key, value in forests.items()}}
    units = ["mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp", "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp",
             "mm/src/overlays/actors/ovl_player_actor/z_player.c", "soh/src/overlays/actors/ovl_player_actor/z_player.c",
             "mm/src/overlays/actors/ovl_En_Door/z_en_door.c", "mm/src/code/z_play.c", "soh/src/code/z_play.c",
             "mm/src/code/sys_math3d.c", "soh/src/code/sys_math3d.c", "combo/BlackMarketEntry.h",
             "combo/BlackMarketKeeperAssets.h", "combo/entrances/NativeForest.h",
             "combo/entrances/NativeForestDependencies.h", "combo/BlackMarketNativeResources.h",
             "combo/entrances/NativeForestResources.h", "combo/entrances/BlackMarketEntrance.h",
             "mm/2s2h/Enhancements/Gameplay/BlackMarketDoorNative.h",
             "soh/soh/Enhancements/Gameplay/OotBlackMarketDoorNative.h",
             "soh/include/tables/scene_table.h", "soh/include/tables/entrance_table.h",
             "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"]
    receipt["production_unit_sha256"] = {path: file_hash(ROOT / path) for path in units}
    receipt["test_unit_sha256"] = {path: file_hash(ROOT / path) for path in (
        "tests/black_market/run_resource_entry_tests.py", "tests/black_market/resource_entry_fixture.h",
        "tests/black_market/resource_entry_test.inc", "tests/black_market/run_native_tests.py",
        "tests/black_market/run_oot_native_tests.py")}
    receipt["archive_boundary"] = "actual files are decoded and indexed by a test adapter; application archive mounting/load order is not executed"
    return path, receipt


def build_and_run(host, work, input_path, includes=()):
    name = "native" if host == "mm" else "oot_native"
    source_path = "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp" if host == "mm" else "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp"
    source = (ROOT / source_path).read_text()
    play_source = (ROOT / ("mm" if host == "mm" else "soh") / "src/code/z_play.c").read_text()
    normalizer = "ComboBlackMarket_NormalizeEntry();" if host == "mm" else "ComboBlackMarketOot_NormalizeEntry();"
    assert play_source.index(normalizer) < play_source.index("gSaveContext.respawnFlag = 0;", play_source.index(normalizer))
    if host == "mm":
        assert re.search(r"gSaveContext.respawnFlag = 0;[\s\S]*?GameInteractor_ExecuteOnSceneInit", play_source)
    fixture = (ROOT / "tests/black_market" / (name + "_test.cpp")).read_text()
    marker = re.search(r'^int main\([^\n]*\)\s*\{', fixture, re.M)
    if marker is None:
        raise ValueError('native resource-entry fixture main is missing: ' + host)
    fixture = fixture[:marker.start()]
    if host == 'mm':
        fixture = '#include "BlackMarketMmShopScene.h"\n' + fixture
    # Remove the original fixture's policy doubles. The new executable always
    # calls the actual production readiness bodies and membership adapters.
    fixture = re.sub(r'extern "C" u8 ResourceMgr_FileExists\(const char\*\) \{ return destination; \}\n', "", fixture)
    fixture = re.sub(r'extern "C" u8 MmAssets_BaseResourceExists\(const char\*\) \{ return destination; \}\n', "", fixture)
    fixture = replace_once(fixture, '#include "entry_diagnostics_fixture.h"', '#include "resource_entry_fixture.generated.h"')
    fixture = replace_once(fixture, "bool PackAvailable() { return pack; }", block_from(source, source.index("bool PackAvailable(")))
    fixture = replace_once(fixture, "/* PRODUCTION */", block_from(source, source.index("bool DestinationAvailable(")) + "\n/* PRODUCTION */")
    if host == "mm":
        start = source.index("void RegisterMarket() {")
        registration = source[start:source.index("    GameInteractor::", start)] + "}\n"
        fixture += "\nSceneTableEntry gSceneTable[SCENE_MAX]{};\nSceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX]{};\n" + registration
        fixture = replace_once(fixture, "    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;\n    return floorExists ? 0 : BGCHECK_Y_MIN;", "    return RealFloor(poly, bg, realFloorProbe);")
        fixture = replace_once(fixture, "    s32* bg, Actor*, Vec3f*) {", "    s32* bg, Actor*, Vec3f* realFloorProbe) {")
    else:
        fixture = replace_once(fixture, 'extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }', 'extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }')
        fixture = replace_once(fixture, "EntranceInfo gEntranceTable[ENTR_MAX];", '#define DEFINE_ENTRANCE(name, scene, spawn, ...) {scene, spawn, 0},\nEntranceInfo gEntranceTable[ENTR_MAX] = {\n#include "tables/entrance_table.h"\n};\n#undef DEFINE_ENTRANCE')
        fixture = replace_once(fixture, "    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;\n    return floorExists ? 0 : BGCHECK_Y_MIN;", "    return RealFloor(poly, bg, realFloorProbe);")
        fixture = replace_once(fixture, "s32* bg, Actor*, Vec3f*) {", "s32* bg, Actor*, Vec3f* realFloorProbe) {")
        helpers = "\n".join(block_from(source, source.index(s)) for s in ("bool CustomKeeperAvailable() {", "bool LoadNativeKeeper() {"))
        fixture = replace_once(fixture, "bool PackAvailable() {", "bool CustomKeeperAvailable();\nbool LoadNativeKeeper();\nbool PackAvailable() {")
        fixture += "\n" + helpers
        scene = block_from(source, source.index("[](s16 scene) {", source.index("RegisterGameHook<GameInteractor::OnSceneInit>")))
        # The real scene hook resets camera ownership even though this fixture
        # does not run the browsing camera. Compile its exact native reset too.
        camera = (ROOT / "combo/BlackMarketShopCameraNative.h").read_text()
        fixture += "\nActor* sBrowseKeeper = nullptr;\nCollisionHeader* sShopOriginalCollision = nullptr;\nstd::vector<CamData> sShopBgCameras;\ns16 sShopBgIndex = -1;\n"
        fixture += block_from(camera, camera.index("void InitBrowsingCameraScene(PlayState*) {")) + "\n"
        fixture += "\nvoid EndBrowsing(PlayState*, bool) { sTalking = false; }\nvoid GrannySceneBegin() {}\nstruct ShelfFixtureStub { void Reset() {} } sShelfFixture;\nauto ResourceSceneInit = " + scene + ";\n"
    fixture += (ROOT / "tests/black_market/resource_entry_test.inc").read_text()
    fixture_path = work / (host + "-fixture.cpp"); fixture_path.write_text(fixture)
    math = (ROOT / ("mm" if host == "mm" else "soh") / "src/code/sys_math3d.c").read_text()
    signatures = (("s32 Math3D_CirSquareVsTriSquareZX(", "s32 Math3D_PointDistSqToLineZX(") if host == "mm" else
                  ("s32 Math3D_CirSquareVsTriSquare(", "s32 Math3D_PointDistToLine2D("))
    signatures += ("s32 Math3D_TriChkPointParaYImpl(", "s32 Math3D_TriChkPointParaYIntersectInsideTri(")
    header = (ROOT / "tests/black_market/resource_entry_fixture.h").read_text()
    # OoT also has a forward declaration of CirSquare near the top.
    math_bodies = []
    for signature in signatures:
        matches = list(re.finditer(re.escape(signature), math))
        definition = next(match.start() for match in matches if math.index("{", match.start()) < math.index(";", match.start()))
        math_bodies.append(block_from(math, definition))
    header = replace_once(header, "/* PRODUCTION_TRIANGLE_MATH */", "\n".join(math_bodies))
    header = header.replace('#include "../../', '#include "' + str(ROOT) + '/')
    (work / "resource_entry_fixture.generated.h").write_text(header)
    driver = (ROOT / "tests/black_market" / ("run_" + name + "_tests.py")).read_text()
    driver = replace_once(driver, "ROOT = Path(__file__).resolve().parents[2]", f"ROOT = Path({str(ROOT)!r})")
    driver = replace_once(driver, f'(ROOT / "tests/black_market/{name}_test.cpp").read_text()', f'Path({str(fixture_path)!r}).read_text()')
    binary_dir = work / (host + "-binary"); binary_dir.mkdir(exist_ok=True)
    driver, changed = re.subn(r'with tempfile.TemporaryDirectory\(prefix="market-[^"\n]+"\) as temp:',
                             f'if True:\n    temp = {str(binary_dir)!r}', driver)
    assert changed == 1
    extra_flags = ["-I" + str(work), *["-I" + str(path.resolve()) for path in includes]]
    if host != "mm":
        extra_flags.append("-DOOT_RESOURCE_ENTRY_TEST")
    driver = replace_once(driver, '"-DF3DEX_GBI_2"', '"-DF3DEX_GBI_2", ' + ", ".join(repr(flag) for flag in extra_flags))
    patch = ROOT / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"
    if host != "mm":
        driver = replace_once(driver, 'str(path), "-o", str(binary)', f'str(path), {str(patch)!r}, "-o", str(binary)')
    driver = replace_once(driver, '[str(binary), args.case]', f'[str(binary), {str(input_path)!r}]')
    runner = work / (host + "-runner.py"); runner.write_text(driver)
    result = subprocess.run([sys.executable, str(runner)], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (work / (host + "-entry.log")).write_text(result.stdout)
    print(result.stdout, end="", flush=True)
    assert result.returncode == 0, f"{host} entry executable failed; see {work / (host + '-entry.log')}"
    return {"host": host, "binary_sha256": file_hash(binary_dir / "test"),
            "generated_fixture_sha256": file_hash(binary_dir / "test.cpp"),
            "log_sha256": file_hash(work / (host + "-entry.log")), "sanitizers": "address,undefined"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundled-only", action="store_true", help="CI structural routes with real bundled bytes; base/keeper readiness remains absent")
    parser.add_argument("--mm-base", type=Path)
    parser.add_argument("--authored-forest", type=Path)
    parser.add_argument("--market-pack", type=Path)
    parser.add_argument("--include", type=Path, action="append", default=[])
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    if args.bundled_only and any((args.mm_base, args.authored_forest, args.market_pack)):
        parser.error("--bundled-only excludes real-input flags; run the full mode separately")
    if not args.bundled_only and not all((args.mm_base, args.authored_forest, args.market_pack)):
        parser.error("full mode requires --mm-base, --authored-forest and --market-pack; nothing is silently skipped")
    work = args.output_dir or Path(tempfile.mkdtemp(prefix="market-real-entry-"))
    work.mkdir(parents=True, exist_ok=True)
    input_path, receipt = build_input(args, work)
    receipt["executables"] = [build_and_run(host, work, input_path, args.include) for host in ("mm", "oot")]
    receipt["command"] = [sys.executable, *sys.argv]
    receipt["result"] = ("PASS bundled structural controller routes; base/keeper readiness absent; configured gameplay untested"
                         if args.bundled_only else "PASS production-controller routes with actual supplied resources; configured gameplay untested")
    (work / "entry-receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print("PASS entry receipt:", work / "entry-receipt.json")


if __name__ == "__main__":
    main()
