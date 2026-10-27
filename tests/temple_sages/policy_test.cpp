#include <cstdio>

#if !__has_include("entrances/TempleSagesPolicy.h")
int main() {
    std::fputs("FAIL: Temple Sages pad policy is not implemented\n", stderr);
    return 1;
}
#else
#include "entrances/TempleSagesPolicy.h"
#include <array>
#include <cstdlib>
#include <limits>

namespace {
void Require(bool value, const char* reason) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", reason);
        std::exit(1);
    }
}
struct Entry {
    int id;
    TempleSages::Point pos;
    struct { short x, y, z; } rot;
    short params;
};
std::array<Entry, 6> Entries() {
    return { { { 0x012E, { 0, -120, 0 }, { 0, 1234, 0 }, static_cast<short>(0xFC00) },
               { 0x012E, { 300, -120, 0 }, { 0, 2345, 0 }, static_cast<short>(0xFC01) },
               { 0x012E, { 600, -120, 0 }, { 0, 3456, 0 }, static_cast<short>(0xFC02) },
               { 0x012E, { 900, -120, 0 }, { 0, 4567, 0 }, static_cast<short>(0xFC03) },
               { 0x012E, { 1200, -120, 0 }, { 0, 5678, 0 }, static_cast<short>(0xFC04) },
               { 0x006C, { 1, 2, 3 }, { 0, 0, 0 }, 0 } } };
}
int playToken, sceneToken, roomToken, otherToken;
TempleSages::Visit Visit() {
    return { &playToken, &sceneToken, &roomToken, 0x43, 0, 2, true };
}
TempleSages::PlayerState Player() {
    return { Visit(), { 2000, -120, 0 }, -120, 32767, 0x3Fu, true, true, true };
}
void Capture(TempleSages::PadCache& cache) {
    const auto entries = Entries();
    cache.Capture(Visit(), entries.data(), entries.size());
}
void Arm(TempleSages::PadCache& cache) {
    Require(!cache.Tick(Player()), "walking clear should arm without travelling");
}

void TestMarkersAndVisits() {
    TempleSages::PadCache cache;
    const auto entries = Entries();
    cache.Capture(Visit(), entries.data(), entries.size());
    Require(cache.Ready(Visit()), "exactly five authored markers must enable this room visit");
    Require(cache.Pads()[0].position.y == -120 && cache.Pads()[4].yaw == 5678,
            "marker floor positions and native yaw must survive capture");
    for (int missing = 0; missing < 5; ++missing) {
        auto invalid = entries;
        invalid[missing].id = 0x12F;
        cache.Capture(Visit(), invalid.data(), invalid.size());
        Require(!cache.Ready(Visit()), "a missing or wrong-actor marker must disable travel");
        invalid = entries;
        invalid[missing].params = static_cast<short>(0xFC06);
        cache.Capture(Visit(), invalid.data(), invalid.size());
        Require(!cache.Ready(Visit()), "out-of-range marker params must not identify a destination");
    }
    // A POC1 six-marker addon must invalidate even an already valid POC2 visit.
    Capture(cache);
    auto stale = entries;
    stale[5] = { 0x012E, { 1500, -120, 0 }, { 0, 6789, 0 }, static_cast<short>(0xFC05) };
    cache.Capture(Visit(), stale.data(), stale.size());
    Require(!cache.Ready(Visit()), "old six-marker layout must fail closed and discard stale poses");
    auto duplicate = entries;
    duplicate[5] = entries[0];
    cache.Capture(Visit(), duplicate.data(), duplicate.size());
    Require(!cache.Ready(Visit()), "duplicate route markers must fail closed");
    Capture(cache);
    for (int bad = 0; bad < 7; ++bad) {
        auto visit = Visit();
        switch (bad) {
            case 0: visit.owner = &otherToken; break;
            case 1: visit.sceneData = &otherToken; break;
            case 2: visit.roomData = &otherToken; break;
            case 3: visit.scene = 0x44; break;
            case 4: visit.room = 1; break;
            case 5: visit.setup = 4; break;
            case 6: visit.roomReady = false; break;
        }
        Require(!cache.Ready(visit), "foreign owner, room, setup or unfinished load must not reuse cached pads");
    }
    auto badFloat = entries;
    badFloat[0].pos.x = std::numeric_limits<float>::quiet_NaN();
    cache.Capture(Visit(), badFloat.data(), badFloat.size());
    Require(!cache.Ready(Visit()), "nonfinite authored position must disable the cache");
    cache.Capture(Visit(), entries.data(), 256);
    Require(!cache.Ready(Visit()), "invalid scene actor counts must be bounded before access");
    cache.Capture(Visit(), static_cast<const Entry*>(nullptr), 5);
    Require(!cache.Ready(Visit()), "missing actor list must disable pads");
}

