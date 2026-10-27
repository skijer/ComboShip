#pragma once
#include "SwordFinishIceData.h"
#include <fast/resource/type/DisplayList.h>
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

namespace SwordFinishIce {

// This material is reachable only through sword-origin ice clones. Raw, aligned
// masks have native I8 layout; the two tiles occupy 2560 of TMEM's 4096 bytes.
inline Gfx* Material() {
    static Gfx commands[] = {
        gsDPPipeSync(),
        gsDPSetCycleType(G_CYC_2CYCLE),
        gsDPSetTextureLUT(G_TT_NONE),
        // Frost controls color; the second mask controls opacity. Keep native
        // vertex lighting and caller ENV alpha so facets shade and fade.
        gsDPSetCombineLERP(ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, TEXEL1, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0,
                           COMBINED, 0, PRIMITIVE, 0),
        gsDPLoadMultiBlock(sSwordFinishIceFrostTex, 0, 0, G_IM_FMT_I, G_IM_SIZ_8b, 32, 16, 0, G_TX_WRAP, G_TX_WRAP, 5,
                           4, G_TX_NOLOD, G_TX_NOLOD),
        gsDPLoadMultiBlock(sSwordFinishIceOpacityTex, 64, 1, G_IM_FMT_I, G_IM_SIZ_8b, 64, 32, 0, G_TX_WRAP, G_TX_WRAP,
                           6, 5, G_TX_NOLOD, G_TX_NOLOD),
        // Reload the existing scroll after texture loads reset tile bounds.
        gsSPDisplayList(0x08000000),
        gsSPEndDisplayList(),
    };
    return commands;
}

inline bool Expanded(unsigned opcode) {
    return opcode == G_SETTIMG_OTR_HASH || opcode == G_DL_OTR_HASH || opcode == G_VTX_OTR_HASH ||
           opcode == G_BRANCH_Z_OTR || opcode == G_MARKER || opcode == G_MTX_OTR || opcode == G_MOVEMEM_OTR;
}

inline bool TextureState(unsigned opcode) {
    return opcode == G_SETTIMG || opcode == G_SETTIMG_OTR_HASH || opcode == G_SETTIMG_OTR_FILEPATH ||
           opcode == G_SETTILE || opcode == G_SETTILESIZE || opcode == G_LOADBLOCK || opcode == G_LOADTILE ||
           opcode == G_LOADTLUT || opcode == G_TEXTURE || opcode == G_SETCOMBINE || opcode == G_SETOTHERMODE_H ||
           opcode == G_SETOTHERMODE_L || opcode == G_RDPSETOTHERMODE || opcode == G_SETPRIMCOLOR ||
           opcode == G_SETENVCOLOR;
}

inline bool Known(unsigned opcode) {
    switch (opcode) {
        case G_NOOP:
        case G_SPNOOP:
        case G_MARKER:
        case G_RDPPIPESYNC:
        case G_RDPTILESYNC:
        case G_RDPLOADSYNC:
        case G_RDPFULLSYNC:
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L:
        case G_RDPSETOTHERMODE:
        case G_TEXTURE:
        case G_SETCOMBINE:
        case G_SETTIMG:
        case G_SETTIMG_OTR_HASH:
        case G_SETTIMG_OTR_FILEPATH:
        case G_SETTILE:
        case G_SETTILESIZE:
        case G_LOADBLOCK:
        case G_LOADTILE:
        case G_LOADTLUT:
        case G_GEOMETRYMODE:
        case G_EXTRAGEOMETRYMODE:
        case G_MOVEWORD:
        case G_MOVEMEM:
        case G_MOVEMEM_OTR:
        case G_MTX:
        case G_MTX_OTR:
        case G_MTX_OTR_FILEPATH:
        case G_POPMTX:
        case G_VTX:
        case G_VTX_OTR_HASH:
        case G_VTX_OTR_FILEPATH:
        case G_VTX_WIDE:
        case G_MODIFYVTX:
        case G_CULLDL:
        case G_TRI1:
        case G_TRI2:
        case G_QUAD:
        case G_LINE3D:
        case G_TRI1_OTR:
        case G_TRI1_WIDE:
        case G_DL:
        case G_SETFOGCOLOR:
        case G_SETBLENDCOLOR:
        case G_SETPRIMDEPTH:
            return true;
        default:
            return false;
    }
}

// Inspect real commands, including the payload words of extended hash opcodes.
// Never guess a DL index or reinterpret a hash payload as an instruction.
inline std::optional<size_t> Boundary(const std::vector<Gfx>& commands, bool combined) {
    if (commands.empty() || commands.size() > 1024)
        return std::nullopt;
    std::optional<size_t> geometry;
    bool texture = false, combiner = false;
    for (size_t index = 0; index < commands.size();) {
        const auto& command = commands[index];
        const unsigned opcode = command.words.w0 >> 24;
        const size_t length = Expanded(opcode) ? 2 : 1;
        if (index + length > commands.size())
            return std::nullopt;
        if (opcode == G_ENDDL) {
            if (index + 1 != commands.size() || !texture || !combiner || (combined && !geometry))
                return std::nullopt;
            return combined ? geometry : std::optional<size_t>(index);
        }
        // Unknown shader/control opcodes, resource-owned color envelopes and
        // segment rebinding cannot be proven compatible with the caller.
        if (!Known(opcode) || (opcode == G_MOVEWORD && ((command.words.w0 >> 16) & 255) == G_MW_SEGMENT))
            return std::nullopt;
        const bool vertex =
            opcode == G_VTX || opcode == G_VTX_OTR_HASH || opcode == G_VTX_OTR_FILEPATH || opcode == G_VTX_WIDE;
        if (vertex) {
            if (!combined)
                return std::nullopt;
            if (!geometry)
                geometry = index;
        }
        // A material hidden in a nested list, branch or later mesh section
        // could overwrite the scoped material. Preserve that source as-is.
        if (opcode == G_DL_OTR_HASH || opcode == G_DL_OTR_FILEPATH || opcode == G_BRANCH_Z_OTR ||
            opcode == G_BRANCH_Z ||
            (opcode == G_DL && (command.words.w1 != 0x08000000 || ((command.words.w0 >> 16) & 255) != G_DL_PUSH)) ||
            (!geometry && (opcode == G_TRI1 || opcode == G_TRI2 || opcode == G_QUAD || opcode == G_LINE3D ||
                           opcode == G_TRI1_OTR || opcode == G_TRI1_WIDE)) ||
            (geometry && TextureState(opcode)))
            return std::nullopt;
        texture |= opcode == G_SETTIMG || opcode == G_SETTIMG_OTR_HASH || opcode == G_SETTIMG_OTR_FILEPATH;
        combiner |= opcode == G_SETCOMBINE;
        index += length;
    }
    return std::nullopt;
}

class Cache {
    struct Snapshot {
        std::shared_ptr<Fast::DisplayList> source;
        std::vector<Gfx> original;
        std::vector<Gfx> commands;
        uint64_t generation = 0;
        bool combined = false;
    };
    std::vector<std::shared_ptr<Snapshot>> snapshots;
    std::shared_ptr<Snapshot> current;
    uint32_t lastFrame = 0;
    uint64_t generation = 0;

