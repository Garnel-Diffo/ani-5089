// Chapitre 2, exercice 34 : le filtre s'applique-t-il ?
//
// Entree :
//   V
//   V lignes "cle=valeur"      (l'etat de la machine)
//   F
//   F lignes, une condition de filtre chacune : des termes "cle=valeur",
//     eventuellement precedes de "!", joints par "&&"
//
// Sortie : une ligne par filtre, OUI s'il s'applique, NON sinon.
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

int main() {
    int v;
    if (!(std::cin >> v)) return 1;
    std::cin.ignore();

    std::unordered_map<std::string, std::string> machine;
    for (int i = 0; i < v; ++i) {
        std::string ligne;
        std::getline(std::cin, ligne);
        size_t eq = ligne.find('=');
        machine[ligne.substr(0, eq)] = ligne.substr(eq + 1);
    }

    int f;
    std::cin >> f;
    std::cin.ignore();

    for (int i = 0; i < f; ++i) {
        std::string ligne;
        std::getline(std::cin, ligne);
        std::istringstream flux(ligne);
        std::string terme;
        bool applique = true;
        while (flux >> terme) {
            if (terme == "&&") continue;
            bool nie = false;
            if (!terme.empty() && terme[0] == '!') { nie = true; terme = terme.substr(1); }
            size_t eq = terme.find('=');
            std::string cle = terme.substr(0, eq), valeur = terme.substr(eq + 1);
            auto it = machine.find(cle);
            bool vrai = (it != machine.end() && it->second == valeur);   // cle absente : faux
            if (nie) vrai = !vrai;
            if (!vrai) applique = false;
        }
        std::cout << (applique ? "OUI" : "NON") << "\n";
    }
    return 0;
}
