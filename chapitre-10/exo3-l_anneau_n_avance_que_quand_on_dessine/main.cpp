// Exercice 3 : l'anneau n'avance que quand on dessine.
//
// Le programme refait le banc des six trames du chapitre. Les trames sautees n'ont rien emprunte,
// donc rien rendu : c'est coherent, et c'est ce qui permet de s'arreter sans rien perdre.
//
// La regle :
//   1. L'anneau commence a l'index 0.
//   2. Quand on dessine, on emprunte l'index courant, puis l'index avance de 1 et revient a 0
//      apres le dernier.
//   3. Quand on ne dessine pas, l'index N'AVANCE PAS et on affiche `-`.
//   4. Le nombre de couches soumises vaut 1 quand on dessine, 0 sinon.
//   5. RENDUES compte les trames dessinees. FINS vaut le nombre total de trames : la fin de
//      trame est appelee a chaque fois, meme sans rendu.
//   6. Si l'anneau a zero image, l'index reste 0.
//
// Entree : une ligne `images N` (le nombre d'images de l'anneau, puis le nombre de trames), puis
// N lignes valant 0 ou 1 : faut-il dessiner a cette trame.
// Sortie : une ligne par trame (son numero depuis 1, OUI ou NON, le nombre de couches soumises,
// puis l'index d'image employe, ou `-` si l'on n'a rien dessine), puis `RENDUES n` et `FINS n`.

#include <iostream>

int main() {
    int images = 0;
    int trames = 0;
    std::cin >> images >> trames;

    int index = 0;     // l'image sur laquelle pointe l'anneau
    int rendues = 0;   // les trames dessinees
    int fins = 0;      // les fins de trame : une par trame, dessinee ou non

    for (int trame = 1; trame <= trames; ++trame) {
        int dessiner = 0;
        if (!(std::cin >> dessiner)) {
            break;
        }

        if (dessiner != 0) {
            std::cout << trame << " OUI 1 " << index << '\n';
            ++rendues;
            if (images > 0) {
                index = (index + 1) % images;   // l'anneau n'avance que sur une trame rendue
            }
        } else {
            std::cout << trame << " NON 0 -\n";   // rien emprunte : l'index reste ou il est
        }
        ++fins;   // la fin de trame est toujours appelee
    }

    std::cout << "RENDUES " << rendues << '\n';
    std::cout << "FINS " << fins << '\n';
    return 0;
}
