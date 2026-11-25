// couches.h : une couche de projection, ce qu'on soumet au systeme a la fin d'une trame.
//
// Le systeme peut superposer une frontiere de securite, une notification, un menu, un clavier
// virtuel. Chacune est une couche, la votre en est une, et le compositeur les empile. La
// projection porte la scene stereo, une vue par oeil : c'est votre monde.
//
// Ce qu'une couche de projection porte :
//   - l'ESPACE dans lequel les poses ont ete demandees (STAGE : l'origine est au plancher ;
//     LOCAL : l'origine est a la hauteur des yeux). L'espace de la couche doit etre celui dans
//     lequel les poses ont ete demandees : melanger les deux ne provoque aucune erreur, cela
//     decale la scene d'une hauteur de personne ;
//   - UNE VUE PAR OEIL : la chaine d'images de l'oeil, l'index de l'image a montrer, et la pose
//     et le champ de vision UTILISES AU RENDU. Le compositeur s'en sert pour la reprojection,
//     quand la tete a bouge entre le rendu et l'affichage : on ne donne pas la pose actuelle, on
//     donne la pose avec laquelle on a dessine.
//
// Rien ne survit a l'appel : la couche decrit, la session consomme. C'est une structure de
// description, remplie sur la pile, consommee immediatement, et oubliee.

#ifndef COUCHES_H
#define COUCHES_H

// Les deux espaces de reference.
enum class EspaceDeReference {
    Stage,   // STAGE : l'origine est au plancher
    Local,   // LOCAL : l'origine est a la hauteur des yeux
};

// Une vue : un oeil.
struct VueDeCouche {
    unsigned long long chaine = 0;   // l'identifiant opaque de la chaine de l'oeil ; 0 : manquante
    unsigned indexImage = 0;         // l'image de la chaine a montrer : la derniere RELACHEE
    int poseLacetCentiemes = 0;      // la pose de la tete AVEC LAQUELLE on a dessine, en centiemes
                                     // de degre (pas la pose actuelle)
    int champCentiemes = 0;          // le champ de vision employe au rendu, en centiemes de degre
};

// Une couche de projection : l'espace, et une vue par oeil.
struct CoucheProjection {
    EspaceDeReference space = EspaceDeReference::Stage;
    VueDeCouche views[2];            // views[0] : l'oeil gauche ; views[1] : l'oeil droit
};

#endif // COUCHES_H
