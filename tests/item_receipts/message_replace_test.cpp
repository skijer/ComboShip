#include "combo/menu/ComboVanillaItemReceiptText.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

enum { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA, LANGUAGE_JPN, LANGUAGE_MAX };
constexpr const char *TODO_TRANSLATE = "TODO_TRANSLATE";

// Retain the real overloads and their implicit localized-message conversion.
// The engine/font are unnecessary for exercising the production replacements.
struct CustomMessage {
  std::vector<std::string> messages;
  CustomMessage(std::string english, std::string german = TODO_TRANSLATE,
                std::string french = TODO_TRANSLATE)
      : messages{std::move(english), std::move(german), std::move(french)} {}
  /* REPLACE_DECLARATIONS */
};

/* PRODUCTION_REPLACE */

int main() {
  using namespace ComboVanillaItemReceiptText;
  const auto *receipt = Find("Master Sword");
  assert(receipt);
  const auto english = Body(*receipt);
  const auto german = Body(*receipt, Language::German);
  const auto french = Body(*receipt, Language::French);
  CustomMessage master(english, german, french);
  std::string receiptName = "Master Sword";
  std::string name = "Master Sword (MM)";
  // The exact lvalue call in BuildComboForeignMessage selects the localized
  // overload. Re-scanning its inserted label used to hang before the grant.
  master.Replace(receiptName.c_str(), name);
  assert(master.messages[LANGUAGE_ENG].find(name) != std::string::npos);
  assert(master.messages[LANGUAGE_ENG].size() == english.size() + 5);
  assert(master.messages[LANGUAGE_ENG].find("Equipment") != std::string::npos);
  assert(master.messages[LANGUAGE_GER] == german);
  assert(master.messages[LANGUAGE_FRA] == french);

  CustomMessage identical("Master Sword", "Master-Schwert", "Épée de Légende");
  identical.Replace("Master Sword", CustomMessage("Master Sword"));
  assert(identical.messages[0] == "Master Sword");

  CustomMessage localized("item item", "item item", "item item");
  localized.Replace("item",
                    CustomMessage("item (MM)", "Objet item", "item item"));
  assert(localized.messages[0] == "item (MM) item (MM)");
  assert(localized.messages[1] == "Objet item Objet item");
  assert(localized.messages[2] == "item item item item");

  CustomMessage fallback("x x", "x x", "x x");
  fallback.Replace("x", CustomMessage("x (MM)"));
  for (const auto &text : fallback.messages)
    assert(text == "x (MM) x (MM)");

  CustomMessage growing("aa aa", "aa aa", "aa aa");
  growing.Replace("aa", std::string("aaaa"));
  for (const auto &text : growing.messages)
    assert(text == "aaaa aaaa");

  CustomMessage adjacent("xxx", "xxx", "xxx");
  adjacent.Replace("x", std::string{});
  for (const auto &text : adjacent.messages)
    assert(text.empty());
  CustomMessage localizedAdjacent("xxx", "xxx", "xxx");
  localizedAdjacent.Replace("x", CustomMessage("", "", ""));
  for (const auto &text : localizedAdjacent.messages)
    assert(text.empty());

  CustomMessage emptySearch("keep", "behalten", "garder");
  const auto before = emptySearch.messages;
  emptySearch.Replace("", std::string("insert"));
  emptySearch.Replace("", CustomMessage("insert"));
  assert(emptySearch.messages == before);

  std::cout << "PASS foreign Master Sword receipt and bounded plain/localized "
               "replacements, "
               "fallbacks, adjacent deletions and empty searches\n";
}
