# Chapitre 1, exercice 1 - Les trois cadences

## Calcul

La durée d'une image, en millisecondes, est 1000 divisé par la fréquence en hertz. On lui retire les huit millisecondes que prennent les capteurs, la transmission, la composition et l'affichage : ce qui reste est pour mon code.

| Cadence | Calcul | Durée d'une image | Moins 8 ms | **Reste pour mon code** |
|---|---|---|---|---|
| 72 Hz | 1000 / 72 = 13,889 | 13,9 ms | 13,9 - 8 | **5,9 ms** |
| 90 Hz | 1000 / 90 = 11,111 | 11,1 ms | 11,1 - 8 | **3,1 ms** |
| 120 Hz | 1000 / 120 = 8,333 | 8,3 ms | 8,3 - 8 | **0,3 ms** |

Les trois nombres à retenir pour le chapitre 10 : **5,9 ms, 3,1 ms et 0,3 ms** (avec les durées d'image 13,9, 11,1 et 8,3 ms).

Avec les valeurs non arrondies, on trouve 5,89, 3,11 et 0,33 ms, qui s'arrondissent aux mêmes chiffres au dixième.

## Ce que je remarque

- Entre 72 et 120 Hz, l'image devient 40 % plus courte (de 13,9 à 8,3 ms), mais ce qui reste pour le code baisse de 95 % (de 5,9 à 0,3 ms), parce que les huit millisecondes ne diminuent pas avec la cadence. Toute la hausse de cadence est payée par l'application.
- À 120 Hz il ne reste que 0,3 ms : autant dire rien. Cela ne veut pas dire qu'un programme à 120 Hz est impossible, mais que ce modèle « on retire 8 ms fixes » ne tient plus à cette cadence : en pratique une partie de ces étapes se recouvre dans le temps. Je garde le calcul demandé, avec cette réserve.
- Autre réserve : la durée d'une image est un intervalle entre deux images, alors que les 20 ms du cours mesurent un retard, du mouvement au photon. L'exercice soustrait des retards d'une durée d'image comme un budget simplifié, et je le fais comme demandé.
- Rapproché du tableau du chapitre : l'étape « votre application décide et dessine » y est estimée à 5 à 11 ms. À 72 Hz le reste (5,9 ms) tombe au bas de cette fourchette, à 90 Hz il tombe en dessous (3,1 ms), à 120 Hz il n'y est plus du tout.
