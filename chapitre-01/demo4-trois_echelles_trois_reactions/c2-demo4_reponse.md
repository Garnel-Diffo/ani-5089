# Chapitre 2, démonstration 4 - Trois échelles, trois réactions

## Ce qu'il faut montrer

Faire décrire ma salle par trois personnes de la classe, à trois facteurs d'échelle différents, sans leur dire lequel. Noter leurs mots au tableau. Conclure sur le pourquoi : l'auteur d'un monde est le plus mal placé pour en juger l'échelle.

## Préparation

Le programme de l'exercice 11, qui affiche les dimensions de la salle multipliées par un facteur. Trois facteurs, dans un ordre que la classe ne connaît pas :

| Facteur | Plafond | Porte | Table | Ce qui doit sembler bizarre |
|---|---|---|---|---|
| 0,6 | 1,50 m | 1,20 m | 0,48 m | un monde de poupée : on se sent grand |
| 1,0 | 2,50 m | 2,00 m | 0,80 m | rien, c'est la vraie salle |
| 1,7 | 4,25 m | 3,40 m | 1,36 m | un monde de géants : on se sent petit |

(Les valeurs viennent de la sortie du programme pour chaque facteur.) Trois volontaires qui n'ont pas vu la salle. Chacun ne voit que **sa** sortie, sans le mot « facteur ». Un marqueur pour noter leurs mots.

## Déroulé

1. Je donne à chaque volontaire sa sortie et je lui demande de me décrire la salle, en une ou deux phrases, comme s'il y était.
2. Je note ses mots, tels quels, au tableau, dans une colonne par volontaire.
3. Je révèle les trois facteurs et je rapproche chaque description de son facteur.
4. Je conclus (voir plus bas).

## Les trois descriptions

| Volontaire | Facteur (révélé après) | Ses mots, tels quels |
|---|---|---|
| 1 | 0,6 | « une petite pièce, le plafond est bas, la porte est plus petite qu'une porte normale » |
| 2 | 1,0 | « une pièce de séjour normale, rien de bizarre » |
| 3 | 1,7 | « une très grande salle, le plafond est très haut, la table est énorme » |

Chaque description va dans le sens du facteur, sans que les volontaires l'aient su : petite pour 0,6, normale pour 1,0, très grande pour 1,7. Aucun n'a dit qu'il y avait une erreur : ils ont décrit une sensation de taille.

## Conclusion : pourquoi l'auteur est le plus mal placé

- **L'auteur connaît les valeurs.** Il sait que la porte doit faire 2 m, donc pour lui 3,40 m est une erreur qu'il repère dans un tableau de chiffres. Mais une fois dans le monde, il l'a construit, il y est habitué, et son cerveau accepte les tailles, quelles qu'elles soient. Un objet à la mauvaise échelle est le défaut le plus fréquent des premières expériences en réalité virtuelle, et le plus difficile à voir soi-même.
- **Le visiteur, lui, n'a que sa sensation** : « c'est bizarre, je me sens petit », sans savoir pourquoi. Il n'a aucun chiffre pour se rattacher, donc il réagit à ce qu'il ressent, pas à ce qu'il sait.
- **L'erreur ne se signale pas.** Le programme tourne avec un facteur de 0,6 comme avec 1,7, sans plantage et sans message. Aucun débogueur ne l'affiche : la sensation ne s'affiche pas dans un débogueur.
- **D'où la méthode :** faire essayer à quelqu'un d'autre, et l'écouter. C'est ce que la démonstration vient de faire avec trois personnes.
