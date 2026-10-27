#pragma once
#include <string>
#include <string_view>
#include <utility>

// SoH's randomizer key receipt uses the named catalog identity, article and
// dungeon color. Keys have no count, layout hint or additional tutorial page.
namespace ComboKeyReceiptText {
inline constexpr const char* kEnglish = "You found [[article]][[color]][[name]]%w!";
inline constexpr const char* kGerman = "Du hast [[article]][[color]][[name]]%w gefunden!";
inline constexpr const char* kFrench = "Vous avez trouvé [[article]][[color]][[name]]%w!";

inline std::string English(std::string_view name, std::string_view article, std::string_view color) {
    std::string text = kEnglish;
    for (const auto& replacement :
         { std::pair{ std::string_view("[[article]]"), article }, std::pair{ std::string_view("[[color]]"), color },
           std::pair{ std::string_view("[[name]]"), name } }) {
        text.replace(text.find(replacement.first), replacement.first.size(), replacement.second);
    }
    return text;
}
} // namespace ComboKeyReceiptText
