// Exercice 4 : la poignee opaque.
//
// Le programme range et rend les poignees d'images sans jamais les interpreter : pour lui, une poignee est un
// entier de soixante-quatre bits dont il ignore le sens (texture Vulkan, pointeur Direct3D, entier OpenGL...).
//
// La regle :
//   Les index vont de 0 a images - 1. Un POSE ou un LIT hors de cet intervalle est refuse.
//   POSE hors anneau ecrit HORS ANNEAU. LIT hors anneau ecrit VIDE. Un POSE accepte n'ecrit rien.
//   Un index qui n'a jamais recu de valeur donne VIDE. Poser une valeur sur un index deja utilise la remplace,
//   et ne compte pas deux fois. POSEES compte les index distincts qui portent une valeur.
//
// Les valeurs atteignent plusieurs milliers de milliards : elles ne tiennent pas dans un entier de trente-deux
// bits, d'ou le uint64_t. Une valeur de zero est une valeur comme une autre : ce n'est PAS VIDE, c'est pourquoi
// le fait d'avoir ete pose est un booleen a part, et non le fait de valoir zero.
//
// Entree : une ligne `images`, un entier N, puis N lignes `POSE index valeur` ou `LIT index`.
// Sortie : une ligne par LIT (la valeur ou VIDE), une ligne par POSE refuse (HORS ANNEAU), puis `POSEES n`.

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    long long images = 0;
    int nombre = 0;
    std::cin >> images >> nombre;

    if (images < 0) {
        images = 0;
    }
    std::vector<std::uint64_t> valeurs(static_cast<std::size_t>(images), 0);
    std::vector<bool> posee(static_cast<std::size_t>(images), false);   // distingue "zero" de "jamais pose"
    int poseesDistinctes = 0;

    for (int ligne = 0; ligne < nombre; ++ligne) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }

        if (commande == "POSE") {
            long long index = 0;
            std::uint64_t valeur = 0;
            std::cin >> index >> valeur;
            if (index < 0 || index >= images) {
                std::cout << "HORS ANNEAU\n";
            } else {
                const std::size_t i = static_cast<std::size_t>(index);
                if (!posee[i]) {
                    posee[i] = true;
                    ++poseesDistinctes;   // un index deja utilise ne compte pas deux fois
                }
                valeurs[i] = valeur;
            }
        } else if (commande == "LIT") {
            long long index = 0;
            std::cin >> index;
            if (index < 0 || index >= images || !posee[static_cast<std::size_t>(index)]) {
                std::cout << "VIDE\n";
            } else {
                std::cout << valeurs[static_cast<std::size_t>(index)] << '\n';
            }
        }
    }

    std::cout << "POSEES " << poseesDistinctes << '\n';
    return 0;
}
