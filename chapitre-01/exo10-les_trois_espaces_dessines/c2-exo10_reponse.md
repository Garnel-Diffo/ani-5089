# Chapitre 2, exercice 10 - Les trois espaces, dessinés

Pas de code dans cet exercice. Le dessin ci-dessous est une coupe de la salle vue de côté, à l'échelle : 1 caractère = 5 cm en profondeur (`z`, horizontal) et 10 cm en hauteur (`y`, vertical). Le nord (`z = -2 m`, le mur devant l'utilisateur) est à gauche, l'avant du repère du module (`-z`) va donc vers la gauche. La salle fait 4 m de profondeur et 2,50 m de haut ; c'est celle de l'exercice 10 du chapitre précédent.

## Le dessin

```
_________TL_____________________________________________________________________
|                                                                              |
|TV                                                                            |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                       O L                                    |
|                                       |                                      |
|                               V       |                                      |
|                                       |                                      |
|                                       |                                      |
|                                       |                                      |
|                                       |                                      |
|                                       |                                      |
|        TS                             |                                      |
|  =============                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
|  |           |                        |                                      |
________________________________________S_______________________________________
```

Légende :

| Repère                | Ce que c'est                                                                                                                                                         |
| ---------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `O`                  | la tête de l'utilisateur debout, yeux à 1,70 m, au centre de la salle (z = 0), au moment du démarrage                                                             |
| `S`                  | origine de**STAGE** : au sol, au centre de la zone de jeu, à la verticale de l'utilisateur                                                                    |
| `L`                  | origine de**LOCAL** : la pose de la tête **au démarrage**, alignée sur la gravité. Elle est donc à 1,70 m du sol, au même endroit que `O`        |
| `V`                  | origine de**VIEW** : entre les deux yeux, **maintenant**. L'utilisateur s'est penché, elle est à (z = -0,40 ; y = 1,50) et bougera à l'image suivante |
| `=====` avec pieds   | la vraie table de la salle, plateau à 0,80 m du sol (z de -1,85 à -1,25)                                                                                           |
| `TS`, `TL`, `TV` | la « table à 80 cm » posée à y = +0,80 dans STAGE, LOCAL et VIEW (à z = -1,55 dans le repère de chacun)                                                       |

C'est le programme qui choisit dans quel espace il demande les poses au casque, et l'espace choisi fixe l'origine : un espace n'est pas une donnée, c'est un contrat sur l'origine.

## Où se retrouve la table à y = +0,80

| Espace                   | Ce que veut dire « y = +0,80 »                       | Hauteur réelle depuis le sol                                                                    | Ce qu'on voit                                                                                                                                                                                                              |
| ------------------------ | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **STAGE** (`TS`) | 0,80 m au-dessus du sol                                | **0,80 m**                                                                                 | La table est à sa place, au niveau de la vraie table.                                                                                                                                                                     |
| **LOCAL** (`TL`) | 0,80 m au-dessus de la tête au démarrage             | 1,70 + 0,80 =**2,50 m** (1,20 + 0,80 = 2,00 m si l'utilisateur était assis au démarrage) | La table flotte au plafond de la salle (2,50 m), et sa hauteur dépend de la posture du démarrage.                                                                                                                        |
| **VIEW** (`TV`)  | 0,80 m au-dessus des yeux, dans le repère de la tête | 1,50 + 0,80 =**2,30 m**, à z = -0,40 - 1,55 = -1,95 m                                     | La table est au plafond, sa moitié arrière sort par le mur du fond (elle occupe z de -2,25 à -1,65 pour 0,60 m de profondeur), et surtout elle**suit la tête** : elle bouge à chaque image, comme un réticule. |

Seul STAGE donne une table à 0,80 m : c'est le seul espace où y = 0 veut dire le plancher, donc le seul où poser un décor a un sens physique.
