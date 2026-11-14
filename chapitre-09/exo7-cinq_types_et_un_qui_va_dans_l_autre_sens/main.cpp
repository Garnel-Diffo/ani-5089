// Exercice 7 : cinq types, et un qui va dans l'autre sens.
//
// Le programme range les actions par type et repere celles qu'on lit quand on devrait les ecrire.
// Quatre des cinq types portent une entree ; le cinquieme est une sortie : lire une vibration
// n'a aucun sens, et rien ne l'empeche.
//
// La regle :
//   1. BOOL, FLOAT, VEC2 et POSE sont des entrees. HAPTIC est la seule sortie.
//   2. LIT sur une sortie est une faute : LIT UNE SORTIE.
//   3. ECRIT sur une entree est une faute : ECRIT UNE ENTREE.
//   4. Les operations correctes ne sont pas ecrites.
//   5. Une operation sur une action qui n'a pas ete declaree est ignoree.
//   6. ENTREES et SORTIES comptent les ACTIONS declarees, pas les operations.
//
// POSE est une entree, meme si on la lit par un appel a part. La sortie peut ne contenir que les
// deux lignes de bilan, quand tout est correct.
//
// Entree : un entier A, puis A lignes `nom type` (le type est BOOL, FLOAT, VEC2, POSE ou HAPTIC),
// puis un entier O, puis O lignes `nom operation` (l'operation est LIT ou ECRIT).
// Sortie : une ligne par operation fautive, dans l'ordre de lecture (le nom, puis LIT UNE SORTIE
// ou ECRIT UNE ENTREE), puis `ENTREES n` et `SORTIES n`.

#include <iostream>
#include <map>
#include <string>

int main() {
    int declarees = 0;
    std::cin >> declarees;

    std::map<std::string, bool> estUneSortie;   // nom -> l'action est-elle une sortie (HAPTIC) ?
    int entrees = 0;
    int sorties = 0;
    for (int i = 0; i < declarees; ++i) {
        std::string nom;
        std::string type;
        if (!(std::cin >> nom >> type)) {
            break;
        }
        const bool sortie = (type == "HAPTIC");
        estUneSortie[nom] = sortie;
        if (sortie) {
            ++sorties;
        } else {
            ++entrees;
        }
    }

    int operations = 0;
    std::cin >> operations;
    for (int i = 0; i < operations; ++i) {
        std::string nom;
        std::string operation;
        if (!(std::cin >> nom >> operation)) {
            break;
        }
        const auto trouvee = estUneSortie.find(nom);
        if (trouvee == estUneSortie.end()) {
            continue;   // une action jamais declaree : on l'ignore
        }
        if (trouvee->second && operation == "LIT") {
            std::cout << nom << " LIT UNE SORTIE\n";
        } else if (!trouvee->second && operation == "ECRIT") {
            std::cout << nom << " ECRIT UNE ENTREE\n";
        }
    }

    std::cout << "ENTREES " << entrees << '\n';
    std::cout << "SORTIES " << sorties << '\n';
    return 0;
}
