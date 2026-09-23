# Chapitre 1, démonstration 1 - Le budget au tableau

## Ce qu'il faut montrer

Dessiner au tableau les vingt millisecondes comme une barre, faire placer par la classe les cinq étapes à leur échelle, faire remarquer ce qui reste pour le code, et faire réagir.

## Préparation

- Une barre de 1 m de long au tableau, qui représente **20 ms** (1 ms = 5 cm), graduée toutes les 2 ms.
- Cinq bandes de papier de couleur, une par étape, découpées à la largeur de la valeur du chapitre mais **cachées** : la classe devra estimer la largeur de chacune avant de les voir.
- Les valeurs du chapitre : capteurs 1 à 2 ms, transmission 1 à 3 ms, application 5 à 11 ms, compositeur 1 à 2 ms, écran 2 à 5 ms.

## Déroulé

1. **Je dessine la barre** et j'écris « 20 ms : du mouvement de la tête au premier photon ». Je précise que c'est un budget : au-delà, l'inconfort commence.
2. **Je demande à la classe** de m'indiquer, sur la barre, où commence et où finit chacune des cinq étapes, dans l'ordre : capteurs, transmission, application, compositeur, écran. Je laisse trois ou quatre volontaires proposer, et je trace leurs propositions à la craie.
3. **Je révèle les valeurs du chapitre** et je colle les bandes sur la barre, avec les valeurs moyennes.
4. **Je fais remarquer ce qui reste pour le code.** L'application a de 5 à 11 ms, soit environ la moitié des 20 ms, pas la totalité. Et si chaque étape prenait le haut de sa fourchette, le total serait 2 + 3 + 11 + 2 + 5 = 23 ms, donc **on dépasserait déjà le budget** sans que rien ne soit anormal.
5. **Je renforce avec un cas concret** : à 90 Hz, une image dure 11,1 ms (exercice 1). Si on retire les 8 ms des autres étapes, il en reste **3,1 ms** pour mon code. Je trace cette petite bande sur la barre : c'est ce qui reste de la barre de 20 ms pour la partie qui m'appartient.
6. **Je fais réagir** : « Qu'est-ce qui vous surprend ? » et « qu'est-ce qu'on peut faire avec 3 ms ? ».

## La barre à l'échelle (valeurs moyennes)

1 caractère = 0,5 ms, la barre fait 20 ms :

```
0   2   4   6   8   10  12  14  16  18  20
+---+---+---+---+---+---+---+---+---+---+
CCCTTTTAAAAAAAAAAAAAAAAKKKEEEEEEE.......
```

| Lettre | Étape | Valeur moyenne prise pour dessiner | Fourchette du chapitre |
|---|---|---|---|
| C | Les capteurs mesurent le mouvement | 1,5 ms | 1 à 2 ms |
| T | Le système transmet la mesure | 2 ms | 1 à 3 ms |
| A | **Votre application décide et dessine** | 8 ms | 5 à 11 ms |
| K | Le compositeur assemble | 1,5 ms | 1 à 2 ms |
| E | L'écran affiche la ligne | 3,5 ms | 2 à 5 ms |
| `.` | Marge restante, avec ces valeurs moyennes | 3,5 ms | |

Avec les valeurs moyennes, le total fait 16,5 ms : il reste 3,5 ms de marge sur les 20. Avec le bas des fourchettes on est à 10 ms, avec le haut à 23 ms, c'est-à-dire au-dessus du budget.

## Ce que je dis à la classe

Le code n'a pas vingt millisecondes : il en a une dizaine, et il doit les tenir **à chaque image, pas en moyenne**. Tout ce qu'on fera dans la suite se déduit de cette barre.

## Les réactions

Ce que la classe a dit, tel quel : pour l'application, la classe propose « presque toute la barre ». Le chapitre lui donne 5 à 11 ms, soit de 25 % à 55 % de la barre de 20 ms : l'estimation de la classe était très au-dessus.

Ce qui a surpris le plus : que les 20 ms ne suffiraient pas si chaque étape prenait le haut de sa fourchette (23 ms).
