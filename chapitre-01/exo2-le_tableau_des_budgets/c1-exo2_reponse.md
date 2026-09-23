# Chapitre 1, exercice 2 - Le tableau des budgets

Le tableau du chapitre a cinq étapes. Pour chacune j'ai cherché une valeur **mesurée**, avec sa source. Quand je n'ai rien trouvé de solide, je le dis dans la colonne de droite : je préfère un « introuvable » à une valeur inventée.

## Le tableau

| Étape | Valeur du chapitre | Ce que j'ai trouvé | Source | Verdict |
|---|---|---|---|---|
| Les capteurs mesurent le mouvement | 1 à 2 ms | Le délai du capteur est indiqué **< 1 ms**. L'unité inertielle d'un Oculus Rift est échantillonnée à **1000 Hz** pour la rotation, donc une mesure par milliseconde. Un tableau de synthèse donne 1 à 2 ms pour l'IMU. | [1], [4] | Confirmé pour l'ordre de grandeur (de moins de 1 à 2 ms) |
| Le système transmet la mesure | 1 à 3 ms | Un seul chiffre : **1 à 4 ms** pour envoyer les données du capteur au processeur *et* calculer la pose 6DoF. C'est un tableau de synthèse qui cite lui-même un message de forum. | [4] | **Introuvable** en source solide |
| Votre application décide et dessine | 5 à 11 ms | Pas de valeur fixe : c'est ce que prend l'application. La borne haute est l'intervalle entre deux images : « à 90 Hz, environ 11 ms ». Un seul appel de dessin avec un shader complexe peut prendre « facilement 10 ms ». Sur un budget de 20 ms, en retirant le capteur (< 1 ms) et l'affichage (5 ms visés), il reste « 14 ms pour le calcul et la communication ». | [1], [3] | Cohérent avec 5 à 11 ms, mais c'est une borne, pas une mesure. Ma propre mesure est dans les exercices 6 et 7 |
| Le compositeur assemble | 1 à 2 ms | Le tableau de synthèse [4] donne 1 à 2 ms et cite l'article d'Antonov [3]. Dans ce que j'ai pu lire de cet article, aucun coût mesuré du compositeur n'apparaît : il demande une préemption de « 2 ms ou moins » et que la correction s'exécute en « moins de 11 ms ». | [3], [4] | **Introuvable** comme mesure (le 2 ms cité est une exigence, pas un coût) |
| L'écran affiche la ligne | 2 à 5 ms | Le délai d'affichage est estimé à **≈ 10 à 15 ms** aujourd'hui, appelé à descendre vers **5 ms**. Sur les casques mesurés, l'écran n'est éclairé que **2 ms** à la fin de chaque image de 11,1 ms (Rift), et 0,33 ms sur un Valve Index. | [1], [2] | Partiel : 2 à 5 ms ressemble à l'objectif de 5 ms plus qu'à la mesure de 10 à 15 ms. Le 2 ms du Rift est une durée d'éclairage, pas un délai |

Les mots entre guillemets viennent des sources (traduits de l'anglais).

## Les sources

1. Elbamby M. S., Perfecto C., Bennis M., Doppler K., *Towards Low-Latency and Ultra-Reliable Virtual Reality*, 2018. arXiv:1801.07587. https://arxiv.org/abs/1801.07587 (lu : délai capteur < 1 ms, délai d'affichage ≈ 10 à 15 ms attendu vers 5 ms, « 14 ms pour le calcul et la communication »).
2. Warburton M., Mon-Williams M., Mushtaq F., Morehead J. R., *Measuring motion-to-photon latency for sensorimotor experiments with virtual reality systems*, Behavior Research Methods, 2022 (DOI 10.3758/s13428-022-01983-5). J'ai lu la préversion : https://www.biorxiv.org/content/10.1101/2022.06.24.497509 (lu : persistance de 0,33 ms pour le Valve Index à 2 ms pour le Rift ; sur un Rift, écran noir 9,1 ms puis éclairé 2 ms par image de 11,1 ms).
3. Antonov M., *Asynchronous Timewarp Examined*, blog développeurs Oculus, 2 mars 2015. https://developers.meta.com/horizon/blog/asynchronous-timewarp-examined/ (lu : « à 90 Hz, l'intervalle entre images est d'environ 11 ms », préemption « d'environ 2 ms ou moins », un appel de dessin complexe « peut facilement prendre 10 ms »).
4. VR & AR Wiki, *Motion-to-photon latency*. https://vrarwiki.com/wiki/Motion-to-photon_latency, et la page *Oculus Rift* https://vrarwiki.com/wiki/Oculus_Rift (rotation : 1000 Hz). Source secondaire : un wiki, qui renvoie lui-même à d'autres pages.

## Ce que je n'ai pas pu vérifier

- L'article d'Abrash, « Latency - the sine qua non of AR and VR » (blog de Valve), n'a pas pu être ouvert (erreur de connexion sécurisée). Je ne m'appuie donc sur aucun chiffre qui en vienne.
- L'article « Building a Sensor for Low Latency VR » (blog Meta Quest) : la connexion a été refusée. Idem.
- L'article de Warburton et coll. : la version publiée demande une connexion, j'ai lu la préversion. Les chiffres cités peuvent différer un peu de la version finale.

## Ce que j'en conclus

- Deux étapes ont une valeur publiée que j'ai pu lire (capteur, écran), et elles sont proches de celles du chapitre, avec une réserve sur l'écran : 2 à 5 ms est plutôt un objectif que ce qui est mesuré (10 à 15 ms dans [1]).
- Deux étapes n'ont pas de mesure solide dans ce que j'ai trouvé : la transmission et le compositeur. Leur fourchette du chapitre reste à prendre comme une estimation.
- L'étape « application » n'est pas une constante : c'est le reste, et c'est celle qui m'appartient.
- Si on additionne les fourchettes du chapitre, on trouve de 10 ms (1 + 1 + 5 + 1 + 2) à 23 ms (2 + 3 + 11 + 2 + 5). Les 20 ms visés ne sont tenus que vers le milieu des fourchettes.
- Pour situer un total mesuré : dans [2], le retard des manettes (pas de la tête) va de 21 à 42 ms en moyenne au début d'un mouvement brusque, selon le casque, et de 2 à 13 ms une fois la prédiction du mouvement en route. La même étude rappelle que des rotations appliquées à un Oculus Rift DK2 avaient donné des retards de 40 à 85 ms dans la plupart des études (1 à 26 ms dans une minorité, selon les réglages) : le budget de 20 ms est un objectif, pas ce que tous les systèmes atteignent.
