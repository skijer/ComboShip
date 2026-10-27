#include "entrances/FountainValley.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
struct Command {
  struct {
    uintptr_t w0{}, w1{};
  } words;
};
int main() {
  std::array<Command, 45> commands{};
  commands[25].words = {0x3200600c, 0};
  commands[26].words = {0x5e7003a0, 0x9feb4255};
  commands[40].words = {0x3200600c, 0x60};
  commands[41].words = {0x5e7003a0, 0x9feb4255};
  commands[27].words = {0x06000204, 0x00000406};
  commands[28].words = {0x06080a00, 0x00080006};
  commands[42].words = {0x06000204, 0x00000406};
  commands[43].words = {0x0604080a, 0x00040a06};
  commands[44].words = {0xdf000000, 0};
#ifdef COMBO_VALLEY_BACKDROP_SIGNATURE
  assert(ComboFountainValley::NativeBackdropLayout(commands));
  commands[26].words.w0++;
  assert(!ComboFountainValley::NativeBackdropLayout(commands));
  commands[26].words.w0--;
  commands[43].words.w1++;
  assert(!ComboFountainValley::NativeBackdropLayout(commands));
#else
  assert(false && "native backdrop has no targeted verified removal policy");
#endif
  std::cout << "PASS exact backdrop layout and foreign resource rejection\n";
}
