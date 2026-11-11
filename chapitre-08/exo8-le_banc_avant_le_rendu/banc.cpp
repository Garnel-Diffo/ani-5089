// banc.cpp : le banc de test de la chaine d'echange, ecrit AVANT la moindre ligne de rendu.
// Il ne demande ni fenetre, ni carte graphique, ni casque : une chaine d'echange, c'est de la comptabilite.
//
// Premiere partie : six tours sur un anneau de trois images, avec les quatre temps a chaque tour (emprunter,
// rendre, puis lire quelle image soumettre). L'anneau doit BOUCLER : 0 1 2 0 1 2.
// Seconde partie : les trois refus : emprunter deux fois de suite, rendre sans avoir emprunte, creer une
// chaine plus grande que le maximum.
//
// Compilation : clang++ -std=c++17 -Wall banc.cpp -o banc          (le programme rend 0 si tout est conforme)

#include "chaine.h"

#include <cstdio>

namespace {

int gEchecs = 0;

// Une verification : on n'ecrit rien si elle passe, et ECHEC si elle rate.
void Verifie(bool condition, const char *quoi) {
    if (!condition) {
        ++gEchecs;
        std::printf("ECHEC : %s\n", quoi);
    }
}

} // namespace

int main() {
    ChaineEchange chaine;
    Verifie(chaine.Create(3), "creer une chaine de 3 images");

    std::printf("anneau de %u images\n", chaine.GetImageCount());
    for (std::uint32_t tour = 1; tour <= 6; ++tour) {
        std::uint32_t emprunte = 99;
        const bool prete = chaine.AcquireImage(emprunte);   // temps 1 (le rendu, temps 2, n'est pas ecrit ici)
        const bool rendue = chaine.ReleaseImage();           // temps 3 : rend l'image ET avance l'anneau
        const std::uint32_t derniere = chaine.GetLastReleasedIndex();   // temps 4 : ce qu'on soumet
        std::printf("tour %u : AcquireImage -> %u, ReleaseImage -> %s, derniere rendue %u, anneau sur %u\n", tour,
                    emprunte, rendue ? "ok" : "refuse", derniere, chaine.GetNextImage());
        Verifie(prete && rendue, "un tour complet reussit");
        Verifie(emprunte == (tour - 1) % 3, "l'anneau prete 0 1 2 0 1 2");
        Verifie(derniere == emprunte, "on soumet la derniere image RENDUE, pas la suivante");
    }

    // Refus 1 : le double emprunt. Le second AcquireImage echoue et ne change rien : le rendu qui suit reussit.
    std::uint32_t index = 0;
    const bool premier = chaine.AcquireImage(index);
    std::uint32_t autre = 77;
    const bool second = chaine.AcquireImage(autre);
    std::printf("refus 1 : AcquireImage deux fois de suite -> %s puis %s (index inchange : %u)\n", premier ? "ok" : "refuse",
                second ? "ok" : "refuse", autre);
    Verifie(premier && !second && autre == 77, "le double emprunt est refuse et ne touche pas l'index");
    Verifie(chaine.ReleaseImage(), "apres le refus, l'emprunt d'origine se rend normalement");

    // Refus 2 : rendre sans avoir emprunte.
    const std::uint32_t avant = chaine.GetNextImage();
    const bool rendu = chaine.ReleaseImage();
    std::printf("refus 2 : ReleaseImage sans emprunt -> %s (anneau sur %u avant, %u apres)\n", rendu ? "ok" : "refuse", avant,
                chaine.GetNextImage());
    Verifie(!rendu && chaine.GetNextImage() == avant, "rendre sans emprunt est refuse et l'anneau ne bouge pas");

    // Refus 3 : creer une chaine plus grande que le maximum.
    ChaineEchange trop;
    const bool creee = trop.Create(ChaineEchange::kImagesMax + 1);
    std::printf("refus 3 : Create(%u) avec un maximum de %u -> %s (chaine valide : %s)\n", ChaineEchange::kImagesMax + 1,
                ChaineEchange::kImagesMax, creee ? "ok" : "refuse", trop.IsValid() ? "oui" : "non");
    Verifie(!creee && !trop.IsValid(), "une chaine plus grande que le maximum est refusee");
    Verifie(trop.Create(ChaineEchange::kImagesMax), "le maximum lui-meme est accepte");

    std::printf("bilan : 6 tours, 3 refus, %d echec(s)\n", gEchecs);
    return gEchecs == 0 ? 0 : 1;
}
