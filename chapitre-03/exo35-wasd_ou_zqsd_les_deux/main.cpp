#include <iostream>
#include <sstream>
#include <string>

int main() {
    int n;
    if (!(std::cin >> n)) return 0;
    std::cin.ignore(); // consomme le saut de ligne apres l'entier

    for (int i = 0; i < n; ++i) {
        std::string ligne;
        std::getline(std::cin, ligne);

        bool avant = false, arriere = false, gauche = false, droite = false;
        std::istringstream iss(ligne);
        std::string touche;
        while (iss >> touche) {
            if (touche == "W" || touche == "Z") avant = true;
            else if (touche == "S") arriere = true;
            else if (touche == "A" || touche == "Q") gauche = true;
            else if (touche == "D") droite = true;
        }

        int pas = (avant ? 1 : 0) - (arriere ? 1 : 0);
        int cote = (droite ? 1 : 0) - (gauche ? 1 : 0);
        std::cout << pas << " " << cote << "\n";
    }

    return 0;
}
