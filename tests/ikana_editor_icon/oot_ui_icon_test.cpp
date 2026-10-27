#define IMGUI_DEFINE_MATH_OPERATORS
#include "combo/menu/ComboItemIconOwnership.h"
#include "fast/resource/type/Texture.h"
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
#include "ship/resource/CrossRMRegistry.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

using s16 = int16_t;
using u8 = uint8_t;
using u32 = uint32_t;
constexpr int EQUIP_TYPE_SHIELD = 1;
constexpr int EXT_EQUIP_OWNED_SHIFT = 16;
constexpr float IMAGE_SIZE = 40.0f;
#ifdef COMBO_BUILD
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
#endif

static void check(bool ok, const char *reason) {
  if (!ok) {
    std::fprintf(stderr, "FAIL: %s\n", reason);
    std::exit(1);
  }
}

namespace Ship {
class ResourceManager;
struct Gui {
  virtual ~Gui() = default;
};
struct Window {
  std::shared_ptr<Gui> gui;
  std::shared_ptr<Gui> GetGui() { return gui; }
};
struct Context {
  static Context *GetRawInstance() {
    static Context instance;
    return &instance;
  }
  std::shared_ptr<ResourceManager> active;
  std::shared_ptr<Window> window = std::make_shared<Window>();
  std::shared_ptr<ResourceManager> GetResourceManager() { return active; }
  void SetResourceManager(std::shared_ptr<ResourceManager> manager) {
    active = manager;
  }
  std::shared_ptr<Window> GetWindow() { return window; }
};
struct ResourceManager {
  bool alt = false;
  bool requireScope = false;
  std::map<std::string, std::shared_ptr<IResource>> files;
  std::shared_ptr<IResource> LoadResource(std::string path,
                                          bool exact = false) {
    if (requireScope)
      check(Context::GetRawInstance()->active.get() == this,
            "resource factories need the selected owner's Context");
    if (path.starts_with("__OTR__"))
      path.erase(0, 7);
    if (!exact && alt && files.contains("alt/" + path))
      return files.at("alt/" + path);
    return files.contains(path) ? files.at(path) : nullptr;
  }
};
struct GuiTextureMetadata {
  uint32_t RendererTextureId;
  int32_t Width, Height;
};
struct OtherResource : Resource<uint8_t> {
  OtherResource() : Resource(std::make_shared<ResourceInitData>()) {}
  uint8_t *GetPointer() override { return nullptr; }
  size_t GetPointerSize() override { return 0; }
};
} // namespace Ship

namespace Fast {
struct GfxRenderingAPI {
  struct Upload {
    uint32_t width, height;
    std::vector<uint8_t> pixels;
  };
  std::map<uint32_t, Upload> textures;
  uint32_t next = 1, selected = 0;
  int uploads = 0, deletions = 0;
  uint32_t NewTexture() { return next++; }
  void SelectTexture(int, uint32_t id) { selected = id; }
  void SetSamplerParameters(int, bool, uint32_t, uint32_t) {}
  void UploadTexture(const uint8_t *pixels, uint32_t width, uint32_t height) {
    ++uploads;
    textures[selected] = {
        width, height, {pixels, pixels + size_t(width) * height * 4}};
  }
  void DeleteTexture(uint32_t id) {
    ++deletions;
    textures.erase(id);
  }
  ImTextureID GetTextureById(int id) {
    return reinterpret_cast<void *>(uintptr_t(id));
  }
};
struct Interpreter {
  GfxRenderingAPI api;
  GfxRenderingAPI *GetCurrentRenderingAPI() { return &api; }
};
struct Fast3dGui : Ship::Gui {
  std::weak_ptr<Interpreter> mInterpreter;
  std::unordered_map<std::string, Ship::GuiTextureMetadata> mGuiTextures;
  ImTextureID GetTextureById(int32_t id);
  bool HasTextureByName(const std::string &name);
  ImTextureID GetTextureByName(const std::string &name);
  ImVec2 GetTextureSize(const std::string &name);
  void LoadGuiTexture(const std::string &name, const Texture &texture,
                      const std::string &palettePath = "",
                      const ImVec4 &tint = ImVec4(1, 1, 1, 1));
  void LoadGuiTexture(const std::string &name, const std::string &path,
                      const std::string &palettePath = "",
                      const ImVec4 &tint = ImVec4(1, 1, 1, 1));
  void UnloadTexture(const std::string &name);
};
} // namespace Fast
#define SPDLOG_WARN(...) ((void)0)

struct Save {
  u32 extEquipOwnedBits = 0;
  uint16_t shieldOwned = FC_SHIELD_MIRROR_OOT;
  u8 vanillaShieldSkin = 0;
} save;
static Save *Nei_Save() { return &save; }
static u8 current[4]{};
static u8 ExtEquip_GetCurrent(s16 type) { return current[type]; }
static void ExtEquip_SetSlot(s16, u8) {
  check(false, "ownership-only grid must not equip another piece");
}
static const char *sExtEquipIconPaths[4][3] = {
    {nullptr, nullptr, nullptr},
    {"__OTR__goddess", nullptr, nullptr},
    {nullptr, nullptr, nullptr},
    {nullptr, nullptr, nullptr},
};
namespace Colors {
constexpr int DarkGray = 0;
}
static void PushStyleButton(int) {
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.2f, .2f, .2f, 1));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.4f, .4f, .4f, 1));
}
static void PopStyleButton() { ImGui::PopStyleColor(2); }

