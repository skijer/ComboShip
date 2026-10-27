// The Wolf implementation is included so private loader/proc behavior is exercised directly.
// No validation or combat implementation is copied into the fixture.
#include "2s2h/GameInteractor/GameInteractor.h"
#include WOLF_IMPLEMENTATION
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>
#include <spdlog/sinks/ostream_sink.h>

static std::string assetDirectory;
static int colliderInitializations, colliderDestructions, attackRegistrations;
static Collider* lastAttack;
static std::vector<u16> fixturePlayerSfx;
static std::map<std::string, int32_t> integerCvars;
static std::map<std::string, float> floatCvars;
static std::map<std::string, std::string> locatedFiles;
static std::ostringstream wolfLogs;
static std::vector<u8> resourceBlob;
static const char* resourceBlobOwner = "mm";
static const char *resourceBlobPath = NeiWolfAsset::kResourcePath;
static void captureWolfLogs() {
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(wolfLogs);
    auto logger = std::make_shared<spdlog::logger>("wolf-fixture", sink);
    logger->set_pattern("%v");
    spdlog::set_default_logger(logger);
    wolfLogs.str("");
}
static bool logged(const char* message) {
    return wolfLogs.str().find(message) != std::string::npos;
}
namespace Ship {
std::string Context::LocateFileAcrossAppDirs(const std::string& p, const std::string&) {
    const auto found = locatedFiles.find(p);
    if (found != locatedFiles.end())
        return found->second;
    return std::filesystem::exists(p) ? std::filesystem::absolute(p).string() : std::string{};
}
} // namespace Ship
extern "C" {
int MM_CopyWolfLinkResource(uint8_t* destination, size_t capacity, size_t* size, const char** owner) {
    if (resourceBlob.empty())
        return 0;
    if (owner)
        *owner = resourceBlobOwner;
    *size = NeiWolfAsset::ResourcePayloadSize(resourceBlob.data(), resourceBlob.size());
    if (!*size || (destination && capacity < *size))
        return -1;
    if (destination)
        std::memcpy(destination, resourceBlob.data(), *size);
    return 1;
}
int MM_CopyWolfLinkModelResource(int, uint8_t *destination, size_t capacity,
                                 size_t *size, const char **owner,
                                 const char **path) {
  if (path)
    *path = resourceBlobPath;
  return MM_CopyWolfLinkResource(destination, capacity, size, owner);
}
const char* Nei_AssetDir(void) {
    return assetDirectory.c_str();
}
int32_t CVarGetInteger(const char* name, int32_t v) {
    const auto found = integerCvars.find(name);
    return found == integerCvars.end() ? v : found->second;
}
float CVarGetFloat(const char* name, float v) {
    const auto found = floatCvars.find(name);
    return found == floatCvars.end() ? v : found->second;
}
SaveContext gSaveContext{};
static RegEditor registers{};
RegEditor* gRegEditor = &registers;
PlayState* gPlayState;
void* ZeldaArena_Malloc(size_t size) {
    return std::calloc(1, size);
}
void ZeldaArena_Free(void* p) {
    std::free(p);
}
s16 sins(u16 a) {
    return (s16)(std::sin(a * (3.14159265358979323846f / 32768)) * 32767);
}
s16 coss(u16 a) {
    return (s16)(std::cos(a * (3.14159265358979323846f / 32768)) * 32767);
}
int GameInteractor_InvertControl(GIInvertType) {
    return 1;
}
s16 Math_Atan2S_XY(f32 x, f32 y) {
    return (s16)(std::atan2(y, x) * 32768 / 3.14159265358979323846f);
}
s16 Camera_GetInputDirYaw(Camera*) {
    return 0;
}
void Player_PlaySfx(Player*, u16 id) {
    fixturePlayerSfx.push_back(id);
}
void AudioSfx_PlaySfx(u16, Vec3f*, u8, f32*, f32*, s8*) {}
void AudioSfx_StopByPosAndId(Vec3f*, u16) {}
s32 Collider_InitCylinder(PlayState*, ColliderCylinder* c) {
    ++colliderInitializations;
    std::memset(c, 0, sizeof(*c));
    return 1;
}
s32 Collider_SetCylinder(PlayState*, ColliderCylinder* c, Actor* a, ColliderCylinderInit* i) {
    c->base.actor = a;
    c->base.atFlags = i->base.atFlags;
    c->elem.atDmgInfo.damage = i->elem.atDmgInfo.damage;
    c->elem.atDmgInfo.dmgFlags = i->elem.atDmgInfo.dmgFlags;
    c->dim = i->dim;
    return 1;
}
s32 Collider_DestroyCylinder(PlayState*, ColliderCylinder*) {
    ++colliderDestructions;
    return 1;
}
s32 CollisionCheck_SetAT(PlayState*, CollisionCheckContext*, Collider* c) {
    ++attackRegistrations;
    lastAttack = c;
    return 1;
}
void ActorShadow_DrawFeet(Actor*, Lights*, PlayState*) {
}
void* Graph_Alloc(GraphicsContext* gfx, size_t bytes) {
#ifdef MM_WOLF_HOST
    const uintptr_t tail = (uintptr_t)gfx->polyOpa.d - ALIGN16(bytes);
    assert(tail >= (uintptr_t)gfx->polyOpa.p);
    gfx->polyOpa.d = (Gfx*)tail;
    return (void*)tail;
#else
    static Mtx m;
    return &m;
#endif
}
#ifdef MM_WOLF_HOST
#include "wolf-native-graph.inc"
#else
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
#endif
void Gfx_SetupDL25_Opa(GraphicsContext*) {
}
void Matrix_SetTranslateRotateYXZ(f32, f32, f32, Vec3s*) {
}
void Matrix_Scale(f32, f32, f32, MatrixMode) {
}
Mtx* Matrix_ToMtx(Mtx* m) {
    return m;
}
void gSPSegment(void*, int, uintptr_t) {
}
void gSPDisplayList(Gfx*, Gfx*) {
}
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
Gfx gCullBackDList[1];
}
static void write32(std::vector<u8>& b, size_t off, u32 v) {
    std::memcpy(b.data() + off, &v, 4);
}
static void write16(std::vector<u8>& b, size_t off, u16 v) {
    std::memcpy(b.data() + off, &v, 2);
}
static void writeFloat(std::vector<u8>& b, size_t off, float f) {
    std::memcpy(b.data() + off, &f, 4);
}
static u32 field(const std::vector<u8>& b, int i) {
    return ReadU32(b.data() + 12 + i * 4);
}
static std::vector<u8> makeAsset() {
    constexpr u32 bones = 37, frames = 60;
    std::vector<u8> b(kHeaderSize, 0);
    std::memcpy(b.data(), kMagic, 8);
    write32(b, 8, 1);
    auto chunk = [&](size_t size) {
        while (b.size() % 4)
            b.push_back(0);
        u32 off = b.size();
        b.resize(b.size() + size);
        return off;
    };
    const u32 v = chunk(60), w = chunk(24), p = chunk(bones * 2), m = chunk(bones * 64), bp = chunk(bones * 12),
              e = chunk(WANM_COUNT * 16);
    std::vector<u32> nameOffsets;
    u32 names = chunk(0);
    for (const char* name : kAnimNames) {
        nameOffsets.push_back(b.size());
        b.insert(b.end(), name, name + std::strlen(name) + 1);
    }
    u32 namesSize = b.size() - names;
    const u32 f = chunk(WANM_COUNT * frames * bones * 36), fs = WANM_COUNT * frames * bones * 36, t = chunk(128);
    u32 header[] = { 3, 1, bones, WANM_COUNT, 8, 8, v, w, p, m, bp, e, names, namesSize, f, fs, t, 128, (u32)b.size() };
    for (int i = 0; i < 19; ++i)
        write32(b, 12 + i * 4, header[i]);
    for (int i = 0; i < 3; ++i) {
        b[w + i * 8 + 4] = 255;
        b[v + i * 20 + 12] = 127;
        b[v + i * 20 + 19] = 255;
        writeFloat(b, v + i * 20, float(i));
    }
    for (u32 i = 0; i < bones; ++i) {
        write16(b, p + i * 2, i == 0 ? 0xffff : i - 1);
        for (int j = 0; j < 4; ++j)
            writeFloat(b, m + i * 64 + j * 20, 1);
    }
    for (int i = 0; i < WANM_COUNT; ++i) {
        write32(b, e + i * 16, nameOffsets[i]);
        write16(b, e + i * 16 + 4, frames);
        write16(b, e + i * 16 + 6, bones);
        writeFloat(b, e + i * 16 + 8, 30);
        write32(b, e + i * 16 + 12, f + i * frames * bones * 36);
    }
    for (u32 i = 0; i < WANM_COUNT * frames * bones; ++i)
        for (int j = 6; j < 9; ++j)
            writeFloat(b, f + i * 36 + j * 4, 1);
    return b;
}
static bool load(const std::vector<u8>& b) {
    sAssetsLoaded = 0;
    std::ofstream out(assetDirectory + "/wolf_link.bin", std::ios::binary);
    out.write((const char*)b.data(), b.size());
    out.close();
    return LoadAssets();
}
static void writeAsset(const std::string& path, const std::vector<u8>& b) {
    std::ofstream out(path, std::ios::binary);
    out.write((const char*)b.data(), b.size());
}
static void rejected(const char* label, const std::vector<u8>& b) {
    int before = sDefIndex;
    if (load(b)) {
        std::fprintf(stderr, "FAIL malformed Wolf accepted: %s\n", label);
        std::exit(1);
    }
    assert(sDefIndex == before);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    assetDirectory = argv[1];
    captureWolfLogs();
    const auto good = makeAsset();
    assert(load(good));
    assert(logged("asset loaded path="));
    assert(sSkin.vertexCount == 3 && sSkin.boneCount == 37);
    auto missingClip = good;
    missingClip[field(missingClip, 12)] = 'x';
    rejected("missing required animation", missingClip);
    assert(!sAssetsLoaded && logged("reason=missing-animation detail=wl_armature_wl_waita"));
    assert(load(good) && "replacing a rejected asset must permit a fresh load");
    auto badHeader = good;
    badHeader[0] = 'X';
    rejected("wrong magic", badHeader);
    assert(logged("reason=magic"));
    badHeader = good;
    write32(badHeader, 8, 3);
    rejected("wrong version", badHeader);
    assert(logged("reason=version"));
    // File location is a fixture boundary; loading and validation remain the
    // complete production implementation. No real TP mesh is claimed here.
    const std::string originalDirectory = assetDirectory;
    const std::string primaryPath = originalDirectory + "/wolf_link.bin";
    const std::string donorPath = originalDirectory + "/soh-wolf-fixture.bin";
    auto primary = good;
    writeFloat(primary, field(primary, 6), 123.0f);
    auto donor = good;
    writeFloat(donor, field(donor, 6), 45.0f);
    writeAsset(primaryPath, primary);
    writeAsset(donorPath, donor);
    assetDirectory = "nei/2ship";
    locatedFiles["nei/2ship/wolf_link.bin"] = primaryPath;
    locatedFiles["nei/soh/wolf_link.bin"] = donorPath;
    sAssetsLoaded = 0;
    assert(LoadAssets() && sAssetPath == primaryPath && sVertices[0].posX == 123.0f);
    writeAsset(primaryPath, badHeader);
    sAssetsLoaded = 0;
    assert(!LoadAssets() && sAssetPath == primaryPath && !sAssetsLoaded);
    writeAsset(primaryPath, primary);
    assert(LoadAssets() && sVertices[0].posX == 123.0f && "fixed primary file must retry normally");
    locatedFiles.erase("nei/2ship/wolf_link.bin");
    std::remove(primaryPath.c_str());
    sAssetsLoaded = 0;
    assert(LoadAssets() && sAssetPath == donorPath && sVertices[0].posX == 45.0f);
    auto invalidDonor = donor;
    invalidDonor[field(invalidDonor, 12)] = 'x';
    writeAsset(donorPath, invalidDonor);
    sAssetsLoaded = 0;
    assert(!LoadAssets() && !sAssetsLoaded && "the SoH fallback must pass the same MM clip validation");
    writeAsset(donorPath, donor);
    assert(LoadAssets() && sAssetPath == donorPath);
    locatedFiles.clear();
    assetDirectory = originalDirectory;
    assert(load(good));
    auto b = good;
    write32(b, field(b, 6), 0x7fc00001);
    rejected("NaN vertex under fast-math", b);
    assert(logged("reason=validation"));
    for (u32 bits : { 0x7f800000u, 0xff800000u, 0x7fc00001u, 0xffc00001u, 0x7f7fffffu }) {
        b = good;
        write32(b, field(b, 9), bits);
        rejected("invalid inverse bind", b);
        b = good;
        write32(b, field(b, 10), bits);
        rejected("invalid bone position", b);
        b = good;
        write32(b, field(b, 14), bits);
        rejected("invalid animation TRS", b);
        b = good;
        write32(b, field(b, 11) + 8, bits);
        rejected("invalid frame rate", b);
    }
    b = good;
    for (int bone = 0; bone < 37; ++bone)
        for (int scale = 6; scale < 9; ++scale)
            writeFloat(b, field(b, 14) + bone * 36 + scale * 4, 64.0f);
    rejected("finite bone scales overflow when composed", b);
    b = good;
    writeFloat(b, field(b, 14), 20000.0f);
    writeFloat(b, field(b, 14) + 36, 20000.0f);
    rejected("finite bone translations exceed composed bounds", b);
    for (u32 bits : { 0x7f800000u, 0xff800000u, 0x7fc00001u, 0x7f7fffffu }) {
        f32 badScale;
        std::memcpy(&badScale, &bits, sizeof(bits));
        floatCvars["gMods.WolfLink.Scale"] = floatCvars["gMods.WolfLink.SpeedScale"] = badScale;
        assert(RenderScale() == .3f && SpeedScale() == 1.0f);
    }
    floatCvars.clear();
    b = good;
    write32(b, 12 + 1 * 4, 0x55555556);
    rejected("triangle multiplication overflow", b);
    b = good;
    write32(b, 12 + 3 * 4, 0x10000001);
    rejected("animation entry multiplication overflow", b);
    b = good;
    write32(b, 12 + 6 * 4, 4);
    rejected("chunk overlaps header", b);
    b = good;
    write32(b, 12 + 9 * 4, field(b, 9) + 1);
    rejected("unaligned inverse bind", b);
    b = good;
    b[field(b, 7)] = 37;
    rejected("weight bone out of range", b);
    b = good;
    b[field(b, 7) + 4] = 0;
    rejected("zero-sum weights", b);
    b = good;
    b[field(b, 7) + 5] = 1;
    rejected("weights after zero influence", b);
    b = good;
    write16(b, field(b, 8) + 2, 1);
    rejected("self parent", b);
    b = good;
    write16(b, field(b, 8), 1);
    rejected("cyclic root", b);
    b = good;
    write16(b, field(b, 8) + 2, 0xffff);
    rejected("disconnected root", b);
    b = good;
    write32(b, field(b, 11), field(b, 12) + field(b, 13) - 1);
    b[field(b, 12) + field(b, 13) - 1] = 'X';
    rejected("unterminated name", b);
    b = good;
    write16(b, field(b, 11) + 4, 0);
    rejected("empty animation", b);
    b = good;
    write32(b, field(b, 11) + 12, field(b, 16));
    rejected("frames outside frame chunk", b);
    b = good;
    write32(b, 12 + 15 * 4, 0xffffffffu);
    rejected("frame chunk overflow", b);
    b = good;
    b.pop_back();
    rejected("truncated file", b);
    std::puts("PASS production Wolf malformed-bin validation and valid format fixture");

    assert(load(good));
    PlayState play{};
    Player player{};
    Camera camera{};
    gPlayState = &play;
    play.cameraPtrs[0] = &camera;
    R_UPDATE_RATE = 3;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actor.world.pos = { 100, 20, 300 };
    player.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    player.cylinder.dim.radius = 12;
    player.cylinder.dim.height = 50;
    player.cylinder.dim.yShift = 2;
    assert(WolfLinkForm_LoadSkeleton(&play));
    assert(sWolf.initialized && sSkin.vtxBuf[0] && sSkin.vtxBuf[1]);
    bool nativeOwnsAction = false;
    auto tick = [&] {
#ifdef MM_WOLF_NATIVE_HANDOFF
        WolfLinkForm_Update(&player, &play, &play.state.input[0], nativeOwnsAction);
#else
        WolfLinkForm_Update(&player, &play);
#endif
        ++play.gameplayFrames;
    };
    play.state.input[0].press.button = BTN_B;
    tick();
    assert(sWolf.proc == PROC_WOLF_WAIT_ATTACK && sWolf.procOwnsPlayer);
    player.actor.world.pos = { 32760, -32760, 32760 };
    SetAttackCollider(&player, &play, 70, 150, 100, 1);
    UpdateAttackCollider(&player, &play);
    assert(sWolf.atCyl.dim.pos.z == 32767 && sWolf.atCyl.elem.atDmgInfo.damage == 4);
    player.actor.world.pos = {};
    SetAttackCollider(&player, &play, kAtWaLr.radius, kAtWaLr.height, kAtWaLr.radiusOffset, 0);
    play.state.input[0].press.button = 0;
    const int beforeAttacks = attackRegistrations;
    for (int i = 0; i < 7; ++i)
        tick();
    assert(attackRegistrations > beforeAttacks && lastAttack == &sWolf.atCyl.base);
    assert(sWolf.atCyl.elem.atDmgInfo.damage == 2 && sWolf.atCyl.elem.atDmgInfo.dmgFlags == DMG_SLASH);
    sWolf.atCyl.base.atFlags |= AT_BOUNCED;
    tick();
    assert(sWolf.proc == PROC_WOLF_ATTACK_REVERSE && player.actor.velocity.y > 0);
    ProcMoveInit(&player);
    ResetCombo();
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actor.velocity.y = 0;
    player.speedXZ = 6;
    sWolf.wasOnGround = 1;
    play.state.input[0].rel.stick_y = 60;
    play.state.input[0].press.button = BTN_A;
    tick();
    assert(sWolf.proc == PROC_WOLF_DASH && player.speedXZ > 6 && sWolf.dashModeTimer > 0);
    play.state.input[0].press.button = 0;
    sWolf.character.curFrame = 4;
    player.actor.bgCheckFlags |= BGCHECKFLAG_WALL;
    tick();
    assert(sWolf.proc == PROC_WOLF_DASH_REVERSE && player.speedXZ < 0);
    ProcMoveInit(&player);
    ResetCombo();
    sWolf.dashModeTimer = 0;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    play.state.input[0].rel.stick_y = 0;
    // Native MM freeze/thaw can own the action even after invincibility expires.
    player.stateFlags1 = PLAYER_STATE1_4000000;
    player.invincibilityTimer = 0;
    player.speedXZ = -4;
    player.actor.velocity.y = 2;
    player.actor.colChkInfo.damage = 6;
    play.state.input[0].press.button = BTN_B;
    tick();
    if (sWolf.procOwnsPlayer || sWolf.atActive || (player.stateFlags3 & PLAYER_STATE3_4)) {
        std::fputs("FAIL Wolf stole native MM damage/freeze action after invincibility expired\n", stderr);
        return 1;
    }
    assert(player.speedXZ == -4 && player.actor.velocity.y == 2 && player.actor.colChkInfo.damage == 6);
    player.stateFlags1 = 0;
    nativeOwnsAction = true;
    tick();
    assert(!sWolf.procOwnsPlayer && !sWolf.atActive && player.speedXZ == -4);
    // Host can hand freeze/thaw action ownership to MM even with the generic damaged flag cleared.
    nativeOwnsAction = false;
    ProcMoveInit(&player);
    player.speedXZ = 0;
    tick();
    assert(sWolf.procOwnsPlayer);
    const int beforeDestroy = colliderDestructions;
#ifdef MM_WOLF_NATIVE_HANDOFF
    WolfLinkForm_Cleanup(&player, &play);
#else
    WolfLinkForm_Cleanup();
#endif
    if ((player.stateFlags3 & PLAYER_STATE3_4) || colliderDestructions != beforeDestroy + 1) {
        std::fputs("FAIL Wolf cleanup retained action ownership or attack collider\n", stderr);
        return 1;
    }
    assert(player.cylinder.dim.radius == 12 && player.cylinder.dim.height == 50 && player.cylinder.dim.yShift == 2);
    assert(player.actor.shape.shadowDraw == ActorShadow_DrawFeet && !sWolf.initialized);
    std::puts(
        "PASS production Wolf bite/attack window/MM damage flags/bounce/dash/wall rebound/native damage ownership");
    return 0;
}
