#pragma once
#include <algorithm>
#include <cstdint>
#include <string>

namespace KeyReceiptFixtures {
struct Entry {
  const char *name;
  const char *article;
  uint8_t color;
  bool mm;
};

// Independent expected MM control bytes, shared by the two real host fixtures.
// These are receipt expectations, not values obtained from the production
// policy.
inline constexpr Entry entries[] = {
    {"Forest Temple Small Key", "a ", 2, false},
    {"Fire Temple Small Key", "a ", 1, false},
    {"Water Temple Small Key", "a ", 3, false},
    {"Spirit Temple Small Key", "a ", 4, false},
    {"Shadow Temple Small Key", "a ", 6, false},
    {"Bottom of the Well Small Key", "a ", 6, false},
    {"Training Ground Small Key", "a ", 4, false},
    {"Gerudo Fortress Small Key", "a ", 4, false},
    {"Ganon's Castle Small Key", "a ", 1, false},
    {"Chest Game Small Key", "a ", 2, false},
    {"Forest Temple Boss Key", "the ", 2, false},
    {"Fire Temple Boss Key", "the ", 1, false},
    {"Water Temple Boss Key", "the ", 3, false},
    {"Spirit Temple Boss Key", "the ", 4, false},
    {"Shadow Temple Boss Key", "the ", 6, false},
    {"Ganon's Castle Boss Key", "the ", 1, false},
    {"Forest Temple Key Ring", "the ", 2, false},
    {"Fire Temple Key Ring", "the ", 1, false},
    {"Water Temple Key Ring", "the ", 3, false},
    {"Spirit Temple Key Ring", "the ", 4, false},
    {"Shadow Temple Key Ring", "the ", 6, false},
    {"Bottom of the Well Key Ring", "the ", 6, false},
    {"Training Ground Key Ring", "the ", 4, false},
    {"Gerudo Fortress Key Ring", "the ", 4, false},
    {"Ganon's Castle Key Ring", "the ", 1, false},
    {"Chest Game Key Ring", "the ", 2, false},
    {"Woodfall Small Key", "a ", 6, true},
    {"Snowhead Small Key", "a ", 2, true},
    {"Great Bay Small Key", "a ", 3, true},
    {"Stone Tower Small Key", "a ", 4, true},
    {"Woodfall Boss Key", "the ", 6, true},
    {"Snowhead Boss Key", "the ", 2, true},
    {"Great Bay Boss Key", "the ", 3, true},
    {"Stone Tower Boss Key", "the ", 4, true},
};

inline std::string Expected(const Entry &entry) {
  return std::string("You found ") + entry.article +
         static_cast<char>(entry.color) + entry.name + '\0' + "!";
}

inline std::string Flatten(std::string body) {
  // OoT's native formatter can place a line break after final punctuation.
  // Compare the receipt's content and color bytes, ignoring that empty line.
  while (!body.empty() && body.back() == '\x11')
    body.pop_back();
  std::replace(body.begin(), body.end(), '\x11', ' ');
  return body;
}
} // namespace KeyReceiptFixtures
