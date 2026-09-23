# Chapitre 2, démonstration 3 - Le tour complet à l'envers

## Ce qu'il faut montrer

Ma vitesse angulaire sans le forçage du chemin court, sur un delta minuscule qui se lit comme un tour presque complet. Puis ajouter les trois lignes du forçage et remontrer.

## Préparation

Le programme de l'exercice 9. Il calcule la vitesse angulaire moyenne entre deux orientations séparées de `dt` et affiche deux lignes : « avec forçage » puis « sans forçage ». Devant la classe, je commence par ne regarder que la ligne « sans forçage » (je cache l'autre), puis je fais apparaître les trois lignes du forçage dans le code (dans la version finale elles sont derrière l'interrupteur `forcer_chemin_court`) et je découvre la ligne « avec forçage ».

Les trois lignes :

```cpp
if (forcer_chemin_court && d.w < 0.0) {
    d = { -d.x, -d.y, -d.z, -d.w };   // -d est la même rotation, par le chemin court
}
```

## Déroulé

**1. Le principe au tableau, avec des angles.** Sur un cercle, aller de 350° à 10° peut se faire en 20° dans un sens ou en 340° dans l'autre. Une soustraction naïve `10 - 350` donne -340 et prend le grand chemin. Un quaternion a le même problème sous une autre forme : `q` et `-q` décrivent la même rotation, donc le delta peut sortir avec le mauvais signe.

**2. Sans le forçage.** Je montre la tête qui a bougé de 2° en 10 ms (donc 200 °/s, un mouvement tout à fait ordinaire). Le second quaternion est l'opposé de l'attendu, ce qui décrit la même orientation :

Entrée :

```
0 0 0 1   0 0 -0.017452406437283512 -0.99984769515639127   0.01
```

Sortie :

```
avec forcage : 0.0000 0.0000 3.4907  (rad/s)
sans forcage : 0.0000 0.0000 -624.8279  (rad/s)
```

Ce que la classe lit sur la ligne « sans forçage » : **-624,83 rad/s, soit -35 800 °/s**. La tête aurait fait 358° dans le mauvais sens en 10 ms, presque un tour complet à l'envers, pour un mouvement réel de 2°. La bonne valeur est +3,49 rad/s (200 °/s).

**3. Ce que ça donnerait dans un casque.** Si ce nombre servait à extrapoler la pose, l'image de l'œil tournerait de plusieurs dizaines de degrés à l'envers : le monde partirait d'un coup dans la mauvaise direction. Aucune erreur, aucun message.

**4. J'ajoute les trois lignes et je remontre.** Même entrée, la version avec forçage :

- avec forçage : **+3,4907 rad/s** (soit 200 °/s), exactement le mouvement réel ;
- sans forçage : toujours -624,83 rad/s, pour comparer.

Sur l'exemple du cours, 350° → 10° autour de `y` en une seconde :

Entrée :

```
0 0.087155742747658174 0 -0.99619469809174555   0 0.087155742747658174 0 0.99619469809174555   1
```

Sortie :

```
avec forcage : 0.0000 0.3491 0.0000  (rad/s)
sans forcage : 0.0000 -5.9341 0.0000  (rad/s)
```

Avec les trois lignes : 0,3491 rad/s, soit 20 °/s, le chemin court. Sans elles : -5,9341 rad/s, soit -340 °/s, le grand chemin.

## Ce que je fais retenir

- Le forçage tient en trois lignes : si le `w` du delta est négatif, on change le signe du quaternion.
- Le cas absurde est facile à rater dans les tests : il faut que `q` et `-q` se croisent, et le programme ne plante pas.
- Comme au chapitre 1 : la faute ne plante pas, elle se sent.
