# Chapitre 2, démonstration 1 - Les deux ordres

## Préparation

- Un stylo (ou une règle) : l'objet. Son centre est l'origine de l'entité ; son bout est le point `p` que je suis, à 1 m du centre pour l'échelle du tableau (1 carreau = 1 m).
- Au tableau, un repère vu de dessus : l'axe `x` vers la droite, l'axe `z` vers le bas du tableau (donc l'avant, `-z`, vers le haut), l'origine du monde marquée d'une croix, le stylo posé sur l'origine, dirigé vers la droite (le bout est en (1, 0, 0)).
- La pose : rotation de 90° autour de l'axe vertical, position (2, 0, 0).

## Déroulé

**1. Ordre du module, « je tourne puis j'avance » (rotation puis translation).** Je fais pivoter le stylo de 90° sur lui-même, autour de son centre : le bout passe de la droite à l'avant, en (0, 0, -1) par rapport au centre. Puis je déplace le stylo de 2 m vers la droite. Le centre est en (2, 0, 0), le bout en **(2, 0, -1)**.

**2. Ordre inverse, « j'avance puis je tourne » (translation puis rotation).** Je pars du même point. Je déplace d'abord le stylo de 2 m vers la droite : le bout est en (3, 0, 0). Puis je tourne de 90° autour de l'**origine du monde** (la croix du tableau, pas le centre du stylo) : tout le stylo décrit un arc de cercle autour de la croix. Le centre passe de (2, 0, 0) à (0, 0, -2) et le bout arrive en **(0, 0, -3)**.

**3. Ce que je dis.** Les deux gestes sont faits avec la même rotation et la même translation. Ce qui change, c'est le **pivot**, le point autour duquel la rotation s'effectue : dans le premier ordre c'est le centre de l'objet (il tourne sur lui-même), dans le second c'est l'origine du monde (il part en orbite). Le mot du cours : quelqu'un qui a déjà vu un objet « partir en orbite » au lieu de tourner sur place a vu cette erreur d'ordre.

Je demande à la classe où sera le bout du stylo dans chaque cas avant de le montrer : le premier est facile, le second surprend souvent, parce qu'on imagine « je tourne » comme « je tourne sur place ».

## Les deux résultats du programme

Le programme de l'exercice 3 applique la pose de rotation de 90° autour de `y` et de position (2, 0, 0) au point (1, 0, 0), dans les deux ordres :

Entrée :

```
2 0 0  0 0.70710678118654752 0 0.70710678118654752  1 0 0
```

Sortie :

```
rotation puis translation    2.0000 0.0000 -1.0000
translation puis rotation    0.0000 0.0000 -3.0000
ecart                        2.8284
```

- rotation puis translation : (2 ; 0 ; -1), comme le stylo qui tourne sur lui-même puis avance ;
- translation puis rotation : (0 ; 0 ; -3), comme le stylo en orbite ;
- l'écart entre les deux est de 2,83 m sur un déplacement de 2 m.

Le programme et le tableau donnent les mêmes points : (2, 0, -1) et (0, 0, -3).
