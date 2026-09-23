# Chapitre 1, démonstration 4 - Le retard réglable

## Ce qu'il faut montrer

Faire essayer mon programme à retard réglable (celui de l'exercice 12) à trois personnes de la classe, devant tout le monde, en montant le retard par paliers. Noter au tableau le seuil de chacune. Conclure sur ce que cela implique pour le budget d'une image.

## Préparation

- Le programme `retard` de l'exercice 12, lancé en plein écran de la salle si possible (le cercle est suivi par tout le monde sur le projecteur, la souris est tenue par le volontaire).
- La valeur du retard **cachée** à l'écran (touche H) : c'est moi qui règle, avec le clavier.
- Un tableau avec trois colonnes (une par volontaire) pour noter les seuils.

## Déroulé

1. Je présente le programme sans dire ce qu'est le retard : « bouge la souris, et dis-moi dès que tu sens quelque chose de différent ».
2. Pour le premier volontaire, je pars de 0 ms et j'augmente **par paliers de 20 ms** (Page haut). À chaque palier je le laisse bouger la souris quelques secondes. Quand il dit sentir quelque chose, j'appuie sur Entrée : le seuil est écrit dans la console et je le note au tableau.
3. Je peux affiner autour du seuil avec les pas de 5 ms (Haut/Bas).
4. Je recommence avec le deuxième et le troisième volontaire, sans leur montrer les valeurs des autres.
5. Quand les trois seuils sont au tableau, je les rapproche des 20 ms du budget, et je fais le calcul avec la classe.

## Les trois seuils

| Volontaire | Seuil (ms) |
|---|---|
| 1 | 40 |
| 2 | 60 |
| 3 | 30 |

Note : ces seuils s'ajoutent au retard que le matériel a déjà (souris, système, écran à 60 Hz), que je n'ai pas mesuré.

## Ce que cela implique pour le budget d'une image

Pour raisonner avec des chiffres du cours :

- À 90 Hz, une image dure **11,1 ms**. Une image qui ne serait pas prête à l'échéance est remplacée par la précédente : la tête a bougé, l'image ne s'est pas mise à jour, et le retard vu par l'utilisateur augmente d'une image entière, soit **11,1 ms** de plus. Deux images ratées, c'est 22,2 ms.
- Le budget total est de **20 ms**, dont une dizaine seulement pour le code (exercice 1 : 3,1 ms de reste à 90 Hz une fois les autres étapes retirées). Une seule image ratée consomme donc plus de la moitié de ce budget, et deux le dépassent.
- Mes seuils sont donc à comparer à ces sauts de 11,1 ms : **le seuil le plus bas (30 ms) est au-dessus de 22,2 ms**, donc au-dessus de deux images ratées ; il correspond à 2,7 images de 11,1 ms, et trois images de suite (33,3 ms) suffiraient à le dépasser. Les deux autres seuils (40 et 60 ms) valent 3,6 et 5,4 images. Aucun seuil n'est en dessous de 11,1 ms : une seule image ratée n'a été sentie par personne.

Les trois seuils sont tous au-dessus des 20 ms du budget, ce qui est attendu : ils s'ajoutent au retard du matériel (souris, système, écran à 60 Hz) et concernent un cercle qui suit la souris, pas le monde entier qui suit la tête. Les mesures de la littérature sur casque sont plus basses : Jerald et Whitton trouvent un seuil moyen de perception du retard de 16,6 ms, et concluent qu'un total acceptable est de l'ordre de 5 ms dans leurs conditions (IEEE VR 2009).

**Conclusion à dire à la classe :** ce qu'on tolère est bien plus bas que ce qu'on croit. Chaque image doit être prête à l'heure, car en rater une ajoute d'un coup un retard de l'ordre de ce que les gens perçoivent en casque, et il n'y a aucune marge pour une image qui traîne : on juge l'expérience à sa pire image, pas à la moyenne. Et comme mes trois volontaires ont des seuils différents (de 30 à 60 ms, du simple au double), il faut viser le plus sensible : celui qui sent à 30 ms, pas celui qui sent à 60 ms.

## Ce que la classe a dit

Les trois volontaires ont donné 40 ms, 60 ms et 30 ms : le plus sensible sent le retard deux fois plus tôt que le moins sensible. Leurs mots, tels quels :

| Volontaire | Seuil | Ce qu'il dit, tel quel |
|---|---|---|
| 1 | 40 ms | « Au début c'est bon, puis d'un coup le rond traîne derrière ma main, comme s'il était accroché avec un élastique. » |
| 2 | 60 ms | « Je ne sentais rien jusqu'à un moment, et là je vois que le rond arrive après. Je ne sais pas dire de combien, mais ça se voit surtout quand je vais vite. » |
| 3 | 30 ms | « Pour moi c'est déjà là, un tout petit peu. Quand je fais des cercles rapides le rond n'est plus sous ma main. » |

Réactions de la classe :

- « On pensait que 20 ms, c'était rien du tout. »
- « Le plus rapide des trois l'a senti à 30 ms, donc en casque, avec toute la scène qui bouge, ça doit être encore pire. »
- Une personne : « Donc si une seule image est en retard, ça fait déjà plus de 11 ms d'un coup ? » Réponse donnée : oui, à 90 Hz une image ratée ajoute 11,1 ms.

Ce que j'en retiens :

- **Les volontaires décrivent une position, pas un retard.** « Le rond traîne derrière ma main », « le rond arrive après », « le rond n'est plus sous ma main » : dans ces citations, personne n'emploie le mot « retard », alors que je le règle en millisecondes. C'est ce que dit le cours : l'utilisateur décrit une sensation, c'est à moi de remonter à la cause.
- **Le retard se voit surtout quand on va vite.** Les volontaires 2 et 3 le disent (« surtout quand je vais vite », « cercles rapides ») : l'écart affiché est la vitesse du geste multipliée par le retard, donc un même retard se remarque à peine sur un geste lent et bien plus sur un geste rapide. C'est le même mécanisme que pour la tête, où le retard se transforme en glissement du monde (exercice 12).
- **Le seuil est personnel.** Le volontaire 3 sent « déjà » le retard à 30 ms, le volontaire 2 seulement à 60 ms : on conçoit pour le plus sensible.
- **La classe fait d'elle-même le lien avec le casque et avec l'image ratée.** La question sur l'image en retard reçoit la bonne réponse : à 90 Hz, une image ratée ajoute 11,1 ms d'un coup, et il en faut entre deux et trois de suite (22,2 à 33,3 ms) pour atteindre le plus bas des trois seuils.
