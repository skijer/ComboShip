#!/usr/bin/env python3
"""Execute production equipment arbitration and Pendant eligibility without renderer dependencies.

Catches the enabled-Pendant activation rejection and a Pendant moveset running
over Wolf's controls. Unrelated body/tool owners are fixture boundaries.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def body(path, signature):
    source = (ROOT / path).read_text()
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


production = body('mm/mods/extended_equipment.c', 'u8 ExtEquip_PendantActive(void) {')
production += '\n' + body('mm/mods/forms/wolf_link_host.cpp', 'const char* OwnerRejection(Player* player) {')
fixture = r'''
#include <cstdio>
#include <cstdlib>
using u8 = unsigned char;
struct Player {};
struct Save { u8 pendantOwned = 1; u8 pendantEffectOff = 0; } save;
static u8 selected, otherForm, mario, pakBody, o2rBody, beetle, kite, trident, customAction;
constexpr int CUSTOM_FORM_NONE = 0;
Save* Nei_Save() { return &save; }
u8 ExtEquip_PendantOwned() { return save.pendantOwned; }
u8 WolfLinkForm_IsSelected() { return selected; }
int CustomForms_ActiveForm() { return otherForm; }
u8 Sm64Mario_IsActive() { return mario; }
int CVarGetInteger(const char*, int) { return 0; }
u8 PakLoader_HasActiveBodyModel() { return pakBody; }
u8 O2rLoader_HasActiveModel() { return o2rBody; }
u8 Beetle_IsFlying() { return beetle; }
u8 KiteSurf_IsActive() { return kite; }
u8 Trident_OwnsPlayerAction() { return trident; }
int CustomItems_BlocksMovement(Player*) { return customAction; }
void check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL equipment arbitration: %s\n", message); std::exit(1); }
}
'''
checks = r'''
int main() {
    Player player;
    check(ExtEquip_PendantActive(), "Pendant must work for human Link");
    check(OwnerRejection(&player) == nullptr, "enabled equipment must not reject Wolf activation");
    selected = 1;
    check(!ExtEquip_PendantActive(), "Pendant moveset must yield to Wolf");
    check(save.pendantOwned == 1 && save.pendantEffectOff == 0, "Wolf must preserve saved equipment preferences");
    selected = 0;
    check(ExtEquip_PendantActive(), "Pendant must resume when Wolf ends");
    save.pendantEffectOff = 1;
    selected = 1;
    check(!ExtEquip_PendantActive(), "disabled Pendant must stay disabled in Wolf");
    selected = 0;
    check(!ExtEquip_PendantActive(), "disabled Pendant must stay disabled after Wolf");
    save.pendantOwned = 0;
    save.pendantEffectOff = 0;
    check(!ExtEquip_PendantActive(), "unowned Pendant must remain inactive");
    for (u8* owner : { &otherForm, &mario, &pakBody, &o2rBody, &beetle, &kite, &trident, &customAction }) {
        *owner = 1;
        check(OwnerRejection(&player) != nullptr, "active body/tool actions must retain ownership");
        *owner = 0;
    }
    std::puts("PASS MM Wolf equipment activation, moveset suspension and saved-toggle restoration");
}
'''
with tempfile.TemporaryDirectory(prefix='wolf-equipment-') as directory:
    source = Path(directory) / 'policy.cpp'
    source.write_text('#include <initializer_list>\n' + fixture + production + checks)
    binary = Path(directory) / 'policy'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
