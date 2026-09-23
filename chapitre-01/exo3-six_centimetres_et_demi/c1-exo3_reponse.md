# Chapitre 1, exercice 3 - Six centimètres et demi

## Protocole

Matériel : une règle graduée en millimètres, un miroir. Six personnes : moi et cinq autres.

- **Pour moi** : je me place à environ 1 m d'un miroir, la règle posée horizontalement sur l'arête du nez juste sous les yeux, et je regarde droit dans mes propres yeux (l'œil regarde alors à 2 m environ, ce qui limite la convergence). Je lis la distance du centre d'une pupille au centre de l'autre.
- **Pour les autres** : la personne regarde un point lointain devant elle (par une fenêtre) pendant que je pose la règle sous ses yeux. Je me place bien en face, je ferme l'œil droit et j'aligne le zéro sur le centre de sa pupille gauche, puis je ferme l'œil gauche et je lis en face de sa pupille droite.
- Chaque mesure est faite trois fois, je garde la valeur du milieu. Les valeurs sont notées en millimètres, au demi-millimètre.

## Les six valeurs

| Personne | Écart entre les pupilles (mm) |
|---|---|
| Moi | 63,0 |
| Personne 1 | 61,5 |
| Personne 2 | 66,0 |
| Personne 3 | 58,5 |
| Personne 4 | 64,0 |
| Personne 5 | 60,0 |

- **Moyenne** : somme des six valeurs divisée par 6 = 373,0 / 6 = 62,17 mm
- **Écart entre la plus petite et la plus grande** : plus grande valeur - plus petite valeur = 66,0 - 58,5 = 7,5 mm

## Comparaison avec la valeur du chapitre

Le chapitre donne une moyenne de **6,5 cm (65 mm)**, et une vraie valeur qui va d'environ 5,5 à 7 cm selon les personnes.

Une autre référence, pour situer : Dodgson (2004), qui a rassemblé les statistiques de la littérature, conclut que l'écart interpupillaire moyen des adultes est d'environ **63 mm**, que la grande majorité des adultes est entre 50 et 75 mm, et que 45 à 80 mm couvre presque tous les adultes (Dodgson N. A., « Variation and extrema of human interpupillary distance », SPIE Stereoscopic Displays and Virtual Reality Systems XI, 2004, DOI 10.1117/12.529999, texte lu sur http://www.neildodgson.com/pubs/EI5291A-05.pdf). Il note aussi qu'il y a « très peu d'accord » sur la moyenne, citée de 58 à 70 mm selon les sources.

Pour comparer mes résultats :

| | Moyenne | Fourchette |
|---|---|---|
| Chapitre | 65 mm | 55 à 70 mm |
| Dodgson 2004 | 63 mm | 50 à 75 mm (grande majorité des adultes) |
| Mes six mesures | 62,17 mm | 58,5 à 66,0 mm |

Ce que je regarde en comparant : ma moyenne est-elle plus proche de 63 ou de 65 mm, et mon plus petit et mon plus grand écart sortent-ils des 55 à 70 mm du chapitre ?

- **Ma moyenne (62,17 mm) est plus proche de 63 mm que de 65 mm** : elle est à 0,8 mm de la valeur de Dodgson (2004) et à 2,8 mm de celle du chapitre (elle est plus petite de 2,8 mm).
- **Aucune de mes valeurs ne sort de la fourchette du chapitre** : mon plus petit écart (58,5 mm) et mon plus grand (66,0 mm) tombent tous deux dans les 55 à 70 mm, et donc aussi dans les 50 à 75 mm de Dodgson.
- Mes six personnes s'étalent sur 7,5 mm entre la plus petite et la plus grande valeur.

## Ce qu'il faut garder en tête (indépendant des valeurs)

- **Six personnes, ce n'est pas la population.** La moyenne de mon échantillon n'a de raison de coller ni à 63 ni à 65 mm, et deux ou trois mm d'écart avec ces références n'ont rien d'anormal. L'écart entre la plus petite et la plus grande de mes six valeurs sert justement à cela : montrer à quel point l'écart entre les pupilles varie d'une personne à l'autre.
- **La règle limite la précision.** Avec une règle et un miroir, l'incertitude de lecture est de l'ordre du millimètre ou deux : la précision d'un pupillomètre n'est pas atteinte.
- **Pourquoi ça compte pour le casque** : le cours dit qu'employer une valeur fausse ne casse rien de visible, le monde paraît simplement plus petit ou plus grand. Un raisonnement géométrique simple, que je n'ai pas mesuré : si le rendu utilise 65 mm pour quelqu'un qui a 55 mm, l'écart est de 10 mm, soit 15 % ; le rapport 55 / 65 = 0,85 donne l'ordre de grandeur de l'erreur d'échelle (monde plus petit quand le rendu écarte trop les caméras, plus grand quand il les écarte trop peu). Avec mes propres valeurs, la personne à 58,5 mm serait à 6,5 mm de 65 mm (rapport 58,5 / 65 = 0,90), celle à 66,0 mm à 1,0 mm seulement. C'est pour cela qu'un programme ne doit pas figer l'écart à 65 mm, mais le lire dans le casque, réglé pour la personne.
