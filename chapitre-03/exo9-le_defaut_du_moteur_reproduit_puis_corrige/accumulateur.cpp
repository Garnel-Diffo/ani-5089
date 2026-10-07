// Accumulateur de souris correct : on ajoute a chaque evenement brut,
// et on consomme (lit puis remet a zero) une seule fois par image.
//
// A comparer avec le moteur fautif (NkEventState.h) qui ECRASE le champ
// au lieu d'y ajouter, et ne le remet jamais a zero : les mouvements
// intermediaires entre deux images sont perdus, et la derive continue
// meme quand la souris s'arrete.
#include "NKWindow/NkWindow.h"

using namespace nkentseu;

static NkI32 g_accumulateurX = 0;
static NkI32 g_accumulateurY = 0;

// Callback sur l'evenement brut : on AJOUTE, on n'ecrase pas.
void SurMouvementBrut(NkMouseRawEvent* e) {
    g_accumulateurX += e->GetDeltaX();
    g_accumulateurY += e->GetDeltaY();
}

void EnregistrerAccumulateur(EventSystem& es) {
    es.SetEventCallback<NkMouseRawEvent>(SurMouvementBrut);
}

// Consommation une fois par image : on lit le total, puis on le remet
// a zero, meme si rien n'a bouge entre les deux images precedentes.
void ConsommerAccumulateurParImage(NkI32& dx, NkI32& dy) {
    dx = g_accumulateurX;
    dy = g_accumulateurY;

    g_accumulateurX = 0;
    g_accumulateurY = 0;
}
