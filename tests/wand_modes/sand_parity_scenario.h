// Run identical inputs against the actual OoT donor and MM port. Mesh allocation
// and floor rectangles are fixture boundaries; placement, input, billing, slab
// update, draw shrink and ring eviction all come from each game's source.
static Actor* SandFloorAt(const std::vector<Actor*>& ready, const Vec3f& feet, float halfX = 30) {
    Actor* floor = nullptr;
    for (Actor* slab : ready) {
        if (!slab->update) continue;
        const float dx = feet.x - slab->world.pos.x, dz = feet.z - slab->world.pos.z;
        const float sine = Math_SinS(slab->shape.rot.y), cosine = Math_CosS(slab->shape.rot.y);
        if (std::fabs(dx * cosine - dz * sine) > halfX || std::fabs(dx * sine + dz * cosine) > 30 ||
            std::fabs(slab->world.pos.y + 1 - feet.y) > .01f) continue;
        // Native dynamic floor traversal retains the first bgId at equal height.
        if (!floor || ((DynaPolyActor*)slab)->bgId < ((DynaPolyActor*)floor)->bgId) floor = slab;
    }
    return floor;
}

static void SandUpdateReady(PlayState& play, const std::vector<Actor*>& ready, Actor* standing) {
    for (Actor* slab : ready) {
        if (!slab->update) continue;
        onSlab = slab == standing;
        slab->update(slab, &play);
    }
    onSlab = false;
}

static void SandTrace(int scenario, int frame, Player& player, float halfX) {
    auto units = [](float value) { return std::lround(value * 1000); };
    std::cout << "SANDTRACE " << scenario << ' ' << frame << ' ' << SandParityMagic() << ' '
              << units(player.actor.world.pos.x) << ',' << units(player.actor.world.pos.z) << ' '
              << actors.size();
    Actor* floor = SandFloorAt(actors, player.actor.world.pos, halfX);
    std::cout << " floor=" << (floor ? ((DynaPolyActor*)floor)->bgId : -1);
    for (Actor* slab : actors) {
        std::cout << " [" << units(slab->world.pos.x) << ',' << units(slab->world.pos.y) << ','
                  << units(slab->world.pos.z) << ',' << slab->shape.rot.y << ','
                  << units(slab->scale.x) << ',' << units(WandSand_Remaining(slab)) << ','
                  << (slab->update != nullptr) << ']';
    }
    std::cout << '\n';
}

static void CheckSandParity() {
    int scenario = 0;
    for (u16 button : {BTN_CLEFT, BTN_DUP}) {
        for (s16 yaw : {0, 0x2000, 0x4000, -0x2000}) {
            for (float halfX : {30.0f, 40.0f}) {
                Player player{}; PlayState play{};
                ResetSandParity(player, play, button, yaw, halfX);
                auto tick = [&](u16 press, u16 held) {
                    play.state.input[0].press.button = press;
                    play.state.input[0].cur.button = held;
                    ++play.gameplayFrames;
                    Wand_TickInput(&play, &player);
                };
                tick(button, button); SandTrace(scenario, 0, player, halfX); // draw
                tick(0, 0); SandTrace(scenario, 1, player, halfX);
                tick(button, button); SandTrace(scenario, 2, player, halfX); // cast
                for (int frame = 0; frame < 100; ++frame) {
                    const auto ready = actors;
                    SandUpdateReady(play, ready, SandFloorAt(ready, player.actor.world.pos, halfX));
                    if (frame == 80) player.actor.shape.rot.y = (s16)(yaw + 0x4000);
                    tick(0, button);
                    if (frame < 60 || frame >= 80) {
                        player.actor.world.pos.x += Math_SinS(player.actor.shape.rot.y) * 6;
                        player.actor.world.pos.z += Math_CosS(player.actor.shape.rot.y) * 6;
                    }
                    SandTrace(scenario, 3 + frame, player, halfX);
                }
                // Successful pressed casts exercise eight-slot overflow and its
                // 24-frame crumble independently of held-road coverage.
                SandParityRefill();
                for (int cast = 0; cast < 12; ++cast) {
                    player.actor.world.pos.z += 60;
                    assert(Wand_Cast(&player, &play, WAND_MODE_SAND));
                    SandTrace(scenario, 103 + cast, player, halfX);
                }
                for (int frame = 0; frame < 24; ++frame) {
                    SandUpdateReady(play, actors, nullptr);
                    SandTrace(scenario, 115 + frame, player, halfX);
                }
                ++scenario;
            }
        }
    }
}
