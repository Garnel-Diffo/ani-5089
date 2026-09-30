// Chapitre 2, exercice 32 : l'ordre dans lequel on donne les bibliotheques.
//
// Meme entree que l'exercice 31. On calcule d'abord la liste complete (comme
// a l'exercice 31), puis on la trie dans l'ordre d'edition de liens : un
// module apparait toujours avant ceux dont il a besoin (tri topologique de
// l'algorithme de Kahn, departage alphabetique). S'il y a un cycle, on
// affiche seulement CYCLE.
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
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
        besoins.try_emplace(nom);
    }

    int m;
    std::cin >> m;
    std::vector<std::string> pile(m);
    for (auto& s : pile) std::cin >> s;

    // Etape 1 : la fermeture complete (comme a l'exercice 31).
    std::unordered_set<std::string> ferme(pile.begin(), pile.end());
    {
        std::vector<std::string> a_visiter = pile;
        while (!a_visiter.empty()) {
            std::string courant = a_visiter.back();
            a_visiter.pop_back();
            auto it = besoins.find(courant);
            if (it == besoins.end()) continue;
            for (const std::string& b : it->second) {
                if (ferme.insert(b).second) a_visiter.push_back(b);
            }
        }
    }

    // Etape 2 : tri topologique de Kahn. "compte[x]" = nombre de modules de la
    // liste qui ont besoin de x. On sort d'abord ceux a zero, alphabetiquement.
    std::map<std::string, int> compte;             // std::map : deja trie par nom
    for (const std::string& s : ferme) compte[s] = 0;
    for (const std::string& s : ferme) {
        auto it = besoins.find(s);
        if (it == besoins.end()) continue;
        for (const std::string& b : it->second) {
            if (ferme.count(b)) ++compte[b];
        }
    }

    std::set<std::string> prets;                    // candidats a compte 0, toujours tries
    for (const auto& [nom, c] : compte)
        if (c == 0) prets.insert(nom);

    std::vector<std::string> ordre;
    ordre.reserve(ferme.size());
    while (!prets.empty()) {
        std::string nom = *prets.begin();           // le premier par ordre alphabetique
        prets.erase(prets.begin());
        ordre.push_back(nom);
        auto it = besoins.find(nom);
        if (it == besoins.end()) continue;
        for (const std::string& b : it->second) {
            if (!ferme.count(b)) continue;
            if (--compte[b] == 0) prets.insert(b);
        }
    }

    if (ordre.size() != ferme.size()) {
        std::cout << "CYCLE\n";
        return 0;
    }
    for (const std::string& s : ordre) std::cout << s << "\n";
    return 0;
}
