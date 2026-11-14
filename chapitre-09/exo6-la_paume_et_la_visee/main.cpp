// Exercice 6 : la paume et la visee.
//
// Le programme dit si chaque objet est attache a la bonne pose de la main droite. La paume
// (GRIP) est la ou l'objet tenu se trouve physiquement ; la visee (AIM) est calibree par le
// constructeur pour que le pointage paraisse naturel. Se tromper de pose est visible tout de
// suite dans un casque, et pas du tout sur un ecran.
//
// La regle :
//   1. TENIR avec GRIP est juste : la paume est la ou l'objet tenu se trouve.
//   2. POINTER avec AIM est juste : la visee est calibree pour que le pointage paraisse naturel.
//   3. TENIR avec AIM donne FLOTTE : l'objet est attache a la visee, qui n'est pas au meme endroit
//      que la paume, et il flotte a quelques centimetres de la main.
//   4. POINTER avec GRIP donne PART DE TRAVERS : le rayon part de la paume, qui n'est pas orientee
//      comme le regard l'attend.
//   5. FAUTES compte les deux symptomes ensemble.
//
// Les deux fautes ne donnent pas le meme symptome : un objet flottant et un rayon de travers ne
// se diagnostiquent pas de la meme facon. OK est ecrit lui aussi, contrairement aux exercices ou
// seules les fautes sortent.
//
// Entree : un entier N, puis N lignes `nom intention pose` (l'intention est TENIR ou POINTER, la
// pose est GRIP ou AIM).
// Sortie : une ligne par objet (le nom, puis OK ou le symptome), puis `FAUTES n`.

#include <iostream>
#include <string>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    int fautes = 0;
    for (int i = 0; i < nombre; ++i) {
        std::string nom;
        std::string intention;
        std::string pose;
        if (!(std::cin >> nom >> intention >> pose)) {
            break;
        }

        std::cout << nom << ' ';
        if (intention == "TENIR" && pose == "AIM") {
            std::cout << "FLOTTE\n";          // tenu par la visee : il flotte hors de la main
            ++fautes;
        } else if (intention == "POINTER" && pose == "GRIP") {
            std::cout << "PART DE TRAVERS\n"; // vise depuis la paume : le rayon part de travers
            ++fautes;
        } else {
            std::cout << "OK\n";
        }
    }

    std::cout << "FAUTES " << fautes << '\n';
    return 0;
}
