// Exercice 7 : ce que la couche quad fait gagner au texte.
//
// Le programme compare la nettete d'un panneau dessine dans la scene et de la meme chose en
// couche quad. Les menus systeme d'un casque sont toujours plus lisibles que le texte des
// applications : ce n'est pas de la magie, c'est une couche. Les rendements sont en milliemes,
// entiers : un rendement de 1000 ne perd rien.
//
// La regle :
//   1. Dans la scene, le texte traverse trois etapes. A chaque etape on multiplie par le
//      rendement, puis on divise par 1000, et on TRONQUE avant l'etape suivante.
//   2. En couche quad, le texte garde ses pixels d'origine : la couche est composee en dernier,
//      a pleine resolution.
//   3. Un texte est LISIBLE quand sa hauteur est superieure ou egale au seuil, et FLOU sinon.
//   4. GAGNES compte les panneaux FLOU dans la scene et LISIBLE en couche quad.
//   5. PERDUS compte les panneaux LISIBLE dans la scene et FLOU en couche quad. Ce compte vaut
//      toujours 0, et la ligne s'affiche quand meme : c'est ce qui rend le gain lisible.
//
// Multiplier les trois rendements d'abord et diviser une seule fois donnerait un autre resultat :
// la regle tronque a chaque etape. Un texte exactement au seuil est LISIBLE, pas FLOU.
//
// Entree : une ligne `rendu deformation reprojection seuil` (les trois rendements de la chaine
// de la scene, puis la hauteur minimale en pixels pour qu'un texte soit lisible), puis un entier
// N, et N lignes `nom pixels` (la hauteur du texte en pixels dans la texture).
// Sortie : une ligne par panneau (le nom, la hauteur en pixels dans la scene, celle en couche
// quad, puis les deux verdicts separes par un espace), puis `GAGNES n` et `PERDUS n`.

#include <iostream>
#include <string>

namespace {

const char *Verdict(long long hauteur, long long seuil) {
    return hauteur >= seuil ? "LISIBLE" : "FLOU";   // comparaison large : le seuil lui-meme passe
}

} // namespace

int main() {
    long long rendu = 0;
    long long deformation = 0;
    long long reprojection = 0;
    long long seuil = 0;
    std::cin >> rendu >> deformation >> reprojection >> seuil;

    int nombre = 0;
    std::cin >> nombre;

    int gagnes = 0;
    int perdus = 0;
    for (int i = 0; i < nombre; ++i) {
        std::string nom;
        long long pixels = 0;
        if (!(std::cin >> nom >> pixels)) {
            break;
        }

        // Les trois etapes de la scene, dans l'ordre de lecture, en tronquant apres chacune.
        long long scene = pixels;
        scene = scene * rendu / 1000;
        scene = scene * deformation / 1000;
        scene = scene * reprojection / 1000;
        const long long quad = pixels;   // la couche quad garde ses pixels d'origine

        const bool flouDansLaScene = scene < seuil;
        const bool flouEnQuad = quad < seuil;
        if (flouDansLaScene && !flouEnQuad) {
            ++gagnes;
        }
        if (!flouDansLaScene && flouEnQuad) {
            ++perdus;
        }
        std::cout << nom << ' ' << scene << ' ' << quad << ' ' << Verdict(scene, seuil) << ' '
                  << Verdict(quad, seuil) << '\n';
    }

    std::cout << "GAGNES " << gagnes << '\n';
    std::cout << "PERDUS " << perdus << '\n';
    return 0;
}
