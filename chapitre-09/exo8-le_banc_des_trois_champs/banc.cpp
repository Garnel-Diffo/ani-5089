// banc.cpp : le banc de test des trois champs d'une action, sans casque ni manette.
//
// Trois actions : `saisir`, liee a une touche ; `tirer`, liee a une autre touche ; `avancer`, liee
// a rien du tout. Six images. La touche de `saisir` est tenue de la deuxieme image a la
// quatrieme ; celle de `tirer` n'est enfoncee que sur la cinquieme.
//
// La sortie est un tableau, une ligne par image, trois colonnes de trois champs (courant,
// change, actif). L'action liee a rien DOIT figurer dans la sortie : sinon personne ne peut voir
// qu'elle est morte.
//
// Compilation : clang++ -std=c++17 -Wall banc.cpp -o banc

#include "actions.h"

#include <cstdio>

namespace {

const int kImages = 6;

// Les trois champs d'une action, sous leurs titres : `courant`, `change`, `actif`.
const char *const kTitres = "courant change actif";

// Ecrit les trois champs d'une action, alignes sous leurs titres.
void EcritLesChamps(const ActionBool &action) {
    std::printf("%7d %6d %5d", action.etat.courant() ? 1 : 0, action.etat.change() ? 1 : 0,
                action.etat.actif() ? 1 : 0);
}

} // namespace

int main() {
    Touche toucheSaisir;   // la touche de `saisir`
    Touche toucheTirer;    // la touche de `tirer`

    ActionBool saisir;
    saisir.nom = "saisir";
    saisir.source = &toucheSaisir;
    ActionBool tirer;
    tirer.nom = "tirer";
    tirer.source = &toucheTirer;
    ActionBool avancer;    // liee a rien : source == nullptr
    avancer.nom = "avancer";

    std::printf("%-5s | %-20s | %-20s | %s\n", "image", saisir.nom.c_str(), tirer.nom.c_str(),
                avancer.nom.c_str());
    std::printf("%-5s | %s | %s | %s\n", "", kTitres, kTitres, kTitres);

    for (int image = 1; image <= kImages; ++image) {
        // Ce que fait le joueur : `saisir` tenue de l'image 2 a l'image 4, `tirer` sur l'image 5.
        toucheSaisir.pressee = (image >= 2 && image <= 4);
        toucheTirer.pressee = (image == 5);

        // On synchronise une fois par image, puis on interroge.
        Synchronise(saisir);
        Synchronise(tirer);
        Synchronise(avancer);

        std::printf("%5d | ", image);
        EcritLesChamps(saisir);
        std::printf(" | ");
        EcritLesChamps(tirer);
        std::printf(" | ");
        EcritLesChamps(avancer);
        std::printf("\n");
    }
    return 0;
}
