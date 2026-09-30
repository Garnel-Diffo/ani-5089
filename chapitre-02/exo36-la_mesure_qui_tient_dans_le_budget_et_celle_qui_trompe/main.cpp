// Chapitre 2, exercice 36 : la mesure qui tient dans le budget, et celle qui trompe.
//
// Entree :
//   budget          (microsecondes)
//   S
//   S lignes "nom debug release"   (microsecondes)
//
// Sortie : une ligne par scene "nom facteur TIENT|DEPASSE", puis "TROMPE n".
#include <iostream>
#include <string>

int main() {
    long long budget;
    if (!(std::cin >> budget)) return 1;

    int s;
    std::cin >> s;

    int trompeuses = 0;
    for (int i = 0; i < s; ++i) {
        std::string nom;
        long long debug, release;
        std::cin >> nom >> debug >> release;

        long long facteur = (debug + release / 2) / release;   // division entiere, arrondie
        bool tient = release <= budget;
        std::cout << nom << " " << facteur << " " << (tient ? "TIENT" : "DEPASSE") << "\n";

        if (debug > budget && release <= budget) ++trompeuses;
    }
    std::cout << "TROMPE " << trompeuses << "\n";
    return 0;
}
