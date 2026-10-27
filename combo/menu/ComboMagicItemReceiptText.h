#pragma once

#include <string>
#include <string_view>

// Shared pickup explanations work in either host, including MM-first seeds.
// Match the concrete catalog identity, never the currently selected power.
namespace ComboMagicItemReceiptText {
enum class Language { English, German, French };
enum class Guide { None, Medallions, All };
struct Entry {
    const char* name;
    bool slate;
    const char* english;
    const char* german;
    const char* french;
    Guide guide = Guide::None;
};

inline const Entry* Find(std::string_view name, int wandRule = 0) {
    if (name == "Elemental Wand") {
        // Existing Wand_RandoMode values: medallions, single item, individual rods.
        static const Entry universal[] = {
            { "Elemental Wand", false,
              "You got the %cElemental Wand%w!&Your %ymedallions%w awaken matching&rod powers. More medallion "
              "rewards&unlock more elements.",
              "Du hast den %cElementarstab%w!&Deine %yMedaillons%w erwecken die&passenden Stabkräfte. "
              "Weitere&Medaillons öffnen neue Elemente.",
              "Vous obtenez la %cBaguette Élémentaire%w!&Vos %ymédaillons%w éveillent les&pouvoirs associés. Trouvez "
              "d'autres&médaillons pour ouvrir les éléments.",
              Guide::Medallions },
            { "Elemental Wand", false,
              "You got the %cElemental Wand%w!&%yAll six%w rod powers are ready!&Sand, wind, water, meteors,&lightning "
              "and shadow!",
              "Du hast den %cElementarstab%w!&%yAlle sechs%w Stabkräfte sind bereit!&Ein Stab vereint Sand, "
              "Wind,&Wasser, Meteore und mehr.",
              "Vous obtenez la %cBaguette Élémentaire%w!&Les %ysix pouvoirs%w sont disponibles!&Un bâton réunit sable, "
              "vent,&eau, météores et plus encore.",
              Guide::All },
            { "Elemental Wand", false,
              "You got the %cElemental Wand%w!&Each rod is found %yseparately%w.&Only the powers you have found&can be "
              "selected.",
              "Du hast den %cElementarstab%w!&Jeder Stab wird %yeinzeln%w gefunden.&Du kannst nur die Kräfte "
              "wählen,&die du bereits gefunden hast.",
              "Vous obtenez la %cBaguette Élémentaire%w!&Chaque bâton se trouve %yséparément%w.&Seuls les pouvoirs "
              "déjà trouvés&peuvent être sélectionnés." },
        };
        return &universal[wandRule == 1 ? 1 : wandRule == 2 ? 2 : 0];
    }
    static const Entry entries[] = {
        { "Sand Rod", false,
          "You got the %ySand Rod%w!&Create a %ytemporary platform%w&in front of you, even in midair.&Keep moving; the "
          "sand crumbles!",
          "Du hast den %ySandstab%w!&Erschaffe eine %ySandplattform%w&vor dir, sogar in der Luft.&Bleib in Bewegung; "
          "sie zerfällt!",
          "Vous obtenez le %yBâton de Sable%w!&Créez une %yplateforme temporaire%w&devant vous, même dans les "
          "airs.&Avancez vite; le sable s'effrite!" },
        { "Tornado Rod", false,
          "You got the %gTornado Rod%w!&Surround yourself with %gwind%w&to boost jumps. Hold %y\xA1%w to rise.&Active "
          "wind steadily uses magic.",
          "Du hast den %gTornadostab%w!&%gWind%w verstärkt deine Sprünge.&Halte %y\xA1%w, um aufzusteigen.&Der Wind "
          "verbraucht stetig Magie.",
          "Vous obtenez le %gBâton Tornade%w!&Le %gvent%w renforce vos sauts.&Maintenez %y\xA1%w pour monter.&Le vent "
          "consomme de la magie." },
        { "Water Rod", false,
          "You got the %bWater Rod%w!&Summon a rideable %bwater column%w.&Cast again to raise or lower it.&Ride the "
          "spout to reach higher.",
          "Du hast den %bWasserstab%w!&Rufe eine tragende %bWassersäule%w.&Wirke erneut zum Heben oder Senken.&Reite "
          "die Fontäne in die Höhe.",
          "Vous obtenez le %bBâton d'Eau%w!&Invoquez une %bcolonne d'eau%w.&Relancez pour la lever ou la "
          "baisser.&Montez dessus pour gagner en hauteur." },
        { "Meteor Rod", false,
          "You got the %rMeteor Rod%w!&Launch a bouncing %rexplosive%w&that bursts after a short fuse.&Aim it toward "
          "enemies!",
          "Du hast den %rMeteorstab%w!&Schleudere eine hüpfende %rBombe%w,&die nach kurzer Zeit explodiert.&Ziele "
          "damit auf deine Feinde!",
          "Vous obtenez le %rBâton Météore%w!&Lancez un %rexplosif%w rebondissant&qui éclate après un court "
          "délai.&Visez vos ennemis!" },
        { "Storm Rod", false,
          "You got the %cStorm Rod%w!&Wield %clightning%w against foes.&Lock on and cast to send a&thunderbolt toward "
          "your target.",
          "Du hast den %cSturmstab%w!&Nutze %cBlitze%w gegen deine Feinde.&Erfasse ein Ziel und wirke,&um einen Blitz "
          "darauf zu senden.",
          "Vous obtenez le %cBâton d'Orage%w!&Frappez vos ennemis avec la %cfoudre%w.&Verrouillez une cible et "
          "lancez&un éclair dans sa direction." },
        { "Shadow Scepter", false,
          "You got the %pShadow Scepter%w!&Send a %pshadow bolt%w after&a nearby enemy to %pstun%w it.&An opening for "
          "your next attack!",
          "Du hast das %pSchattenzepter%w!&Ein %pSchattenblitz%w verfolgt einen&nahen Feind und %pbetäubt%w ihn.&Nutze "
          "die Chance zum Angriff!",
          "Vous obtenez le %pSceptre d'Ombre%w!&Un %ptrait d'ombre%w poursuit un&ennemi proche pour "
          "l'%pétourdir%w.&Profitez-en pour attaquer!" },
        { "Sheikah Slate", true,
          "You got the %cSheikah Slate%w!&An ancient tablet that channels&the %crunes%w you discover.&Find runes to "
          "unlock new powers.",
          "Du hast den %cSheikah-Stein%w!&Eine uralte Tafel nutzt die&%cModule%w, die du entdeckst.&Finde Module für "
          "neue Kräfte.",
          "Vous obtenez la %cTablette Sheikah%w!&Cette tablette utilise les&%crunes%w que vous découvrez.&Trouvez-en "
          "pour ouvrir des pouvoirs." },
        { "Rune: Remote Bomb", true,
          "You learned %cRemote Bomb%w!&Summon a %cremote bomb%w.&Throw it, then cast again to&%cdetonate%w it when "
          "you choose.",
          "Deine Tafel lernt %cFernzündbombe%w!&Erschaffe eine %cBombe%w.&Wirf sie und wirke erneut,&um sie selbst zu "
          "%czünden%w.",
          "Votre tablette apprend %cBombe à Distance%w!&Invoquez une %cbombe%w.&Lancez-la, puis relancez la rune&pour "
          "la %cfaire exploser%w à volonté." },
        { "Rune: Stasis", true,
          "You learned %yStasis%w!&%yFreeze%w an enemy or object.&Hits store damage on enemies&or launch force on "
          "objects.",
          "Deine Tafel lernt %yStasis%w!&%yFriert%w einen Feind oder ein Objekt.&Treffer speichern Feindschaden&oder "
          "Schleuderkraft für Objekte.",
          "Votre tablette apprend %yCinetis%w!&%yFigez%w un ennemi ou un objet.&Les coups accumulent des dégâts&ou une "
          "force de lancement." },
        { "Rune: Cryonis", true,
          "You learned %bCryonis%w!&Raise a climbable %bice pillar%w&from water. %y\x9F%w places it;&%y\xA0%w cancels.",
          "Deine Tafel lernt %bCryonis%w!&Eine kletterbare %bEissäule%w&steigt aus Wasser. %y\x9F%w "
          "platziert;&%y\xA0%w bricht ab.",
          "Votre tablette apprend %bCryonis%w!&Érigez un %bpilier de glace%w&sur l'eau. %y\x9F%w le place;&%y\xA0%w "
          "annule." },
        { "Rune: Master Cycle", true,
          "You learned %gMaster Cycle%w!&Summon a rideable %gmotorcycle%w.&%y\x9F%w accelerates; %y\xA0%w "
          "brakes.&Stopped, stick centered: %y\xA0%w exits.",
          "Deine Tafel lernt das %gEponator-Modul%w!&Rufe ein fahrbares %gMotorrad%w.&%y\x9F%w beschleunigt; %y\xA0%w "
          "bremst.&Im Stand, Stick los: %y\xA0%w steigt ab.",
          "Votre tablette apprend %gMaster Cycle%w!&Invoquez une %gmoto%w.&%y\x9F%w accélère; %y\xA0%w freine.&À "
          "l'arrêt, stick centré: %y\xA0%w descend." },
        { "Rune: Sheikah Sensor", true,
          "You learned %pSheikah Sensor%w!&Find a %pdesired item's location%w.&Each accepted hint costs one&%rHeart "
          "Container%w, permanently.",
          "Deine Tafel lernt den %pSheikah-Sensor%w!&Finde ein %pWunsch-Item%w.&Jeder bestätigte Hinweis kostet&einen "
          "%rHerzcontainer%w, dauerhaft.",
          "Votre tablette apprend %pCapteur Sheikah%w!&Localisez un %pobjet désiré%w.&Chaque indice accepté coûte "
          "un&%rréceptacle de coeur%w, pour toujours." },
    };
    for (const auto& entry : entries)
        if (name == entry.name)
            return &entry;
    return nullptr;
}

inline std::string PowerGuide(Guide guide, Language language) {
    if (guide == Guide::None)
        return {};
    const bool medallions = guide == Guide::Medallions;
    if (language == Language::German) {
        return std::string("^%y") + (medallions ? "Geist: Sandstab" : "Sandstab") +
               "%w&Schafft eine kurze Sandplattform.&%g" + (medallions ? "Wald: Tornadostab" : "Tornadostab") +
               "%w&Halte %y\xA1%w: Aufwind braucht Magie.^%b" + (medallions ? "Wasser: Wasserstab" : "Wasserstab") +
               "%w&Wirke zum Heben/Senken der Fontäne.&%r" + (medallions ? "Feuer: Meteorstab" : "Meteorstab") +
               "%w&Wirft eine hüpfende Bombe.^%c" + (medallions ? "Licht: Sturmstab" : "Sturmstab") +
               "%w&Ziel erfassen, dann Blitze wirken.&%p" +
               (medallions ? "Schatten: Schattenzepter" : "Schattenzepter") + "%w&Verfolgt und betäubt einen Feind.";
    }
    if (language == Language::French) {
        return std::string("^%y") + (medallions ? "Esprit: Bâton de Sable" : "Bâton de Sable") +
               "%w&Crée une plateforme temporaire.&%g" + (medallions ? "Forêt: Bâton Tornade" : "Bâton Tornade") +
               "%w&Maintenez %y\xA1%w: vol contre magie.^%b" + (medallions ? "Eau: Bâton d'Eau" : "Bâton d'Eau") +
               "%w&Relancez: monte/baisse la colonne.&%r" + (medallions ? "Feu: Bâton Météore" : "Bâton Météore") +
               "%w&Lance un explosif rebondissant.^%c" + (medallions ? "Lumière: Bâton d'Orage" : "Bâton d'Orage") +
               "%w&Verrouillez et lancez la foudre.&%p" + (medallions ? "Ombre: Sceptre d'Ombre" : "Sceptre d'Ombre") +
               "%w&Poursuit et étourdit un ennemi.";
    }
    return std::string("^%y") + (medallions ? "Spirit: Sand Rod" : "Sand Rod") +
           "%w&Make a temporary sand platform.&%g" + (medallions ? "Forest: Tornado Rod" : "Tornado Rod") +
           "%w&Hold %y\xA1%w to rise on magic wind.^%b" + (medallions ? "Water: Water Rod" : "Water Rod") +
           "%w&Cast to raise/lower its water spout.&%r" + (medallions ? "Fire: Meteor Rod" : "Meteor Rod") +
           "%w&Throw a bouncing explosive.^%c" + (medallions ? "Light: Storm Rod" : "Storm Rod") +
           "%w&Lock on, then cast lightning.&%p" + (medallions ? "Shadow: Shadow Scepter" : "Shadow Scepter") +
           "%w&Seek a nearby foe and stun it.";
}

inline std::string Body(const Entry& entry, Language language = Language::English) {
    std::string body = language == Language::German   ? entry.german
                       : language == Language::French ? entry.french
                                                      : entry.english;
    body += PowerGuide(entry.guide, language);
    if (language == Language::German) {
        body += entry.slate ? "^Auf %y\xA1%w: ziehen, dann wirken.&Halte %y\xA2%w für die Module.&In Pause öffnet "
                              "%y\x9F%w das Modul-Rad;&der Stick wählt die Kraft."
                            : "^Auf %y\xA1%w: ziehen, dann wirken.&Halte %y\xA2%w für die Stäbe.&In Pause öffnet "
                              "%y\x9F%w das Stab-Rad;&der Stick wählt die Kraft.";
    } else if (language == Language::French) {
        body += entry.slate ? "^Sur %y\xA1%w: sortir, puis lancer.&Maintenez %y\xA2%w pour les runes.&En pause, "
                              "%y\x9F%w ouvre la roue;&le stick choisit la rune."
                            : "^Sur %y\xA1%w: sortir, puis lancer.&Maintenez %y\xA2%w pour les bâtons.&En pause, "
                              "%y\x9F%w ouvre la roue;&le stick choisit le pouvoir.";
    } else {
        body += entry.slate ? "^Equip to %y\xA1%w: draw, then cast.&Hold %y\xA2%w to choose a rune.&In pause, %y\x9F%w "
                              "opens the rune wheel;&move the stick to pick a power."
                            : "^Equip to %y\xA1%w: draw, then cast.&Hold %y\xA2%w to choose a rod.&In pause, %y\x9F%w "
                              "opens the rod wheel;&move the stick to pick a power.";
    }
    return body;
}

struct Medallion {
    const char* name;
    const char* power;
    char color;
    const char* german;
    const char* french;
};

inline const Medallion* FindMedallion(std::string_view name, int wandRule = 0) {
    if (wandRule != 0)
        return nullptr;
    static const Medallion medallions[] = {
        { "Spirit Medallion", "Sand Rod", 'y', "Geist-Medaillon", "Médaillon de l'Esprit" },
        { "Forest Medallion", "Tornado Rod", 'g', "Wald-Medaillon", "Médaillon de la Forêt" },
        { "Water Medallion", "Water Rod", 'b', "Wasser-Medaillon", "Médaillon de l'Eau" },
        { "Fire Medallion", "Meteor Rod", 'r', "Feuer-Medaillon", "Médaillon du Feu" },
        { "Light Medallion", "Storm Rod", 'c', "Licht-Medaillon", "Médaillon de la Lumière" },
        { "Shadow Medallion", "Shadow Scepter", 'p', "Schatten-Medaillon", "Médaillon de l'Ombre" },
    };
    for (const auto& medallion : medallions)
        if (name == medallion.name)
            return &medallion;
    return nullptr;
}

inline std::string MedallionSuffix(const Medallion& medallion, Language language = Language::English) {
    // Explain the unlocked power without claiming that the shared wand slot was granted.
    auto body = Body(*Find(medallion.power), language);
    body.erase(0, body.find('%'));
    const char* intro = language == Language::German ? "Dieses Medaillon erweckt eine&Kraft für den %cElementarstab%w:^"
                        : language == Language::French
                            ? "Ce médaillon éveille un pouvoir&de la %cBaguette Élémentaire%w:^"
                            : "This medallion awakens a power&for your %cElemental Wand%w:^";
    return intro + body;
}

inline std::string MedallionReceipt(const Medallion& medallion, Language language = Language::English) {
    std::string prefix = language == Language::German   ? "Du hast das "
                         : language == Language::French ? "Vous obtenez le "
                                                        : "You got the ";
    const char* name = language == Language::German   ? medallion.german
                       : language == Language::French ? medallion.french
                                                      : medallion.name;
    return prefix + '%' + medallion.color + name + "%w!^" + MedallionSuffix(medallion, language);
}
} // namespace ComboMagicItemReceiptText