static std::map<std::string, ImTextureID> imageButtons;
static std::map<std::string, ImRect> rectangles;
namespace ImGui {
// Observe the application boundary without replacing the real widget/input
// path.
bool ObservedImageButton(const char *label, ImTextureID texture,
                         const ImVec2 &size, const ImVec2 &uv0,
                         const ImVec2 &uv1, const ImVec4 &background,
                         const ImVec4 &tint) {
  imageButtons[label] = texture;
  const bool clicked =
      ImageButton(label, texture, size, uv0, uv1, background, tint);
  rectangles[label] = GImGui->LastItemData.Rect;
  return clicked;
}
} // namespace ImGui
#define ImageButton ObservedImageButton
#include "oot_icon_production.inc"
#undef ImageButton

void ImGuiTestEngineHook_ItemAdd(ImGuiContext *, ImGuiID, const ImRect &,
                                 const ImGuiLastItemData *) {}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext *context, ImGuiID,
                                  const char *label, ImGuiItemStatusFlags) {
  rectangles[label] = context->LastItemData.Rect;
}
void ImGuiTestEngineHook_Log(ImGuiContext *, const char *, ...) {}
const char *ImGuiTestEngine_FindItemDebugLabel(ImGuiContext *, ImGuiID) {
  return "";
}

static auto host = std::make_shared<Ship::ResourceManager>();
static auto oot = std::make_shared<Ship::ResourceManager>();
static auto mm = std::make_shared<Ship::ResourceManager>();
static auto interpreter = std::make_shared<Fast::Interpreter>();
static auto gui = std::make_shared<Fast::Fast3dGui>();
static const char *privateKey =
    "textures/icon_item_custom/gItemIconShieldOfIkanaTex";
static const char *ootMirror =
    "textures/icon_item_static/gItemIconShieldMirrorTex";
static const char *mmMirror = "icon_item_static_yar/gItemIconMirrorShieldTex";

static std::shared_ptr<Fast::Texture> texture(uint8_t marker,
                                              uint16_t size = 32) {
  auto result = std::make_shared<Fast::Texture>();
  result->Type = Fast::TextureType::RGBA32bpp;
  result->Width = result->Height = size;
  result->ImageDataSize = uint32_t(size) * size * 4;
  result->mImageBuffer =
      std::make_shared<std::vector<char>>(result->ImageDataSize, char(marker));
  result->ImageData = reinterpret_cast<uint8_t *>(result->mImageBuffer->data());
  for (size_t alpha = 3; alpha < result->ImageDataSize; alpha += 4)
    result->ImageData[alpha] = 255;
  return result;
}
static void frame() {
  imageButtons.clear();
  rectangles.clear();
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(800, 500));
  ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoSavedSettings);
  DrawGrid();
  ImGui::End();
  ImGui::Render();
  check(Ship::Context::GetRawInstance()->active == host,
        "icon registration restores the host Context");
}
static void shown(uint8_t marker, uint16_t size, const char *reason) {
  frame();
  frame();
  check(imageButtons.contains("Shield of Ikana"), reason);
  auto handle = imageButtons.at("Shield of Ikana");
  check(handle == gui->GetTextureByName(COMBO_IKANA_SHIELD_ICON),
        "cell uses the unchanged canonical GUI cache key");
  const auto &upload =
      interpreter->api.textures.at(uint32_t(uintptr_t(handle)));
  check(upload.width == size && upload.height == size,
        "GUI upload retains native/HD resource dimensions");
  check(upload.pixels[0] == marker && upload.pixels[3] == 255,
        "GUI receives pixels from the actual selected owner");
}
static void absent(const char *reason) {
  frame();
  check(!imageButtons.contains("Shield of Ikana"), reason);
  check(rectangles.contains("Shield of Ikana"),
        "unavailable icons retain the existing clickable text fallback");
  check(!gui->HasTextureByName(COMBO_IKANA_SHIELD_ICON),
        "unavailable source removes its stale GUI registration");
}
static void clickIkana() {
  const auto center = rectangles.at("Shield of Ikana").GetCenter();
  auto &io = ImGui::GetIO();
  io.AddMousePosEvent(center.x, center.y);
  frame();
  io.AddMouseButtonEvent(0, true);
  frame();
  io.AddMouseButtonEvent(0, false);
  frame();
  frame();
}

