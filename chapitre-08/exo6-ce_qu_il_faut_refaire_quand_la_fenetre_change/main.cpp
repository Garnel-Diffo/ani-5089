// Exercice 6 : ce qu'il faut refaire quand la fenetre change.
//
// Redimensionner les cibles ne suffit pas : la chaine garde des poignees qui ne pointent plus vers les bonnes
// images. Il faut la DETRUIRE, la RECREER, et lui REENREGISTRER ses images. Le programme dit si un programme
// survit a un redimensionnement.
//
// Chaque ligne d'entree decrit ce que fait un programme au redimensionnement : son nom, puis une suite de mots
// parmi REDIMENSIONNE, DETRUIT, RECREE et REENREGISTRE. Une ligne peut porter un seul mot apres le nom, ou
// quatre : on lit donc la ligne ENTIERE, pas un nombre fixe de mots.
//
// Les regles, dans cet ordre :
//   1. DETRUIT et RECREE sont tous deux la, mais RECREE vient avant DETRUIT : MAUVAIS ORDRE (on detruit ce
//      qu'on vient de creer). Ce test passe avant tous les autres, meme si REENREGISTRE est la.
//   2. Sinon, si DETRUIT ou RECREE manque : CHAINE PERIMEE.
//   3. Sinon, si REENREGISTRE manque : POIGNEES PERDUES.
//   4. Sinon : SURVIT.
//   5. REDIMENSIONNE ne suffit jamais a lui seul, et son absence n'est pas testee : on l'ignore.
//
// Entree : un entier N, puis N lignes `nom mot mot ...`.
// Sortie : une ligne par programme (le nom, SURVIT ou la raison), puis `SURVIVANTS n`.

#include <iostream>
#include <sstream>
#include <string>

namespace {

// Enleve le retour chariot d'une fin de ligne Windows, s'il y en a un.
void EnleverRetourChariot(std::string &ligne) {
    while (!ligne.empty() && (ligne.back() == '\r' || ligne.back() == '\n')) {
        ligne.pop_back();
    }
}

} // namespace

int main() {
    std::string ligne;
    if (!std::getline(std::cin, ligne)) {
        std::cout << "SURVIVANTS 0\n";
        return 0;
    }
    int nombre = 0;
    {
        std::istringstream flux(ligne);
        flux >> nombre;
    }

    int survivants = 0;
    int lus = 0;
    while (lus < nombre && std::getline(std::cin, ligne)) {
        EnleverRetourChariot(ligne);
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) {
            continue;   // ligne vide : ce n'est pas un programme
        }
        ++lus;

        // Position (dans la ligne) du premier exemplaire de chaque mot qui compte, ou -1 s'il manque.
        int positionDetruit = -1;
        int positionRecree = -1;
        bool reenregistre = false;
        int position = 0;
        std::string mot;
        while (flux >> mot) {
            if (mot == "DETRUIT" && positionDetruit < 0) {
                positionDetruit = position;
            } else if (mot == "RECREE" && positionRecree < 0) {
                positionRecree = position;
            } else if (mot == "REENREGISTRE") {
                reenregistre = true;
            }
            // REDIMENSIONNE (ou tout autre mot) : ignore.
            ++position;
        }

        const bool detruit = positionDetruit >= 0;
        const bool recree = positionRecree >= 0;

        std::cout << nom << ' ';
        if (detruit && recree && positionRecree < positionDetruit) {
            std::cout << "MAUVAIS ORDRE\n";
        } else if (!detruit || !recree) {
            std::cout << "CHAINE PERIMEE\n";
        } else if (!reenregistre) {
            std::cout << "POIGNEES PERDUES\n";
        } else {
            std::cout << "SURVIT\n";
            ++survivants;
        }
    }

    std::cout << "SURVIVANTS " << survivants << '\n';
    return 0;
}
