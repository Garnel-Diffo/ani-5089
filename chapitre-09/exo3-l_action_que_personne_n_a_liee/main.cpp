// Exercice 3 : l'action que personne n'a liee.
//
// Le programme repere les actions declarees mais jamais liees a un organe : une action inactive
// rend `courant` faux, et rien ne le dit a part un bouton relache.
//
// La regle :
//   1. Une action est ACTIVE si l'usage qu'elle demande figure dans la liste des usages fournis.
//   2. Sinon elle est MORTE : elle donnera toujours un etat faux, et aucun message ne le dira.
//   3. Deux actions peuvent demander le meme usage : elles sont alors actives toutes les deux.
//   4. MORTES compte les actions mortes ; la ligne est toujours ecrite.
//
// Un usage fourni que personne ne reclame n'est pas une erreur, et n'est pas ecrit. L'ordre
// d'ecriture est celui des declarations, pas celui des usages fournis.
//
// Entree : un entier A, puis A lignes `nom usage` (les actions declarees et l'usage demande),
// puis un entier L, puis L lignes (les usages que le materiel branche fournit vraiment).
// Sortie : une ligne par action, dans l'ordre de declaration (le nom, puis ACTIVE ou MORTE),
// puis `MORTES n`.

#include <iostream>
#include <set>
#include <string>
#include <vector>

int main() {
    int declarees = 0;
    std::cin >> declarees;

    std::vector<std::string> noms;
    std::vector<std::string> usages;
    for (int i = 0; i < declarees; ++i) {
        std::string nom;
        std::string usage;
        if (!(std::cin >> nom >> usage)) {
            break;
        }
        noms.push_back(nom);
        usages.push_back(usage);
    }

    int fournis = 0;
    std::cin >> fournis;
    std::set<std::string> fournisseurs;   // les usages que le materiel fournit
    for (int i = 0; i < fournis; ++i) {
        std::string usage;
        if (!(std::cin >> usage)) {
            break;
        }
        fournisseurs.insert(usage);
    }

    int mortes = 0;
    for (std::size_t i = 0; i < noms.size(); ++i) {
        const bool active = fournisseurs.count(usages[i]) != 0;
        std::cout << noms[i] << (active ? " ACTIVE" : " MORTE") << '\n';
        if (!active) {
            ++mortes;
        }
    }

    std::cout << "MORTES " << mortes << '\n';
    return 0;
}