int main() {
  auto *context = Ship::Context::GetRawInstance();
  context->active = host;
  gui->mInterpreter = interpreter;
  context->window->gui = gui;
  oot->requireScope = mm->requireScope = true;
  Ship::CrossRMRegistry::Register("oot", oot);
  Ship::CrossRMRegistry::Register("mm", mm);
  auto stock = texture('N');
  auto hd = texture('H', 512);
  hd->Flags = TEX_FLAG_LOAD_AS_RAW;
  host->files[privateKey] = texture('X');
  host->files[ootMirror] = texture('O');
  host->files[mmMirror] = texture('W');
  host->files["goddess"] = texture('G');
  oot->files[ootMirror] = texture('O');
  mm->files[mmMirror] = texture('M');
#ifdef COMBO_BUILD
  auto owner = mm;
#else
  auto owner = host;
#endif
  owner->files[privateKey] = stock;
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(900, 600);
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  GImGui->TestEngineHookItems = true;
  shown('N', 32,
        "cold OoT equipment cell must render its MM-owned native Ikana icon");
  const auto stableId = gui->GetTextureByName(COMBO_IKANA_SHIELD_ICON);
  const int stableUploads = interpreter->api.uploads;
  frame();
  check(gui->GetTextureByName(COMBO_IKANA_SHIELD_ICON) == stableId &&
            interpreter->api.uploads == stableUploads,
        "unchanged selected resource does not reupload every frame");
  check(imageButtons.contains("Goddess Shield"),
        "ordinary local equipment icon registration stays intact");
  const auto goddessId = imageButtons.at("Goddess Shield");
  clickIkana();
  check(ExtEquip_HasItem(1, 3) &&
            save.shieldOwned == (FC_SHIELD_MIRROR_OOT | FC_SHIELD_IKANA),
        "image cell grants canonical Ikana while preserving the OoT Mirror "
        "owner");
  clickIkana();
  check(!ExtEquip_HasItem(1, 3) && save.shieldOwned == FC_SHIELD_MIRROR_OOT,
        "image cell preserves the existing OoT toggle-to-remove semantics");
  owner->files[std::string("alt/") + privateKey] = hd;
  owner->alt = true;
  shown('H', 512,
        "owner Alt enables HD art in a previously warm native GUI cell");
  owner->alt = false;
  shown('N', 32, "disabling owner Alt restores native art in a warm GUI cell");
  check(gui->GetTextureByName("__OTR__goddess") == goddessId,
        "Ikana refresh does not evict another equipment icon");
  owner->files.erase(privateKey);
  absent("removed private stock cannot keep a previously registered image");
  clickIkana();
  check(ExtEquip_HasItem(1, 3),
        "text fallback retains the existing grant interaction");
  clickIkana();
  check(!ExtEquip_HasItem(1, 3) && save.shieldOwned == FC_SHIELD_MIRROR_OOT,
        "text fallback retains the existing remove interaction without "
        "changing OoT Mirror");
  owner->alt = true;
  shown('H', 512, "enabled Alt-only private icon is usable");
  owner->files[std::string("alt/") + privateKey] =
      std::make_shared<Ship::OtherResource>();
  absent("a selected wrong-type Alt resource cannot retain old art or borrow "
         "stock");
  owner->files.erase(std::string("alt/") + privateKey);
  owner->files[privateKey] = std::make_shared<Ship::OtherResource>();
  absent("a wrong-type base resource uses the safe text fallback");
  for (int defect = 0; defect < 6; ++defect) {
    auto invalid = texture('B');
    if (defect == 0)
      invalid->ImageData = nullptr;
    if (defect == 1)
      invalid->Width = 0;
    if (defect == 2)
      invalid->Height = 0;
    if (defect == 3)
      invalid->ImageDataSize = 1;
    if (defect == 4)
      invalid->Type = Fast::TextureType::RGBA16bpp;
    if (defect == 5)
      invalid->ImageDataSize += 4;
    owner->files[privateKey] = invalid;
    absent("invalid private RGBA32 data never reaches the GUI upload");
  }
  owner->files[privateKey] = stock;
  owner->alt = false;
  shown('N', 32, "valid stock can recover after a missing/invalid resource");
#ifdef COMBO_BUILD
  host->alt = true;
  shown('N', 32, "host Alt does not choose MM owner's icon variant");
  Ship::CrossRMRegistry::Unregister("mm");
  absent("an absent MM owner cannot borrow a same-named host or ordinary "
         "Mirror image");
  auto replacement = std::make_shared<Ship::ResourceManager>();
  replacement->requireScope = true;
  replacement->files[privateKey] = texture('R');
  Ship::CrossRMRegistry::Register("mm", replacement);
  shown('R', 32,
        "a replaced named owner refreshes the existing canonical GUI key");
  gComboGameMode = COMBO_GAME_OOT;
  oot->files[privateKey] = texture('S');
  shown('S', 32,
        "Solo OoT uses its own private stock copy without substituting "
        "ordinary Mirror");
  oot->files[std::string("alt/") + privateKey] = hd;
  oot->alt = true;
  shown('H', 512, "Solo OoT uses its local owner's private Alt override");
  Ship::CrossRMRegistry::Unregister("oot");
  absent(
      "missing named Solo OoT owner does not borrow a current foreign manager");
#endif
  ImGui::DestroyContext();
  std::puts(
      "PASS OoT real equipment UI/native+HD GUI uploads, warm-cache refresh, "
      "named-owner isolation, safe text fallback and ownership clicks");
}
