# Chapitre 2, exercice 2 - La pose appliquée

## Ce que fait le programme

La structure `Pose` contient une position (`Vec3`, en mètres) et une orientation (`Quat`, un quaternion unitaire). Il n'y a pas d'échelle : un casque ne redimensionne pas la tête de son porteur.

La fonction `Appliquer(pose, p)` applique la pose à un point dans l'ordre du module, **rotation puis translation** :

```
p_espace = orientation * p_entite + position
```

Pour la rotation je n'ai pas construit de matrice. J'utilise la forme développée de `q v q*`, avec `u = (x, y, z)` la partie vectorielle du quaternion et `w` sa partie scalaire :

```
Tourner(q, v) = (w² - u·u) v + 2 (u·v) u + 2 w (u × v)
```

Elle est équivalente à `q v q*` quand le quaternion est unitaire, ce que l'énoncé garantit (« déjà normalisé »). Je ne renormalise donc pas à l'entrée : la fonction fait confiance au contrat.

Format d'entrée (10 réels, séparés par des espaces ou des retours à la ligne) :

```
px py pz  qx qy qz qw  x y z
```

La position est lue avant l'orientation, comme dans l'énoncé (« une position et un quaternion »), et le quaternion est donné dans l'ordre `x y z w`. La sortie est le point transformé, sur une ligne, quatre décimales.

## Code

```cpp
// Chapitre 2, exercice 2 : la pose appliquee.
// Une pose = une position (en metres) + une orientation (quaternion unitaire).
// Pas d'echelle. Ordre d'application : on tourne d'abord, on deplace ensuite.
//   p_espace = orientation * p_entite + position
#include <cmath>
#include <cstdio>
#include <iostream>

struct Vec3 { double x, y, z; };
struct Quat { double x, y, z, w; };          // (x, y, z) = partie vectorielle, w = scalaire
struct Pose { Vec3 position; Quat orientation; };

Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator*(double s, Vec3 v) { return { s * v.x, s * v.y, s * v.z }; }
double Produit(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 Vectoriel(Vec3 a, Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

// q v q* developpe : (w^2 - |u|^2) v + 2 (u.v) u + 2 w (u x v), avec u = (x, y, z).
// Pas de matrice a construire, et pas de quaternion intermediaire.
Vec3 Tourner(Quat q, Vec3 v) {
    Vec3 u = { q.x, q.y, q.z };
    return (q.w * q.w - Produit(u, u)) * v + (2.0 * Produit(u, v)) * u + (2.0 * q.w) * Vectoriel(u, v);
}

Vec3 Appliquer(const Pose& pose, Vec3 p) {
    return Tourner(pose.orientation, p) + pose.position;
}

// Evite d'afficher "-0.0000" quand un resultat est un zero negatif ou infime.
double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

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
    Vec3 r = Appliquer(pose, p);
    std::printf("%.4f %.4f %.4f\n", Propre(r.x), Propre(r.y), Propre(r.z));
    return 0;
}
```

## Essais

Pose : position (1, 2, 3), rotation de 90° autour de l'axe vertical `y` (quaternion (0, sin 45°, 0, cos 45°)), point (1, 0, 0).
À la main : la rotation de 90° autour de `y` envoie (1, 0, 0) sur (0, 0, -1) (l'avant), puis on ajoute la position : (1, 2, 2).

Entrée :

```
1 2 3  0 0.70710678118654752 0 0.70710678118654752  1 0 0
```

Sortie :

```
1.0000 2.0000 2.0000
```

Pose identité (rotation nulle, position nulle) : le point ne bouge pas.

Entrée :

```
0 0 0  0 0 0 1  4 5 6
```

Sortie :

```
4.0000 5.0000 6.0000
```

Un cas plus « casque » : une pose à 1,60 m de haut, tournée de 90° autour de `x` (règle de la main droite : `y` bascule vers `z`, donc l'avant de la tête, qui était `-z`, pointe maintenant vers `+y`, la tête regarde vers le haut), et un point à un mètre devant (0, 0, -1). Le point suit la rotation : il passe à un mètre au-dessus de la tête, donc à 2,60 m du sol.

Entrée :

```
0 1.6 0  0.70710678118654752 0 0 0.70710678118654752  0 0 -1
```

Sortie :

```
0.0000 2.6000 0.0000
```
