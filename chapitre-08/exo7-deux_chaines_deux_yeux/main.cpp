// Exercice 7 : deux chaines, deux yeux.
//
// Chaque oeil a sa propre chaine : son propre anneau, son propre booleen d'emprunt, son propre compteur de
// rendus, independants de l'autre oeil. Les regles d'emprunt et de rendu sont celles de l'anneau ordinaire
// (exercice 1) : EMPRUNTE reussit si rien n'est emprunte et prete l'image sur laquelle pointe l'anneau ;
// REND reussit si quelque chose est emprunte, rend l'image PUIS fait avancer l'anneau ; une commande refusee
// ecrit REFUSE et ne change rien.
//
// Apres chaque commande on calcule l'ecart, en valeur absolue, entre le nombre de rendus acceptes de l'oeil
// gauche et celui de l'oeil droit. On compte les rendus ACCEPTES, pas les commandes : un rendu refuse ne
// change pas l'ecart.
//   DESYNC   est le premier tour ou cet ecart DEPASSE 1 (strictement), ou 0 s'il ne l'a jamais depasse :
//            un ecart de 1 est normal, les deux yeux ne rendent jamais exactement en meme temps.
//   DECALAGE est l'ecart a la fin.
//
// Entree : une ligne `images` (la taille de chaque anneau), un entier N, puis N lignes `oeil commande`
//          ou `oeil` vaut G ou D et `commande` vaut EMPRUNTE ou REND.
// Sortie : une ligne par commande (tour, oeil, commande, index prete ou rendu, ou REFUSE), puis `DECALAGE n`
//          et `DESYNC n`, dans cet ordre.

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

// La chaine d'un oeil : un anneau, un booleen, un compteur. Aucun etat partage avec l'autre oeil.
struct ChaineOeil {
    int images = 0;
    int suivant = 0;
    bool empruntee = false;
    int indexEmprunte = 0;
    int rendus = 0;

    // Retourne true si l'emprunt est accepte ; `index` recoit alors l'image prete.
    bool Emprunte(int &index) {
        if (empruntee) {
            return false;
        }
        empruntee = true;
        indexEmprunte = suivant;
        index = indexEmprunte;
        return true;
    }

    // Retourne true si le rendu est accepte ; `index` recoit alors l'image rendue.
    bool Rend(int &index) {
        if (!empruntee) {
            return false;
        }
        empruntee = false;
        index = indexEmprunte;
        ++rendus;
        if (images > 0) {
            suivant = (suivant + 1) % images;   // l'anneau avance au rendu
        }
        return true;
    }
};

} // namespace

int main() {
    int images = 0;
    int nombre = 0;
    std::cin >> images >> nombre;

    ChaineOeil gauche;
    ChaineOeil droit;
    gauche.images = images;
    droit.images = images;

    int desync = 0;   // premier tour ou l'ecart a depasse 1 (0 : jamais)

    for (int tour = 1; tour <= nombre; ++tour) {
        std::string oeil;
        std::string commande;
        if (!(std::cin >> oeil >> commande)) {
            break;
        }

        ChaineOeil &chaine = (oeil == "G") ? gauche : droit;
        int index = 0;
        bool accepte = false;
        if (commande == "EMPRUNTE") {
            accepte = chaine.Emprunte(index);
        } else if (commande == "REND") {
            accepte = chaine.Rend(index);
        }

        std::cout << tour << ' ' << oeil << ' ' << commande << ' ';
        if (accepte) {
            std::cout << index << '\n';
        } else {
            std::cout << "REFUSE\n";
        }

        const int ecart = std::abs(gauche.rendus - droit.rendus);
        if (ecart > 1 && desync == 0) {
            desync = tour;
        }
    }

    std::cout << "DECALAGE " << std::abs(gauche.rendus - droit.rendus) << '\n';
    std::cout << "DESYNC " << desync << '\n';
    return 0;
}
