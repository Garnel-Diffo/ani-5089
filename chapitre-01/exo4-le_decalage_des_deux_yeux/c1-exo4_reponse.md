# Chapitre 1, exercice 4 - Le décalage des deux yeux

## Formule de la valeur attendue

Deux yeux séparés de **e** (63 mm) voient un doigt à la distance d sous deux directions différentes. Projetées sur un mur à la distance D, ses deux positions apparentes sont séparées de :

```
décalage = e × (D - d) / d
```

## Mes trois mesures

Mur à la distance D = 4,0 m.

| Doigt à | Décalage mesuré sur le mur | Valeur attendue par la formule (avec mon D) |
|---|---|---|
| 0,30 m | 74 cm | 77,7 cm |
| 1,00 m | 17 cm | 18,9 cm |
| 3,00 m | 2 cm | 2,1 cm |

## Ce que ces mesures annoncent du travail du chapitre 9

- **Le décalage dépend fortement de la distance.** Il est énorme pour un objet à 30 cm (des dizaines de centimètres sur le mur), petit à 1 m, presque nul à 3 m. Un objet proche se décale nettement d'un œil à l'autre, un objet lointain presque pas : c'est ce décalage qui donne le relief.
- **On ne peut pas le fabriquer avec une seule image.** Comme le décalage n'est pas le même pour chaque objet mais dépend de sa distance à l'œil, il n'existe pas de « décalage horizontal » unique qu'on appliquerait à une image déjà dessinée. Il faut deux images prises depuis deux positions, écartées de l'écart entre les yeux : **la scène doit être dessinée deux fois**, avec deux matrices de vue décalées d'environ ± e/2 le long de l'axe horizontal de la tête. C'est ce que le chapitre 9 devra produire : une vue par œil.
