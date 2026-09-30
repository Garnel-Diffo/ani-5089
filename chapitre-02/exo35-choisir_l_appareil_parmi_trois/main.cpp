// Chapitre 2, exercice 35 : choisir l'appareil parmi trois.
//
// Entree :
//   D
//   D lignes "serie etat modele"   (etat = device | unauthorized | offline)
//   une derniere ligne : la serie demandee, ou "-" si aucune cible demandee
//
// Sortie : le numero de serie retenu, ou une ligne ERREUR ... selon le cas.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Appareil { std::string serie, etat, modele; };

int main() {
    int d;
    if (!(std::cin >> d)) return 1;
    std::vector<Appareil> appareils(d);
    for (auto& a : appareils) std::cin >> a.serie >> a.etat >> a.modele;

    std::string cible;
    std::cin >> cible;

    if (cible != "-") {
        // Une cible est demandee.
        auto it = std::find_if(appareils.begin(), appareils.end(),
                                [&](const Appareil& a) { return a.serie == cible; });
        if (it == appareils.end()) {
            std::cout << "ERREUR cible introuvable\n";
            return 0;
        }
        if (it->etat != "device") {
            std::cout << "ERREUR serie est " << it->etat << "\n";
            return 0;
        }
        std::cout << it->serie << "\n";
        return 0;
    }

    // Aucune cible demandee : on ne garde que les appareils en etat "device".
    std::vector<std::string> pretes;
    for (const auto& a : appareils)
        if (a.etat == "device") pretes.push_back(a.serie);

    if (pretes.empty()) {
        std::cout << "ERREUR aucun appareil\n";
    } else if (pretes.size() == 1) {
        std::cout << pretes[0] << "\n";
    } else {
        std::sort(pretes.begin(), pretes.end());
        std::cout << "ERREUR plusieurs appareils\n";
        for (const std::string& s : pretes) std::cout << s << "\n";
    }
    return 0;
}
