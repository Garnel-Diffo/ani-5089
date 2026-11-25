// Exercice 2 : une trame commencee doit etre terminee.
//
// Le programme suit les trois gestes de la trame et dit ou la boucle se bloque. Le systeme compte
// les trames : il en attend une fin pour chaque debut, et regle son rythme dessus. Une trame
// commencee et jamais finie le laisse en attente.
//
// La trame passe par trois etats : REPOS, ATTENDUE, COMMENCEE. On demarre au REPOS.
//   1. ATTEND  au REPOS       : on passe a ATTENDUE, verdict OK.
//   2. ATTEND  en ATTENDUE    : faute DEJA ATTENDUE, l'etat ne change pas.
//   3. ATTEND  en COMMENCEE   : faute TRAME EN COURS, l'etat ne change pas. C'est le blocage.
//   4. COMMENCE en ATTENDUE   : on passe a COMMENCEE, verdict OK.
//   5. COMMENCE au REPOS      : faute PAS ATTENDUE, l'etat ne change pas.
//   6. COMMENCE en COMMENCEE  : faute DEJA COMMENCEE, l'etat ne change pas.
//   7. FINIT   en COMMENCEE   : on revient au REPOS, verdict OK.
//   8. FINIT   dans les deux autres etats : faute PAS COMMENCEE, l'etat ne change pas.
//   9. FAUTES compte toutes les fautes. BLOQUE retient le numero du PREMIER TRAME EN COURS, et
//      vaut 0 s'il n'y en a aucun.
//
// Une faute ne change jamais l'etat : c'est ce qui permet au geste suivant d'etre correct.
//
// Entree : un entier N, puis N lignes, chacune portant un geste : ATTEND, COMMENCE ou FINIT.
// Sortie : une ligne par geste (son numero depuis 1, le geste, puis OK ou la faute), puis
// `FAUTES n` et `BLOQUE i`.

#include <iostream>
#include <string>

namespace {

enum class Etat { Repos, Attendue, Commencee };

} // namespace

int main() {
    int nombre = 0;
    std::cin >> nombre;

    Etat etat = Etat::Repos;
    int fautes = 0;
    int bloque = 0;   // le numero du premier TRAME EN COURS ; 0 s'il n'y en a pas eu

    for (int i = 1; i <= nombre; ++i) {
        std::string geste;
        if (!(std::cin >> geste)) {
            break;
        }

        const char *faute = nullptr;   // reste nullptr si le geste est accepte
        if (geste == "ATTEND") {
            if (etat == Etat::Repos) {
                etat = Etat::Attendue;
            } else if (etat == Etat::Attendue) {
                faute = "DEJA ATTENDUE";
            } else {
                faute = "TRAME EN COURS";
            }
        } else if (geste == "COMMENCE") {
            if (etat == Etat::Attendue) {
                etat = Etat::Commencee;
            } else if (etat == Etat::Repos) {
                faute = "PAS ATTENDUE";
            } else {
                faute = "DEJA COMMENCEE";
            }
        } else if (geste == "FINIT") {
            if (etat == Etat::Commencee) {
                etat = Etat::Repos;
            } else {
                faute = "PAS COMMENCEE";
            }
        }

        std::cout << i << ' ' << geste << ' ';
        if (faute == nullptr) {
            std::cout << "OK\n";
        } else {
            std::cout << faute << '\n';
            ++fautes;
            if (bloque == 0 && std::string(faute) == "TRAME EN COURS") {
                bloque = i;   // on garde le PREMIER blocage, pas le dernier
            }
        }
    }

    std::cout << "FAUTES " << fautes << '\n';
    std::cout << "BLOQUE " << bloque << '\n';
    return 0;
}
