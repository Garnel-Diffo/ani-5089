// actions.h : l'etat d'une action booleenne, a trois champs, et la synchronisation qui le calcule.
//
// Une action n'est pas un bouton : c'est une intention (saisir, tirer, avancer) qu'on declare,
// et que le systeme relie, ou non, a un organe du materiel. Son etat, a chaque image, tient en
// trois champs :
//
//   - current (courant) : l'etat de l'action a cette image. C'est le niveau : tenir un objet se
//     teste sur `current`.
//   - changed (change)  : l'etat a bascule depuis la synchronisation precedente. C'est ce qui
//     distingue un front d'un niveau : saisir au moment de l'appui se teste sur
//     `current && changed`, lacher au moment du relachement sur `!current && changed`.
//   - active (actif)    : une source materielle est liee a l'action. Une action inactive rend
//     `current` faux, et rien ne la distingue alors d'un bouton relache, sauf ce champ.
//
// On synchronise une fois par image, a un endroit precis de la boucle, puis on interroge : on
// consomme l'etat a un instant precis, pas n'importe ou.

#ifndef ACTIONS_H
#define ACTIONS_H

#include <string>

// L'etat d'une action booleenne : les trois champs, rien d'autre.
struct EtatActionBool {
    bool current = false;   // courant : le niveau
    bool changed = false;   // change  : a bascule depuis la synchronisation precedente
    bool active = false;    // actif   : une source materielle est liee

    // Les memes champs, sous leurs noms francais, pour le banc.
    bool courant() const { return current; }
    bool change() const { return changed; }
    bool actif() const { return active; }
};

// Une touche : la source materielle du banc, que le banc enfonce ou relache a la main.
struct Touche {
    bool pressee = false;
};

// Une action declaree : son nom (qui ne sert qu'au diagnostic), la source a laquelle elle est
// liee (nullptr : rien n'est lie), son etat, et le `current` de la synchronisation precedente.
struct ActionBool {
    std::string nom;
    const Touche *source = nullptr;
    EtatActionBool etat;
    bool precedent = false;   // 0 avant la premiere synchronisation
};

// Synchronise une action : a appeler une fois par image, avant d'interroger son etat.
//   1. active   : une source est liee.
//   2. current  : l'etat de la source si l'action est active, FAUX sinon.
//   3. changed  : current differe de celui de la synchronisation precedente. On compare l'etat de
//      l'ACTION, pas celui de la touche : une action qui devient inactive produit donc un
//      `changed`, comme un relachement.
inline void Synchronise(ActionBool &action) {
    action.etat.active = (action.source != nullptr);
    action.etat.current = action.etat.active && action.source->pressee;
    action.etat.changed = (action.etat.current != action.precedent);
    action.precedent = action.etat.current;
}

#endif // ACTIONS_H
