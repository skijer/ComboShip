#ifndef COMBO_CAPE_RECEIPT_CHOICE_H
#define COMBO_CAPE_RECEIPT_CHOICE_H

#include <string>
#include <string_view>

namespace ComboCapeReceiptChoice {
enum class Language { English, German, French };

inline bool IsCape(const char* name) {
    return name && std::string_view(name) == "Magic Cape";
}

// Append after the complete description and any check attribution. Keep the
// choice on its own final page, outside either engine's automatic word wrap.
inline std::string Append(std::string body, bool mm, Language language) {
    const char* title = "Magic Cape appearance:";
    const char* show = "Show cape";
    const char* hide = "Hide cape";
    if (language == Language::German) {
        title = "Magischer Umhang:";
        show = "Umhang zeigen";
        hide = "Umhang verbergen";
    } else if (language == Language::French) {
        title = "Apparence de la Cape:";
        show = "Afficher la cape";
        hide = "Masquer la cape";
    }
    const char end = mm ? '\277' : '\002';
    if (!body.empty() && body.back() == end)
        body.pop_back();
    if (mm) {
        body.append("\020\000", 2);
        body += title;
        body += "\002\021\302";
        body += show;
        body += '\021';
        body += hide;
        body.append("\000\277", 2);
    } else {
        body += '\004';
        body += title;
        body += "\033\005\102\001\001";
        body += show;
        body += '\001';
        body += hide;
        body += "\005\100\002";
    }
    return body;
}
} // namespace ComboCapeReceiptChoice
#endif
