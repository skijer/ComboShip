#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include "2s2h/CustomMessage/CustomMessage.h"
#include "sword_element_message.h"
#if __has_include("ComboCapeReceiptChoice.h")
#include "ComboCapeReceiptChoice.h"
#endif

extern "C" {
PlayState* gPlayState;
SaveContext gSaveContext;
float sNESFontWidths[160];
// This fixture exercises cape receipts with optional sword enhancements off.
int32_t CVarGetInteger(const char*, int32_t fallback) {
    return fallback;
}
void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation*) {
}
void Message_SetCapeVisibilityChoice(int) {
}
}

/* PRODUCTION_FORMATTERS */

template <typename Entry> void RequestCapeChoice(Entry& entry) {
    if constexpr (requires { entry.capeVisibilityChoice; })
        entry.capeVisibilityChoice = true;
}

static std::string LoadedBody() {
    return std::string(reinterpret_cast<char*>(gPlayState->msgCtx.font.msgBuf.schar + MESSAGE_HEADER_SIZE),
                       gPlayState->msgCtx.msgLength - MESSAGE_HEADER_SIZE);
}

int main(int argc, char** argv) {
    static PlayState play{};
    gPlayState = &play;
    CustomMessage::Entry cape;
    cape.textboxType = 2;
    cape.icon = 0xF5;
    cape.autoFormat = false;
    cape.msg = "You found the Magic Cape!\020Magic costs less.\020(Bank reward)\277";
    RequestCapeChoice(cape);
    CustomMessage::LoadCustomMessageIntoFont(cape);
    const std::string body = LoadedBody();
    const auto choice = body.find('\302');
    assert(choice != std::string::npos && "cape pickup has no visibility choice");
    assert(body.find("(Bank reward)") < choice && "attribution displaced the choice page");
    assert(body.find("Show cape", choice) != std::string::npos);
    assert(body.find("Hide cape", choice) != std::string::npos);
    assert(body.find('\277') == body.size() - 1 && "an early END hides the choice");
    assert(play.msgCtx.font.msgBuf.schar[2] == 0xF5);
    assert(cape.msg.back() == static_cast<char>(0xBF));
    // Loading a second time must not add another choice to the stored receipt.
    CustomMessage::LoadCustomMessageIntoFont(cape);
    assert(LoadedBody() == body);
    CustomMessage::Entry ordinary;
    ordinary.autoFormat = false;
    ordinary.msg = "You found a Bow!\277";
    CustomMessage::LoadCustomMessageIntoFont(ordinary);
    assert(LoadedBody() == ordinary.msg && "cape prompt leaked into an unrelated receipt");
    // The auto-formatted fallback receipt also needs the prompt after its END.
    cape.autoFormat = true;
    cape.msg = "You found the Magic Cape!";
    CustomMessage::LoadCustomMessageIntoFont(cape);
    assert(LoadedBody().find('\302') != std::string::npos);
    assert(LoadedBody().find('\277') == LoadedBody().size() - 1);
    // Optional inputs are the actual fallback bytes emitted by CheckQueue.
    for (int i = 1; i < argc; ++i) {
        std::ifstream input(argv[i], std::ios::binary);
        assert(input);
        cape.msg.assign(std::istreambuf_iterator<char>(input), {});
        CustomMessage::LoadCustomMessageIntoFont(cape);
        const auto fallback = LoadedBody();
        assert(fallback.find('\302') != std::string::npos);
        assert(fallback.find('\034') == std::string::npos && "FADE hides the cape choice");
        assert(fallback.find('\277') == fallback.size() - 1);
    }
    std::cout << "PASS MM cape receipt, attribution, icon, repeated loading and ordinary receipts\n";
}
