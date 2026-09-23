# Chapitre 2, exercice 3 - L'ordre inverse

## Ce que fait le programme

Il reprend la structure `Pose` de l'exercice précédent et applique la même pose au même point de deux façons :

- `AppliquerRotationPuisTranslation` : `orientation * p + position` (l'ordre du module) ;
- `AppliquerTranslationPuisRotation` : `orientation * (p + position)` (l'ordre inverse).

Il affiche les deux résultats et la distance qui les sépare. Le format d'entrée est le même qu'à l'exercice 2 : `px py pz  qx qy qz qw  x y z`.

## Code

```cpp
// Chapitre 2, exercice 3 : l'ordre inverse.
// Deux facons d'appliquer la meme pose a un point :
//   - la bonne  : rotation puis translation   (orientation * p + position)
//   - l'inverse : translation puis rotation   (orientation * (p + position))
#include <cmath>
#include <cstdio>
#include <iostream>

struct Vec3 { double x, y, z; };
struct Quat { double x, y, z, w; };
struct Pose { Vec3 position; Quat orientation; };

Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 operator*(double s, Vec3 v) { return { s * v.x, s * v.y, s * v.z }; }
double Produit(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 Vectoriel(Vec3 a, Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
double Norme(Vec3 v) { return std::sqrt(Produit(v, v)); }

Vec3 Tourner(Quat q, Vec3 v) {
    Vec3 u = { q.x, q.y, q.z };
    return (q.w * q.w - Produit(u, u)) * v + (2.0 * Produit(u, v)) * u + (2.0 * q.w) * Vectoriel(u, v);
}

// Ordre du module : on tourne d'abord, on deplace ensuite.
Vec3 AppliquerRotationPuisTranslation(const Pose& pose, Vec3 p) {
    return Tourner(pose.orientation, p) + pose.position;
}

// Ordre inverse : on deplace d'abord, on tourne ensuite.
Vec3 AppliquerTranslationPuisRotation(const Pose& pose, Vec3 p) {
    return Tourner(pose.orientation, p + pose.position);
}

double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

void Afficher(const char* nom, Vec3 v) {
    std::printf("%-28s %.4f %.4f %.4f\n", nom, Propre(v.x), Propre(v.y), Propre(v.z));
}

int main() {
    Pose pose;
    Vec3 p;
    // Entree : px py pz  qx qy qz qw  x y z
    if (!(std::cin >> pose.position.x >> pose.position.y >> pose.position.z
                    >> pose.orientation.x >> pose.orientation.y >> pose.orientation.z >> pose.orientation.w
                    >> p.x >> p.y >> p.z)) {
        std::fprintf(stderr, "Il faut 10 reels : px py pz qx qy qz qw x y z\n");
        return 1;
    }
    Vec3 a = AppliquerRotationPuisTranslation(pose, p);
    Vec3 b = AppliquerTranslationPuisRotation(pose, p);
    Afficher("rotation puis translation", a);
    Afficher("translation puis rotation", b);
    std::printf("%-28s %.4f\n", "ecart", Propre(Norme(a - b)));
    return 0;
}
```

## Essais : les deux ordres ne donnent pas la même chose

Pose : rotation de 90° autour de `y`, position (1, 2, 3), point (1, 0, 0).

Entrée :

```
1 2 3  0 0.70710678118654752 0 0.70710678118654752  1 0 0
```

Sortie :

```
rotation puis translation    1.0000 2.0000 2.0000
translation puis rotation    3.0000 2.0000 -2.0000
ecart                        4.4721
```

Dans l'ordre du module, le point tourne autour de l'origine de l'entité (on tourne « sur soi-même »), puis on déplace : on tombe en (1, 2, 2). Dans l'ordre inverse, on déplace d'abord le point en (2, 2, 3), puis on le fait tourner autour de l'origine du monde : il « part en orbite » et arrive en (3, 2, -2). L'écart est de 4,47 m sur ce petit exemple, ce n'est pas un détail d'arrondi.

## Une pose et un point pour lesquels les deux coïncident

Les deux résultats sont égaux quand

```
R p + t = R (p + t)   ⟺   R p + t = R p + R t   ⟺   R t = t
```

Ce qui compte n'est pas le point `p` mais la position `t` : les deux ordres coïncident **pour n'importe quel point** si et seulement si la rotation laisse la position inchangée, c'est-à-dire :

- si `t` est portée par l'axe de la rotation (tourner autour d'un axe ne déplace pas ce qui est sur l'axe) ;
- ou si la position est nulle (`t = 0`) ;
- ou si la rotation est l'identité.

Exemple pris pour la démonstration : rotation de 90° autour de `y`, position (0, 3, 0) (donc sur l'axe `y`), point (1, 0, 0). Les deux ordres donnent (0, 3, -1).

Entrée :

```
0 3 0  0 0.70710678118654752 0 0.70710678118654752  1 0 0
```

Sortie :

```
rotation puis translation    0.0000 3.0000 -1.0000
translation puis rotation    0.0000 3.0000 -1.0000
ecart                        0.0000
```

Les deux autres cas, pour vérifier : orientation identité, puis position nulle avec une rotation quelconque (45° autour de `y`).

Entrée :

```
1 2 3  0 0 0 1  4 5 6
```

Sortie :

```
rotation puis translation    5.0000 7.0000 9.0000
translation puis rotation    5.0000 7.0000 9.0000
ecart                        0.0000
```

Entrée :

```
0 0 0  0 0.3826834323650898 0 0.9238795325112867  1 2 3
```

Sortie :

```
rotation puis translation    2.8284 2.0000 1.4142
translation puis rotation    2.8284 2.0000 1.4142
ecart                        0.0000
```

## Pourquoi c'est important

Si on se trompe d'ordre, le programme ne plante pas : les objets se mettent simplement à tourner autour de l'origine du monde au lieu de tourner sur place. C'est une erreur qui se voit à l'écran mais qu'aucun message n'annonce.
