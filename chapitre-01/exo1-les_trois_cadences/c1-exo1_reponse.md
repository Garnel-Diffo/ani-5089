# Chapitre 1, exercice 1 - Les trois cadences

## Calcul

La durée d'une image, en millisecondes, est 1000 divisé par la fréquence en hertz. On lui retire les huit millisecondes que prennent les capteurs, la transmission, la composition et l'affichage : ce qui reste est pour mon code.

| Cadence | Calcul             | Durée d'une image | Moins 8 ms | **Reste pour mon code** |
| ------- | ------------------ | ------------------ | ---------- | ----------------------------- |
| 72 Hz   | 1000 / 72 = 13,889 | 13,9 ms            | 13,9 - 8   | **5,9 ms**              |
| 90 Hz   | 1000 / 90 = 11,111 | 11,1 ms            | 11,1 - 8   | **3,1 ms**              |
| 120 Hz  | 1000 / 120 = 8,333 | 8,3 ms             | 8,3 - 8    | **0,3 ms**              |

Les trois nombres à retenir pour le chapitre 10 : **5,9 ms, 3,1 ms et 0,3 ms** (avec les durées d'image 13,9, 11,1 et 8,3 ms).
