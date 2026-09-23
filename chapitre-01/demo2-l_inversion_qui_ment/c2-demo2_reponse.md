# Chapitre 2, démonstration 2 - L'inversion qui ment

## Ce qu'il faut montrer

Mon inversion générale qui rend l'identité sur une matrice dégénérée, sans le moindre message. Faire imaginer à la classe ce que cela donnerait dans un casque, puis le dire : la caméra revient à l'origine, sans rotation, et rien ne l'explique.

## Préparation

Le programme de l'exercice 7, lancé sur une pose quelconque. Il affiche d'abord la comparaison sur une pose valide, puis sur une pose dégénérée fixée dans le code : position (1 ; 1,7 ; -2) et quaternion nul (0, 0, 0, 0).

Je prépare deux choses au tableau avant de lancer :

- la matrice de vue « attendue » de la pose valide, pour qu'on voie à quoi ressemble une vraie réponse (la rotation `Rᵀ` et la translation `-Rᵀ t`) ;
- une petite scène dessinée : un utilisateur à 1,70 m de haut, à 2 m devant le mur du fond, qui regarde vers ce mur.

## Déroulé

**1. La pose valide.** Je montre la sortie pour la pose valide : les deux versions donnent la même matrice, à 2 × 10⁻¹⁶ près. On se dit que l'inversion générale marche.

```
=== Pose valide ===
A - inversion generale :
    0.7071    0.0000   -0.7071   -2.1213
    0.0000    1.0000    0.0000   -1.7000
    0.7071    0.0000    0.7071    0.7071
    0.0000    0.0000    0.0000    1.0000
B - conjugue + translation opposee :
    0.7071    0.0000   -0.7071   -2.1213
    0.0000    1.0000    0.0000   -1.7000
    0.7071    0.0000    0.7071    0.7071
    0.0000    0.0000    0.0000    1.0000
ecart maximum sur les 16 coefficients : 2.220e-16
```

**2. La pose dégénérée.** Je lance le même programme sur la pose dont le quaternion est nul, le cas d'un suivi perdu. La version A (inversion générale) répond, sans erreur, sans avertissement, sans code de retour particulier :

```
=== Pose degeneree : position (1, 1.7, -2), quaternion nul ===
A - inversion generale :
    1.0000    0.0000    0.0000    0.0000
    0.0000    1.0000    0.0000    0.0000
    0.0000    0.0000    1.0000    0.0000
    0.0000    0.0000    0.0000    1.0000
B - conjugue + translation opposee :
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    1.0000
ecart maximum sur les 16 coefficients : 1.000e+00
```

La sortie de A est une matrice identité parfaite : elle a l'air d'une matrice de vue ordinaire. C'est ce qui trompe.

**3. Je demande à la classe.** « Vous portez le casque, la pose de votre tête devient invalide pendant une image, et le programme dessine avec cette matrice. Qu'est-ce que vous voyez ? » Je laisse deviner. Réponses attendues : « rien », « un écran noir », « ça plante ».

**4. Je le dis.** Aucune des trois. La caméra revient à l'origine du monde, sans rotation, à la hauteur de l'origine, qui est le sol (ou le centre de la tête, selon l'espace choisi). L'utilisateur est téléporté sur le point de départ, la tête droite, alors qu'il s'est peut-être retourné : l'image change d'un coup, sans raison visible, et ça se sent (le mouvement de la tête et l'image ne correspondent plus). Et **rien ne l'explique** : pas de message, pas de plantage, pas de journal.

**5. Le contre-exemple.** La version B (inverse analytique) donne dans le même cas une matrice écrasée, avec une rotation nulle. C'est laid, mais visible : une image cassée se remarque, se teste (la norme du quaternion vaut 0) et se corrige, alors qu'une image simplement fausse se fait passer pour une image correcte.

## Ce que je fais retenir

C'est la catégorie de faute annoncée au chapitre 1 : elle ne plante pas, elle se sent. La phrase du module le dit pour l'inverse analytique : « exacte, et sans le garde-fou “singulier” qui rendrait silencieusement l'identité en cas de bug amont ».

## Ce que la classe a répondu

Réponses à la question de l'étape 3 : « un écran noir », « l'image qui saute », « on a l'impression de tomber au sol ».

Une seule des trois (« un écran noir ») est parmi les réponses attendues, et elle est fausse. Les deux autres se rapprochent de ce qui se passe vraiment : « l'image qui saute » décrit le changement brusque de point de vue, et « on a l'impression de tomber au sol » rejoint la caméra ramenée à la hauteur de l'origine, qui est le sol dans l'espace choisi. Ce qu'aucune des trois réponses ne dit, c'est que **rien ne l'explique** : ni message, ni plantage, ni journal. C'est cette absence de message qui fait de cette faute une faute qui se sent au lieu de se voir.
