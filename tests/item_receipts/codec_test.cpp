#include "combo/menu/ComboItemReceiptText.h"
#include <cassert>
#include <iostream>

using namespace ComboItemReceiptText;

int main() {
  const auto nei =
      FromNeiMarkup("You got %cDeku Leaf%w!&Press %y\xA1%w.^Glide.");
  const char want[] = "You got \x05"
                      "Deku Leaf\x00!\x11Press \x04\xB2\x00.\x10Glide.";
  assert(nei == std::string(want, sizeof(want) - 1));
  assert(FromNeiMarkup("\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA") ==
         "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB");
  assert(FromNeiMarkup("\xAB") == "D-Pad"); // MM has no control-pad texture
  assert(FromNeiMarkup("Poké Ball") == "Pok\x9D Ball");
  assert(FromNeiMarkup("Îî") == "\x8B\xA2");
  std::string accent;
  assert(FromOotMessage("Pok\x96 Ball\x02", accent) &&
         accent == "Pok\x9D Ball");
  assert(FromOotMessage("Press \xAB.\x02", accent) && accent == "Press D-Pad.");

  std::string body;
  const char nativeOot[] = "\x08You got \x05\x46"
                           "Bow\x05\x40!\x09\x01Press \x9F.\x04"
                           "Aim.\x13\x03\x02";
  assert(
      FromOotMessage(std::string_view(nativeOot, sizeof(nativeOot) - 1), body));
  const char wantOot[] = "\x17You got \x04"
                         "Bow\x00!\x18\x11Press \xB0.\x10"
                         "Aim.";
  assert(body == std::string(wantOot, sizeof(wantOot) - 1));
  assert(
      !FromOotMessage("\x05", body)); // truncated parameter, no reads past end
  const char defaultColor[] =
      "\x05\x00White\x02"; // CustomMessage's QM_WHITE encoding
  assert(FromOotMessage(
      std::string_view(defaultColor, sizeof(defaultColor) - 1), body));
  assert(body == std::string("\x00White", 6));
  assert(!FromOotMessage("Choose\x1B", body));
  assert(!FromOotMessage("Follow\x07\x10\x20", body));
  const char iconInArguments[] = "\x12\x13\x13\x05\x00\x13\x20Native lesson.\x02";
  const char iconlessArguments[] = "\x12\x13\x13\x05\x00Native lesson.\x02";
  assert(WithoutOotItemIcons(std::string_view(iconInArguments, sizeof(iconInArguments) - 1), body));
  assert(
      body == std::string(iconlessArguments, sizeof(iconlessArguments) - 1) &&
      "icon removal must preserve zero colors and icon-like control arguments");
  assert(!WithoutOotItemIcons("\x13", body));

  const char mm[] =
      "\x17You got the Bow!\x18\x10Press \xB2 to aim.\x1E\xBF\x01\x19\xBF";
  assert(FromMMMessage(std::string_view(mm, sizeof(mm) - 1), body));
  assert(body == "\x17You got the Bow!\x18\x10Press \xB2 to aim.");
  assert(!FromMMMessage("Hello\x1C\x01", body));
  assert(!FromMMMessage("Buy?\xC2", body));

  float widths[160];
  for (float &w : widths)
    w = 10;
  body = "one\x11two\x11three\x11"
         "four\x11"
         "five";
  Wrap(body, widths, 160, 240);
  assert(body == "one\x11two\x11three\x10"
                 "four\x11"
                 "five");
  body = "first word second word third word fourth word fifth word sixth word";
  Wrap(body, widths, 160, 110);
  std::string plain;
  unsigned lines = 1;
  for (uint8_t c : body) {
    if (c == 0x10) {
      assert(lines <= 3);
      lines = 1;
      plain += ' ';
    } else if (c == 0x11) {
      ++lines;
      assert(lines <= 3);
      plain += ' ';
    } else
      plain += c;
  }
  assert(plain ==
         "first word second word third word fourth word fifth word sixth word");
  std::cout << "receipt colors, buttons, pages, control parameters and "
               "lossless wrapping passed\n";
}
