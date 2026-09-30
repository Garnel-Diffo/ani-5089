// Chapitre 2, exercice 31 : les dependances qui n'arrivent pas toutes seules.
//
// Entree :
//   N
//   N lignes "module besoin1 besoin2 ..."   (les modules connus et ce dont chacun a besoin)
//   M
//   M noms de modules que le projet nomme directement
//
// Sortie : la fermeture transitive des besoins du projet, un nom par ligne,
// triee par ordre alphabetique. Aucun module n'apparait deux fois.
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

int main() {
    int n;
    if (!(std::cin >> n)) return 1;
    std::cin.ignore();

    std::unordered_map<std::string, std::vector<std::string>> besoins;
    for (int i = 0; i < n; ++i) {
        std::string ligne;
        std::getline(std::cin, ligne);
        std::istringstream flux(ligne);
        std::string nom;
        flux >> nom;
        std::string b;
        while (flux >> b) besoins[nom].push_back(b);
        besoins.try_emplace(nom);   // le module existe meme sans besoin liste
    }

    int m;
    std::cin >> m;
    std::vector<std::string> pile(m);
    for (auto& s : pile) std::cin >> s;

    std::unordered_set<std::string> vus(pile.begin(), pile.end());
    while (!pile.empty()) {
        std::string courant = pile.back();
        pile.pop_back();
        auto it = besoins.find(courant);
        if (it == besoins.end()) continue;   // un besoin absent des N lignes : pas de besoin a lui
        for (const std::string& b : it->second) {
            if (vus.insert(b).second) pile.push_back(b);
        }
    }

    std::vector<std::string> resultat(vus.begin(), vus.end());
    std::sort(resultat.begin(), resultat.end());
    for (const std::string& s : resultat) std::cout << s << "\n";
    return 0;
}
