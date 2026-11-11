// Exercice 2 : les quatre temps, dans cet ordre.
//
// Le programme verifie qu'une trace d'appels respecte la discipline de la chaine d'echange. La chaine garde
// DEUX etats, qui ne sont pas le meme :
//   - `acquise`  : une image est acquise (empruntee, pas encore rendue) ;
//   - `enAttente` : une image rendue attend sa soumission.
//
// Les regles :
//   ACQUIERT echoue si une image est deja acquise : DEJA ACQUISE. Sinon l'image devient acquise.
//   DESSINE  echoue si aucune image n'est acquise : PAS ACQUISE. Sinon il ne change rien.
//   REND     echoue si aucune image n'est acquise : RIEN ACQUIS. Sinon l'image cesse d'etre acquise
//            et attend sa soumission.
//   SOUMET   echoue si aucune image n'attend sa soumission : PAS RENDUE. Sinon l'attente est levee.
//   Un appel refuse ne change aucun des deux etats. FAUTES compte les appels refuses.
//
// Une image peut etre rendue sans jamais etre soumise : cela ne bloque pas l'acquisition suivante, expres.
//
// Entree : un entier N, puis N lignes ACQUIERT, DESSINE, REND ou SOUMET.
// Sortie : une ligne par appel (numero du tour depuis 1, l'appel, OK ou la raison), puis `FAUTES n`.

#include <iostream>
#include <string>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    bool acquise = false;     // une image est acquise
    bool enAttente = false;   // une image rendue attend sa soumission
    int fautes = 0;           // nombre d'appels refuses

    for (int tour = 1; tour <= nombre; ++tour) {
        std::string appel;
        if (!(std::cin >> appel)) {
            break;
        }

        const char *raison = nullptr;   // reste nullptr si l'appel est accepte
        if (appel == "ACQUIERT") {
            if (acquise) {
                raison = "DEJA ACQUISE";
            } else {
                acquise = true;
            }
        } else if (appel == "DESSINE") {
            if (!acquise) {
                raison = "PAS ACQUISE";
            }
        } else if (appel == "REND") {
            if (!acquise) {
                raison = "RIEN ACQUIS";
            } else {
                acquise = false;
                enAttente = true;
            }
        } else if (appel == "SOUMET") {
            if (!enAttente) {
                raison = "PAS RENDUE";
            } else {
                enAttente = false;
            }
        }

        std::cout << tour << ' ' << appel << ' ';
        if (raison == nullptr) {
            std::cout << "OK\n";
        } else {
            std::cout << raison << '\n';
            ++fautes;
        }
    }

    std::cout << "FAUTES " << fautes << '\n';
    return 0;
}
