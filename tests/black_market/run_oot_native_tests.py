#!/usr/bin/env python3
"""Compile actual OoT native shop transaction/recovery bodies with real engine types."""
from pathlib import Path
import argparse, os, re, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--include", type=Path, action="append", default=[])
parser.add_argument("--case", choices=("all", "door-marker", "door-entry", "door-return", "door-open", "door-exit", "receipt"), default="all")
args = parser.parse_args()
compile_env = dict(os.environ)
if compile_env.get("COMBO_NEI_FIXTURE_CPATH"):
    # The extracted real room cleanup owns ResourceManager-backed state; use
    # its project-pinned header dependencies without changing dispatcher scope.
    compile_env["CPATH"] = os.pathsep.join(filter(None, (
        compile_env["COMBO_NEI_FIXTURE_CPATH"], compile_env.get("CPATH", ""))))
source = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp").read_text()
signatures = ["bool Interior(", "bool Exterior(", "bool Active(", "bool Safe(",
              "void RememberTravel(", "bool MatchesTravel(", "void StageReturn(", "void Transition(",
              "const Item* Offer(", "PurchaseResult Status(",
              "bool ReceiptContext(", "void CancelReceipt(", "bool PendingReceiptEntry(",
              "bool FixedMarketGrant(", "void ReceiveMarketItem(", "bool Purchase(", "void MonitorReceipt(",
              "void PlayerUpdate(", "void BindSolo(", "void SaveMarket(", "void RegisterKeeperActor(", "void PresentTitle(", "void DrawOffer(",
              'extern "C" void ComboBlackMarketOot_NormalizeEntry(',
              'extern "C" void ComboBlackMarketOot_ReportClear(',
              'extern "C" int ComboBlackMarketOot_TryDoorEntrance(']
# Resource-entry fixtures also extract the actual scene callback, which owns
# cleanup of an attached room graph. Compile its real state and helpers.
room_state = source[source.index("BlackMarketRoomMagic::Resources sRoomMagicResources;"):
                    source.index("void BeginRoomMagic(")]
body = room_state + "\n" + "\n".join(block_from(source, source.index(s)) for s in signatures)
body += "\n" + "\n".join(block_from(source, source.index(s)) for s in
                          ("void LogDoorAttempt(", "void LogEntryRecovery("))
fixture = (ROOT / "tests/black_market/oot_native_test.cpp").read_text().replace("/* PRODUCTION */", body)
fixture = fixture.replace("/* CARD_RESOURCE */", re.search(r'alignas\(2\) constexpr char kCard\[\][^;]*;', source)[0])
player_source = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
cleanup_signatures = ["BAD_RETURN(s32) Player_ZeroSpeedXZ(Player* this) {", "void Player_ApplyYawFromAnim(",
                      "void Player_ResetAnimMovement(", "void Player_FinishAnimMovement(",
                      "s32 Player_SetupAction(", "void func_8083C0E8(Player* this, PlayState* play) {"]
cleanup = "\n".join(block_from(player_source, player_source.index(s)) for s in cleanup_signatures)
fixture = fixture.replace("/* NATIVE_DOOR_CLEANUP */", re.sub(r"\bthis\b", "current", cleanup))
hook_signature = "                    s32 marketDoor = ComboBlackMarketOot_TryDoorEntrance("
start = player_source.index(hook_signature)
hook = player_source[start:player_source.index("#endif", start)]
fixture = fixture.replace("/* NATIVE_DOOR_OPEN */", re.sub(r"\bthis\b", "current", hook))
with tempfile.TemporaryDirectory(prefix="market-oot-native-") as temp:
    path = Path(temp) / "test.cpp"; path.write_text(fixture)
    binary = Path(temp) / "test"
    includes = ("tests/black_market", "soh/include", "soh/include/PR", "soh", "soh/soh", "soh/src", "soh/assets",
                "libultraship/include", "combo", "combo/menu")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    *["-I" + str(ROOT / p) for p in includes],
                    *["-I" + str(p) for p in args.include], str(path), "-o", str(binary)], check=True, env=compile_env)
    subprocess.run([str(binary), args.case], check=True, env={**compile_env, "ASAN_OPTIONS": "detect_leaks=0"})
