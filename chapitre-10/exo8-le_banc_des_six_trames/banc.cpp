// banc.cpp : le banc des six trames, sans casque ni fenetre ni carte graphique.
//
// Six trames, dont deux sans rien dessiner : la tete tourne d'un degre par trame, et le casque est
// pose sur la table pendant les trames 3 et 4 (shouldRender vaut faux). Chaque trame suit les
// gestes de la boucle, dans l'ordre : attente (WaitFrame), debut (BeginFrame), puis, s'il faut
// dessiner, localisation des vues, acquisition, rendu, liberation et remplissage de la couche ;
// et enfin la FIN DE TRAME (EndFrame), TOUJOURS, hors du test de rendu : sinon les trames 3 et 4
// resteraient ouvertes. La fin de trame est soumise quand meme : zero couche, mais une soumission.
//
// La session simulee ne compose rien. Elle valide : elle refuse un geste dans le desordre, et une
// vraie session bloquerait alors la boucle a l'attente suivante.
//
// La sortie est un tableau, une ligne par trame : son numero, OUI ou NON (a-t-on dessine), le
// nombre de couches soumises, puis l'index d'image employe, ou `-` si l'on n'a rien dessine.
//
// Compilation : clang++ -std=c++17 -Wall banc.cpp -o banc

#include "couches.h"

#include <cstdio>

namespace {

const int kTrames = 6;
const unsigned kImages = 3;               // l'anneau d'images de chaque oeil
const long long kPeriodeMicro = 13889;    // une trame a 72 hertz, en microsecondes

// Ce que WaitFrame rend : faut-il dessiner, et l'instant d'affichage PREDIT pour CETTE trame.
struct EtatDeTrame {
    bool shouldRender = false;
    long long displayTime = 0;
};

// La session simulee : un etat, trois gestes, et le refus des gestes dans le desordre.
class SessionSimulee {
public:
    // L'attente : refusee si la trame precedente a ete commencee et jamais finie.
    bool WaitFrame(int trame, EtatDeTrame &etat) {
        if (mCommencee) {
            return false;   // une vraie session bloquerait ici, et la boucle ne reprendrait jamais
        }
        etat.shouldRender = !(trame == 3 || trame == 4);   // le casque est pose pendant 3 et 4
        etat.displayTime = trame * kPeriodeMicro;
        return true;
    }

    bool BeginFrame() {
        if (mCommencee) {
            return false;
        }
        mCommencee = true;
        return true;
    }

    // La fin de trame : on y soumet les couches, zero ou plus.
    bool EndFrame(long long displayTime, const CoucheProjection *couches, int nombre) {
        (void)displayTime;
        (void)couches;
        if (!mCommencee) {
            return false;
        }
        mCommencee = false;
        mCouchesSoumises += nombre;
        ++mFins;
        return true;
    }

    int Fins() const { return mFins; }

private:
    bool mCommencee = false;
    int mCouchesSoumises = 0;
    int mFins = 0;
};

// L'anneau d'images d'un oeil : l'index n'avance que quand on RELACHE une image rendue.
struct Anneau {
    unsigned suivant = 0;
    unsigned Acquiert() const { return suivant; }
    void Relache() { suivant = (suivant + 1) % kImages; }
};

} // namespace

int main() {
    SessionSimulee session;
    Anneau anneau[2];   // un anneau par oeil, independants
    int rendues = 0;

    std::printf("image rend couches index\n");
    for (int trame = 1; trame <= kTrames; ++trame) {
        // 1. L'attente de trame.
        EtatDeTrame etat;
        if (!session.WaitFrame(trame, etat)) {
            std::printf("BLOQUE a la trame %d : la trame precedente n'a jamais ete finie\n", trame);
            return 1;
        }
        // 2. Le debut de trame.
        if (!session.BeginFrame()) {
            std::printf("FAUTE a la trame %d : debut refuse\n", trame);
            return 1;
        }

        CoucheProjection couche;
        int couches = 0;
        int index = -1;
        // 3. Le rendu, seulement s'il faut dessiner.
        if (etat.shouldRender) {
            const int lacet = 100 * trame;   // la tete tourne d'un degre par trame
            for (int oeil = 0; oeil < 2; ++oeil) {
                const unsigned image = anneau[oeil].Acquiert();   // acquisition
                // ... le rendu de l'oeil dans l'image `image` n'est pas ecrit ici ...
                anneau[oeil].Relache();                           // liberation : l'anneau avance
                couche.views[oeil].chaine = 1000ull + (unsigned long long)oeil;
                couche.views[oeil].indexImage = image;            // l'image montree
                couche.views[oeil].poseLacetCentiemes = lacet;    // la pose du RENDU
                couche.views[oeil].champCentiemes = 9000;
            }
            index = (int)couche.views[0].indexImage;
            couches = 1;
            ++rendues;
        }

        // 4. La fin de trame : TOUJOURS, hors du test de rendu, meme quand on n'a rien dessine.
        if (!session.EndFrame(etat.displayTime, couches > 0 ? &couche : nullptr, couches)) {
            std::printf("FAUTE a la trame %d : fin refusee\n", trame);
            return 1;
        }

        if (index >= 0) {
            std::printf("%d OUI %d %d\n", trame, couches, index);
        } else {
            std::printf("%d NON %d -\n", trame, couches);
        }
    }

    std::printf("RENDUES %d\n", rendues);
    std::printf("FINS %d\n", session.Fins());
    return 0;
}
