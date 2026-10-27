#pragma once
#include <string_view>

// Cold-donor fallback: the same localized receipt bodies authored in OoT's
// NEI registry. Song teaching/ocarina/cutscene text IDs are deliberately not used.
namespace ComboSongReceiptText {
struct Entry {
    const char* name;
    const char* english;
    const char* german;
    const char* french;
};
inline const Entry* Find(std::string_view name) {
    static const Entry entries[] = {
        { "Sonata of Awakening", "You learned the %gSonata of Awakening%w!&It stirs the sleeping from&their slumber.",
          "Du lernst die %gSonate des Erwachens%w!&Sie weckt die Schlafenden&aus ihrem Schlummer.",
          "Vous apprenez la %gSonate de l'Éveil%w!&Elle tire les dormeurs&de leur sommeil." },
        { "Goron Lullaby", "You learned the %rGoron Lullaby%w!&A soothing melody that lulls&even Gorons to sleep.",
          "Du lernst das %rGoronen-Wiegenlied%w!&Eine sanfte Melodie, die sogar&Goronen einschläfert.",
          "Vous apprenez la %rBerceuse Goron%w!&Une mélodie apaisante qui&endort même les Gorons." },
        { "Goron Lullaby Intro", "You learned the %rGoron Lullaby Intro%w!&The opening bars of the&Goron's lullaby.",
          "Du lernst das %rGoronen-Wiegenlied (Intro)%w!&Die ersten Takte des&Goronen-Wiegenlieds.",
          "Vous apprenez l'%rIntro de la Berceuse Goron%w!&Les premières mesures de&la berceuse Goron." },
        { "New Wave Bossa Nova", "You learned the %bNew Wave Bossa Nova%w!&The song that awakens&new life in the bay.",
          "Du lernst die %bNew Wave Bossa Nova%w!&Das Lied, das neues Leben&in der Bucht weckt.",
          "Vous apprenez la %bNouvelle Vague Bossa Nova%w!&Le chant qui éveille&une vie nouvelle dans la baie." },
        { "Elegy of Emptiness", "You learned the %yElegy of Emptiness%w!&It leaves behind a hollow&shell of yourself.",
          "Du lernst die %yElegie der Leere%w!&Sie hinterlässt eine hohle&Hülle deiner selbst.",
          "Vous apprenez l'%yÉlégie du Néant%w!&Elle laisse derrière vous&une coquille vide." },
        { "Oath to Order", "You learned the %pOath to Order%w!&The song that calls the&four giants of Termina.",
          "Du lernst den %pSchwur der Ordnung%w!&Das Lied, das die vier&Giganten Terminas ruft.",
          "Vous apprenez le %pChant de l'Ordre%w!&Le chant qui appelle les&quatre géants de Termina." },
        { "Saria's Song (MM)", "You learned %gSaria's Song%w!&A melody carried over&from a distant forest.",
          "Du lernst %gSarias Lied%w!&Eine Melodie aus einem&fernen Wald.",
          "Vous apprenez le %gChant de Saria%w!&Une mélodie venue d'une&forêt lointaine." },
        { "Epona's Song (MM)", "You learned %yEpona's Song%w!&A tune shared between&a girl and her horse.",
          "Du lernst %yEponas Lied%w!&Eine Weise zwischen einem&Mädchen und seinem Pferd.",
          "Vous apprenez le %yChant d'Epona%w!&Un air partagé entre&une fille et son cheval." },
        { "Song of Soaring", "You learned the %pSong of Soaring%w!&Warp swiftly to any owl&statue you have touched.",
          "Du lernst das %pLied des Aufschwungs%w!&Reise flink zu jeder berührten&Eulenstatue.",
          "Vous apprenez le %pChant de l'Envol%w!&Téléportez-vous vers toute&statue-chouette activée." },
        { "Song of Storms (MM)", "You learned the %wSong of Storms%w!&Summon rain and thunder&at will.",
          "Du lernst das %wLied des Sturms%w!&Rufe Regen und Donner&nach Belieben herbei.",
          "Vous apprenez le %wChant de l'Orage%w!&Invoquez pluie et tonnerre&à volonté." },
        { "Sun's Song (MM)", "You learned the %ySun's Song%w!&It turns night to day&and day to night.",
          "Du lernst das %ySonnenlied%w!&Es verwandelt Nacht in Tag&und Tag in Nacht.",
          "Vous apprenez le %yChant du Soleil%w!&Il transforme la nuit en jour&et le jour en nuit." },
        { "Song of Time (MM)", "You learned the %cSong of Time%w!&It bends the flow of the&three days of Termina.",
          "Du lernst die %cHymne der Zeit%w!&Sie beugt den Lauf der&drei Tage Terminas.",
          "Vous apprenez le %cChant du Temps%w!&Il plie le cours des&trois jours de Termina." },
        { "Song of Healing",
          "You learned the %pSong of Healing%w!&It soothes troubled souls&and seals them into masks.",
          "Du lernst das %pLied der Heilung%w!&Es beruhigt verstörte Seelen&und bannt sie in Masken.",
          "Vous apprenez le %pChant de l'Apaisement%w!&Il apaise les âmes troublées&et les scelle en masques." },
        { "Song of Double Time", "You learned the %cSong of Double Time%w!&Skip ahead to the next&dawn or dusk.",
          "Du lernst das %cLied der doppelten Zeit%w!&Springe zur nächsten&Dämmerung vor.",
          "Vous apprenez le %cChant de l'Accéléré%w!&Sautez à l'aube ou&au crépuscule suivant." },
        { "Inverted Song of Time", "You learned the %cInverted Song of Time%w!&It slows the passage of&the three days.",
          "Du lernst die %cUmgekehrte Hymne der Zeit%w!&Sie verlangsamt den Lauf&der drei Tage.",
          "Vous apprenez le %cChant du Temps Inversé%w!&Il ralentit l'écoulement&des trois jours." },
    };
    for (const auto& entry : entries)
        if (name == entry.name)
            return &entry;
    return nullptr;
}
} // namespace ComboSongReceiptText
