// Exercice 5 : la couche que le simulateur refuse en silence.
//
// Le programme refait le controle que fait le simulateur sur ce qu'on lui soumet. Un bon
// simulateur est plus severe que le vrai materiel, pas plus permissif : ce controle-la attrape
// une faute qui, autrement, ne se voit que dans un casque.
//
// La regle, dans cet ordre :
//   1. Si la trame n'a pas ete commencee, la fin de trame rend faux : verdict REFUSEE. Aucun
//      autre controle n'a lieu.
//   2. Si l'on ne devait pas dessiner et qu'aucune chaine n'est presente : OK. C'est la trame
//      du casque pose.
//   3. Si l'on ne devait pas dessiner mais qu'au moins une chaine est presente : COUCHE SANS
//      RENDU.
//   4. Si l'on devait dessiner et que les deux chaines sont presentes : OK.
//   5. Si l'on devait dessiner et qu'aucune chaine n'est presente : SOUMISSION VIDE.
//   6. Si l'on devait dessiner et qu'il en manque une seule : OEIL NOIR G quand la gauche manque,
//      OEIL NOIR D quand la droite manque.
//   7. REFUSEES compte les verdicts REFUSEE. AVERTIES compte les autres verdicts qui ne sont pas
//      OK : une trame refusee n'est pas avertie, les deux comptes sont separes.
//
// Une trame sans rendu et sans chaine est correcte, pas fautive : c'est le cas normal du casque
// pose.
//
// Entree : un entier N, puis N lignes `nom commence rend gauche droite`. Les quatre derniers
// champs valent 0 ou 1 : la trame a-t-elle ete commencee, fallait-il dessiner, et chacune des
// deux chaines d'oeil est-elle presente dans la couche soumise.
// Sortie : une ligne par trame (le nom, puis le verdict), puis `REFUSEES n` et `AVERTIES n`.

#include <iostream>
#include <string>

int main() {
    int nombre = 0;
    std::cin >> nombre;

    int refusees = 0;
    int averties = 0;
    for (int i = 0; i < nombre; ++i) {
        std::string nom;
        int commence = 0;
        int rend = 0;
        int gauche = 0;
        int droite = 0;
        if (!(std::cin >> nom >> commence >> rend >> gauche >> droite)) {
            break;
        }

        const char *verdict = "OK";
        if (commence == 0) {
            // Le debut de trame se teste en premier : plus aucun autre controle n'a lieu.
            verdict = "REFUSEE";
            ++refusees;
        } else if (rend == 0) {
            if (gauche != 0 || droite != 0) {
                verdict = "COUCHE SANS RENDU";
                ++averties;
            }   // sinon : le casque pose, correct
        } else if (gauche == 0 && droite == 0) {
            verdict = "SOUMISSION VIDE";
            ++averties;
        } else if (gauche == 0) {
            verdict = "OEIL NOIR G";
            ++averties;
        } else if (droite == 0) {
            verdict = "OEIL NOIR D";
            ++averties;
        }
        std::cout << nom << ' ' << verdict << '\n';
    }

    std::cout << "REFUSEES " << refusees << '\n';
    std::cout << "AVERTIES " << averties << '\n';
    return 0;
}
