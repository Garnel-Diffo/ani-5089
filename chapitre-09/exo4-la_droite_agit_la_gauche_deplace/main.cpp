// Exercice 4 : la droite agit, la gauche deplace.
//
// Le programme dit a quelle main appartient chaque usage, et repere les applications qui se
// jouent a une seule main. Il n'y a pas de main par defaut.
//
// Les usages connus :
//   SELECT, GRAB, AIM_POSE, GRIP_POSE, HAPTIC et MOVE_RIGHT sont a droite ;
//   MENU, MOVE, SELECT_LEFT, GRAB_LEFT, AIM_POSE_LEFT, GRIP_POSE_LEFT et HAPTIC_LEFT sont a
//   gauche.
//
// La regle :
//   1. Un usage qui se termine par `_LEFT` appartient a la main gauche.
//   2. MENU et MOVE appartiennent aussi a la gauche, sans suffixe : c'est le schema classique, la
//      droite agit et la gauche deplace.
//   3. Tous les autres usages appartiennent a la main droite, MOVE_RIGHT compris : le suffixe
//      `_RIGHT` n'est pas une decoration, il corrige la convention.
//   4. Une application demande DEUX MAINS des qu'elle utilise au moins un usage de chaque main.
//      Sinon elle tient en UNE MAIN : une application sans aucun usage tient en une main, zero et
//      zero ne font pas deux mains.
//   5. Un usage inconnu est ignore, et ne compte pour aucune des deux mains.
//
// Entree : un entier N, puis N lignes : le nom d'une application, puis les usages qu'elle emploie,
// separes par des espaces (il peut y en avoir un seul, plusieurs, ou aucun : on lit la ligne
// entiere).
// Sortie : une ligne par application (le nom, le nombre d'usages de la main droite, le nombre
// pour la main gauche, puis UNE MAIN ou DEUX MAINS), puis `DEUX MAINS n`.

#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace {

// Les usages de la main droite : ceux qui n'ont pas de suffixe _LEFT, sauf MENU et MOVE, plus
// MOVE_RIGHT.
const std::set<std::string> kDroite = { "SELECT", "GRAB", "AIM_POSE", "GRIP_POSE", "HAPTIC",
                                        "MOVE_RIGHT" };

// Les usages de la main gauche : ceux qui se terminent par _LEFT, plus MENU et MOVE.
const std::set<std::string> kGauche = { "MENU", "MOVE", "SELECT_LEFT", "GRAB_LEFT",
                                        "AIM_POSE_LEFT", "GRIP_POSE_LEFT", "HAPTIC_LEFT" };

} // namespace

int main() {
    std::string ligne;
    if (!std::getline(std::cin, ligne)) {
        std::cout << "DEUX MAINS 0\n";
        return 0;
    }
    int nombre = 0;
    {
        std::istringstream flux(ligne);
        flux >> nombre;
    }

    int deuxMains = 0;
    int lues = 0;
    while (lues < nombre && std::getline(std::cin, ligne)) {
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) {
            continue;   // ligne vide : ce n'est pas une application
        }
        ++lues;

        int droite = 0;
        int gauche = 0;
        std::string usage;
        while (flux >> usage) {
            if (kDroite.count(usage) != 0) {
                ++droite;
            } else if (kGauche.count(usage) != 0) {
                ++gauche;
            }
            // un usage inconnu est ignore
        }

        const bool deux = droite > 0 && gauche > 0;
        std::cout << nom << ' ' << droite << ' ' << gauche << (deux ? " DEUX MAINS" : " UNE MAIN")
                  << '\n';
        if (deux) {
            ++deuxMains;
        }
    }

    std::cout << "DEUX MAINS " << deuxMains << '\n';
    return 0;
}
