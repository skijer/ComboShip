#pragma once

#include <string>
#include <string_view>

// Author the color at the receipt source, including MM-first/standalone awards
// that have no donor textbox. Only the soul name sits inside the color span.
namespace ComboBossSoulReceiptText {
enum class Language { English, German, French };

struct Soul {
    const char* name;
    const char* german;
    const char* french;
    const char* color;
};

inline const Soul* Find(std::string_view name) {
    static constexpr Soul souls[] = {
        { "Gohma's Soul", "Gohmas Seele", "Âme de Gohma", "%g" },
        { "King Dodongo's Soul", "König Dodongos Seele", "Âme du Roi Dodongo", "%r" },
        { "Barinade's Soul", "Barinades Seele", "Âme de Barinade", "%b" },
        { "Phantom Ganon's Soul", "Phantom-Ganons Seele", "Âme de Ganon Spectral", "%g" },
        { "Volvagia's Soul", "Volvagias Seele", "Âme de Vulcania", "%r" },
        { "Morpha's Soul", "Morphas Seele", "Âme de Morpha", "%b" },
        { "Bongo Bongo's Soul", "Bongo Bongos Seele", "Âme de Bongo Bongo", "%p" },
        { "Twinrova's Soul", "Twinrovas Seele", "Âme du Duo Maléfique", "%y" },
        { "Ganon's Soul", "Ganons Seele", "Âme de Ganon", "%r" },
        // Green is the existing marker consumed by ComboBossSoulSpanColor in
        // both renderers. Keep these five exact soul-flame RGB overrides.
        { "Soul of Odolwa", "Seele von Odolwa", "Âme de Odolwa", "%g" },
        { "Soul of Goht", "Seele von Goht", "Âme de Goht", "%g" },
        { "Soul of Gyorg", "Seele von Gyorg", "Âme de Gyorg", "%g" },
        { "Soul of Twinmold", "Seele von Twinmold", "Âme de Twinmold", "%g" },
        { "Soul of Majora", "Seele von Majora", "Âme de Majora", "%g" },
    };
    for (const auto& soul : souls)
        if (name == soul.name)
            return &soul;
    return nullptr;
}

inline std::string Body(const Soul& soul, Language language = Language::English) {
    const bool mm = std::string_view(soul.name).substr(0, 8) == "Soul of ";
    std::string body;
    if (language == Language::German)
        body = mm ? "Du hast die " : "Du hast ";
    else if (language == Language::French)
        body = "Vous avez trouvé l'";
    else
        body = mm ? "You found the " : "You found ";
    body += soul.color;
    body += language == Language::German ? soul.german : language == Language::French ? soul.french : soul.name;
    body += language == Language::German ? "%w gefunden!" : "%w!";
    return body;
}
} // namespace ComboBossSoulReceiptText
