// Exercice 5 : la zone morte du manche.
//
// Le programme transforme la valeur brute d'un manche en vitesse de deplacement. Un manche
// relache n'envoie jamais exactement zero : sans precaution, le joueur derive tout seul dans le
// decor, ce qui, en realite virtuelle, est le plus court chemin vers la nausee.
//
// Les valeurs du manche et les vitesses sont en milliemes, et ce sont des entiers : un manche
// pousse a fond vaut 1000. On calcule donc en entiers (long long, pour ne pas deborder).
//
// La regle :
//   1. Si la valeur absolue du manche est STRICTEMENT inferieure au seuil, la vitesse est 0 et le
//      tour compte dans MORTS.
//   2. Sinon, on rescale la course utile : on retire le seuil (reste = |valeur| - seuil), et on
//      rapporte ce qui reste a la plage qui va du seuil a 1000.
//   3. Le resultat est ensuite eleve au carre, pour plus de precision pres du seuil, puis
//      multiplie par le maximum :
//          vitesse = maximum * reste * reste / ((1000 - seuil) * (1000 - seuil)).
//   4. La vitesse garde le signe du manche.
//   5. Toutes les divisions sont entieres, et on multiplie AVANT de diviser : diviser d'abord
//      donnerait zero partout.
//
// Un seuil de 0 n'enleve rien (rien n'est strictement inferieur a 0) : c'est le reglage d'un
// clavier. Une valeur egale au seuil n'est pas morte : son reste vaut 0, donc sa vitesse aussi,
// mais un tour a vitesse nulle et un tour mort sont deux choses differentes.
//
// Entree : une ligne `seuil maximum`, un entier N, puis N lignes : la valeur brute du manche a
// chaque tour (elle peut etre negative).
// Sortie : une ligne par tour (la vitesse, en milliemes), puis `MORTS n`.

#include <iostream>

int main() {
    long long seuil = 0;
    long long maximum = 0;
    int nombre = 0;
    std::cin >> seuil >> maximum >> nombre;

    const long long plage = 1000 - seuil;   // la course utile, de seuil a 1000
    int morts = 0;

    for (int tour = 0; tour < nombre; ++tour) {
        long long valeur = 0;
        if (!(std::cin >> valeur)) {
            break;
        }
        const long long absolue = valeur < 0 ? -valeur : valeur;

        long long vitesse = 0;
        if (absolue < seuil) {
            ++morts;   // zone morte : vitesse nulle, et le tour est compte
        } else if (plage > 0) {
            const long long reste = absolue - seuil;
            // On multiplie avant de diviser, et on divise des nombres positifs : le signe est
            // remis ensuite, ce qui rend la vitesse symetrique.
            vitesse = maximum * reste * reste / (plage * plage);
            if (valeur < 0) {
                vitesse = -vitesse;
            }
        }
        std::cout << vitesse << '\n';
    }

    std::cout << "MORTS " << morts << '\n';
    return 0;
}
