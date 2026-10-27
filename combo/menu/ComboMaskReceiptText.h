#pragma once

#include <string_view>

// OoT mask descriptions and form tutorials for permanent randomizer grants.
// Gerudo and Keaton controls match the current fighters in both hosts.
// Host-independent text also works before the donor game has been started.
namespace ComboMaskReceiptText {
struct Entry {
    const char* name;
    const char* english;
    const char* german;
    const char* french;
};

inline constexpr Entry kSkull = {
    "Skull Mask",
    "You got a %rSkull Mask%w!&Wear it with %y\xA1%w to show it off!^"
    "You feel like a monster while&you wear this mask!",
    "Du hast die %rSchädelmaske%w!&Trage sie mit %y\xA1%w und zeige sie!^"
    "Mit dieser Maske fühlst du&dich wie ein Monster!",
    "Vous obtenez le %rMasque de Mort%w!&Portez-le avec %y\xA1%w pour le montrer!^"
    "Avec ce masque, vous vous&sentez comme un monstre!",
};

inline constexpr Entry kSpooky = {
    "Spooky Mask",
    "You got a %rSpooky Mask%w!&Wear it with %y\xA1%w to show it off!^"
    "You can scare many people&with this mask!",
    "Du hast die %rGeistermaske%w!&Trage sie mit %y\xA1%w und zeige sie!^"
    "Mit dieser Maske kannst du&viele Leute erschrecken!",
    "Vous obtenez le %rMasque d'Effroi%w!&Portez-le avec %y\xA1%w pour le montrer!^"
    "Ce masque vous permet de&faire peur à bien des gens!",
};

inline constexpr Entry kTruth = {
    "Mask of Truth",
    "You got the %rMask of Truth%w!&Wear it with %y\xA1%w to show it off!^"
    "Show it to many people!",
    "Du hast die %rMaske der Wahrheit%w!&Trage sie mit %y\xA1%w und zeige sie!^"
    "Zeige sie vielen Leuten!",
    "Vous obtenez le %rMasque de Vérité%w!&Portez-le avec %y\xA1%w pour le montrer!^"
    "Montrez-le à beaucoup de gens!",
};

inline constexpr Entry kGerudo = {
    "Gerudo Mask",
    "You got the %rGerudo Mask%w!&Equip to %y\xA1%w or %yD-Pad%w.^"
    "Use it to become a&%rGerudo warrior%w.&Use it again to return.^"
    "%y\xA0%w draws your twin swords.&Press %y\xA0%w to chain slashes.&Forward + %y\xA0%w: lunging slash.^"
    "With swords drawn, hold %y\x9F%w&while moving to %csprint%w.&In the air, %y\x9F%w: %caerial slash%w.^"
    "Hits build %rrage%w.&%y\xA3%w + %y\xA0%w: %rUrbosa's Fury%w.&More rage means a stronger blast!",
    "Du hast die %rGerudo-Maske%w!&Lege sie auf %y\xA1%w oder %yD-Pad%w.^"
    "Nutze sie für die Form einer&%rGerudo-Kriegerin%w.&Erneut nutzen: zurückverwandeln.^"
    "%y\xA0%w zieht beide Schwerter.&%y\xA0%w: Hiebe aneinanderreihen.&Vorwärts + %y\xA0%w: Sprunghieb.^"
    "Mit gezogenen Schwertern:&Halte beim Laufen %y\x9F%w: %cSprint%w.&In der Luft: %y\x9F%w für %cLufthieb%w.^"
    "Treffer sammeln %rZorn%w.&%y\xA3%w + %y\xA0%w: %rUrbosas Zorn%w.&Mehr Zorn verstärkt den Schlag!",
    "Vous obtenez le %rMasque Gerudo%w!&Assignez-le à %y\xA1%w ou au %yD-Pad%w.^"
    "Utilisez-le pour devenir une&%rguerrière Gerudo%w.&Réutilisez-le pour revenir.^"
    "%y\xA0%w dégaine vos deux sabres.&%y\xA0%w: enchaînez les coups.&Avant + %y\xA0%w: attaque bondissante.^"
    "Sabres en main, maintenez %y\x9F%w&en mouvement pour %csprinter%w.&En l'air: %y\x9F%w pour %cfrapper%w.^"
    "Les coups gagnent de la %rrage%w.&%y\xA3%w + %y\xA0%w: %rColère d'Urbosa%w.&Plus de rage renforce l'attaque!",
};

inline constexpr Entry kKeaton = {
    "Keaton Mask",
    "You got the %rKeaton Mask%w!&Equip to %y\xA1%w or %yD-Pad%w.^"
    "Use it to become a %rKeaton%w.&Use it again to return.&Both masks share this form.^"
    "%y\xA0%w chains three punches.&Hold %y\xA0%w after punching to charge.&Release it for a %cmagic shot%w.^"
    "%y\x9F%w: long jump.&In the air, %y\xA0%w: kick.&Hold %y\x9F%w at a wall to climb.^"
    "Ordinary walls and charged&shots consume %gmagic%w.&Hold %y\xA3%w to %creflect projectiles%w.",
    "Du hast die %rKeaton-Maske%w!&Lege sie auf %y\xA1%w oder %yD-Pad%w.^"
    "Nutze sie für die %rKeaton-Form%w.&Erneut nutzen: zurückverwandeln.&Beide Masken teilen diese Form.^"
    "%y\xA0%w: drei Fausthiebe in Folge.&Halte %y\xA0%w nach dem Hieb: aufladen.&Loslassen: %cMagiegeschoss%w.^"
    "%y\x9F%w: weiter Sprung.&In der Luft: %y\xA0%w für einen Tritt.&Halte %y\x9F%w an einer Wand: klettern.^"
    "Normale Wände und aufgeladene&Schüsse verbrauchen %gMagie%w.&Halte %y\xA3%w: %cGeschosse abwehren%w.",
    "Vous obtenez le %rMasque de Renard%w!&Assignez-le à %y\xA1%w ou au %yD-Pad%w.^"
    "Il vous transforme en %rKeaton%w.&Réutilisez-le pour revenir.&Les deux masques ont cette forme.^"
    "%y\xA0%w: enchaînez trois coups.&Gardez %y\xA0%w après un coup: charge.&Relâchez: %ctir magique%w.^"
    "%y\x9F%w: saut en longueur.&En l'air: %y\xA0%w pour frapper du pied.&Gardez %y\x9F%w contre un mur: grimper.^"
    "Les murs ordinaires et les&tirs chargés coûtent de la %gmagie%w.&Gardez %y\xA3%w: %crenvoyer les tirs%w.",
};

inline constexpr const Entry* Find(std::string_view name) {
    if (name == kSkull.name)
        return &kSkull;
    if (name == kSpooky.name)
        return &kSpooky;
    if (name == kTruth.name || name == "Mask of Truth (MM)")
        return &kTruth;
    if (name == kGerudo.name)
        return &kGerudo;
    if (name == kKeaton.name || name == "Keaton Mask (MM)")
        return &kKeaton;
    return nullptr;
}
} // namespace ComboMaskReceiptText
