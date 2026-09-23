# Chapitre 2, exercice 11 - Le monde à la mauvaise échelle

## Le programme

Il affiche les dimensions de la salle et de son mobilier (la salle de l'exercice 10 du chapitre 1 : 5 m × 4 m × 2,50 m, une porte de 2 m, des fenêtres, une table de 0,80 m avec une lampe, un livre et une tasse), toutes multipliées par un facteur lu à l'entrée. Les valeurs « vraies » sont dans un tableau constant en haut du fichier, en mètres. Un facteur nul ou négatif est refusé.

## Code

```cpp
// Chapitre 2, exercice 11 : le monde a la mauvaise echelle.
// Affiche les dimensions de la salle et de son mobilier (en metres),
// multipliees par un facteur lu a l'entree.
#include <cstdio>
#include <iostream>

struct Mesure { const char* nom; double metres; };

// Dimensions "vraies" de la salle (voir le plan de l'exercice 10 du chapitre 1).
const Mesure SALLE[] = {
    { "salle : longueur",            5.00 },
    { "salle : largeur",             4.00 },
    { "salle : hauteur sous plafond", 2.50 },
    { "porte : largeur",             0.90 },
    { "porte : hauteur",             2.00 },
    { "fenetre : largeur",           1.20 },
    { "fenetre : hauteur",           1.10 },
    { "fenetre : hauteur de l'appui", 0.90 },
    { "table : longueur",            1.20 },
    { "table : profondeur",          0.60 },
    { "table : hauteur",             0.80 },
    { "lampe : hauteur",             0.40 },
    { "livre : longueur",            0.24 },
    { "livre : largeur",             0.17 },
    { "livre : epaisseur",           0.03 },
    { "tasse : hauteur",             0.09 },
    { "tasse : diametre",            0.08 },
};

int main() {
    double facteur;
    if (!(std::cin >> facteur) || facteur <= 0.0) {
        std::fprintf(stderr, "Il faut un facteur strictement positif.\n");
        return 1;
    }
    std::printf("facteur d'echelle : %.3f\n", facteur);
    for (const Mesure& m : SALLE) {
        std::printf("%-30s %8.3f m\n", m.nom, m.metres * facteur);
    }
    return 0;
}
```

## Les trois facteurs

J'ai pris trois facteurs qui font sentir l'écart sans être absurdes : **0,6** (le monde plus petit), **1,0** (la vraie salle), **1,7** (le monde plus grand). Ci-dessous, la sortie du programme pour chacun : c'est ce qu'on montre à la personne qui doit décrire la salle.

Facteur 0,6 :

```
facteur d'echelle : 0.600
salle : longueur                  3.000 m
salle : largeur                   2.400 m
salle : hauteur sous plafond      1.500 m
porte : largeur                   0.540 m
porte : hauteur                   1.200 m
fenetre : largeur                 0.720 m
fenetre : hauteur                 0.660 m
fenetre : hauteur de l'appui      0.540 m
table : longueur                  0.720 m
table : profondeur                0.360 m
table : hauteur                   0.480 m
lampe : hauteur                   0.240 m
livre : longueur                  0.144 m
livre : largeur                   0.102 m
livre : epaisseur                 0.018 m
tasse : hauteur                   0.054 m
tasse : diametre                  0.048 m
```

Facteur 1,0 :

```
facteur d'echelle : 1.000
salle : longueur                  5.000 m
salle : largeur                   4.000 m
salle : hauteur sous plafond      2.500 m
porte : largeur                   0.900 m
porte : hauteur                   2.000 m
fenetre : largeur                 1.200 m
fenetre : hauteur                 1.100 m
fenetre : hauteur de l'appui      0.900 m
table : longueur                  1.200 m
table : profondeur                0.600 m
table : hauteur                   0.800 m
lampe : hauteur                   0.400 m
livre : longueur                  0.240 m
livre : largeur                   0.170 m
livre : epaisseur                 0.030 m
tasse : hauteur                   0.090 m
tasse : diametre                  0.080 m
```

Facteur 1,7 :

```
facteur d'echelle : 1.700
salle : longueur                  8.500 m
salle : largeur                   6.800 m
salle : hauteur sous plafond      4.250 m
porte : largeur                   1.530 m
porte : hauteur                   3.400 m
fenetre : largeur                 2.040 m
fenetre : hauteur                 1.870 m
fenetre : hauteur de l'appui      1.530 m
table : longueur                  2.040 m
table : profondeur                1.020 m
table : hauteur                   1.360 m
lampe : hauteur                   0.680 m
livre : longueur                  0.408 m
livre : largeur                   0.289 m
livre : epaisseur                 0.051 m
tasse : hauteur                   0.153 m
tasse : diametre                  0.136 m
```

Un facteur invalide est refusé :

Entrée :

```
0
```

Message d'erreur (sortie d'erreur, code retour 1) :

```
Il faut un facteur strictement positif.
```

## Les trois descriptions

| Personne | Facteur (caché à la personne) | Ses mots, tels quels |
|---|---|---|
| 1 | 0,6 | « une petite pièce, le plafond est bas, la porte est plus petite qu'une porte normale » |
| 2 | 1,0 | « une pièce de séjour normale, rien de bizarre » |
| 3 | 1,7 | « une très grande salle, le plafond est très haut, la table est énorme » |
