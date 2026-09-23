# Chapitre 2, exercice 4 - L'inverse d'une pose

## Le calcul

Une pose applique `p' = R p + t`, avec `R` la rotation du quaternion `q` et `t` la position. Pour revenir en arrière :

```
p = R⁻¹ (p' - t) = R⁻¹ p' - R⁻¹ t
```

Pour un quaternion unitaire, l'inverse de la rotation est celle du **conjugué** `q* = (-x, -y, -z, w)`. La pose inverse a donc :

- pour orientation : le conjugué de `q` ;
- pour position : `-(q* appliqué à t)`, c'est-à-dire la position opposée tournée par le conjugué.

`Inverser(pose)` est écrite exactement comme ça, à la main, sans matrice et sans inversion générale. Il n'y a pas de cas « singulier » à cacher : tant que le quaternion est unitaire, la pose est toujours inversible.

## Code

```cpp
// Chapitre 2, exercice 4 : l'inverse d'une pose, ecrit a la main.
// Si  p' = R p + t  alors  p = R^-1 (p' - t) = R^-1 p' - R^-1 t.
// Pour un quaternion unitaire, R^-1 est le conjugue. L'inverse de la pose
// a donc pour orientation le conjugue, et pour position -(conjugue * t).
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

Quat Conjugue(Quat q) { return { -q.x, -q.y, -q.z, q.w }; }

Vec3 Appliquer(const Pose& pose, Vec3 p) {
    return Tourner(pose.orientation, p) + pose.position;
}

// Pas d'inversion generale de matrice : ici, une pose qu'on ne sait pas
// inverser n'existe pas (le quaternion est unitaire), donc rien a garder en secret.
Pose Inverser(const Pose& pose) {
    Quat c = Conjugue(pose.orientation);
    return { Tourner(c, -1.0 * pose.position), c };
}

// Evite d'afficher "-0.000000" pour un zero negatif ou infime.
double Propre(double v) { return std::fabs(v) < 5e-7 ? 0.0 : v; }

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
    Vec3 aller = Appliquer(pose, p);                 // point dans l'espace
    Vec3 retour = Appliquer(Inverser(pose), aller);  // et on revient
    Vec3 ecart = retour - p;
    std::printf("point de depart   : %.6f %.6f %.6f\n", Propre(p.x), Propre(p.y), Propre(p.z));
    std::printf("apres la pose     : %.6f %.6f %.6f\n", Propre(aller.x), Propre(aller.y), Propre(aller.z));
    std::printf("apres l'inverse   : %.6f %.6f %.6f\n", Propre(retour.x), Propre(retour.y), Propre(retour.z));
    std::printf("ecart (norme)     : %.3e\n", Norme(ecart));
    return 0;
}
```

Le programme lit une pose et un point (`px py pz  qx qy qz qw  x y z`), applique la pose au point, applique l'inverse au résultat, et affiche l'écart au point de départ.

## Vérification

Pose : position (1, 2, 3), quaternion (0,5 ; 0,5 ; 0,5 ; 0,5) (rotation de 120° autour de (1, 1, 1), qui envoie `x` sur `y`, `y` sur `z` et `z` sur `x`), point (4, -1, 2).
À la main : la rotation donne (2, 4, -1), puis on ajoute la position : (3, 6, 2). Le programme trouve bien (3, 6, 2), puis revient à (4, -1, 2).

Entrée :

```
1 2 3  0.5 0.5 0.5 0.5  4 -1 2
```

Sortie :

```
point de depart   : 4.000000 -1.000000 2.000000
apres la pose     : 3.000000 6.000000 2.000000
apres l'inverse   : 4.000000 -1.000000 2.000000
ecart (norme)     : 0.000e+00
```

Un deuxième essai avec un quaternion quelconque, (0,1 ; 0,70710678… ; 0 ; 0,7), dont la norme est bien 1 (0,01 + 0,5 + 0,49), et une position de type « casque » :

Entrée :

```
0.3 1.6 -2  0.1 0.70710678118654752 0 0.7  1 1 1
```

Sortie :

```
point de depart   : 1.000000 1.000000 1.000000
apres la pose     : 1.431371 2.581421 -2.869949
apres l'inverse   : 1.000000 1.000000 1.000000
ecart (norme)     : 5.439e-16
```

L'écart est de l'ordre de 5 × 10⁻¹⁶, soit l'erreur d'arrondi d'un `double` : il est nul aux arrondis près, comme demandé.

Dernier essai, pour voir ce que vaut le contrat « quaternion unitaire » : le même quaternion, mais arrondi à quatre décimales (0,1 ; 0,7071 ; 0 ; 0,7). Sa norme² tombe à 0,99999 :

Entrée :

```
0.3 1.6 -2  0.1 0.7071 0 0.7  1 1 1
```

Sortie :

```
point de depart   : 1.000000 1.000000 1.000000
apres la pose     : 1.431370 2.581410 -2.869930
apres l'inverse   : 0.999981 0.999981 0.999981
ecart (norme)     : 3.322e-05
```

L'écart n'est plus nul (3 × 10⁻⁵) : l'aller-retour revient à `|q|⁴` près, soit 0,999981 fois le point de départ. Rien ne plante et l'erreur est minuscule, mais elle s'accumule si on compose beaucoup de poses. C'est pour cela qu'on renormalise les quaternions au moment où on les fabrique (par exemple après une intégration), pas dans `Inverser`.
