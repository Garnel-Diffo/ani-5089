// Exercice 2 : tirer, tenir, et l'objet qui tremble.
//
// Le programme compare ce que fait un jeu selon qu'il teste le front ou le niveau de la gachette,
// sur la meme suite d'etats.
//
// La regle :
//   1. Le front est une image ou l'etat presse passe de 0 a 1 ; a la premiere image, on compare
//      avec 0.
//   2. Le niveau est une image ou l'etat presse vaut 1.
//   3. Un jeu qui tire sur le front tire une balle par front ; un jeu qui tire sur le niveau tire
//      une balle par image de niveau.
//   4. Un jeu qui tient sur le niveau tient l'objet a chaque image de niveau ; un jeu qui tient
//      sur le front ne le tient que sur l'image du front, et l'objet lui echappe ensuite.
//
// Les deux derniers nombres comptent le MEME objet tenu de deux facons, pas deux objets. Un
// relachement n'est pas un front montant : seul le passage de 0 a 1 compte.
//
// Entree : un entier N, puis N lignes valant 0 ou 1 (l'etat presse de la gachette a cette image).
// Sortie : quatre lignes, dans cet ordre : `BALLES FRONT n`, `BALLES NIVEAU n`, `TENU NIVEAU n`,
// `TENU FRONT n`.

#include <iostream>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    int precedent = 0;   // l'etat presse de l'image d'avant ; 0 avant la premiere image
    int fronts = 0;      // les images ou l'etat passe de 0 a 1
    int niveaux = 0;     // les images ou l'etat vaut 1

    for (int image = 0; image < nombre; ++image) {
        int presse = 0;
        if (!(std::cin >> presse)) {
            break;
        }
        const int courant = (presse != 0) ? 1 : 0;
        if (courant == 1 && precedent == 0) {
            ++fronts;
        }
        if (courant == 1) {
            ++niveaux;
        }
        precedent = courant;
    }

    // Tirer sur le front donne autant de balles que de fronts, tirer sur le niveau autant que de
    // niveaux ; tenir sur le niveau tient autant d'images que de niveaux, tenir sur le front autant
    // que de fronts.
    std::cout << "BALLES FRONT " << fronts << '\n';
    std::cout << "BALLES NIVEAU " << niveaux << '\n';
    std::cout << "TENU NIVEAU " << niveaux << '\n';
    std::cout << "TENU FRONT " << fronts << '\n';
    return 0;
}
