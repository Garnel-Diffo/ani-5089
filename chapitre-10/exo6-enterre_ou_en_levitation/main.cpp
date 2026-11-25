// Exercice 6 : enterre, ou en levitation.
//
// Le programme calcule le decalage cause par un melange d'espaces. Melanger les deux ne provoque
// aucune erreur : vous vous retrouvez enterre dans le sol, ou en levitation a un metre soixante-
// dix. Les hauteurs sont en millimetres, entieres.
//
// La regle :
//   1. STAGE a son origine au plancher. LOCAL a la sienne a la hauteur des yeux.
//   2. Si les deux espaces sont les memes, le decalage vaut 0 et le verdict est OK.
//   3. Poses en STAGE declarees en LOCAL : la scene monte de la hauteur des yeux. Decalage
//      positif, verdict LEVITATION.
//   4. Poses en LOCAL declarees en STAGE : la scene descend d'autant. Decalage negatif, verdict
//      ENTERRE.
//   5. DECALEES compte les cas dont le verdict n'est pas OK.
//   6. Une hauteur de zero donne un decalage de zero, mais le verdict reste LEVITATION ou
//      ENTERRE : c'est le melange des espaces qui le decide, pas la valeur. De meme, si N vaut
//      zero, aucune ligne de cas ne s'affiche et seule la ligne `DECALEES 0` sort.
//
// Un decalage positif s'ecrit sans le signe plus.
//
// Entree : une ligne avec la hauteur des yeux au-dessus du plancher, puis un entier N, puis N
// lignes `nom espacePose espaceCouche` (chaque espace vaut STAGE ou LOCAL).
// Sortie : une ligne par cas (le nom, le decalage en millimetres avec son signe, puis OK,
// LEVITATION ou ENTERRE), puis `DECALEES n`.

#include <iostream>
#include <string>

int main() {
    long long hauteur = 0;
    int nombre = 0;
    std::cin >> hauteur >> nombre;

    int decalees = 0;
    for (int i = 0; i < nombre; ++i) {
        std::string nom;
        std::string espacePose;
        std::string espaceCouche;
        if (!(std::cin >> nom >> espacePose >> espaceCouche)) {
            break;
        }

        long long decalage = 0;
        const char *verdict = "OK";
        if (espacePose != espaceCouche) {
            if (espacePose == "STAGE") {
                decalage = hauteur;    // la scene monte de la hauteur des yeux
                verdict = "LEVITATION";
            } else {
                decalage = -hauteur;   // la scene descend d'autant
                verdict = "ENTERRE";
            }
            ++decalees;   // le melange decide du verdict, meme quand la hauteur est nulle
        }
        std::cout << nom << ' ' << decalage << ' ' << verdict << '\n';
    }

    std::cout << "DECALEES " << decalees << '\n';
    return 0;
}
