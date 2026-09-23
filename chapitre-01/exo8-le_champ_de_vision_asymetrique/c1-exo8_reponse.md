# Chapitre 1, exercice 8 - Le champ de vision asymétrique

## Les quatre angles

Casque : **HTC Vive** (premier modèle), **œil gauche**. Angles mesurés depuis l'axe optique de la lentille, tels que le casque les annonce à l'application par OpenVR :

| Côté | Angle |
|---|---|
| gauche | **54,41°** |
| droite | **51,35°** |
| bas | **55,91°** |
| haut | **55,67°** |

Source : Richard Musil, « VR headset rendered FOV calculation » (29 juillet 2019), site *VR Docs* : https://risa2000.github.io/vrdocs/docs/hmd_fov_calculation.html. Valeurs collectées auprès du casque par OpenVR (version sans masque de zone cachée).

## Ce qui se passerait avec un champ symétrique de même surface

**En une phrase :** l'image serait décalée par rapport à l'optique, d'environ 30 pixels sur les 1080 de la largeur d'un œil (de l'ordre de 4° au centre), dans un sens pour l'œil gauche et dans l'autre pour le droit, ce qui déforme légèrement le monde vers les bords et fausse un peu la profondeur, sans qu'aucun message ne l'annonce.
