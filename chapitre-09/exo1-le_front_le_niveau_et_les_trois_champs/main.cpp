// Exercice 1 : le front, le niveau et les trois champs.
//
// Le programme calcule, image par image, les trois champs de l'etat d'une action booleenne :
//   - `courant` : l'etat de l'action a cette image (le niveau) ;
//   - `change`  : l'etat a bascule depuis l'image d'avant (c'est ce qui donne un front) ;
//   - `actif`   : une source materielle est liee a l'action.
//
// La regle :
//   1. `actif` vaut `lie`.
//   2. `courant` vaut `presse` si l'action est active, et 0 sinon : une action inactive donne
//      toujours `courant` faux, et rien ne la distingue alors d'un bouton relache.
//   3. `change` vaut 1 quand `courant` differe de celui de l'image d'avant ; a la premiere
//      image, on compare avec 0. Il se calcule sur `courant`, pas sur `presse` : une manette qui
//      s'eteint produit donc un `change`.
//   4. Un front de saisie est une image ou `courant` et `change` valent tous deux 1 : FRONTS les
//      compte. Le front de relachement a aussi `change` a 1, mais `courant` est faux : il ne
//      compte pas dans FRONTS.
//   5. Un niveau est une image ou `courant` vaut 1, quelle que soit la valeur de `change` :
//      NIVEAUX les compte.
//
// Entree : un entier N, puis N lignes `presse lie` (chacun vaut 0 ou 1).
// Sortie : une ligne par image (son numero depuis 1, puis `courant`, `change` et `actif`), puis
// `FRONTS n` et `NIVEAUX n`.

#include <iostream>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    int precedent = 0;   // le `courant` de l'image d'avant ; 0 avant la premiere image
    int fronts = 0;      // les images ou l'action est saisie : courant ET change
    int niveaux = 0;     // les images ou l'action est tenue : courant

    for (int image = 1; image <= nombre; ++image) {
        int presse = 0;
        int lie = 0;
        if (!(std::cin >> presse >> lie)) {
            break;
        }

        const int actif = (lie != 0) ? 1 : 0;
        const int courant = (actif == 1 && presse != 0) ? 1 : 0;   // inactive : toujours faux
        const int change = (courant != precedent) ? 1 : 0;         // sur courant, pas sur presse
        precedent = courant;

        if (courant == 1 && change == 1) {
            ++fronts;
        }
        if (courant == 1) {
            ++niveaux;
        }
        std::cout << image << ' ' << courant << ' ' << change << ' ' << actif << '\n';
    }

    std::cout << "FRONTS " << fronts << '\n';
    std::cout << "NIVEAUX " << niveaux << '\n';
    return 0;
}
