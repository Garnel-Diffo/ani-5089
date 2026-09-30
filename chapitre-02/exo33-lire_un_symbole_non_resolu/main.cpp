// Chapitre 2, exercice 33 : lire un symbole non resolu.
//
// Entree :
//   P
//   P lignes "prefixe module"
//   L
//   L lignes de message (certaines contiennent undefined reference to 'SYMBOLE')
//
// Sortie : les modules a ajouter, par ordre alphabetique, un par ligne, sans
// doublon, puis, seulement s'il reste des symboles qu'aucun prefixe n'explique,
// une derniere ligne "INCONNU n".
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

const std::string MARQUEUR = "undefined reference to '";

int main() {
    int p;
    if (!(std::cin >> p)) return 1;
    std::vector<std::pair<std::string, std::string>> prefixes(p);  // (prefixe, module)
    for (auto& [pref, mod] : prefixes) std::cin >> pref >> mod;
    std::cin.ignore();

    int l;
    std::cin >> l;
    std::cin.ignore();

    std::set<std::string> modules;   // std::set : deja trie, sans doublon
    int inconnus = 0;

    for (int i = 0; i < l; ++i) {
        std::string ligne;
        std::getline(std::cin, ligne);
        size_t pos = ligne.find(MARQUEUR);
        if (pos == std::string::npos) continue;              // pas la bonne ligne : on l'ignore
        size_t debut = pos + MARQUEUR.size();
        size_t fin = ligne.find('\'', debut);
        if (fin == std::string::npos) continue;               // ligne malformee, par prudence
        std::string symbole = ligne.substr(debut, fin - debut);

        // Le prefixe le plus long qui commence le symbole.
        const std::string* meilleur = nullptr;
        size_t meilleure_longueur = 0;
        for (const auto& [pref, mod] : prefixes) {
            if (symbole.size() >= pref.size() && symbole.compare(0, pref.size(), pref) == 0) {
                if (pref.size() > meilleure_longueur) { meilleure_longueur = pref.size(); meilleur = &mod; }
            }
        }
        if (meilleur) modules.insert(*meilleur);
        else ++inconnus;
    }

    for (const std::string& m : modules) std::cout << m << "\n";
    if (inconnus != 0) std::cout << "INCONNU " << inconnus << "\n";
    return 0;
}
