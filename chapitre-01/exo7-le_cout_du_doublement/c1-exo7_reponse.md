# Chapitre 1, exercice 7 - Le coût du doublement

## Le programme

Le même que pour l'exercice 6 : le banc de mesure `banc` (6000 carrés, logique + rendu). Son code est dans `../exo6-la_pire_image/c1-exo6_reponse.md`. Il a trois modes, choisis par le deuxième argument :

- `complet` : logique et rendu ;
- `rendu` : le rendu seul, **sans la logique** (les carrés ne bougent plus) ;
- `logique` : la logique seule, sans rien dessiner.

Le rendu est mesuré en deux morceaux : le **dessin** dans le tampon mémoire, et la **présentation**, la recopie vers la fenêtre.

```
banc 6000 rendu 1000
banc 6000 logique 1000
```

## La mesure : le rendu seul

Cinq essais de mille images par mode, sur la même machine que l'exercice 6. Valeurs retenues : médiane des moyennes des cinq essais (un essai du mode `rendu` a été perturbé par le système, avec une image de 408 ms).

| | Moyenne par image (médiane des 5 essais) | Fourchette des 5 essais |
|---|---|---|
| Dessin dans le tampon (mode `rendu`) | **3,52 ms** | de 3,04 à 4,52 ms |
| Présentation à l'écran (mode `rendu`) | **1,53 ms** | de 1,40 à 4,15 ms |
| **Rendu seul** = dessin + présentation | **5,05 ms** | |
| Logique seule (mode `logique`) | **0,97 ms** | de 0,95 à 1,08 ms |

Le rendu seul prend donc **environ 5,1 ms** par image, la logique **environ 1 ms** : le rendu pèse cinq fois plus que la logique dans ce programme.

## Estimation : le rendu fait deux fois

Un casque demande une image par œil. Deux estimations :

- **Estimation simple** : tout le rendu est doublé, 2 × 5,05 = **10,1 ms**.
- **Estimation plus fine** : seul le dessin est fait deux fois, la présentation une seule (on recopie les deux vues d'un coup), 2 × 3,52 + 1,53 = **8,6 ms**.

Avec la logique (0,97 ms, qui n'est pas doublée) : de **9,5 ms** (estimation fine) à **11,1 ms** (estimation simple) par image.

## Ce qu'il resterait pour le reste

On compare au budget de l'application calculé à l'exercice 1 (durée d'une image moins 8 ms), et on regarde ce qui reste pour le reste du programme.

| Cadence | Budget de l'application | Rendu doublé (estimation fine, 8,6 ms) | Ce qui reste pour le reste | Avec la logique (0,97 ms) |
|---|---|---|---|---|
| 72 Hz | 5,9 ms | 8,6 ms | **-2,7 ms** | -3,6 ms |
| 90 Hz | 3,1 ms | 8,6 ms | **-5,5 ms** | -6,4 ms |
| 120 Hz | 0,3 ms | 8,6 ms | **-8,3 ms** | -9,2 ms |

Il ne reste **rien**, et même moins que rien : le rendu doublé dépasse à lui seul le budget de l'application, à toutes les cadences. Avec l'estimation simple (10,1 ms) c'est pire encore (-4,2 ms à 72 Hz).

## Ma conclusion sur ce qu'il faudrait réduire

C'est le **dessin** qu'il faut réduire d'abord : il fait 3,5 ms sur les 5,1 ms du rendu, et c'est lui qu'on doublera. La présentation (1,5 ms) et la logique (1,0 ms) sont à peu près fixes.

Pour que tout tienne, il faut `2 × dessin + présentation + logique ≤ budget`, donc `dessin ≤ (budget - 1,53 - 0,97) / 2` :

| Cadence | Dessin maximal admissible | À comparer à 3,52 ms |
|---|---|---|
| 72 Hz | 1,7 ms | à diviser par **2** |
| 90 Hz | 0,3 ms | à diviser par **12** |
| 120 Hz | impossible | la présentation et la logique dépassent déjà le budget |

À 72 Hz, il suffirait de diviser le coût du dessin par deux. À 90 Hz il faudrait le diviser par douze, ce qui revient à changer de méthode de rendu. À 120 Hz, sur cette machine, ce programme est hors de portée.

Où gagner ? Mon programme dessine 6000 carrés de 14 × 14 pixels, soit 1,18 million de pixels écrits pour une image de 480 000 pixels : chaque pixel est écrit en moyenne 2,45 fois. Réduire cette surcharge, dessiner moins de carrés (éliminer ceux qui sont cachés ou hors champ), ou baisser la résolution de chaque vue sont les pistes les plus directes. Pour la présentation, il faudrait la faire une seule fois par image avec les deux vues.
