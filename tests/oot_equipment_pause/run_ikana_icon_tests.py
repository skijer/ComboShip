#!/usr/bin/env python3
"""Exercise the real pause probe, CPU resource resolver and graphics owner lookup.

Regression: an MM-owned Shield of Ikana exists, but the OoT-local preflight
rejects its routed name before the graphics interpreter can draw it. Archive
storage is the controlled boundary; production parsing, MQ behavior, resource
scope, pause rejection and renderer selection are compiled verbatim.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def fixture():
    resource = (ROOT / 'soh/soh/ResourceManagerHelpers.cpp').read_text()
    pause = (ROOT / 'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c').read_text()
    graphics = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
    scope = (ROOT / 'libultraship/include/ship/resource/ResourceManagerScope.h').read_text()
    scope = scope[scope.index('namespace Ship {'):]
    prefix = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "combo/ComboGameMode.h"
#include "combo/menu/ComboItemIconOwnership.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
#ifdef COMBO_BUILD
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#endif
namespace Ship {
class ResourceManager;
static const ResourceManager* CurrentOwner();
struct InitData { uint32_t Type; };
struct IResource {
    explicit IResource(uint32_t type) : init{type} {}
    virtual ~IResource() = default;
    InitData* GetInitData() { return &init; }
    virtual void* GetRawPointer() = 0;
    InitData init;
};
class ResourceManager {
  public:
    bool alt = false;
    bool observeOwner = false;
    std::map<std::string, std::shared_ptr<IResource>> files;
    std::shared_ptr<IResource> LoadResource(std::string path) {
        if (observeOwner) assert(CurrentOwner() == this && "routed CPU load needs its owner's Context");
        if (path.starts_with("__OTR__")) path.erase(0, 7);
        if (alt && files.contains("alt/" + path)) return files.at("alt/" + path);
        return files.contains(path) ? files.at(path) : nullptr;
    }
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        const bool previous = observeOwner;
        observeOwner = false; // renderer lookups do not use CPU factory scope
        auto result = LoadResource(path);
        observeOwner = previous;
        return result;
    }
};
class Context {
  public:
    static Context* GetRawInstance() { static Context context; return &context; }
    std::shared_ptr<ResourceManager> GetResourceManager() { return active; }
    void SetResourceManager(std::shared_ptr<ResourceManager> manager) { active = manager; }
    std::shared_ptr<ResourceManager> active;
};
class CrossRMRegistry {
  public:
    static inline std::map<std::string, std::shared_ptr<ResourceManager>> owners;
    static std::shared_ptr<ResourceManager> Get(const std::string& owner) {
        return owners.contains(owner) ? owners.at(owner) : nullptr;
    }
};
static const ResourceManager* CurrentOwner() { return Context::GetRawInstance()->GetResourceManager().get(); }
}
namespace Fast {
enum class ResourceType : uint32_t { Texture = 1, DisplayList = 2 };
struct Texture : Ship::IResource {
    explicit Texture(char marker, int size=32) : IResource(1), Width(size), Height(size) { pixels[0]=marker; }
    std::array<char, 4> pixels{};
    int Width, Height;
    void* GetRawPointer() override { return pixels.data(); }
};
struct DisplayList : Ship::IResource {
    DisplayList() : IResource(2), Instructions{17, 23} {}
    std::vector<int> Instructions;
    void* GetRawPointer() override { return Instructions.data(); }
};
}
namespace SOH {
enum class ResourceType : uint32_t { SOH_Array = 3 };
struct Array : Ship::IResource {
    Array() : IResource(3), Vertices{29, 31} {}
    std::vector<int> Vertices;
    void* GetRawPointer() override { return Vertices.data(); }
};
}
static bool mq;
bool ResourceMgr_IsGameMasterQuest() { return mq; }
static auto host = std::make_shared<Ship::ResourceManager>();
static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() { return host; }
'''
    production = '\n'.join((scope,
        function(resource, 'std::shared_ptr<Ship::IResource> ResourceMgr_GetResourceByNameHandlingMQ('),
        function(resource, 'extern "C" char* ResourceMgr_GetResourceDataByNameHandlingMQ('),
        function(resource, 'extern "C" char* ResourceMgr_LoadTexOrDListByName('),
        function(graphics, 'static std::shared_ptr<Fast::Texture> ComboLoadTextureResource(')))
    production += '\nstatic void* PauseProbe(void* extIcon) {\n'
    production += function(pause, 'if ((extIcon != NULL) && (ResourceMgr_LoadTexOrDListByName(')
    production += '\nreturn extIcon;\n}\n'
    checks = r'''
int main() {
    using namespace Ship;
    auto* context = Context::GetRawInstance();
    context->SetResourceManager(host);
    auto mm = std::make_shared<ResourceManager>();
    mm->observeOwner = true;
    CrossRMRegistry::owners["mm"] = mm;
    const char* native = "textures/icon_item_custom/gItemIconShieldOfIkanaTex";
    // Competing original Mirror mods must not override Ikana's private resource.
    host->files["textures/icon_item_static/gItemIconShieldMirrorTex"] = std::make_shared<Fast::Texture>('O');
    host->files["icon_item_static_yar/gItemIconMirrorShieldTex"] = std::make_shared<Fast::Texture>('X');
    mm->files["icon_item_static_yar/gItemIconMirrorShieldTex"] = std::make_shared<Fast::Texture>('M');
    auto vanilla = std::make_shared<Fast::Texture>('V');
    auto hd = std::make_shared<Fast::Texture>('H', 512);
    auto wrongHost = std::make_shared<Fast::Texture>('X');
    mm->files[native] = vanilla;
    mm->files[std::string("alt/") + native] = hd;
    host->files[native] = wrongHost;
#ifdef COMBO_BUILD
    const auto icon = const_cast<char*>(COMBO_IKANA_SHIELD_ICON);
    assert(PauseProbe(icon) == icon && "OoT pause rejects an available vanilla MM-owned Ikana shield");
    assert(ResourceMgr_LoadTexOrDListByName(icon) == vanilla->pixels.data());
    assert(ComboLoadTextureResource(icon) == vanilla);
    assert(context->GetResourceManager() == host);
    mm->alt = true;
    assert(PauseProbe(icon) == icon && "HD Ikana icon must survive the same pause preflight");
    assert(ResourceMgr_LoadTexOrDListByName(icon) == hd->pixels.data());
    auto rendered = ComboLoadTextureResource(icon);
    assert(rendered == hd && rendered->Width == 512 && rendered->Height == 512);
    assert(context->GetResourceManager() == host && "foreign probing leaked the MM context");
    host->alt = true;
    mm->alt = false;
    assert(ResourceMgr_LoadTexOrDListByName(icon) == vanilla->pixels.data());
    CrossRMRegistry::owners.erase("mm");
    assert(!PauseProbe(icon) && "missing MM owner must not use OoT's same-named shield");
    assert(!ComboLoadTextureResource(icon));
    CrossRMRegistry::owners["mm"] = mm;
    mm->files.erase(native);
    assert(!PauseProbe(icon) && "missing texture must still be rejected to avoid stale TMEM");
    mm->files[native] = vanilla;
    assert(!ResourceMgr_LoadTexOrDListByName("__OTR__@:invalid"));
    assert(!ResourceMgr_LoadTexOrDListByName("__OTR__@mm"));
    assert(!ResourceMgr_LoadTexOrDListByName("__OTR__@unknown:missing"));
    auto dl = std::make_shared<Fast::DisplayList>();
    auto array = std::make_shared<SOH::Array>();
    mm->files["test/dl"] = dl;
    mm->files["test/array"] = array;
    assert(ResourceMgr_LoadTexOrDListByName("__OTR__@mm:test/dl") == reinterpret_cast<char*>(dl->Instructions.data()));
    assert(ResourceMgr_LoadTexOrDListByName("__OTR__@mm:test/array") == reinterpret_cast<char*>(array->Vertices.data()));
    assert(!ResourceMgr_LoadTexOrDListByName(nullptr));
    CrossRMRegistry::owners.erase("mm");
    gComboGameMode = COMBO_GAME_OOT;
    const auto soloIcon = const_cast<char*>(COMBO_IKANA_SHIELD_ICON);
    assert(!std::strcmp(soloIcon, "__OTR__textures/icon_item_custom/gItemIconShieldOfIkanaTex") &&
           "runtime Solo OoT must use the local canonical MM icon instead of an absent MM owner");
    assert(PauseProbe(soloIcon) == soloIcon);
    CwItemReceiptPresentation receipt{};
    receipt.singleBox = 1;
    CwItemIconInfo receiptIcon{};
    receiptIcon.path = soloIcon;
    receiptIcon.width = receiptIcon.height = 32;
    assert(ComboReceipt_CopyIcon(&receipt, &receiptIcon, "oot"));
    assert(!std::strcmp(receipt.iconPath, soloIcon) && "Solo OoT receipt must preserve the local MM icon route");
    CrossRMRegistry::owners["mm"] = mm;
    gComboGameMode = COMBO_GAME_MM;
    assert(!std::strcmp(COMBO_IKANA_SHIELD_ICON,
                        "__OTR__@mm:textures/icon_item_custom/gItemIconShieldOfIkanaTex"));
    assert(PauseProbe(const_cast<char*>(COMBO_IKANA_SHIELD_ICON)));
    gComboGameMode = COMBO_GAME_BOTH;
    std::puts("PASS: vanilla/HD Ikana pause drawing, MM ownership, independent Alt states, context restoration, missing-resource rejection and routed DL/array probes");
#else
    const auto icon = const_cast<char*>(COMBO_IKANA_SHIELD_ICON);
    assert(PauseProbe(icon) == icon && "standalone native icon probing regressed");
    assert(ResourceMgr_LoadTexOrDListByName(icon) == wrongHost->pixels.data());
    assert(ComboLoadTextureResource(icon) == wrongHost);
    std::puts("PASS: standalone uses its existing native archive path");
#endif
    host->files["scenes/mq/test"] = vanilla;
    mq = true;
    assert(ResourceMgr_LoadTexOrDListByName("__OTR__scenes/nonmq/test") == vanilla->pixels.data());
    assert(!PauseProbe(const_cast<char*>("__OTR__missing/local/icon")));
    assert(context->GetResourceManager() == host);
    std::puts("PASS: existing OoT MQ translation and missing local-icon guard");
}
'''
    return prefix + production + checks


def main():
    with tempfile.TemporaryDirectory(prefix='ikana-pause-icon-') as temporary:
        root = Path(temporary)
        source = root / 'test.cpp'
        source.write_text(fixture())
        for name, flags in (('combo', ['-DCOMBO_BUILD']), ('standalone', [])):
            binary = root / name
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(ROOT), *flags, str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
