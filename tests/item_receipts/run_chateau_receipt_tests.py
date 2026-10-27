#!/usr/bin/env python3
"""Run production Chateau receipt formatting and its save-menu hook boundary.

The small fixture replaces font staging and hook metadata, not callback text,
formatting, registration IDs, deferred hook removal, or hook execution order.
Glyph ranges are checked before any native font-width access.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()


def function(source, signature, body_only=False):
    start = source.index(signature)
    begin = source.index("{", start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin if body_only else start:end]


def initializer(source, declaration):
    return function(source, declaration) + ";"


def macro(source, name):
    lines = source.splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith("#define " + name + "("))
    end = start
    while lines[end].endswith("\\"):
        end += 1
    return "\n".join(lines[start:end + 1])


def hook_body(source, signature):
    # The #endif belongs to the alternative source-location method signature.
    return function(source, signature, True)[1:-1].replace("\n#endif\n", "\n", 1)


manager = (ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp").read_text()
header = (ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.h").read_text()
items = (ROOT / "soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp").read_text()
hooks = (ROOT / "soh/soh/Enhancements/game-interactor/GameInteractor.h").read_text()
better = (ROOT / "soh/soh/Enhancements/QoL/BetterSaveMenu.cpp").read_text()
types = (ROOT / "soh/soh/Enhancements/custom-message/CustomMessageTypes.h").read_text()
cow = (ROOT / "soh/src/overlays/actors/ovl_En_Cow/z_en_cow.c").read_text()
message = (ROOT / "soh/src/code/z_message_PAL.c").read_text()

registration = function(items, "void RegisterChateauRomaniMessage()")
registration_id = re.search(r"COND_ID_HOOK\(OnOpenText,\s*([^,]+),", registration)[1].strip()
cow_entry = function(cow, "static GetItemEntry EnCow_GetChateauEntry(")
cow_id = re.search(r"entry\.textId\s*=\s*([^;]+);", cow_entry)[1].strip()
continue_start = better.index("COND_ID_HOOK(OnOpenText, TEXT_CONTINUE_OVERWORLD_MSG")
continue_registration = function(better[continue_start:], "COND_ID_HOOK(") + ");"
continue_id = re.search(r"COND_ID_HOOK\(OnOpenText,\s*([^,]+),", continue_registration)[1].strip()
definitions = {}


def declare_id(expression):
    if re.fullmatch(r"0[xX][0-9A-Fa-f]+|[0-9]+", expression):
        return
    assert re.fullmatch(r"[A-Z][A-Z0-9_]*", expression), f"Unsupported receipt ID expression: {expression}"
    value = re.search(r"\b" + re.escape(expression) + r"\s*=\s*([^,\n]+)", types)
    assert value, f"Receipt ID is missing from CustomMessageTypes.h: {expression}"
    value = value[1].strip()
    declare_id(value)
    definitions[expression] = value


for expression in (registration_id, cow_id, continue_id):
    declare_id(expression)
ids = "\n".join(f"constexpr uint16_t {name} = {value};" for name, value in definitions.items())
ids += f"\nconstexpr uint16_t kChateauRegistrationId = {registration_id};"
ids += f"\nconstexpr uint16_t kCowReceiptId = {cow_id};"
ids += f"\nconstexpr uint16_t kContinueId = {continue_id};"

maps = "\n".join(initializer(manager, "static const std::unordered_map<std::string, " + kind + "> " + name)
                 for name, kind in (("textBoxSpecialCharacters", "char"),
                                    ("percentColors", "std::string"), ("colorToPercent", "std::string")))
widths = initializer(message, "f32 sFontWidths[144]").replace("f32 ", "float ", 1)
start = better.index("static CustomMessage continueOverworldMsg =")
continuation = better[start:better.index(";", start) + 1]
message_macros = "\n".join(re.findall(r"^#define QM_[A-Z_]+[^\n]+", header, re.M))
message_macros += "\n" + re.search(r'^#define TODO_TRANSLATE[^\n]+', (ROOT / "soh/include/z64.h").read_text(), re.M)[0]
methods = "\n".join(function(manager, signature) for signature in (
    "CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_, TextBoxType type_,",
    "CustomMessage::CustomMessage(std::string english_, TextBoxType type_, TextBoxPosition position_)",
    "void CustomMessage::Format()", "void CustomMessage::FormatString(",
    "void CustomMessage::ReplaceSpecialCharacters(", "void CustomMessage::ReplaceColors(",
    "void CustomMessage::ReplaceAltarIcons(", "void CustomMessage::EncodeColors(",
    "std::string CustomMessage::MESSAGE_END(", "std::string CustomMessage::NEWLINE(",
    "std::string CustomMessage::WAIT_FOR_INPUT(", "std::string CustomMessage::PLAYER_NAME(",
    "std::string CustomMessage::TWO_WAY_CHOICE(", "std::string CustomMessage::COLOR(",
    "std::string CustomMessage::ITEM_OBTAINED(",
))
parts = {
    "MESSAGE_MACROS": message_macros,
    "MESSAGE_DATA": maps + "\n" + widths,
    "MESSAGE_METHODS": methods,
    "CHATEAU_CALLBACK": function(items, "void BuildChateauRomaniMessage("),
    "CONTINUE_MESSAGE": continuation,
    "REGISTER_HOOK_BODY": hook_body(hooks, "HOOK_ID RegisterGameHookForID(int32_t id, typename H::fn h)"),
    "UNREGISTER_HOOK_BODY": hook_body(hooks, "void UnregisterGameHookForID(HOOK_ID hookId)"),
    "DISPATCH_HOOK_BODY": hook_body(hooks, "void ExecuteHooksForID(int32_t id, Args&&... args)"),
    "RECEIPT_IDS": ids,
    "CONDITIONAL_ID_HOOK": macro(hooks, "COND_ID_HOOK"),
    "CHATEAU_REGISTRATION": registration,
    "CONTINUE_REGISTRATION": continue_registration,
}
fixture = (ROOT / "tests/item_receipts/chateau_receipt_test.cpp").read_text()
for name, value in parts.items():
    marker = "/* " + name + " */"
    assert fixture.count(marker) == 1, f"Missing or duplicate fixture marker: {marker}"
    assert value, f"Empty production extraction: {name}"
    fixture = fixture.replace(marker, value)

flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"]
if args.sanitize:
    flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
              "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
with tempfile.TemporaryDirectory(prefix="chateau-receipt-") as directory:
    cpp = Path(directory) / "chateau.cpp"
    cpp.write_text(fixture)
    binary = Path(directory) / "chateau"
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags,
                    "-I" + str(ROOT), str(cpp), "-o", str(binary)], check=True)
    try:
        result = subprocess.run([str(binary)], timeout=5)
    except subprocess.TimeoutExpired:
        raise SystemExit("FAIL: Chateau receipt/hook regression did not terminate")
    raise SystemExit(result.returncode)