void TestReturnVisits() {
    for (int route = 0; route < 5; ++route) {
        TempleSages::PadCache cache;
        auto visit = Visit();
        visit.scene = 86 + route;
        visit.room = 3;
        const TempleSages::Pad pad{{100, 20, 300}, 123};
        cache.CaptureReturn(visit, route, pad);
        Require(cache.Ready(visit) && cache.Active(route), "destination cache must capture only its own route");
        auto player = Player();
        player.visit = visit;
        player.position = {1000, 20, 300};
        player.floorHeight = 20;
        cache.Tick(player);
        player.position = pad.position;
        player.questItems = 1u << ((route + 1) % 5);
        Require(!cache.Tick(player), "another medallion cannot unlock a return");
        player.questItems = 1u << route;
        Require(cache.Tick(player).value_or(-1) == route, "matching medallion returns only its route");
        Require(!cache.Tick(player), "arrival or repeated overlap cannot bounce");
        auto other = visit;
        ++other.room;
        Require(!cache.Ready(other), "room changes invalidate a destination cache");
        cache.CaptureReturn(visit, route, pad);
        Require(!cache.Tick(player), "recapture while on effect requires leaving before reentry");
        for (int i = 0; i < 5; ++i)
            Require(cache.Active(i) == (i == route), "return cannot expose other route positions");
    }
}

void TestRewardsAndEntrances() {
    constexpr int expected[] = { 0x600, 0x4F6, 0x604, 0x1F1, 0x568 };
    for (int route = 0; route < 5; ++route) {
        TempleSages::PadCache cache;
        Capture(cache);
        Arm(cache);
        auto player = Player();
        player.position = { static_cast<float>(route * 300), -120, 0 };
        player.questItems = 0;
        Require(!cache.Tick(player), "no medallion must leave every pad locked");
        player.questItems = 1u << ((route + 1) % 5);
        Require(!cache.Tick(player), "another medallion must not unlock this pad");
        player.questItems = 1u << route;
        const auto chosen = cache.Tick(player);
        Require(chosen && *chosen == route, "matching inventory reward must choose its native route");
        Require(TempleSages::Entrance(*chosen) == expected[route], "route must preserve its fixed native warp landing");
        Require(!cache.Tick(player), "remaining on the pad must not trigger twice");
    }
    Require(TempleSages::Entrance(-1) == -1 && TempleSages::Entrance(5) == -1 &&
            TempleSages::Entrance(6) == -1,
            "invalid routes must have no entrance");
    Require(!TempleSages::Owned(5, 0x3Fu) && !TempleSages::Owned(-1, 0x3Fu) &&
            !TempleSages::Owned(6, 0x3Fu),
            "Light and invalid routes must have no owned portal");
}

