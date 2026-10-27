#ifndef FAIRY_BOTTLE_GLASS_COMMANDS_H
#define FAIRY_BOTTLE_GLASS_COMMANDS_H

#include <map>

struct AssetCommand {
    std::string tag;
    std::map<std::string, std::string> attributes;
    bool operator==(const AssetCommand&) const = default;
};
using GpuMaterialState = std::map<std::string, std::string>;
struct GlassTriangle {
    std::string path;
    AssetCommand vertices;
    AssetCommand triangle;
    GpuMaterialState material;
    bool operator==(const GlassTriangle&) const = default;
};
struct GlassTrace {
    std::vector<GlassTriangle> triangles;
    GpuMaterialState material;
};
using AssetOverrideMap = std::map<std::string, std::vector<AssetCommand>>;

// Generated from each owner's packaged XML. The production draw fixture supplies
// the resource path and GPU owner; evaluate nested calls in submission order so
// a prim-color override before the shared material cannot pass this regression.
#include "fairy_asset_commands.inc"

static void ExecuteGlassCommands(const std::string& owner, std::string path, GlassTrace& trace, AssetCommand& vertices,
                                 const AssetOverrideMap* overrides = nullptr, int depth = 0) {
    assert(depth < 16 && "cyclic bottle display-list dependency");
    if (path.starts_with("__OTR__"))
        path.erase(0, 7);
    if (path.starts_with("@"))
        path.erase(0, path.find(':') + 1);
    const auto key = owner + ":" + path;
    const auto& commands = overrides && overrides->contains(key) ? overrides->at(key) : assetCommandLists.at(key);
    for (const auto& command : commands) {
        if (command.tag == "CallDisplayList") {
            ExecuteGlassCommands(owner, command.attributes.at("Path"), trace, vertices, overrides, depth + 1);
        } else if (command.tag == "LoadVertices") {
            vertices = command;
        } else if (command.tag == "Triangle1" || command.tag == "Triangles2") {
            trace.triangles.push_back({ path, vertices, command, trace.material });
        } else if (command.tag == "ClearGeometryMode" || command.tag == "SetGeometryMode") {
            for (const auto& [mode, value] : command.attributes)
                trace.material["Geometry." + mode] = command.tag == "ClearGeometryMode" ? "0" : value;
        } else if (command.tag.starts_with("Set") || command.tag == "Texture" || command.tag == "PipelineMode" ||
                   command.tag == "Grayscale") {
            const auto prefix = command.tag + ".";
            for (auto field = trace.material.begin(); field != trace.material.end();) {
                if (field->first.starts_with(prefix))
                    field = trace.material.erase(field);
                else
                    ++field;
            }
            for (const auto& [key, value] : command.attributes)
                trace.material[command.tag + "." + key] = value;
        } else if (command.tag == "EndDisplayList") {
            break;
        }
    }
}

static GlassTrace TraceGlass(const std::string& owner, const std::string& path,
                             const AssetOverrideMap* overrides = nullptr) {
    GlassTrace trace;
    AssetCommand vertices;
    ExecuteGlassCommands(owner, path, trace, vertices, overrides);
    return trace;
}

static void CheckReadableGlass(const std::string& owner, const std::string& path) {
    auto baseline = TraceGlass(owner, "objects/combo_bottle_gi/EmptyXlu");
    auto candidate = TraceGlass(owner, path);
    size_t glassTriangles = 0;
    assert(candidate.triangles.size() == baseline.triangles.size());
    for (size_t i = 0; i < candidate.triangles.size(); ++i) {
        auto& actual = candidate.triangles[i];
        if (actual.path.ends_with("gGiBlueFireChamberstickDL_tri_1")) {
            ++glassTriangles;
            assert(actual.material.at("SetPrimColor.A") == "80" &&
                   "fairy glass still uses faint empty-bottle alpha at GPU triangle submission");
            assert(actual.material.at("SetPrimColor.R") == "255" && actual.material.at("SetPrimColor.G") == "255" &&
                   actual.material.at("SetPrimColor.B") == "255");
            assert(actual.material.at("SetRenderMode.Mode2") == "G_RM_AA_ZB_XLU_SURF2");
            assert(actual.material.at("SetCycleType.G_CYC_2CYCLE") == "1");
            assert(actual.material.at("SetCombineLERP.Ad0") == "G_ACMUX_1" &&
                   actual.material.at("SetCombineLERP.Aa1") == "G_ACMUX_COMBINED" &&
                   actual.material.at("SetCombineLERP.Ac1") == "G_ACMUX_PRIMITIVE" &&
                   actual.material.at("SetCombineLERP.Ad1") == "G_ACMUX_0");
            assert(actual.material.at("SetTextureImage.Path") ==
                   "objects/combo_bottle_gi/shell/MetalPainted_1TestFromSkilar");
            assert(baseline.triangles[i].material.at("SetPrimColor.A") == "32" &&
                   "ordinary bottles must retain their accepted glass opacity");
            actual.material["SetPrimColor.A"] = "32";
        }
        assert(actual == baseline.triangles[i] && "fairy glass changed shell geometry or unrelated material state");
    }
    assert(glassTriangles > 0 && candidate.material == baseline.material &&
           "fairy glass must restore the same state as the shared shell");
    assert(candidate.material.at("SetPrimColor.A") == "255" && candidate.material.at("Geometry.G_TEXTURE_GEN") == "0" &&
           candidate.material.at("SetCycleType.G_CYC_1CYCLE") == "1");
}

#endif
