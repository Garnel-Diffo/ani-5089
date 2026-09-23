# Chapitre 1, exercice 8 - Le champ de vision asymétrique

## Les quatre angles

Casque : **HTC Vive** (premier modèle), **œil gauche**. Angles mesurés depuis l'axe optique de la lentille, tels que le casque les annonce à l'application par OpenVR :

| Côté | Angle |
|---|---|
| gauche | **54,41°** |
| droite | **51,35°** |
| bas | **55,91°** |
| haut | **55,67°** |

Le champ de vision total de cet œil est de 105,76° en horizontal (54,41 + 51,35) et 111,58° en vertical (55,91 + 55,67).

Source : Richard Musil, « VR headset rendered FOV calculation » (29 juillet 2019), page du site *VR Docs* : https://risa2000.github.io/vrdocs/docs/hmd_fov_calculation.html. L'auteur précise que les angles « sont collectés auprès du casque (du pilote) par OpenVR puis annoncés à l'application par l'API ». Ce sont les valeurs de la version « sans HAM », c'est-à-dire sans le masque de zone cachée (la version « avec HAM » réduit le côté droit à 46,75°). Ces angles se déduisent des valeurs que renvoie la fonction `GetProjectionRaw` d'OpenVR, dont la documentation décrit chacune comme la tangente du demi-angle entre l'axe central et le plan de coupe correspondant (https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetProjectionRaw).

Pour l'œil droit du même casque la page donne : gauche 51,29°, droite 54,36°, bas 55,87°, haut 55,61°, soit l'image miroir.

## L'asymétrie en chiffres

- **Horizontalement** : 54,41° à gauche contre 51,35° à droite, soit 3,06° de plus du côté gauche. L'œil gauche voit plus large vers l'extérieur (du côté de la tempe), moins vers le nez. Cela va dans le sens de l'anatomie : le nez limite le champ du côté intérieur.
- **Verticalement** : presque symétrique (55,91° contre 55,67°, 0,24° d'écart).
- En termes de projection, ce qui compte est la tangente des angles : `tan(54,41°) = 1,397` à gauche et `tan(51,35°) = 1,250` à droite. L'axe de la lentille n'est pas au milieu de l'image : il en est à 52,8 % de la largeur en partant de la gauche (`1,397 / (1,397 + 1,250)`).

## Ce qui se passerait avec un champ symétrique de même surface

**En une phrase :** l'image serait décalée par rapport à l'optique, d'environ 30 pixels sur les 1080 de la largeur d'un œil (de l'ordre de 4° au centre), dans un sens pour l'œil gauche et dans l'autre pour le droit, ce qui déforme légèrement le monde vers les bords et fausse un peu la profondeur, sans qu'aucun message ne l'annonce.

Le calcul derrière, avec ses hypothèses :

- Un champ symétrique de même surface dans le plan de projection aurait les mêmes largeur et hauteur en tangentes, centrées : demi-largeur `(1,397 + 1,250) / 2 = 1,324` (52,9°), demi-hauteur `(1,478 + 1,464) / 2 = 1,471` (55,8°).
- Le centre du champ réel (l'axe de la lentille) et celui du champ symétrique diffèrent de `(1,397 - 1,250) / 2 = 0,073` en tangente horizontalement (0,007 verticalement, négligeable). Sur un panneau de 1080 pixels de large (le Vive affiche 1080 × 1200 pixels par œil), cela fait environ 30 pixels, soit environ 4° près du centre de l'image.
- Le décalage est en sens contraire pour les deux yeux (image miroir), donc ce n'est pas une simple rotation de tout le monde : les deux yeux voient le même objet à des positions qui se séparent un peu trop ou pas assez, ce qui fausse la disparité, donc la profondeur perçue.

Limites : c'est un calcul simple dans un modèle de projection sans distorsion des lentilles, avec des pixels répartis uniformément dans le plan de projection. La distorsion réelle et sa correction changeront les valeurs. Je n'ai pas essayé un vrai casque : c'est un ordre de grandeur, pas une mesure.

C'est la raison pour laquelle le cours dit qu'on ne peut pas construire la matrice de projection avec la fonction habituelle (un angle et un rapport de forme) : il faut quatre angles séparés.