void TestTravelModes() {
    using TempleSages::TravelMode;
    using TempleSages::NormalizeTravelMode;
    using TempleSages::Unlocked;
    Require(NormalizeTravelMode(0) == TravelMode::Off && NormalizeTravelMode(1) == TravelMode::DungeonRewards &&
                NormalizeTravelMode(2) == TravelMode::Free,
            "saved dropdown values must select the requested travel policy");
    for (int invalid : { -100, -1, 4, 100 }) {
        Require(NormalizeTravelMode(invalid) == TravelMode::DungeonRewards,
                "unknown saved values must preserve reward-based travel");
        Require(!Unlocked(0, 0, static_cast<TravelMode>(invalid)) &&
                    Unlocked(0, 1, static_cast<TravelMode>(invalid)),
                "invalid mode values must not bypass reward requirements");
        TempleSages::PadCache cache;
        Capture(cache);
        auto player = Player();
        player.travelMode = static_cast<TravelMode>(invalid);
        player.questItems = 0;
        Require(!cache.Tick(player), "invalid saved mode must not travel outside pads");
        player.position.x = 0;
        Require(!cache.Tick(player), "invalid saved mode must not unlock a rewardless pad");
        player.questItems = 1;
        Require(cache.Tick(player).value_or(-1) == 0,
                "invalid saved mode must allow the matching medallion route");
    }
    Require(Player().travelMode == TravelMode::DungeonRewards,
            "existing aggregate callers must retain reward-based travel by default");
    for (auto mode : { TravelMode::Off, TravelMode::DungeonRewards, TravelMode::Free }) {
        for (uint32_t rewards : { 0u, 0x20u, 0x01u, 0x0Au, 0x1Fu }) {
            for (int route = 0; route < 5; ++route) {
                const bool expected = mode == TravelMode::Free ||
                                      (mode == TravelMode::DungeonRewards && (rewards & (1u << route)));
                Require(Unlocked(route, rewards, mode) == expected,
                        "mode must gate each of the five routes without treating Light as a destination");
                TempleSages::PadCache cache;
                Capture(cache);
                auto player = Player();
                player.travelMode = mode;
                player.questItems = rewards;
                Require(!cache.Tick(player), "walking clear may arm but must never travel");
                player.position.x = float(route * 300);
                const auto chosen = cache.Tick(player);
                Require(chosen.has_value() == expected && (!chosen || *chosen == route),
                        "real pad traversal must obey Off, Dungeon rewards and Free policies");
                Require(player.questItems == rewards, "travel policy must never forge or remove inventory rewards");
                Require(cache.Ready(Visit()) && cache.Pads()[4].position.x == 1200,
                        "Off and locked travel must retain all five authored visual marker poses");
            }
            for (int invalidRoute : { -1, 5, 6, 100 })
                Require(!Unlocked(invalidRoute, rewards, mode), "no mode may create a sixth or invalid route");
        }
    }
}

void TestWarpSongGating() {
    // Saved mode 3 is the new option; literal masks independently pin OoT's
    // Minuet/Bolero/Serenade/Requiem/Nocturne ownership bits.
    const auto mode = static_cast<TempleSages::TravelMode>(3);
    constexpr uint32_t songs[] = { 0x40, 0x80, 0x100, 0x200, 0x400 };
    Require(TempleSages::NormalizeTravelMode(3) == mode, "saved Warp songs option must retain its own gate");
    for (int route = 0; route < 5; ++route) {
        for (uint32_t songSet = 0; songSet < 32; ++songSet) {
            const uint32_t questItems = songSet << 6;
            const bool expected = (questItems & songs[route]) != 0;
            for (uint32_t extra : { 0u, 0x3Fu, 0x800u, 0x3F000u }) {
                const uint32_t inventory = questItems | extra;
                Require(TempleSages::Unlocked(route, inventory, mode) == expected,
                        "each portal requires its own song; rewards, Prelude and ordinary songs cannot bypass it");
                for (bool returning : { false, true }) {
                    TempleSages::PadCache cache;
                    auto player = Player();
                    if (returning)
                        cache.CaptureReturn(player.visit, route, { { float(route * 300), -120, 0 }, 0 });
                    else
                        Capture(cache);
                    player.travelMode = mode;
                    player.questItems = inventory;
                    Require(!cache.Tick(player), "song-gated approach may only arm outside the portal");
                    player.position.x = float(route * 300);
                    const auto chosen = cache.Tick(player);
                    Require(chosen.has_value() == expected && (!chosen || *chosen == route),
                            "outbound and return traversal must both obey matching song ownership");
                    Require(player.questItems == inventory, "song gating must not modify quest ownership");
                }
            }
        }
        Require(!TempleSages::Unlocked(route, songs[route], TempleSages::TravelMode::DungeonRewards),
                "song ownership alone must not unlock dungeon-reward mode");
    }
    for (int invalidRoute : { -1, 5, 6, 100 })
        Require(!TempleSages::Unlocked(invalidRoute, 0xFFFFFFFFu, mode),
                "song mode must not create Light or invalid routes");
}

