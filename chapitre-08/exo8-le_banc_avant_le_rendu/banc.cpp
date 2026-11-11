// banc.cpp : le banc de test de la chaine d'echange, ecrit AVANT la moindre ligne de rendu.
// Il ne demande ni fenetre, ni carte graphique, ni casque : une chaine d'echange, c'est de la
// comptabilite.
//
// Premiere partie : six tours sur un anneau de trois images, avec les quatre temps a chaque tour
// (emprunter, rendre, puis lire quelle image soumettre). L'anneau doit BOUCLER : 0 1 2 0 1 2.
// La sortie montre ce bouclage de trois facons, pour qu'on le lise sans effort :
//   - la suite brute des images pretees, un nombre par ligne ;
//   - le detail de chaque tour, avec un seul nombre d'image par ligne ;
//   - la boucle ecrite d'une traite (0 -> 1 -> 2 -> 0 ...), puis les images rendues et la
//     position de l'anneau apres chaque rendu.
// Seconde partie : les trois refus : emprunter deux fois de suite, rendre sans avoir emprunte,
// creer une chaine plus grande que le maximum.
//
// Compilation : clang++ -std=c++17 -Wall banc.cpp -o banc   (le programme rend 0 si tout va bien)

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

// Ecrit `titre : v0 <separateur> v1 <separateur> v2 ...` sur une seule ligne.
void EcritLaSuite(const char *titre, const std::uint32_t *valeurs, std::uint32_t nombre,
                  const char *separateur) {
    std::printf("%s : %u", titre, valeurs[0]);
    for (std::uint32_t i = 1; i < nombre; ++i) {
        std::printf("%s%u", separateur, valeurs[i]);
    }
    std::printf("\n");
}

} // namespace

int main() {
    ChaineEchange chaine;
    Verifie(chaine.Create(3), "creer une chaine de 3 images");

    // Les six tours, joues d'abord, ecrits ensuite.
    const std::uint32_t kTours = 6;
    std::uint32_t pretees[kTours] = {};      // l'image pretee a chaque tour (temps 1)
    bool rendues[kTours] = {};               // le rendu a-t-il ete accepte (temps 3) ?
    std::uint32_t dernieres[kTours] = {};    // la derniere image rendue (temps 4)
    std::uint32_t anneau[kTours] = {};       // l'image sur laquelle pointe l'anneau apres le rendu
    for (std::uint32_t tour = 0; tour < kTours; ++tour) {
        std::uint32_t emprunte = 99;
        const bool prete = chaine.AcquireImage(emprunte);   // temps 1 (le rendu, temps 2, n'est
                                                            // pas ecrit ici)
        rendues[tour] = chaine.ReleaseImage();              // temps 3 : rend l'image ET avance
        pretees[tour] = emprunte;
        dernieres[tour] = chaine.GetLastReleasedIndex();    // temps 4 : ce qu'on soumet
        anneau[tour] = chaine.GetNextImage();
        Verifie(prete && rendues[tour], "un tour complet reussit");
        Verifie(emprunte == tour % 3, "l'anneau prete 0 1 2 0 1 2");
        Verifie(dernieres[tour] == emprunte,
                "on soumet la derniere image RENDUE, pas la suivante");
    }

    std::printf("anneau de %u images, %u tours\n", chaine.GetImageCount(), kTours);
    std::printf("images pretees, une par ligne :\n");
    for (std::uint32_t tour = 0; tour < kTours; ++tour) {
        std::printf("%u\n", pretees[tour]);
    }
    for (std::uint32_t tour = 0; tour < kTours; ++tour) {
        std::printf("tour %u : AcquireImage -> image %u, ReleaseImage -> %s\n", tour + 1,
                    pretees[tour], rendues[tour] ? "ok" : "refuse");
    }
    EcritLaSuite("bouclage", pretees, kTours, " -> ");
    EcritLaSuite("dernieres rendues", dernieres, kTours, " ");
    EcritLaSuite("anneau apres chaque rendu", anneau, kTours, " ");

    // Refus 1 : le double emprunt. Le second AcquireImage echoue et ne change rien : le rendu qui
    // suit reussit.
    std::uint32_t index = 0;
    const bool premier = chaine.AcquireImage(index);
    std::uint32_t autre = 77;
    const bool second = chaine.AcquireImage(autre);
    std::printf("refus 1 : AcquireImage deux fois de suite -> %s puis %s (index inchange : %u)\n",
                premier ? "ok" : "refuse", second ? "ok" : "refuse", autre);
    Verifie(premier && !second && autre == 77,
            "le double emprunt est refuse et ne touche pas l'index");
    Verifie(chaine.ReleaseImage(), "apres le refus, l'emprunt d'origine se rend normalement");

    // Refus 2 : rendre sans avoir emprunte.
    const std::uint32_t avant = chaine.GetNextImage();
    const bool rendu = chaine.ReleaseImage();
    std::printf("refus 2 : ReleaseImage sans emprunt -> %s (anneau sur %u avant, %u apres)\n",
                rendu ? "ok" : "refuse", avant, chaine.GetNextImage());
    Verifie(!rendu && chaine.GetNextImage() == avant,
            "rendre sans emprunt est refuse et l'anneau ne bouge pas");

    // Refus 3 : creer une chaine plus grande que le maximum.
    ChaineEchange trop;
    const bool creee = trop.Create(ChaineEchange::kImagesMax + 1);
    std::printf("refus 3 : Create(%u) avec un maximum de %u -> %s (chaine valide : %s)\n",
                ChaineEchange::kImagesMax + 1, ChaineEchange::kImagesMax, creee ? "ok" : "refuse",
                trop.IsValid() ? "oui" : "non");
    Verifie(!creee && !trop.IsValid(), "une chaine plus grande que le maximum est refusee");
    Verifie(trop.Create(ChaineEchange::kImagesMax), "le maximum lui-meme est accepte");

    std::printf("bilan : %u tours, 3 refus, %d echec(s)\n", kTours, gEchecs);
    return gEchecs == 0 ? 0 : 1;
}
