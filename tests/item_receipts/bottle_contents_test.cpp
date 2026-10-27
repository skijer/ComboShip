#include "combo/menu/ComboVanillaItemReceiptText.h"
#include "soh/mods/items/custom_bottles.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace ComboVanillaItemReceiptText;
int main() {
    struct Content { BottleContent id; const char* name; const char* lesson; };
    const Content contents[] = {
        {BOTTLE_C_RUTO_LETTER,"Bottle with Ruto's Letter","King Zora"},
        {BOTTLE_C_BIG_POE,"Bottle with Big Poe","Ghost Shop"},
        {BOTTLE_C_BLUE_FIRE,"Bottle with Blue Fire","red ice"},
        {BOTTLE_C_BLUE_POTION,"Bottle with Blue Potion","hearts and magic"},
        {BOTTLE_C_RED_POTION,"Bottle with Red Potion","hearts"},
        {BOTTLE_C_GREEN_POTION,"Bottle with Green Potion","magic"},
        {BOTTLE_C_FAIRY,"Bottle with Fairy","revive"},
        {BOTTLE_C_FISH,"Bottle with Fish","release"},
        {BOTTLE_C_BUG,"Bottle with Bugs","holes"},
        {BOTTLE_C_POE,"Bottle with Poe","risk"},
        {BOTTLE_C_MILK,"Bottle with Milk","two"},
        {BOTTLE_C_GOLD_DUST,"Bottle With Gold Dust","smith"},
        {BOTTLE_C_HOT_SPRING_WATER,"Bottle with Hot Spring Water","spring water"},
        {BOTTLE_C_DEKU_PRINCESS,"Bottle with Deku Princess","Deku King's"},
        {BOTTLE_C_SEAHORSE,"Bottle with Seahorse","Great Bay"},
        {BOTTLE_C_SPRING_WATER,"Bottle with Spring Water","water"},
        {BOTTLE_C_ZORA_EGG,"Bottle with Zora Egg","egg"},
        {BOTTLE_C_HYLIAN_LOACH,"Bottle with Hylian Loach","rare fish"},
        {BOTTLE_C_OBABA_DRINK,"Bottle with Obaba's Drink","brew"},
        {BOTTLE_C_CHATEAU,"Bottle of Chateau Romani","magic"},
        {BOTTLE_C_MAGIC_MUSHROOM,"Bottle with Magic Mushroom","mushroom"},
    };
    std::array<bool,BOTTLE_C_COUNT> covered{};
    for (const auto& content : contents) {
        assert(!covered[content.id]); covered[content.id]=true;
        const auto* entry=Find(content.name);
        if (!entry) std::fprintf(stderr,"Missing bottle lesson: %s\n",content.name);
        assert(entry && "every bottle content needs a specific lesson");
        assert(Body(*entry).find(content.lesson)!=std::string::npos);
        for (auto locale : {Language::English,Language::German,Language::French}) {
            const auto body=Body(*entry,locale);
            assert(body.size()<1269 && body.find('\xA1')!=std::string::npos);
            const auto page=body.rfind('^');
            assert(page!=std::string::npos && body.size()-page>15);
            assert(body.find("You found")==std::string::npos);
        }
    }
    for (bool present : covered) assert(present && "new bottle enum lacks a receipt fixture");
    for (const char* name : {"Empty Bottle","Milk (Half)","Bottle of Milk","Milk Refill", "Fairy",
        "Red Potion Refill","Green Potion Refill","Blue Potion Refill","Chateau Romani Refill",
        "Gold Dust Refill","Bottled Hot Spring Water","Deku Princess","Bottled Seahorse",
        "Bottled Spring Water","Zora Egg","Hylian Loach","Obaba's Special Drink"}) {
        assert(Find(name) && "bottle/refill alias lacks a receipt");
        assert(Body(*Find(name)).find('\xA1')!=std::string::npos);
    }
    assert(!Find("Odd Potion") && !Find("Weird Egg"));
    puts("PASS all 21 bottle contents, empty/half milk/refill aliases and three localized lessons");
}
