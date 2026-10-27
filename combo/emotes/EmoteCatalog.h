#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ComboEmotes {
inline constexpr size_t kFrameBytes = 134;
inline constexpr size_t kJointValues = 66;
inline constexpr size_t kSlots = 4;
struct Entry {
    int id;
    const char* name;
    const char* animation;
    uint32_t frames;
    const char* author;
    const char* audio;
};
// Stable IDs are persisted in CVars. Do not reorder IDs when adding emotes.
inline constexpr std::array<Entry, 8> kCatalog{ {
    { 1, "Kawaii Wave", "wave.bin", 81, "Malon Rose", "" },
    { 2, "Angry", "angry.bin", 384, "Malon Rose", "" },
    { 3, "Backflip", "backflip.bin", 84, "Malon Rose", "" },
    { 4, "Bashful", "bashful.bin", 220, "Malon Rose", "" },
    { 5, "Dab", "dab.bin", 29, "Neirn", "" },
    { 6, "Malon Sing", "malon_sing.bin", 1359, "Unesag", "malon_sing.pcm" },
    { 7, "Stylish Sheathing", "sheathing.bin", 65, "Author unspecified in supplied pack", "sheathing.pcm" },
    { 8, "Tiny Little Adiantum", "adiantum.bin", 80, "MalonRose", "" },
} };
inline const Entry* Find(int id) {
    return id > 0 && id <= static_cast<int>(kCatalog.size()) ? &kCatalog[id - 1] : nullptr;
}
inline constexpr std::array<int, kSlots> kDefaultEmotes{ 1, 4, 5, 2 };
inline constexpr uint32_t kModifier1 = 0x40;
inline constexpr std::array<uint32_t, kSlots> kDefaultChords{ kModifier1 | 0x800, kModifier1 | 0x400,
                                                              kModifier1 | 0x200, kModifier1 | 0x100 };
inline constexpr std::array<const char*, kSlots> kSlotKeys{ "gCombo.Emotes.Slot1", "gCombo.Emotes.Slot2",
                                                            "gCombo.Emotes.Slot3", "gCombo.Emotes.Slot4" };
inline constexpr std::array<const char*, kSlots> kChordKeys{ "gCombo.Emotes.Chord1", "gCombo.Emotes.Chord2",
                                                             "gCombo.Emotes.Chord3", "gCombo.Emotes.Chord4" };
} // namespace ComboEmotes