  public:
    Gfx* Get(const std::shared_ptr<Fast::DisplayList>& source, uint32_t frame, bool combined) {
        if (!generation || lastFrame != frame) {
            ++generation;
            lastFrame = frame;
            // Retain retired command/string owners across submitted frames.
            std::erase_if(snapshots,
                          [&](const auto& item) { return item != current && generation - item->generation > 2; });
        }
        // Native and Alt/custom roots use the same structural proof. An
        // authored mesh is preserved exactly; unknown branching falls back.
        if (!source || !source->GetInitData() || source->UCode != ucode_f3dex2)
            return nullptr;
        for (const auto& item : snapshots) {
            if (item->source == source && item->combined == combined &&
                item->original.size() == source->Instructions.size() &&
                std::equal(
                    item->original.begin(), item->original.end(), source->Instructions.begin(),
                    [](const Gfx& a, const Gfx& b) { return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1; })) {
                current = item;
                current->generation = generation;
                return current->commands.data();
            }
        }
        const auto boundary = Boundary(source->Instructions, combined);
        // Bound simultaneous reloads without invalidating submitted pointers.
        if (!boundary || snapshots.size() >= 8)
            return nullptr;
        auto item = std::make_shared<Snapshot>();
        item->source = source; // Retain native filepath strings and source data.
        item->original = source->Instructions;
        item->commands = item->original;
        const Gfx call = gsSPDisplayList(Material());
        item->commands.insert(item->commands.begin() + *boundary, call);
        item->generation = generation;
        item->combined = combined;
        snapshots.push_back(item);
        current = std::move(item);
        return current->commands.data();
    }
    void Clear() {
        current.reset();
        snapshots.clear();
        generation = 0;
    }
};
} // namespace SwordFinishIce
