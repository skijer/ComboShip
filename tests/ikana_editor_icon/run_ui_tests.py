#!/usr/bin/env python3
"""Click production MM equipment editor with the pinned Dear ImGui input engine."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
parser=argparse.ArgumentParser();parser.add_argument('--source-rev');args=parser.parse_args()
def read(path):
    return (subprocess.check_output(['git','show',args.source_rev+':'+path],cwd=ROOT).decode()
            if args.source_rev else (ROOT/path).read_text())
editor=read('mm/2s2h/DeveloperTools/SaveEditor.cpp')
start=editor.index('    static const char* sNeiEquipTypeNames[4]')
end=editor.index('    // Upgrade-column passive toggles',start)
production='void DrawEditor() {\n'+editor[start:end]+'\n}\n'
equip=read('mm/mods/extended_equipment.c')
production='\n'.join(function(equip,n) for n in ('ExtEquip_GetBit','ExtEquip_HasItem','ExtEquip_GiveItem','ExtEquip_RemoveItem'))+'\n'+production
prefix=r'''
#include <imgui.h>
#include <imgui_internal.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using s16=int16_t; using u8=uint8_t; using u32=uint32_t;
#define EQUIP_TYPE_SHIELD 1
#define EXT_EQUIP_OWNED_SHIFT 16
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
struct Save { u32 extEquipOwnedBits=0; u16 shieldOwned=FC_SHIELD_MIRROR_OOT; u8 vanillaShieldSkin=2; } state;
Save* Nei_Save() { return &state; }
u8 ExtEquip_GetCurrent(s16) { return 0; }
void ExtEquip_SetSlot(s16,u8) { assert(false); }
#define GET_CUR_EQUIP_VALUE(type) 0
#define EQUIP_VALUE_SHIELD_MIRROR 2
#define EQUIP_VALUE_SHIELD_NONE 0
void Inventory_ChangeEquipment(int) { assert(false); }
ImRect target;
bool found=false;
bool disabled=false;
void ImGuiTestEngineHook_ItemAdd(ImGuiContext*,ImGuiID,const ImRect&,const ImGuiLastItemData*) {}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx,ImGuiID,const char* label,ImGuiItemStatusFlags flags) {
 if (!strcmp(label,"Shield of Ikana")) {
  target=ctx->LastItemData.Rect;found=true;
  disabled=(ctx->LastItemData.ItemFlags & ImGuiItemFlags_Disabled)!=0;
 }
}
void ImGuiTestEngineHook_Log(ImGuiContext*,const char*,...) {}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*,ImGuiID) { return ""; }
'''.replace('u16 shieldOwned','uint16_t shieldOwned')
checks=r'''
void frame() {
 ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(800,500));
 ImGui::Begin("Editor",nullptr,ImGuiWindowFlags_NoSavedSettings);DrawEditor();ImGui::End();ImGui::Render();
}
int main() {
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(900,600);
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);GImGui->TestEngineHookItems=true;
 frame();frame();assert(found && "current editor lacks an Ikana ownership button");
 assert(!disabled && !ExtEquip_HasItem(1,3) && state.shieldOwned==FC_SHIELD_MIRROR_OOT);
 const auto center=target.GetCenter();
 for (int edit=0;edit<2;++edit) {
  io.AddMousePosEvent(center.x,center.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();
  assert(ExtEquip_HasItem(1,3));
  assert(state.shieldOwned==(FC_SHIELD_MIRROR_OOT|FC_SHIELD_IKANA));
  assert(disabled && "owned Ikana must use the same disabled grant button as sibling equipment");
 }
 ImGui::DestroyContext();puts("PASS real ImGui Ikana button: unowned/enabled, owned/disabled, repeat click retains ownership, idle drawing preserves save");
}
'''
imgui=Path(os.environ['IMGUI_DIR'])
with tempfile.TemporaryDirectory() as temp:
 p=Path(temp)/'test.cpp';p.write_text(prefix+production+checks);binary=Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DIMGUI_ENABLE_TEST_ENGINE','-I'+str(ROOT),'-I'+str(imgui),str(p),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')],'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
