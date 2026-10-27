// Support contains the actual host attachment functions and native engine types.
/* SUPPORT */

static uint64_t choices;
static unsigned begins, ends;
static std::set<unsigned> visitedGotos;
static bool FlowChoice(unsigned index) { return (choices >> index) & 1; }
static void FlowRecordGoto(unsigned index) { visitedGotos.insert(index); }
static void FlowBegin() { ++begins; BeginRoomMagic(); }
static void FlowEnd() { ++ends; RestoreRoomMagic(); }
static void FlowTailUse() { assert(!sRoomMagicPlay && !sRoomMagicOriginal); }
/* HOOK_ALIASES */
/* BRIDGE */
/* DRAW_FLOW */

int main() {
    PlayState play{}, other{};
    gPlayState = &play;
    const std::string path = std::string("__OTR__") + BlackMarketRoomMagic::SourceRoom(oot);
    Gfx translucent[] = { gsSPEndDisplayList() };
    Entry entry{ reinterpret_cast<Gfx*>(const_cast<char*>(path.c_str())), translucent };
    const Entry originalEntry = entry;
    Shape original{};
    SetShape(original, entry);
    const Shape originalShape = original;
    ShapePointer(play) = &original;
    integers[BlackMarketRoomMagic::kCvar] = 1;
    unsigned bypassedEnd = 0;
    for (choices = 0; choices < (uint64_t(1) << kChoiceCount); ++choices) {
        begins = ends = 0;
        RunNativeDrawExitFlow(&play);
        if (begins && !ends)
            ++bypassedEnd;
        assert(ShapePointer(play) == &original && "A completed native draw retained the private room shape");
        assert(!sRoomMagicPlay && !sRoomMagicOriginal);
        assert(std::memcmp(&originalShape, &original, sizeof(original)) == 0);
        assert(std::memcmp(&originalEntry, &entry, sizeof(entry)) == 0);
    }
    assert(bypassedEnd > 0 && visitedGotos.size() == kGotoCount);
    BeginRoomMagic();
    assert(ShapePointer(play) == &sRoomMagicShape);
    RestoreAfterDraw(&other); // The private bridge cannot restore another draw's owner.
    assert(ShapePointer(play) == &sRoomMagicShape);
    RestoreAfterDraw(&play);
    assert(ShapePointer(play) == &original);
    RestoreAfterDraw(&play); // Normal end-hook cleanup makes the final cleanup a safe no-op.
    assert(ShapePointer(play) == &original);
    BeginRoomMagic();
    Shape otherShape{};
    ShapePointer(play) = &otherShape;
    RestoreAfterDraw(&play);
    assert(ShapePointer(play) == &otherShape && !sRoomMagicPlay && !sRoomMagicOriginal);
    std::printf("PASS %s production-derived native draw flow: %u branches, %u goto edges, %u end-hook bypass paths; exact same-draw restoration and ownership guard\n",
                oot ? "oot" : "mm", kChoiceCount, kGotoCount, bypassedEnd);
}