void TestModeChangeAndReloadLatch() {
    using TempleSages::TravelMode;
    // Every directed transition must clear a previously armed approach,
    // including Free -> Dungeon rewards with an already-owned medallion.
    for (auto before : { TravelMode::Off, TravelMode::DungeonRewards, TravelMode::Free, static_cast<TravelMode>(3) }) {
        for (auto after : { TravelMode::Off, TravelMode::DungeonRewards, TravelMode::Free, static_cast<TravelMode>(3) }) {
            if (before == after)
                continue;
            TempleSages::PadCache cache;
            Capture(cache);
            auto player = Player();
            player.questItems |= 0x7C0u;
            player.travelMode = before;
            Require(!cache.Tick(player), "clear approach must not travel");
            player.position.x = 0;
            player.travelMode = after;
            Require(!cache.Tick(player) && !cache.Tick(player),
                    "changing travel mode on a portal must require a new deliberate entry");
            player.position.x = 2000;
            Require(!cache.Tick(player), "stepping clear after mode change must only arm");
            player.position.x = 0;
            Require(cache.Tick(player).has_value() == (after != TravelMode::Off),
                    "re-entering after mode change must follow the new policy");
            player.position.x = 2000;
            cache.Tick(player);
            Capture(cache);
            player.position.x = 0;
            Require(!cache.Tick(player), "room recapture must clear the mode and entry latch");
            cache.Reset();
            Capture(cache);
            Require(!cache.Tick(player), "explicit reset must clear the mode and entry latch");
        }
    }
    TempleSages::PadCache cache;
    Capture(cache);
    Arm(cache);
    auto player = Player();
    player.position.x = 0;
    player.travelMode = TravelMode::Free;
    player.controllable = false;
    Require(!cache.Tick(player), "blocked control must never travel during mode change");
    player.controllable = true;
    Require(!cache.Tick(player), "a mode change during blocked control must still reset the entry latch");
}

void TestEntryAndGrounding(TempleSages::TravelMode mode) {
    TempleSages::PadCache cache;
    Capture(cache);
    auto player = Player();
    player.travelMode = mode;
    player.position = { 0, -120, 0 };
    Require(!cache.Tick(player), "loading directly onto a pad must not travel");
    auto clear = player;
    clear.position.x = 2000;
    Require(!cache.Tick(clear), "walking clear must arm without travelling in any enabled mode");
    for (int bad = 0; bad < 10; ++bad) {
        auto blocked = player;
        switch (bad) {
            case 0: blocked.controllable = false; break;
            case 1: blocked.grounded = false; break;
            case 2: blocked.floorValid = false; break;
            case 3: blocked.floorNormalY = 23169; break;
            case 4: blocked.floorHeight = std::numeric_limits<float>::quiet_NaN(); break;
            case 5: blocked.position.x = std::numeric_limits<float>::infinity(); break;
            case 6: blocked.position.y = -108; break;
            case 7: blocked.position.y = -132; break;
            case 8: blocked.floorHeight = -140; break;
            case 9: blocked.visit.roomReady = false; break;
        }
        Require(!cache.Tick(blocked), "unsafe control, floor, height or load state must not trigger");
    }
    const float angle = cache.Pads()[0].yaw * (6.28318530718f / 65536.0f);
    auto local = [angle](float side, float front) {
        return TempleSages::Point{std::cos(angle)*side+std::sin(angle)*front, -120,
                                  -std::sin(angle)*side+std::cos(angle)*front};
    };
    for (auto point : {local(38.1f,5),local(0,-1),local(0,18.1f),local(63,0)}) {
        player.position = point;
        Require(!cache.Tick(player), "adjacent, behind or beyond tight portal face must not trigger");
    }
    player.position = local(37.9f,5);
    player.position.y = -108.01f;
    player.floorNormalY = 23170;
    Require(cache.Tick(player).value_or(-1) == 0, "valid inner-radius and strict-height boundary must activate");
    Require(!cache.Tick(clear), "walking clear must rearm without travelling in any enabled mode");
    Capture(cache);
    Require(!cache.Tick(player), "recapturing a room header must reset deliberate-entry latch");
    cache.Reset();
    Require(!cache.Ready(Visit()) && !cache.Tick(Player()), "play destruction must discard poses and arming");
}
} // namespace

int main() {
    TestMarkersAndVisits();
    TestReturnVisits();
    TestRewardsAndEntrances();
    TestTravelModes();
    TestWarpSongGating();
    TestModeChangeAndReloadLatch();
    TestEntryAndGrounding(TempleSages::TravelMode::DungeonRewards);
    TestEntryAndGrounding(TempleSages::TravelMode::Free);
    std::puts("PASS Temple Sages policy: five markers, Off/rewards/Free/Warp songs, native routes, mode/reload entry latch, inventory preservation and safety boundaries");
}
#endif
