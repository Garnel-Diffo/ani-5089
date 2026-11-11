// Exercice 1 : l'anneau qui boucle.
//
// Le programme suit quelle image la chaine d'echange prete a chaque tour. L'etat tient en trois choses :
//   - le nombre d'images de l'anneau (`images`) ;
//   - `suivant`, l'index sur lequel pointe l'anneau (il part de 0) ;
//   - un seul booleen : une image est-elle deja empruntee ?
//
// La regle :
//   EMPRUNTE reussit seulement si rien n'est emprunte : il prete l'image sur laquelle pointe l'anneau
//            et marque l'emprunt. L'anneau NE bouge PAS.
//   REND     reussit seulement si quelque chose est emprunte : il rend l'image empruntee, PUIS fait avancer
//            l'anneau : suivant = (suivant + 1) % images.
//   Une commande refusee ecrit REFUSE a la place de l'index et ne change RIEN (ni l'anneau, ni le booleen).
//   PRETS compte les EMPRUNTE acceptes.
//
// Entree : une ligne `images`, un entier N, puis N lignes EMPRUNTE ou REND.
// Sortie : une ligne par commande (numero du tour depuis 1, commande, index ou REFUSE), puis `PRETS n`.

#include <iostream>
#include <string>

int main() {
    int images = 0;
    int nombre = 0;
    std::cin >> images >> nombre;

    int suivant = 0;          // l'index sur lequel pointe l'anneau
    bool empruntee = false;   // une image est-elle empruntee en ce moment ?
    int indexEmprunte = 0;    // laquelle (n'a de sens que si `empruntee` est vrai)
    int prets = 0;            // nombre d'emprunts acceptes

    for (int tour = 1; tour <= nombre; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) {
            break;
        }

        std::cout << tour << ' ' << commande << ' ';
        if (commande == "EMPRUNTE" && !empruntee) {
            empruntee = true;
            indexEmprunte = suivant;
            ++prets;
            std::cout << indexEmprunte << '\n';
        } else if (commande == "REND" && empruntee) {
            empruntee = false;
            std::cout << indexEmprunte << '\n';
            // L'anneau avance au moment de rendre, pas au moment d'emprunter.
            if (images > 0) {
                suivant = (suivant + 1) % images;
            }
        } else {
            std::cout << "REFUSE\n";
        }
    }

    std::cout << "PRETS " << prets << '\n';
    return 0;
}
