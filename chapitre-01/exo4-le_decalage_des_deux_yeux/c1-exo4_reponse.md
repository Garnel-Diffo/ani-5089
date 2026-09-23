# Chapitre 1, exercice 4 - Le décalage des deux yeux

## Protocole

1. Se placer face à un mur du fond, à une distance connue **D** (à mesurer au mètre ruban, par exemple 4 m).
2. Tenir un doigt devant le visage à une distance **d** du nez : d'abord 30 cm, puis 1 m, puis 3 m. La distance est vérifiée avec une ficelle ou un mètre ruban.
3. Regarder le doigt, fermer un œil puis l'autre, et repérer sur le mur où le doigt semble se trouver dans chaque cas (un repère au mur, ou un ami qui note la position).
4. Mesurer, sur le mur, la distance entre les deux positions apparentes du doigt.

## Valeurs attendues par le calcul

Deux yeux séparés de **e** (j'ai pris 63 mm, la moyenne de Dodgson, voir l'exercice 3) voient un doigt à la distance d sous deux directions différentes. Projetées sur un mur à la distance D, ses deux positions apparentes sont séparées de

```
décalage = e × (D - d) / d
```

Le décalage angulaire (la disparité) est de l'ordre de `e/d - e/D` en radians. Pour un mur à 4 m et à 5 m :

| Doigt à d | Mur à 4 m : décalage sur le mur | Mur à 4 m : disparité | Mur à 5 m : décalage | Mur à 5 m : disparité |
|---|---|---|---|---|
| 0,30 m | 77,7 cm | 11,1° | 98,7 cm | 11,3° |
| 1,00 m | 18,9 cm | 2,7° | 25,2 cm | 2,9° |
| 3,00 m | 2,1 cm | 0,3° | 4,2 cm | 0,5° |

(Ces valeurs sont des calculs avec e = 63 mm, pas des mesures. Un mur à 3 m ne convient pas pour la dernière ligne, puisque le doigt serait au mur.)

## Mes trois mesures

Mur à la distance D = 4,0 m.

| Doigt à | Décalage mesuré sur le mur | Valeur attendue par la formule (avec mon D) |
|---|---|---|
| 0,30 m | 74 cm | 77,7 cm |
| 1,00 m | 17 cm | 18,9 cm |
| 3,00 m | 2 cm | 2,1 cm |

Les valeurs mesurées sont un peu sous le calcul : de 3,7 cm à 0,30 m (- 4,8 %), 1,9 cm à 1,00 m (- 10 %) et 0,1 cm à 3,00 m (- 4,8 %), soit un écart relatif de 5 à 10 %. Cela peut s'expliquer par un repérage au mur à quelques centimètres près. Les trois valeurs suivent la même tendance que la formule : le décalage tombe de 74 cm à 2 cm quand le doigt s'éloigne de 30 cm à 3 m, soit un facteur 37 (le calcul en donne 77,7 / 2,1 = 37).

## Ce que ces mesures annoncent du travail du chapitre 9

Ce que le calcul prédit, et que l'expérience doit montrer :

- **Le décalage dépend fortement de la distance.** Il est énorme pour un objet à 30 cm (des dizaines de centimètres sur le mur), petit à 1 m, presque nul à 3 m. Un objet proche se décale nettement d'un œil à l'autre, un objet lointain presque pas : c'est ce décalage qui donne le relief.
- **On ne peut pas le fabriquer avec une seule image.** Comme le décalage n'est pas le même pour chaque objet mais dépend de sa distance à l'œil, il n'existe pas de « décalage horizontal » unique qu'on appliquerait à une image déjà dessinée. Il faut deux images prises depuis deux positions, écartées de l'écart entre les yeux : **la scène doit être dessinée deux fois**, avec deux matrices de vue décalées d'environ ± e/2 le long de l'axe horizontal de la tête. C'est ce que le chapitre 9 devra produire : une vue par œil.
- **La valeur de e compte.** Le décalage est proportionnel à e : une valeur fausse ne change rien au principe, mais change l'échelle perçue du monde (voir l'exercice 3).
- **Pour l'optimisation** (une piste que je note, pas un résultat) : à partir de quelques mètres, le décalage devient de quelques centimètres, donc les deux images d'un décor lointain se ressemblent beaucoup.
