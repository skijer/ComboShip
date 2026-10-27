#!/usr/bin/env python3
"""Reject unavailable MM pause icons without changing their owner or Alt choice.

The selector, typed availability query, registry and renderer owner lookup are
production code, with real Fast::Texture objects. Resource-manager lookup and
Context are controlled seams. --source-rev supports the same RED check against
the unchanged candidate parent, where the query does not exist.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


def fixture(read):
    pause = read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c')
    resource = read('mm/2s2h/BenPort.cpp')
    graphics = read('libultraship/src/fast/interpreter.cpp')
    helper = 'static std::shared_ptr<Fast::Texture> MM_LoadRgba32Texture('
    query = 'extern "C" uint8_t MM_IsRgba32TextureAvailable('
    production = function(resource, helper) + '\n' if helper in resource else ''
    production += function(resource, query) + '\n' if query in resource else ''
    if query in resource:
        production += '#define HAS_TYPED_QUERY\n'
    production += function(resource, 'extern "C" uint8_t MM_GetTextureVisibleCenter(') + '\n'
    production += function(pause, 'static void* KaleidoEquip_OotTex(') + '\n'
    production += function(graphics, 'static std::shared_ptr<Fast::Texture> ComboLoadTextureResource(')
    prefix = r'''
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include "combo/menu/ComboItemIconOwnership.h"
#include "fast/resource/type/Texture.h"
#include "ship/resource/CrossRMRegistry.h"
using u8 = uint8_t;
#ifdef COMBO_BUILD
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#endif
namespace Ship {
struct ResourceManager {
    bool alt = false;
    std::map<std::string, std::shared_ptr<IResource>> files;
    std::shared_ptr<IResource> LoadResourceProcess(std::string path) {
        if (path.starts_with("__OTR__")) path.erase(0, 7);
        if (alt && files.contains("alt/" + path)) return files.at("alt/" + path);
        return files.contains(path) ? files.at(path) : nullptr;
    }
};
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    std::shared_ptr<ResourceManager> active;
    std::shared_ptr<ResourceManager> GetResourceManager() { return active; }
    void SetResourceManager(std::shared_ptr<ResourceManager> manager) { active = manager; }
};
struct OtherResource : Resource<uint8_t> {
    OtherResource() : Resource(std::make_shared<ResourceInitData>()) {}
    uint8_t* GetPointer() override { return nullptr; }
    size_t GetPointerSize() override { return 0; }
};
}
static std::shared_ptr<Fast::Texture> makeTexture(char marker, uint16_t size = 32) {
    auto texture = std::make_shared<Fast::Texture>();
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->Width = texture->Height = size;
    texture->ImageDataSize = uint32_t(size) * size * 4;
    texture->mImageBuffer = std::make_shared<std::vector<char>>(texture->ImageDataSize, marker);
    texture->ImageData = reinterpret_cast<uint8_t*>(texture->mImageBuffer->data());
    return texture;
}
static auto host = std::make_shared<Ship::ResourceManager>();
static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() { return host; }
static bool localBase, localAlt, localAltEnabled, donorAvailable;
u8 ResourceMgr_FileExists(const char*) { return localBase; }
u8 ResourceMgr_FileAltExists(const char*) { return localAlt; }
bool ResourceMgr_IsAltAssetsEnabled(void) { return localAltEnabled; }
int NeiResource_Available(const char*) { return donorAvailable; }
const char* NeiResource_Route(const char*) { return "__OTR__@oot:imported"; }
static void check(bool ok, const char* label) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}
'''
    checks = r'''
int main() {
    using namespace Ship;
    Context::GetRawInstance()->active = host;
    auto mm = std::make_shared<ResourceManager>();
    const char* key = "textures/icon_item_custom/gItemIconShieldOfIkanaTex";
    const char* native = "__OTR__textures/icon_item_custom/gItemIconShieldOfIkanaTex";
    const char* routed = "__OTR__@mm:textures/icon_item_custom/gItemIconShieldOfIkanaTex";
    const char* oldNative = "__OTR__icon_item_static_yar/gItemIconMirrorShieldTex";
    auto stock = makeTexture('V');
    auto hd = makeTexture('H', 512);
    auto wrongHost = makeTexture('X');
    host->files[key] = wrongHost;
    host->files["icon_item_static_yar/gItemIconMirrorShieldTex"] = wrongHost;
    mm->files["icon_item_static_yar/gItemIconMirrorShieldTex"] = wrongHost;
#ifdef COMBO_BUILD
    CrossRMRegistry::Register("mm", mm);
    auto owner = mm;
    const char* expected = routed;
#else
    (void)routed;
    auto owner = host;
    const char* expected = native;
    owner->files.erase(key);
#endif
    // The previous cell's art and both ordinary Mirror overrides are present.
    // Missing private resources must still yield no submitted texture path.
    check(KaleidoEquip_OotTex(native) == nullptr, "missing private Ikana texture must not be submitted");
    check(KaleidoEquip_OotTex(oldNative) == nullptr, "legacy MM alias cannot borrow ordinary Mirror art");
    check(KaleidoEquip_OotTex(nullptr) == nullptr, "null input stays empty");
    owner->files[key] = stock;
#ifdef HAS_TYPED_QUERY
    check(!MM_IsRgba32TextureAvailable(nullptr), "null query input is unavailable");
    for (const char* invalid : {"__OTR__@mm", "__OTR__@:missing", "__OTR__@missing:icon", "__OTR__@mm:"})
        check(!MM_IsRgba32TextureAvailable(invalid), "malformed/missing explicit owner cannot borrow host art");
    Context::GetRawInstance()->active = nullptr;
    check(!MM_IsRgba32TextureAvailable(native), "unowned query cannot load without an active manager");
#ifdef COMBO_BUILD
    check(MM_IsRgba32TextureAvailable(routed), "explicit owner works without an active local manager");
#endif
    Context::GetRawInstance()->active = host;
#endif
    for (const char* alias : {native, oldNative}) {
        const auto selected = static_cast<const char*>(KaleidoEquip_OotTex(alias));
        check(selected && !std::strcmp(selected, expected), "stock icon retains its private owner path");
        check(ComboLoadTextureResource(selected) == stock, "submitted stock path resolves the selected owner");
    }
    owner->files[std::string("alt/") + key] = hd;
    owner->alt = true;
    check(ComboLoadTextureResource(static_cast<const char*>(KaleidoEquip_OotTex(native))) == hd,
          "owning Alt HD texture stays selected");
    owner->files.erase(key);
    check(KaleidoEquip_OotTex(native) != nullptr, "enabled Alt-only texture remains usable");
    owner->alt = false;
    check(KaleidoEquip_OotTex(native) == nullptr, "disabled Alt-only texture cannot fill missing stock");
    owner->files[key] = stock;
    owner->files.erase(std::string("alt/") + key);
    owner->alt = true;
    check(ComboLoadTextureResource(static_cast<const char*>(KaleidoEquip_OotTex(native))) == stock,
          "missing Alt texture retains ordinary stock fallback");
    owner->files[std::string("alt/") + key] = std::make_shared<OtherResource>();
    check(KaleidoEquip_OotTex(native) == nullptr, "selected non-texture Alt cannot be submitted");
    owner->alt = false;
    owner->files[key] = std::make_shared<OtherResource>();
    check(KaleidoEquip_OotTex(native) == nullptr, "non-texture private entry cannot be submitted");
    for (int defect = 0; defect < 5; ++defect) {
        auto invalid = makeTexture('B');
        if (defect == 0) invalid->ImageData = nullptr;
        if (defect == 1) invalid->Width = 0;
        if (defect == 2) invalid->Height = 0;
        if (defect == 3) invalid->ImageDataSize = 1;
        if (defect == 4) invalid->Type = Fast::TextureType::RGBA16bpp;
        owner->files[key] = invalid;
        check(KaleidoEquip_OotTex(native) == nullptr, "invalid RGBA32 tile cannot be submitted");
    }
    owner->files[key] = stock;
#ifdef COMBO_BUILD
    host->alt = true;
    check(ComboLoadTextureResource(static_cast<const char*>(KaleidoEquip_OotTex(native))) == stock,
          "host Alt does not replace MM owner's selected stock");
    CrossRMRegistry::Unregister("mm");
    check(KaleidoEquip_OotTex(native) == nullptr, "missing MM owner cannot borrow host's private art");
    gComboGameMode = COMBO_GAME_OOT;
    check(ComboLoadTextureResource(static_cast<const char*>(KaleidoEquip_OotTex(native))) == wrongHost,
          "Solo OoT retains the local private resource route");
    CrossRMRegistry::Register("mm", mm);
    gComboGameMode = COMBO_GAME_MM;
    check(ComboLoadTextureResource(static_cast<const char*>(KaleidoEquip_OotTex(native))) == stock,
          "Solo MM retains MM ownership");
#endif
    float x = -1, y = -1;
    check(MM_GetTextureVisibleCenter(expected, &x, &y) && x == .5f && y == .5f,
          "shared typed loader preserves visible-center result");
    auto transparent = makeTexture(0);
    owner->files[key] = transparent;
    check(KaleidoEquip_OotTex(native) != nullptr, "availability does not scan or require visible alpha");
    check(!MM_GetTextureVisibleCenter(expected, &x, &y), "transparent artwork retains absent visible center");
    const char* ootMirror = "__OTR__textures/icon_item_static/gItemIconShieldMirrorTex";
    localBase = true;
    check(KaleidoEquip_OotTex(ootMirror) == ootMirror, "ordinary OoT Mirror path stays independent");
    localBase = false;
    donorAvailable = true;
    check(!std::strcmp(static_cast<const char*>(KaleidoEquip_OotTex(ootMirror)), "__OTR__@oot:imported"),
          "unrelated imported icon retains donor availability policy");
    check(Context::GetRawInstance()->GetResourceManager() == host, "availability query preserves active context");
    std::puts("PASS MM pause missing/wrong-type icon rejection, private stock/HD/Alt selection and owner isolation");
}
'''
    registry = read('libultraship/src/ship/resource/CrossRMRegistry.cpp')
    registry = registry[registry.index('namespace Ship {'):]
    return prefix + registry + production + checks


def native_header_checks(directory, read):
    # BenPort's C++ declarations need only a Context forward declaration here;
    # no window, audio or log subsystem is constructed for an ABI syntax check.
    (directory / 'ship').mkdir()
    (directory / 'ship/Context.h').write_text(
        '#pragma once\n#include <cstdint>\n#include <cstddef>\n#include <string>\n'
        '#include <memory>\nnamespace Ship { class Context; }\n')
    flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0', '-I' + str(directory)]
    flags += ['-I' + str(ROOT / path) for path in
              ('', 'mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
               'libultraship/include', 'combo', 'combo/menu')]
    pause = read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c')
    for language, compiler, standard in (('c', 'CC', 'gnu11'), ('cpp', 'CXX', 'c++20')):
        source = directory / ('native.' + language)
        prefix = '#include "z64.h"\n' if language == 'c' else ''
        prefix += '#include "mm/2s2h/BenPort.h"\n'
        prefix += 'static uint8_t (*query)(const char*) = &MM_IsRgba32TextureAvailable;\n'
        if language == 'c':
            prefix += '#include "combo/menu/ComboItemIconOwnership.h"\n'
            prefix += '#include "mm/2s2h/Rando/NeiResourceRouting.h"\n'
            prefix += function(pause, 'static void* KaleidoEquip_OotTex(')
        source.write_text(prefix)
        warning = '-Werror=implicit-function-declaration' if language == 'c' else '-Werror'
        result = subprocess.run([os.environ.get(compiler, 'cc' if language == 'c' else 'c++'),
                                 '-std=' + standard, *flags, warning, '-fsyntax-only', str(source)],
                                capture_output=True, text=True)
        if result.returncode:
            print(result.stdout + result.stderr)
        result.check_returncode()
        scope = 'declaration and native pause syntax' if language == 'c' else 'declaration'
        print(f'PASS actual BenPort {language} {scope} '
              f'({result.stderr.count("warning:")} existing header warnings)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-rev', help='read production source from an exact Git revision')
    args = parser.parse_args()

    def read(path):
        if args.source_rev:
            return subprocess.check_output(['git', 'show', args.source_rev + ':' + path], cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    with tempfile.TemporaryDirectory(prefix='ikana-missing-icon-') as temporary:
        path = Path(temporary)
        source = path / 'test.cpp'
        source.write_text(fixture(read))
        (path / 'spdlog').mkdir()
        (path / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        for name, flags in (('combo', ['-DCOMBO_BUILD']), ('standalone', [])):
            binary = path / name
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(path), '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                            *flags, str(source), str(ROOT / 'libultraship/src/ship/resource/Resource.cpp'),
                            str(ROOT / 'libultraship/src/fast/resource/type/Texture.cpp'),
                            '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
        native_header_checks(path, read)


if __name__ == '__main__':
    main()
