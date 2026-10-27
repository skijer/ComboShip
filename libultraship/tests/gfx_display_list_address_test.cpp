// The crash's opcode stream begins T,c,i,R: bytes 3,19,35,51 of the child-eye
// OTR path. Exercise the production dispatch that must never execute that text.
#include "fast/lus_gbi.h"
#undef GIMMCMD
#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Texture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>

alignas(8) static char sEyePath[] = "__OTR__objects/object_link_child/gLinkChildEyesRollRightTex";
alignas(8) static char sMouthPath[] = "__OTR__objects/object_link_child/gLinkChildMouth1Tex";

namespace Ship {
class ResourceManager {
  public:
    bool OtrSignatureCheck(const char* path);
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        assert(std::strcmp(path, sEyePath) == 0 || std::strcmp(path, sMouthPath) == 0);
        ++loads;
        return resource;
    }
    std::shared_ptr<IResource> resource;
    unsigned loads = 0;
};
#include "dl_signature.inc"
} // namespace Ship

namespace Fast {
class Interpreter {
  public:
    uintptr_t mSegmentPointers[16]{};
    void* SegAddr(uintptr_t address);
};
static auto sInterpreter = std::make_shared<Interpreter>();
static std::weak_ptr<Interpreter> mInstance = sInterpreter;
static auto sManager = std::make_shared<Ship::ResourceManager>();
[[maybe_unused]] static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() {
    return sManager;
}
struct StackSink {
    F3DGfx* target = nullptr;
    unsigned calls = 0;
    unsigned branches = 0;
    void call(F3DGfx*, F3DGfx* destination) {
        ++calls;
        target = destination;
    }
    void branch(F3DGfx*) {
        ++branches;
    }
} g_exec_stack;

#define SPDLOG_ERROR(...) ((void)0)
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#include "dl_address_production.inc"

static void Submit(uintptr_t address, F3DGfx* expected, bool indexed, bool branch) {
    F3DGfx packet{};
    packet.words.w0 = 0xDE000000 | (branch ? 0x10000 : 0);
    packet.words.w1 = address;
    auto* command = &packet;
    g_exec_stack = {};
    bool jumped = indexed ? gfx_dl_index_handler(sInterpreter.get(), &command) : gfx_dl_handler_common(sInterpreter.get(), &command);
    if (expected == nullptr) {
        assert(!jumped && command == &packet);
        assert(g_exec_stack.calls == 0 && g_exec_stack.branches == 0);
    } else if (branch) {
        assert(jumped && command == expected);
        assert(g_exec_stack.calls == 0 && g_exec_stack.branches == 1);
    } else {
        assert(!jumped && command == &packet);
        if (g_exec_stack.target != expected) {
            std::fputs("FAIL: segmented eye/mouth DL executed OTR text instead of the resolved display list\n", stderr);
            std::abort();
        }
        assert(g_exec_stack.calls == 1 && g_exec_stack.branches == 0);
    }
}
} // namespace Fast

int main() {
    using namespace Fast;
    auto display = std::make_shared<DisplayList>();
    display->Instructions = { gsDPPipeSync(), gsSPEndDisplayList() };
    auto* expected = reinterpret_cast<F3DGfx*>(display->Instructions.data());
    assert(sEyePath[3] == 'T' && sEyePath[19] == 'c' && sEyePath[35] == 'i' && sEyePath[51] == 'R');
    for (bool indexed : { false, true }) {
        for (unsigned segment : { 8u, 9u }) {
            const uintptr_t address = (segment << 24) | (indexed ? 0 : 1);
            sInterpreter->mSegmentPointers[segment] = reinterpret_cast<uintptr_t>(segment == 8 ? sEyePath : sMouthPath);
            for (bool branch : { false, true }) {
                // Resolve again after each simulated asset change. A texture with
                // the same path must never be cast or dispatched as a display list.
                sManager->resource = display;
                Submit(address, expected, indexed, branch);
                Submit(address + (indexed ? 1 : sizeof(F3DGfx)), expected + 1, indexed, branch);
                Submit(address + (indexed ? 2 : 2 * sizeof(F3DGfx)), nullptr, indexed, branch);
                if (!indexed) {
                    Submit(address + 2, nullptr, indexed, branch);
                }
                sManager->resource = std::make_shared<Texture>();
                Submit(address, nullptr, indexed, branch);
                sManager->resource.reset();
                Submit(address, nullptr, indexed, branch);
                sManager->resource = std::make_shared<DisplayList>();
                Submit(address, nullptr, indexed, branch);
                sManager->resource = display;
                Submit(address, expected, indexed, branch);
            }
            // Normal, already resolved command buffers and offsets still work.
            sInterpreter->mSegmentPointers[segment] = reinterpret_cast<uintptr_t>(expected);
            const auto loads = sManager->loads;
            Submit(address, expected, indexed, false);
            Submit(address, expected, indexed, true);
            Submit(address + (indexed ? 1 : sizeof(F3DGfx)), expected + 1, indexed, false);
            assert(sManager->loads == loads);
            sInterpreter->mSegmentPointers[segment] = 0;
            Submit(address, nullptr, indexed, false);
            Submit(address, nullptr, indexed, true);
        }
    }
    Submit(reinterpret_cast<uintptr_t>(expected), expected, false, false);
    Submit(reinterpret_cast<uintptr_t>(expected), expected, false, true);
    sManager->resource = display;
    Submit(reinterpret_cast<uintptr_t>(sEyePath), expected, false, false);
    Submit(0, nullptr, false, false);
    std::puts("PASS production raw/indexed DL dispatch: eye/mouth OTR resolution, Alt type changes, missing/empty "
              "resources, call/branch semantics, raw buffers, offsets and unbound segments");
}
