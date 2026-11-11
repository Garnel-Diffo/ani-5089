// Exercice 3 : les images en vol.
//
// Le programme compte les images soumises que la carte graphique n'a pas finies, et dit quand la boucle se
// bloque. Un compteur suffit.
//
// La regle :
//   SOUMET ajoute une image en vol, SAUF si le nombre en vol a deja atteint `vol` : la boucle attend alors
//          (ATTEND) et le compteur ne bouge pas.
//   FINI   retire une image en vol, sauf s'il n'y en a aucune : le compteur reste a zero et la commande
//          passe quand meme (PASSE). Un FINI sans image en vol n'est pas une faute : la carte peut signaler
//          la fin d'un travail que nous ne suivons pas.
//   Une commande qui fait attendre la boucle ecrit ATTEND, les autres PASSE.
//   BLOQUE est le PREMIER tour qui a ecrit ATTEND, ou 0 s'il n'y en a pas eu.
//
// Le nombre ecrit est celui APRES la commande, y compris quand elle a fait attendre la boucle.
//
// Entree : une ligne `vol`, un entier N, puis N lignes SOUMET ou FINI.
// Sortie : une ligne par commande (tour depuis 1, commande, images en vol, PASSE ou ATTEND), puis `BLOQUE n`.

#include <iostream>
#include <string>

int main() {
    int vol = 0;
    int nombre = 0;
    std::cin >> vol >> nombre;

    int enVol = 0;      // images soumises que la carte n'a pas finies
    int bloque = 0;     // premier tour ou la boucle a du attendre (0 : jamais)

    for (int tour = 1; tour <= nombre; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }

        bool attend = false;
        if (commande == "SOUMET") {
            if (enVol < vol) {
                ++enVol;
            } else {
                attend = true;   // le maximum est atteint : la boucle attend, le compteur ne bouge pas
            }
        } else if (commande == "FINI") {
            if (enVol > 0) {
                --enVol;         // jamais en dessous de zero
            }
        }

        if (attend && bloque == 0) {
            bloque = tour;       // on garde le PREMIER blocage, pas le dernier
        }
        std::cout << tour << ' ' << commande << ' ' << enVol << ' ' << (attend ? "ATTEND" : "PASSE") << '\n';
    }

    std::cout << "BLOQUE " << bloque << '\n';
    return 0;
}
