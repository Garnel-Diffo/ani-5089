# Chapitre 2, exercice 1 - Les trois directions

## Ce que fait le programme

Il définit une seule fois la convention du module : repère direct, main droite, `+x` vers la droite, `+y` vers le haut, et l'avant qui regarde les `z` négatifs. Les trois fonctions `Avant()`, `Haut()` et `Droite()` rendent chacune un vecteur unitaire :

| Fonction | Vecteur |
|---|---|
| `Avant()` | (0, 0, -1) |
| `Haut()` | (0, 1, 0) |
| `Droite()` | (1, 0, 0) |

Le `main` lit trois réels, les traite comme un point, et affiche son produit scalaire avec `Avant()`, puis `Haut()`, puis `Droite()`. Trois lignes, quatre décimales, rien d'autre sur la sortie standard (pas d'étiquette, pour qu'on puisse relire la sortie avec un autre programme).

Comme le point est ici un vecteur depuis l'origine, chaque produit scalaire est simplement la coordonnée du point dans cette direction : `-z` pour l'avant, `y` pour le haut, `x` pour la droite. Le signe moins de l'avant est justement le piège du chapitre.

## Code

```cpp
// Chapitre 2, exercice 1 : les trois directions.
// Convention du module (celle d'OpenXR) : repere direct, main droite,
// +x vers la droite, +y vers le haut, et l'avant regarde les z negatifs.
#include <cmath>
#include <cstdio>
#include <iostream>

struct Vec3 { double x, y, z; };

// La convention est ecrite ici, une fois, et nulle part ailleurs.
Vec3 Avant()  { return { 0.0, 0.0, -1.0 }; }
Vec3 Haut()   { return { 0.0, 1.0,  0.0 }; }
Vec3 Droite() { return { 1.0, 0.0,  0.0 }; }

double Produit(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Un produit nul peut sortir en "-0.0000" (zero negatif). On l'evite pour
// ne pas faire croire a une erreur de signe.
double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

int main() {
    Vec3 p;
    if (!(std::cin >> p.x >> p.y >> p.z)) {
        std::fprintf(stderr, "Il faut trois reels : x y z\n");
        return 1;
    }
    std::printf("%.4f\n", Propre(Produit(p, Avant())));
    std::printf("%.4f\n", Propre(Produit(p, Haut())));
    std::printf("%.4f\n", Propre(Produit(p, Droite())));
    return 0;
}
```

Compilation et lancement (Visual Studio, invite de commandes x64) :

```
cl /EHsc /O2 /std:c++17 c2exo1.cpp
echo 1 2 3 | c2exo1
```

## Essais

Un point devant, en haut et à droite : (1, 2, 3). Le `z` vaut 3, donc le point est derrière, et l'avant sort à -3.

Entrée :

```
1 2 3
```

Sortie :

```
-3.0000
2.0000
1.0000
```

Un point (-1, -2, 0) : le produit avec l'avant est nul. Le programme affiche `0.0000` et pas `-0.0000` : un zéro négatif (`-1 × 0` donne `-0.0` en virgule flottante) s'afficherait avec son signe, et on croirait à une erreur.

Entrée :

```
-1 -2 0
```

Sortie :

```
0.0000
-2.0000
-1.0000
```

Un point placé à 4 mètres devant (z = -4) : l'avant sort à +4, ce qui est bien le sens voulu.

Entrée :

```
0.5 0 -4
```

Sortie :

```
4.0000
0.0000
0.5000
```

Entrée invalide : le programme le dit sur la sortie d'erreur et rend un code de retour non nul.

Entrée :

```
abc
```

Message d'erreur (sortie d'erreur, code retour 1) :

```
Il faut trois reels : x y z
```

## Remarque

Le jour où on tape `position.z -= vitesse` en doutant du signe, on ne se pose pas la question dans le code du jeu : on appelle `Avant()`. La réponse est à un seul endroit du programme, et si la convention devait changer, une seule ligne changerait.
