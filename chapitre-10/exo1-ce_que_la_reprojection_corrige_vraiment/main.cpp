// Exercice 1 : ce que la reprojection corrige vraiment.
//
// Le programme mesure l'erreur qui reste a l'ecran selon la pose qu'on a soumise. Soumettre la
// pose la plus fraiche parait plus juste, et ne corrige rien du tout : la couche doit porter la
// pose AVEC LAQUELLE on a dessine, pas celle d'apres. Tous les angles sont en centiemes de
// degre, entiers.
//
// La regle :
//   1. SANS est l'erreur qu'on aurait sans reprojection : rendu - reel.
//   2. Le compositeur applique une correction de reel - soumis.
//   3. L'image corrigee parait donc vue depuis rendu + (reel - soumis), et l'erreur qui reste
//      vaut AVEC = rendu - soumis.
//   4. Le verdict se lit sur les valeurs ABSOLUES :
//        - AVEC vaut 0 : JUSTE ;
//        - sinon, si |AVEC| est plus petit que |SANS| : PARTIEL ;
//        - sinon, s'il est egal : SANS EFFET ;
//        - sinon : PIRE.
//   5. JUSTES compte les verdicts JUSTE, PIRES les verdicts PIRE.
//
// SANS et AVEC peuvent etre negatifs ; seul le verdict se decide sur les valeurs absolues. Quand
// la tete n'a pas bouge, SANS vaut 0 : toute erreur non nulle est alors PIRE.
//
// Entree : un entier N, puis N lignes `rendu reel soumis` (l'angle de la tete quand on a dessine,
// celui au moment de l'affichage, celui qu'on a mis dans la couche).
// Sortie : une ligne par cas (`SANS x AVEC y`, puis le verdict), puis `JUSTES n` et `PIRES n`.

#include <cstdlib>
#include <iostream>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    int justes = 0;
    int pires = 0;
    for (int i = 0; i < nombre; ++i) {
        long long rendu = 0;
        long long reel = 0;
        long long soumis = 0;
        if (!(std::cin >> rendu >> reel >> soumis)) {
            break;
        }

        const long long sans = rendu - reel;     // l'erreur sans reprojection
        const long long avec = rendu - soumis;   // l'erreur qui reste apres la correction

        const char *verdict = "PIRE";
        if (avec == 0) {
            verdict = "JUSTE";                   // AVEC nul avant tout autre test
            ++justes;
        } else if (std::llabs(avec) < std::llabs(sans)) {
            verdict = "PARTIEL";
        } else if (std::llabs(avec) == std::llabs(sans)) {
            verdict = "SANS EFFET";
        } else {
            ++pires;
        }
        std::cout << "SANS " << sans << " AVEC " << avec << ' ' << verdict << '\n';
    }

    std::cout << "JUSTES " << justes << '\n';
    std::cout << "PIRES " << pires << '\n';
    return 0;
}
