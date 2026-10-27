#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// Prefer the real OoT get-item message banks. These lessons also remain
// available for MM-first pickups before those banks have been loaded.
namespace ComboVanillaItemReceiptText {
enum class Kind {
    Sword,
    Master,
    Knife,
    Biggoron,
    Shield,
    Mirror,
    Fish,
    Bugs,
    Poe,
    BigPoe,
    Empty,
    RedPotion,
    GreenPotion,
    BluePotion,
    Fairy,
    BlueFire,
    Milk,
    HalfMilk,
    RutoLetter,
    GoldDust,
    HotSpringWater,
    DekuPrincess,
    Seahorse,
    SpringWater,
    ZoraEgg,
    HylianLoach,
    ObabaDrink,
    Chateau,
    MagicMushroom
};
enum class Language { English, German, French };
struct Entry {
    const char* name;
    uint16_t textId;
    const char* englishName;
    const char* germanName;
    const char* frenchName;
    Kind kind;
};
inline constexpr Entry entries[] = {
    { "Kokiri Sword", 0xA4, "the Kokiri Sword", "das Kokiri-Schwert", "l'Épée Kokiri", Kind::Sword },
    { "Master Sword", 0, "the Master Sword", "das Master-Schwert", "l'Épée de Légende", Kind::Master },
    { "Giant's Knife", 0x4B, "the Giant's Knife", "das Langschwert", "la Lame des Géants", Kind::Knife },
    { "Biggoron's Sword", 0x0C, "Biggoron's Sword", "das Biggoron-Schwert", "l'Épée de Biggoron", Kind::Biggoron },
    { "Deku Shield", 0x4C, "a Deku Shield", "einen Deku-Schild", "un Bouclier Mojo", Kind::Shield },
    { "Hylian Shield", 0x4D, "a Hylian Shield", "einen Hylia-Schild", "un Bouclier Hylien", Kind::Shield },
    { "Mirror Shield", 0x4E, "the Mirror Shield", "den Spiegelschild", "le Bouclier Miroir", Kind::Mirror },
    { "Bottle with Fish", 0x47, "a Bottle with Fish", "eine Flasche mit Fisch", "une Bouteille avec un Poisson",
      Kind::Fish },
    { "Bottle with Bugs", 0x7A, "a Bottle with Bugs", "eine Flasche mit Wanzen", "une Bouteille avec des Insectes",
      Kind::Bugs },
    { "Bottle with Poe", 0x97, "a Bottle with Poe", "eine Flasche mit einem Geist", "une Bouteille avec un Esprit",
      Kind::Poe },
    { "Bottle with Big Poe", 0xF9, "a Bottle with Big Poe", "eine Flasche mit Seele", "une Bouteille avec une Âme",
      Kind::BigPoe },
    { "Empty Bottle", 0x42, "an Empty Bottle", "eine leere Flasche", "une Bouteille vide", Kind::Empty },
    { "Bottle with Red Potion", 0x43, "a Bottle with Red Potion", "eine Flasche rotes Elixier",
      "une Bouteille de Potion Rouge", Kind::RedPotion },
    { "Bottle with Green Potion", 0x44, "a Bottle with Green Potion", "eine Flasche grünes Elixier",
      "une Bouteille de Potion Verte", Kind::GreenPotion },
    { "Bottle with Blue Potion", 0x45, "a Bottle with Blue Potion", "eine Flasche blaues Elixier",
      "une Bouteille de Potion Bleue", Kind::BluePotion },
    { "Bottle with Fairy", 0x46, "a Bottle with Fairy", "eine Flasche mit Fee", "une Bouteille avec une Fée",
      Kind::Fairy },
    { "Bottle with Blue Fire", 0x5D, "a Bottle with Blue Fire", "eine Flasche blaues Feuer",
      "une Bouteille de Flamme Bleue", Kind::BlueFire },
    { "Bottle with Milk", 0x98, "a Bottle with Milk", "eine Flasche Milch", "une Bouteille de Lait", Kind::Milk },
    { "Milk (Half)", 0, "a Half Bottle of Milk", "eine halbe Flasche Milch", "une demi-Bouteille de Lait",
      Kind::HalfMilk },
    { "Bottle with Ruto's Letter", 0x99, "a Bottle with Ruto's Letter", "Rutos Brief in einer Flasche",
      "une Bouteille avec la Lettre de Ruto", Kind::RutoLetter },
    { "Bottle With Gold Dust", 0, "a Bottle with Gold Dust", "eine Flasche Goldstaub", "une Bouteille de Poudre d'Or",
      Kind::GoldDust },
    { "Bottle with Hot Spring Water", 0, "Bottled Hot Spring Water", "heißes Quellwasser",
      "une Bouteille d'Eau de Source Chaude", Kind::HotSpringWater },
    { "Bottle with Deku Princess", 0, "the Deku Princess in a Bottle", "die Deku-Prinzessin in einer Flasche",
      "la Princesse Mojo dans une Bouteille", Kind::DekuPrincess },
    { "Bottle with Seahorse", 0, "a Bottled Seahorse", "ein Seepferdchen in einer Flasche",
      "un Hippocampe en Bouteille", Kind::Seahorse },
    { "Bottle with Spring Water", 0, "Bottled Spring Water", "Quellwasser in einer Flasche",
      "une Bouteille d'Eau de Source", Kind::SpringWater },
    { "Bottle with Zora Egg", 0, "a Bottled Zora Egg", "ein Zora-Ei in einer Flasche", "un Oeuf Zora en Bouteille",
      Kind::ZoraEgg },
    { "Bottle with Hylian Loach", 0, "a Bottled Hylian Loach", "eine Hylia-Grundel in einer Flasche",
      "une Loche d'Hyrule en Bouteille", Kind::HylianLoach },
    { "Bottle with Obaba's Drink", 0, "Obaba's Special Drink", "Obabas Spezialgetränk", "la Boisson Spéciale d'Obaba",
      Kind::ObabaDrink },
    { "Bottle of Chateau Romani", 0, "a Bottle of Chateau Romani", "eine Flasche Chateau Romani",
      "une Bouteille de Château Romani", Kind::Chateau },
    { "Bottle with Magic Mushroom", 0, "a Bottle with Magic Mushroom", "eine Flasche mit Zauberpilz",
      "une Bouteille avec un Champignon Magique", Kind::MagicMushroom },
    { "Red Potion Refill", 0x43, "a Red Potion Refill", "rotes Elixier zum Nachfüllen", "une recharge de Potion Rouge",
      Kind::RedPotion },
    { "Green Potion Refill", 0x44, "a Green Potion Refill", "grünes Elixier zum Nachfüllen",
      "une recharge de Potion Verte", Kind::GreenPotion },
    { "Blue Potion Refill", 0x45, "a Blue Potion Refill", "blaues Elixier zum Nachfüllen",
      "une recharge de Potion Bleue", Kind::BluePotion },
    { "Milk Refill", 0x98, "a Milk Refill", "Milch zum Nachfüllen", "une recharge de Lait", Kind::Milk },
    { "Chateau Romani Refill", 0, "a Chateau Romani Refill", "Chateau Romani zum Nachfüllen",
      "une recharge de Château Romani", Kind::Chateau },
    { "Gold Dust Refill", 0, "a Gold Dust Refill", "Goldstaub zum Nachfüllen", "une recharge de Poudre d'Or",
      Kind::GoldDust },
};
inline const Entry* Find(std::string_view name) {
    for (const auto& entry : entries)
        if (name == entry.name)
            return &entry;
    // Exact catalog, refill and captured-content spellings used by both hosts.
    struct Alias {
        const char* name;
        const char* canonical;
    };
    static constexpr Alias aliases[] = {
        { "Bottle of Milk", "Bottle with Milk" },
        { "Milk", "Milk Refill" },
        { "Half Bottle of Milk", "Milk (Half)" },
        { "Bottle with Half Milk", "Milk (Half)" },
        { "Fairy", "Bottle with Fairy" },
        { "Fish", "Bottle with Fish" },
        { "Bugs", "Bottle with Bugs" },
        { "Poe", "Bottle with Poe" },
        { "Big Poe", "Bottle with Big Poe" },
        { "Blue Fire", "Bottle with Blue Fire" },
        { "Ruto's Letter", "Bottle with Ruto's Letter" },
        { "Red Potion", "Red Potion Refill" },
        { "Green Potion", "Green Potion Refill" },
        { "Blue Potion", "Blue Potion Refill" },
        { "Bottle with Gold Dust", "Bottle With Gold Dust" },
        { "Gold Dust", "Gold Dust Refill" },
        { "Bottled Hot Spring Water", "Bottle with Hot Spring Water" },
        { "Hot Spring Water", "Bottle with Hot Spring Water" },
        { "Deku Princess", "Bottle with Deku Princess" },
        { "Bottled Seahorse", "Bottle with Seahorse" },
        { "Seahorse", "Bottle with Seahorse" },
        { "Bottled Spring Water", "Bottle with Spring Water" },
        { "Spring Water", "Bottle with Spring Water" },
        { "Zora Egg", "Bottle with Zora Egg" },
        { "Hylian Loach", "Bottle with Hylian Loach" },
        { "Obaba's Special Drink", "Bottle with Obaba's Drink" },
        { "Obaba's Drink", "Bottle with Obaba's Drink" },
        { "Chateau Romani", "Bottle of Chateau Romani" },
        { "Bottle with Chateau Romani", "Bottle of Chateau Romani" },
        { "Magic Mushroom", "Bottle with Magic Mushroom" },
    };
    for (const auto& alias : aliases)
        if (name == alias.name)
            for (const auto& entry : entries)
                if (std::string_view(alias.canonical) == entry.name)
                    return &entry;
    return nullptr;
}
inline std::string BottleLesson(Kind kind, Language language) {
    struct Lesson {
        Kind kind;
        const char* english;
        const char* german;
        const char* french;
    };
    static constexpr Lesson lessons[] = {
        { Kind::Empty, "Swing the empty bottle to catch&fish, bugs, fairies or other contents.",
          "Schwinge die leere Flasche, um&Fische, Käfer oder Feen zu fangen.",
          "Utilisez la bouteille vide pour&capturer poissons, insectes ou fées." },
        { Kind::RedPotion, "Drink it to restore your hearts.", "Trinke es, um deine Herzen&aufzufüllen.",
          "Buvez-la pour restaurer&votre énergie vitale." },
        { Kind::GreenPotion, "Drink it to restore your magic.", "Trinke es, um deine Magie&aufzufüllen.",
          "Buvez-la pour restaurer&votre magie." },
        { Kind::BluePotion, "Drink it to restore&your hearts and magic.",
          "Trinke es, um deine Herzen&und Magie aufzufüllen.",
          "Buvez-la pour restaurer&votre énergie vitale et votre magie." },
        { Kind::Fairy, "Release it to recover hearts.&A bottled fairy can also revive you&if you fall in battle.",
          "Lass sie frei, um Herzen zu heilen.&Eine Fee in der Flasche belebt dich&auch nach einer Niederlage.",
          "Libérez-la pour récupérer de la vie.&Une fée en bouteille peut aussi&vous ranimer après une défaite." },
        { Kind::BlueFire, "Release blue fire to melt red ice.", "Lass blaues Feuer frei, um&rotes Eis zu schmelzen.",
          "Libérez la flamme bleue pour&faire fondre la glace rouge." },
        { Kind::Milk, "This bottle holds two drinks.&Each drink restores five hearts.",
          "Die Flasche enthält zwei Portionen.&Jede heilt fünf Herzen.",
          "Cette bouteille contient deux doses.&Chaque dose restaure cinq coeurs." },
        { Kind::HalfMilk, "One drink remains in this bottle.&It restores five hearts.",
          "Eine Portion bleibt in der Flasche.&Sie heilt fünf Herzen.",
          "Il reste une dose dans la bouteille.&Elle restaure cinq coeurs." },
        { Kind::RutoLetter, "A letter from Princess Ruto.&Show it to King Zora in Hyrule.",
          "Ein Brief von Prinzessin Ruto.&Zeige ihn König Zora in Hyrule.",
          "Une lettre de la Princesse Ruto.&Montrez-la au Roi Zora en Hyrule." },
        { Kind::GoldDust, "Rare powder prized by smiths.&Keep it for the Mountain Smithy&in Termina.",
          "Seltenes Pulver für Schmiede.&Bewahre es für die Bergschmiede&in Termina auf.",
          "Une poudre précieuse pour les forgerons.&Gardez-la pour la forge&de la montagne en Termina." },
        { Kind::HotSpringWater, "Warm spring water from Termina.&In Hyrule, it melts red ice.",
          "Heißes Quellwasser aus Termina.&In Hyrule schmilzt es rotes Eis.",
          "De l'eau de source chaude de Termina.&En Hyrule, elle fond la glace rouge." },
        { Kind::DekuPrincess, "The Deku King's daughter.&Bring her home to the Deku Palace&in Termina.",
          "Die Tochter des Deku-Königs.&Bring sie zum Deku-Palast&in Termina zurück.",
          "La fille du Roi Mojo.&Ramenez-la au Palais Mojo&en Termina." },
        { Kind::Seahorse, "A Great Bay companion.&It can guide you through Pinnacle Rock&in Termina.",
          "Ein Begleiter aus der Schädelbucht.&Es führt dich am Nadelfelsen&in Termina.",
          "Un compagnon de la Grande Baie.&Il peut vous guider au Pic de Pierre&en Termina." },
        { Kind::SpringWater, "Cool, clear water in a bottle.&Use it to water thirsty plants&in Termina.",
          "Kühles, klares Wasser in der Flasche.&Gieße damit durstige Pflanzen&in Termina.",
          "De l'eau fraîche dans une bouteille.&Arrosez les plantes assoiffées&en Termina." },
        { Kind::ZoraEgg, "A fragile Zora egg.&Keep it safe for the Marine Lab&in Termina.",
          "Ein zerbrechliches Zora-Ei.&Bewahre es für das Meereslabor&in Termina auf.",
          "Un fragile oeuf Zora.&Gardez-le pour le laboratoire marin&en Termina." },
        { Kind::HylianLoach, "A rare fish prized by anglers.&Keep your catch safe in its bottle.",
          "Ein seltener Fisch für Angler.&Bewahre den Fang in der Flasche auf.",
          "Un poisson rare prisé des pêcheurs.&Gardez votre prise dans la bouteille." },
        { Kind::ObabaDrink, "A peculiar Termina brew.&Keep this special drink in its bottle.",
          "Ein besonderes Getränk aus Termina.&Bewahre es in seiner Flasche auf.",
          "Un breuvage spécial de Termina.&Gardez cette boisson en bouteille." },
        { Kind::Chateau, "Drink this special milk to restore&hearts and gain unlimited magic&until its effect expires.",
          "Diese Milch heilt deine Herzen&und gibt dir unbegrenzte Magie,&bis ihre Wirkung endet.",
          "Ce lait restaure votre vie&et donne une magie illimitée&jusqu'à la fin de son effet." },
        { Kind::MagicMushroom, "A fragrant Termina mushroom.&The Mask of Scents can find more.",
          "Ein duftender Termina-Pilz.&Die Geruchsmaske findet weitere.",
          "Un champignon parfumé de Termina.&Le Masque des Odeurs en trouve d'autres." },
    };
    for (const auto& lesson : lessons)
        if (lesson.kind == kind)
            return language == Language::German   ? lesson.german
                   : language == Language::French ? lesson.french
                                                  : lesson.english;
    return {};
}
inline std::string Body(const Entry& entry, Language language = Language::English) {
    const bool german = language == Language::German, french = language == Language::French;
    const bool sword = entry.kind <= Kind::Biggoron;
    const bool shield = entry.kind == Kind::Shield || entry.kind == Kind::Mirror;
    const std::string color = sword ? "%c" : shield ? "%b" : "%g";
    std::string body = (german   ? "Du hast "
                        : french ? "Vous obtenez "
                                 : "You got ") +
                       color +
                       (german   ? entry.germanName
                        : french ? entry.frenchName
                                 : entry.englishName) +
                       "%w!^";
    if (sword || shield) {
        body += german   ? "Wähle es im Ausrüstungs-Menü&und lege es mit %y\x9F%w an.^"
                : french ? "Sélectionnez-le dans l'Équipement&et équipez-le avec %y\x9F%w.^"
                         : "Select it on the Equipment&Subscreen and equip with %y\x9F%w.^";
        body += sword ? (german   ? "%y\xA0%w: Hieb. Halte %y\xA0%w für&eine Wirbelattacke."
                         : french ? "%y\xA0%w: frappez. Maintenez %y\xA0%w&pour une attaque circulaire."
                                  : "Press %y\xA0%w to swing. Hold %y\xA0%w&to charge a Spin Attack.")
                      : (german   ? "Halte %y\xA3%w, um den Schild&zur Abwehr zu heben."
                         : french ? "Maintenez %y\xA3%w pour lever&le bouclier et vous protéger."
                                  : "Hold %y\xA3%w to raise the shield&and defend yourself.");
    } else {
        body += german   ? "Lege die Flasche auf %y\xA1%w.&Drücke die Taste, um sie zu nutzen.^"
                : french ? "Assignez la bouteille à %y\xA1%w.&Appuyez pour l'utiliser.^"
                         : "Assign the bottle to %y\xA1%w.&Press its button to use it.^";
    }
    switch (entry.kind) {
        case Kind::Master:
            body += german   ? "^Die legendäre Klinge&bannt das Böse."
                    : french ? "^Cette lame légendaire&repousse le mal."
                             : "^This legendary blade&repels evil.";
            break;
        case Kind::Knife:
        case Kind::Biggoron:
            body += german   ? "^Die Zweihandklinge verhindert&die gleichzeitige Schildabwehr."
                    : french ? "^Cette lame à deux mains empêche&de lever le bouclier en frappant."
                             : "^This two-handed blade prevents&shield defense while wielded.";
            break;
        case Kind::Mirror:
            body += german   ? "^Seine polierte Fläche reflektiert&Licht und bestimmte Angriffe."
                    : french ? "^Sa surface réfléchit la lumière&et certaines attaques."
                             : "^Its polished surface reflects&light and certain attacks.";
            break;
        case Kind::Fish:
            body += german   ? "Du kannst den Fisch&aus der Flasche freilassen."
                    : french ? "Vous pouvez relâcher le poisson&contenu dans la bouteille."
                             : "You can release the fish&from its bottle.";
            break;
        case Kind::Bugs:
            body += german   ? "Lass die Käfer frei. Sie suchen&nach kleinen Löchern im Boden."
                    : french ? "Relâchez les insectes. Ils cherchent&de petits trous dans le sol."
                             : "Release the bugs. They look for&small holes in the ground.";
            break;
        case Kind::Poe:
            body += german   ? "Trinke den Geist auf eigene Gefahr:&Die Wirkung auf Herzen schwankt!"
                    : french ? "Buvez cet esprit à vos risques:&son effet sur la vie varie!"
                             : "Drink this spirit at your own risk:&its effect on hearts varies!";
            break;
        case Kind::BigPoe:
            body += german   ? "Bring diese Seele zum Geisterladen&am Eingang von Hyrule-Stadt."
                    : french ? "Apportez cette âme au marchand&de fantômes à l'entrée du Bourg."
                             : "Take this soul to the Ghost Shop&at the entrance to Hyrule Market.";
            break;
        default:
            body += BottleLesson(entry.kind, language);
            break;
    }
    return body;
}
} // namespace ComboVanillaItemReceiptText
