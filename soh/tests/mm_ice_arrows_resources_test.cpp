// Real resource owning classes and production C bridge. The RM/archive boundary is controlled.
#include "soh/ActorDB.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/CrossRMRegistry.h>
#include "soh/resource/type/CollisionHeader.h"
#include <fast/resource/type/DisplayList.h>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
extern "C" {
#include "soh/Enhancements/Items/MmIceArrows.h"
#include "overlays/actors/ovl_Bg_MmIcefloe/z_bg_mm_icefloe.h"
#include "overlays/actors/ovl_Bg_MmIceblock/z_bg_mm_iceblock.h"
void BgMmIcefloe_Init(Actor*, PlayState*) {
}
void BgMmIcefloe_Destroy(Actor*, PlayState*) {
}
void BgMmIcefloe_Update(Actor*, PlayState*) {
}
void BgMmIcefloe_Draw(Actor*, PlayState*) {
}
void BgMmIceblock_Init(Actor*, PlayState*) {
}
void BgMmIceblock_Destroy(Actor*, PlayState*) {
}
void BgMmIceblock_Update(Actor*, PlayState*) {
}
void BgMmIceblock_Draw(Actor*, PlayState*) {
}
}
#define REQUIRE(c)                                                    \
    do {                                                              \
        if (!(c)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)

static std::shared_ptr<Ship::ResourceManager> own, foreign;
static Ship::Context* context;
static std::unordered_map<std::string, std::shared_ptr<Ship::IResource>> cache;
static std::string actorName;
static ActorDBInit registered;
static int id = -1, registrations, ownLoads, foreignLoads, alt;
ActorDB* ActorDB::Instance;
ActorDB::ActorDB() {
}
ActorDB::Entry::Entry() {
}
ActorDB::Entry& ActorDB::AddEntry(const ActorDBInit& profile) {
    static Entry entry;
    registered = profile;
    actorName = profile.name;
    ++registrations;
    id = profile.name == "Bg_MmIceblock" ? 661 : 660;
    entry.entry.id = id;
    return entry;
}
int ActorDB::RetrieveId(const std::string& name) {
    return name == actorName ? id : -1;
}

namespace Ship {
Context::Context(std::string, std::string, std::string) {
}
Context::~Context() {
}
Context* Context::GetRawInstance() {
    return context;
}
std::shared_ptr<ResourceManager> Context::GetResourceManager() const {
    return own;
}
ResourceManager::ResourceManager() {
}
ResourceManager::~ResourceManager() {
}
std::shared_ptr<ResourceManager> CrossRMRegistry::GetOrActive(const std::string& name) {
    REQUIRE(name == "oot");
    return own;
}
std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& path, bool exact,
                                                         std::shared_ptr<ResourceInitData>) {
    (this == own.get() ? ownLoads : foreignLoads)++;
    REQUIRE(!exact); // Match normal native SoH asset resolution, including Alt assets.
    std::string key = path.starts_with("__OTR__") ? path.substr(7) : path;
    return cache[(alt ? "alt/" : "") + key];
}
} // namespace Ship

static void Resources(bool block) {
    cache.clear();
    ownLoads = foreignLoads = 0;
    auto meta = std::make_shared<Ship::ResourceInitData>();
    auto col = std::make_shared<SOH::CollisionHeader>(meta);
    col->vertices = block ? std::vector<Vec3s>{ { -600, 0, -600 }, { 600, 1000, 600 } }
                          : std::vector<Vec3s>{ { -1473, -1985, -1696 }, { 1473, 150, 1696 } };
    col->collisionHeaderData.minBounds = col->vertices[0];
    col->collisionHeaderData.maxBounds = col->vertices[1];
    col->collisionHeaderData.vtxList = col->vertices.data();
    col->collisionHeaderData.numVertices = 2;
    auto dl = std::make_shared<Fast::DisplayList>(meta);
    dl->Instructions.push_back({});
    cache[(alt ? "alt/" : "") + std::string(block ? "objects/object_demo_kekkai/gWaterTrialIceBlockCol"
                                                  : "objects/object_spot08_obj/gZorasFountainIcebergCol")] = col;
    cache[(alt ? "alt/" : "") + std::string(block ? "objects/object_demo_kekkai/gWaterTrialIceBlockDL"
                                                  : "objects/object_spot08_obj/gZorasFountainIcebergDL")] = dl;
}

int main() {
    Ship::Context testContext("test", "test", "");
    context = &testContext;
    own = std::make_shared<Ship::ResourceManager>();
    foreign = std::make_shared<Ship::ResourceManager>();
    ActorDB db;
    ActorDB::Instance = &db;
    REQUIRE(MmIceArrows_GetActorId() == 660 && registrations == 1);
    REQUIRE(registered.objectId == OBJECT_GAMEPLAY_KEEP && registered.category == ACTORCAT_BG);
    REQUIRE(registered.instanceSize == sizeof(BgMmIcefloe) && registered.destroy == BgMmIcefloe_Destroy);
    REQUIRE(MmIceArrows_GetActorId() == 660 && registrations == 1);
    // ActorDB can be recreated when a game is restarted. No stale static actor ID survives it.
    actorName.clear();
    id = -1;
    REQUIRE(MmIceArrows_GetActorId() == 660 && registrations == 2);
    REQUIRE(MmIceArrows_GetOctorokBlockActorId() == 661 && registrations == 3);
    REQUIRE(registered.instanceSize == sizeof(BgMmIceblock) && registered.destroy == BgMmIceblock_Destroy);
    REQUIRE(MmIceArrows_GetOctorokBlockActorId() == 661 && registrations == 3);
    for (alt = 0; alt < 2; ++alt) {
        for (bool block : { false, true }) {
            Resources(block);
            std::weak_ptr<Ship::IResource> weakCol = cache.begin()->second;
            CollisionHeader* col = nullptr;
            Gfx* dl = nullptr;
            void* handle =
                block ? MmIceArrows_AcquireBlockResources(&col, &dl) : MmIceArrows_AcquireResources(&col, &dl);
            REQUIRE(handle && col && dl && col->vtxList[1].x == (block ? 600 : 1473));
            REQUIRE(ownLoads == 2 && foreignLoads == 0);
            cache.clear();
            REQUIRE(!weakCol.expired());
            REQUIRE(col->vtxList[0].y == (block ? 0 : -1985)); // valid after cache eviction/Alt reload.
            MmIceArrows_ReleaseResources(handle);
            REQUIRE(weakCol.expired());
        }
    }
    cache.clear();
    CollisionHeader* col = reinterpret_cast<CollisionHeader*>(1);
    Gfx* dl = reinterpret_cast<Gfx*>(1);
    REQUIRE(!MmIceArrows_AcquireResources(&col, &dl) && col == nullptr && dl == nullptr);
    REQUIRE(!MmIceArrows_AcquireBlockResources(&col, &dl) && col == nullptr && dl == nullptr);
    own.reset();
    context = nullptr;
    REQUIRE(!MmIceArrows_AcquireResources(&col, &dl) && col == nullptr && dl == nullptr);
    ActorDB::Instance = nullptr;
    REQUIRE(MmIceArrows_GetActorId() == -1);
    REQUIRE(MmIceArrows_GetOctorokBlockActorId() == -1);
    MmIceArrows_ReleaseResources(nullptr);
    puts("MM ice-arrow native resource ownership: PASS");
}
