// Use the full production class/formatter. The native renderer is represented
// only by safe capacity/glyph checks; this fixture never writes font textures.
#include "combo/menu/ComboMagicItemReceiptText.h"
#include "combo/menu/ComboMaskReceiptText.h"
#include "combo/menu/ComboToolReceiptText.h"
#include "combo/menu/ComboVanillaItemReceiptText.h"
#include "soh/soh/Enhancements/custom-message/CustomMessageManager.h"
#include <iostream>
#include <stdexcept>

using CatalogIconType = int32_t; // CATALOG_ICON_TYPE
struct CatalogEntry {
  const char *label;
  CatalogIconType icon;
  const char *english;
  const char *german;
  const char *french;
};
#include "pickup_catalog.inc"

static void CheckBody(const std::string &body) {
  if (body.size() > 1280)
    throw std::runtime_error("exceeds native message buffer");
  size_t decoded = 0, glyphs = 0;
  bool ended = false;
  for (size_t i = 0; i < body.size();) {
    const uint8_t c = body[i];
    const size_t command = c == 0x15 ? 4 : ComboReceipt_CommandSize(c, false);
    if (command > body.size() - i)
      throw std::runtime_error("truncated control operands");
    decoded += command + (c == 0x0F ? 7 : 0);
    if (c == 0x0F)
      glyphs += 8;
    if (command == 1 && c >= 0x21) {
      if (c - 0x20 >= 144)
        throw std::runtime_error("unsupported native glyph byte " +
                                 std::to_string(c));
      ++glyphs;
    }
    if (decoded > 200 || glyphs > 256)
      throw std::runtime_error("exceeds native per-page decode/font capacity");
    if (c == 0x02) {
      ended = true;
      if (i + command != body.size())
        throw std::runtime_error("early message terminator");
    }
    if (c == 0x04 || c == 0x0C)
      decoded = glyphs = 0;
    i += command;
  }
  if (!ended)
    throw std::runtime_error("missing message terminator");
}

int main() {
  size_t checked = 0, failures = 0;
  auto check = [&](const std::string &label, CustomMessage message,
                   CatalogIconType icon) {
    try {
      // Match BuildCustomItemMessage's Spooky Mask placeholder: its raw
      // icon operand is '&', which must survive markup conversion.
      const bool spooky = icon == ITEM_MASK_SPOOKY;
      if (spooky)
        message.AutoFormat(ITEM_CUSTOM);
      else { /* CATALOG_AUTO_FORMAT */
      }
      if (spooky)
        message.Replace(CustomMessage::ITEM_OBTAINED(ITEM_CUSTOM),
                        CustomMessage::ITEM_OBTAINED(icon));
      for (uint8_t language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA}) {
        ++checked;
        try {
          const auto body = message.GetForLanguage(language, MF_RAW);
          if (body.size() < 2 || body[0] != '\x13' ||
              uint8_t(body[1]) != uint8_t(icon))
            throw std::runtime_error("receipt changed its icon operand byte");
          CheckBody(body);
        } catch (const std::exception &error) {
          std::cerr << "FAIL " << label << " language " << int(language) << ": "
                    << error.what() << '\n';
          ++failures;
        }
      }
    } catch (const std::exception &error) {
      std::cerr << "FAIL " << label << ": " << error.what() << '\n';
      ++failures;
    }
  };
  for (const auto &entry : catalog) {
    check(entry.label, CustomMessage(entry.english, entry.german, entry.french),
          entry.icon);
  }
  namespace Vanilla = ComboVanillaItemReceiptText;
  for (const auto &entry : Vanilla::entries) {
    check(entry.name,
          CustomMessage(Vanilla::Body(entry, Vanilla::Language::English),
                        Vanilla::Body(entry, Vanilla::Language::German),
                        Vanilla::Body(entry, Vanilla::Language::French)),
          ITEM_CUSTOM);
  }
  namespace Magic = ComboMagicItemReceiptText;
  for (const char *name : magicNames) {
    for (int rule = 0; rule < 3; ++rule) {
      const auto *magic = Magic::Find(name, rule);
      if (!magic)
        throw std::runtime_error("missing real magic catalog entry");
      CustomMessage message(Magic::Body(*magic, Magic::Language::English),
                            Magic::Body(*magic, Magic::Language::German),
                            Magic::Body(*magic, Magic::Language::French));
      /* MAGIC_AUTO_FORMAT */
      for (uint8_t language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA}) {
        ++checked;
        try {
          CheckBody(message.GetForLanguage(language, MF_RAW));
        } catch (const std::exception &error) {
          std::cerr << "FAIL " << name << " rule " << rule << " language "
                    << int(language) << ": " << error.what() << '\n';
          ++failures;
        }
      }
    }
  }
  size_t iconCases = 0;
  for (uint8_t icon : {0x04, 0x23, 0x26, 0x40, 0x5E, 0x5F}) {
    const std::string text =
        "First page.^Second page with an authored $w icon.";
    CustomMessage message(text, text, text);
    message.AutoFormat(static_cast<ItemID>(icon));
    for (uint8_t language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA}) {
      ++iconCases;
      try {
        const auto body = message.GetForLanguage(language, MF_RAW);
        const auto token = CustomMessage::ITEM_OBTAINED(icon);
        // The first page is short and explicit. Its icon operand can also be
        // 0x04, so use the authored boundary instead of a raw byte search.
        const auto firstPage = token + "First page.";
        const auto page = firstPage.size();
        if (!body.starts_with(firstPage) || body.size() <= page ||
            body[page] != '\x04' || body.substr(page + 1, 2) != token)
          throw std::runtime_error(
              "markup consumed a leading or page icon operand");
        if (body.find(CustomMessage::ITEM_OBTAINED(ITEM_CUSTOM), page + 3) ==
            std::string::npos)
          throw std::runtime_error(
              "formatting changed the authored embedded icon");
        CheckBody(body);
      } catch (const std::exception &error) {
        std::cerr << "FAIL icon operand " << int(icon) << ": " << error.what()
                  << '\n';
        ++failures;
      }
    }
  }
  std::cout << "Pickup catalog formatter: " << checked
            << " locale receipts and " << iconCases
            << " markup-valued icon cases checked, " << failures
            << " failures\n";
  return failures ? 1 : 0;
}
