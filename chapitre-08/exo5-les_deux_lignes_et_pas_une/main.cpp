// Exercice 5 : les deux lignes, et pas une.
//
// Relier un renderer a une cible hors ecran demande DEUX appels :
//   rendu->SetFinalColorTarget(poigneeDeLaCible);              // le premier dit OU dessiner
//   rendu->SetRenderSizeOverride(largeurCible, hauteurCible);  // le second dit EN QUELLE TAILLE
// Sans le second, le renderer garde la taille de la fenetre et la passe s'execute sur une zone plus grande
// que l'image visee : le travail ne se termine jamais, et le programme se fige sans rien dire.
//
// Le programme repere les cibles a moitie reliees et dit lesquelles bloqueront.
//
// La regle :
//   1. Si `cible` vaut 0, la passe s'execute a la taille de la FENETRE et le verdict est PAS RELIEE.
//   2. Si `cible` et `taille` valent 1, la passe s'execute en `lc hc` et le verdict est OK.
//   3. Si `cible` vaut 1 et `taille` vaut 0, la passe s'execute a la taille de la FENETRE. Le verdict est
//      BLOQUE si cette taille differe de celle de la cible (une seule dimension differente suffit), et
//      SANS EFFET si elle est la meme : c'est la version dangereuse, qui marche jusqu'au jour ou
//      quelqu'un change la taille, et elle n'est PAS comptee comme bloquante.
//   4. BLOQUANTES compte les verdicts BLOQUE.
// Le cas `cible` a 0 est teste avant tout le reste.
//
// Entree : une ligne `largeur hauteur` (la fenetre), un entier N, puis N lignes `nom cible taille lc hc`.
// Sortie : une ligne par cible (nom, taille reellement utilisee, verdict), puis `BLOQUANTES n`.

#include <iostream>
#include <string>

int main() {
    long long largeur = 0;
    long long hauteur = 0;
    int nombre = 0;
    std::cin >> largeur >> hauteur >> nombre;

    int bloquantes = 0;

    for (int i = 0; i < nombre; ++i) {
        std::string nom;
        int cible = 0;
        int taille = 0;
        long long lc = 0;
        long long hc = 0;
        if (!(std::cin >> nom >> cible >> taille >> lc >> hc)) {
            break;
        }

        if (cible == 0) {
            // Rien n'est relie : la passe dessine dans la fenetre.
            std::cout << nom << ' ' << largeur << ' ' << hauteur << " PAS RELIEE\n";
        } else if (taille == 1) {
            // Les deux appels sont la : la passe s'execute a la taille de la cible.
            std::cout << nom << ' ' << lc << ' ' << hc << " OK\n";
        } else {
            // La cible est posee sans la taille : la passe garde la taille de la fenetre.
            const bool differe = (largeur != lc) || (hauteur != hc);
            std::cout << nom << ' ' << largeur << ' ' << hauteur << (differe ? " BLOQUE" : " SANS EFFET") << '\n';
            if (differe) {
                ++bloquantes;
            }
        }
    }

    std::cout << "BLOQUANTES " << bloquantes << '\n';
    return 0;
}
