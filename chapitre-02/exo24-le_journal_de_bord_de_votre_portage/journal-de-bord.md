# Journal de bord de mon portage

## Entrée 1

Symptôme : `jenga build` compile `main.cpp` sans erreur, puis échoue à l'édition de liens avec `LINK : fatal error LNK1104: impossible d'ouvrir le fichier 'LIBCMT.lib'`.
J'ai cru : que mon `projet.jenga` était mal écrit, ou que Jenga n'avait pas trouvé le bon compilateur.
C'était : le shell dans lequel j'ai lancé `jenga build` n'avait pas les variables d'environnement de Visual Studio (`vcvars64.bat` jamais appelé). Jenga avait bien détecté `cl.exe`, mais sans les chemins vers les bibliothèques système, l'éditeur de liens ne trouve rien.
Temps perdu : 10

## Entrée 2

Symptôme : `g++ --version` se termine par `Segmentation fault` côté shell, et par le code de sortie `-1073741819` (0xC0000005, violation d'accès) côté PowerShell.
J'ai cru : que j'avais mal orthographié la commande, ou que ce `g++` acceptait mal l'option `--version`.
C'était : ce `g++` était un exécutable 32 bits ancien, installé par un IDE tiers, incompatible avec cette machine : il plante dès le lancement, quelle que soit la commande.
Temps perdu : 8

## Entrée 3

Symptôme : dans un script `.bat`, `sdkmanager.bat` (pourtant dans le dossier courant après un `cd /d`) répond « n'est pas reconnu en tant que commande interne ou externe ».
J'ai cru : que le PATH n'incluait pas ce dossier, ou que le fichier avait été renommé.
C'était : le nom du dossier de mon compte contient un espace, et le `cd /d` suivi du nom nu du programme ne suffisait pas dans ce contexte précis ; il a fallu appeler le chemin complet entre guillemets.
Temps perdu : 5

## Entrée 4

Symptôme : un téléchargement de 639 Mo avance à 20 kilo-octets par seconde, ce qui l'aurait fait durer plus de huit heures.
J'ai cru : que le fichier distant était simplement gros, et qu'il fallait attendre.
C'était : deux téléchargements lancés en même temps se partageaient la même bande passante limitée ; lancé seul, le même fichier avance à plus de trois mégaoctets par seconde.
Temps perdu : 15

## Entrée 5

Symptôme : `cat -A sortie-build.txt` montre des séquences comme `^[[36m` et `M-bM-^UM-^Q` au lieu du texte coloré attendu.
J'ai cru : que la commande de build avait échoué silencieusement ou corrompu le fichier.
C'était : la sortie de Jenga contient des séquences de couleur ANSI, normales dans un terminal qui les interprète, mais illisibles telles quelles dans un fichier texte ; il a fallu les retirer pour obtenir une copie propre.
Temps perdu : 6

## Entrée 6

Symptôme : `UnicodeEncodeError: 'charmap' codec can't encode characters in position 9-76: character maps to <undefined>`.
J'ai cru : que le fichier lui-même était mal enregistré.
C'était : le fichier était correct ; c'est la console qui l'affichait dans un encodage (cp1252) incapable de montrer les caractères de dessin de boîte (`╔`, `║`) que Jenga utilise pour son bandeau. Le fichier écrit sur le disque était intact.
Temps perdu : 4

## Entrée 7

Symptôme : un second téléchargement (le paquet du système Android) reste affiché à 40 % plusieurs minutes de suite, sans que le pourcentage ne bouge.
J'ai cru : que le processus avait planté sans le signaler.
C'était : le processus Java qui portait le téléchargement était toujours vivant mais son temps processeur n'augmentait presque plus : le téléchargement était réellement bloqué, pas seulement lent. Il a fallu le constater par le temps CPU cumulé, pas par l'affichage.
Temps perdu : 12

## Entrée 8

Symptôme : `jenga build --platform android-arm64` compile et lie `libPlantage.so` sans erreur, affiche « Build Successful », puis s'arrête net après « Building APK for Plantage (arm64-v8a) », code de sortie 1, sans le moindre message.
J'ai cru : que l'étape d'empaquetage de l'APK manquait un outil du SDK (aapt2, android.jar), ou que le build-tools installé était incomplet.
C'était : le dossier de build se trouvait sous un chemin de plus de 260 caractères (le nom complet de mon dossier de projet, très long, imbriqué dans plusieurs sous-dossiers). `zipalign.exe`, un outil natif ancien, ne sait pas ouvrir un fichier au-delà de cette limite historique de Windows et échoue sans écrire de message ; Jenga, de son côté, ne relaie aucune erreur quand cette étape échoue. Rejouer le même build depuis un chemin court (`C:\andbuild\...`) a suffi à faire apparaître l'APK.
Temps perdu : 25

## Entrée 9

Symptôme : l'APK s'installe et se lance sur le téléphone, mais se ferme instantanément ; `adb logcat` montre `UnsatisfiedLinkError: dlopen failed: library "libc++_shared.so" not found`.
J'ai cru : que `libPlantage.so` lui-même était mal compilé ou corrompu.
C'était : le NDK lie par défaut le run-time C++ dans une bibliothèque partagée séparée (`libc++_shared.so`) que l'APK doit embarquer en plus de `libPlantage.so`. Le chemin de compilation que j'utilisais (une seule architecture ciblée) ne copiait pas ce fichier dans l'APK, contrairement au chemin multi-architectures. Demander explicitement un run-time statique (`androidstl("c++_static")` dans le fichier `.jenga`) a supprimé le besoin de cette bibliothèque séparée.
Temps perdu : 18
