// Requires a user-supplied NEIWOLF1 payload. Never generates a model substitute.
#define main wolf_core_fixture_main
#include "core_runtime_test.cpp"
#undef main
#include <iterator>

static std::vector<u8> readRealAsset(const char* path) {
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

int main(int argc, char** argv) {
  assert(argc == 3 || argc == 4);
  assetDirectory = argv[1];
  captureWolfLogs();
  const auto real = readRealAsset(argv[2]);
  assert(NeiWolfAsset::Validate(real));
  assert(load(real));
  assert(sSkin.vertexCount == field(real, 0) &&
         sSkin.boneCount == field(real, 2));
  assert(sAnimations.size() == field(real, 3));
  for (const char *name : kAnimNames)
    assert(FindAnim(name) >= 0);

  // The archive-selected resource wins over the loose file. LUS adds 16 zero
  // bytes; the production payload-size check must not include them in
  // totalSize.
  resourceBlob = real;
  resourceBlob.resize(real.size() + 16, 0);
  resourceBlobOwner = "oot";
  sAssetsLoaded = 0;
  assert(LoadAssets() &&
         sAssetPath == "@oot:objects/forms/wolf_link/gWolfLinkData");
  assert(sBlob == real &&
         "the model must own its exact payload after the resource copy");
  resourceBlob.back() = 1;
  sAssetsLoaded = 0;
  assert(!LoadAssets() && !sAssetsLoaded && logged("reason=resource-blob"));
  resourceBlob.back() = 0;
  assert(LoadAssets() && "fixing a rejected resource permits retry");
  resourceBlob.clear();

  if (argc == 4) {
    const auto hd = readRealAsset(argv[3]);
    assert(NeiWolfAsset::Validate(hd));
    integerCvars["gMods.WolfLink.UseHDModel"] = 0;
    resourceBlob = real;
    sAssetsLoaded = 0;
    assert(LoadAssets() && sSkin.vertexCount == field(real, 0));
    const s32 registration = sDefIndex;
    const auto *owned = sBlob.data();
    sWolf.initialized = 1;
    integerCvars["gMods.WolfLink.UseHDModel"] = 1;
    resourceBlob = hd;
    resourceBlobPath = NeiWolfAsset::kHDResourcePath;
    assert(LoadAssets() && sBlob.data() == owned &&
           sSkin.vertexCount == field(real, 0));
    sWolf.initialized = 0;
    assert(LoadAssets() && sSkin.vertexCount == field(hd, 0) &&
           sDefIndex == registration);
    for (int change = 0; change < 20; ++change) {
      const bool useHD = change % 2;
      integerCvars["gMods.WolfLink.UseHDModel"] = useHD;
      resourceBlob = useHD ? hd : real;
      resourceBlobPath =
          useHD ? NeiWolfAsset::kHDResourcePath : NeiWolfAsset::kResourcePath;
      assert(LoadAssets() && sBlob == resourceBlob &&
             sDefIndex == registration);
    }
    resourceBlob.clear();
    resourceBlobPath = NeiWolfAsset::kResourcePath;
    integerCvars.erase("gMods.WolfLink.UseHDModel");
    assert(load(real));
    std::puts("PASS MM model change defers during an active transform; 20 "
              "switches reuse one registration");
  }

    // Corruptions of the actual v2 export, including its additional audio table,
    // must be rejected before registration, and a corrected file must reload.
    for (int mutation = 0; mutation < 7; ++mutation) {
        auto bad = real;
        const u32 audio = field(bad, 19);
        switch (mutation) {
            case 0:
                write32(bad, 12 + 19 * 4, 0xfffffffcu);
                break;
            case 1:
                write32(bad, audio, 0xffffffffu);
                break;
            case 2:
                write32(bad, audio + 4 + 40, 0xfffffffeu);
                break;
            case 3:
                write32(bad, audio + 4 + 36, 0x80000001u);
                break;
            case 4:
                std::memset(bad.data() + audio + 4, 'X', 32);
                break;
            case 5:
                write32(bad, audio + 4 + 32, 0);
                break;
            case 6:
                for (u32 i = 0; i < field(bad, 3); ++i) {
                    const u32 name = ReadU32(bad.data() + field(bad, 11) + i * 16);
                    if (std::strcmp((const char*)bad.data() + name, kAnimNames[0]) == 0)
                        bad[name] = 'X';
                }
                break;
        }
        rejected("corrupted real v2 export", bad);
        assert(!sAssetsLoaded);
        assert(load(real));
    }

    PlayState play{};
    Player player{};
    Camera camera{};
    GraphicsContext gfx{};
    Gfx opa[256]{}, xlu[16]{}, overlay[16]{};
    gPlayState = &play;
    play.state.gfxCtx = &gfx;
    play.cameraPtrs[0] = &camera;
    R_UPDATE_RATE = 3;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    player.cylinder.dim.radius = 12;
    player.cylinder.dim.height = 50;
    const auto prepareArenas = [&] {
        gfx.polyOpa.p = opa;
        gfx.polyOpa.d = opa + 256;
        gfx.polyXlu.p = xlu;
        gfx.polyXlu.d = xlu + 16;
        gfx.overlay.p = overlay;
        gfx.overlay.d = overlay + 16;
    };
    size_t submissions = 0;
    for (int cycle = 0; cycle < 4; ++cycle) {
      std::vector<u8> current = real;
      if (argc == 4) {
        const bool useHD = cycle % 2;
        integerCvars[NeiWolfAsset::kHDModelCVar] = useHD;
        current = useHD ? readRealAsset(argv[3]) : real;
        resourceBlob = current;
        resourceBlobPath =
            useHD ? NeiWolfAsset::kHDResourcePath : NeiWolfAsset::kResourcePath;
      }
        WolfLinkForm_Select(1);
        assert(WolfLinkForm_LoadSkeleton(&play));
        assert(sWolf.initialized && sSkin.vtxBuf[0] && sSkin.vtxBuf[1]);
        assert(sSkin.vertexCount == field(current, 0) &&
               sSkin.boneCount == field(current, 2));
        if (argc == 4) {
          const auto *owned = sBlob.data();
          integerCvars[NeiWolfAsset::kHDModelCVar] = !(cycle % 2);
          assert(WolfLinkForm_LoadSkeleton(&play) && sBlob.data() == owned);
        }
        for (const auto& clip : sAnimations) {
            sWolf.character.ssbbAnim = &clip;
            for (const float frame : { 0.0f, std::min(0.5f, (float)clip.numFrames - 1), (float)clip.numFrames - 1 }) {
                sWolf.character.curFrame = frame;
                assert(SSBBSkin_ComputePose(&sWolf.character));
                for (s32 bone = 0; bone < sSkin.boneCount; ++bone) {
                    Vec3f position{};
                    assert(SSBBSkin_GetBoneWorldPos(&sWolf.character, bone, &position));
                    assert(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z));
                }
                prepareArenas();
                assert(SSBBSkin_Draw(&sWolf.character, &play, &player.actor.world.pos, &player.actor.shape.rot));
                assert(gfx.polyOpa.p > opa && gfx.polyOpa.p < gfx.polyOpa.d);
                ++submissions;
            }
        }
        prepareArenas();
        assert(WolfLinkForm_Draw(&play, &player));
        player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
        ProcMoveInit(&player);
        ResetCombo();
        play.state.input[0] = {};
        play.state.input[0].press.button = BTN_B;
        WolfLinkForm_Update(&player, &play, &play.state.input[0], 0);
        assert(sWolf.proc == PROC_WOLF_WAIT_ATTACK && sWolf.procOwnsPlayer);
        play.state.input[0].press.button = 0;
        for (int tick = 0; tick < 6; ++tick)
            WolfLinkForm_Update(&player, &play, &play.state.input[0], 0);
        assert(sWolf.atCylInit);
        ProcMoveInit(&player);
        ResetCombo();
        player.speedXZ = 6;
        play.state.input[0].rel.stick_y = 60;
        play.state.input[0].press.button = BTN_A;
        WolfLinkForm_Update(&player, &play, &play.state.input[0], 0);
        assert(sWolf.proc == PROC_WOLF_DASH && sWolf.procOwnsPlayer);
        WolfLinkForm_Cleanup(&player, &play);
        WolfLinkForm_Select(0);
        assert(!sWolf.initialized && !sSkin.vtxBuf[0] && !sSkin.vtxBuf[1]);
        assert(player.cylinder.dim.radius == 12 && player.cylinder.dim.height == 50);
        assert(player.actor.shape.shadowDraw == ActorShadow_DrawFeet);
    }
    std::printf("PASS real Wolf v2: %u vertices, %u bones, %zu clips, %zu skin/draw submissions, four action/lifecycle "
                "cycles\n",
                sSkin.vertexCount, sSkin.boneCount, sAnimations.size(), submissions);
    return 0;
}
