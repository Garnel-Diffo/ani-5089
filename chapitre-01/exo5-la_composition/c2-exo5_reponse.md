# Chapitre 2, exercice 5 - La composition

## Le calcul

On compose deux poses : `A` est la pose du parent, `B` celle de l'enfant, exprimée dans le repère du parent. La composition doit donner la pose de l'enfant dans le repère du grand-parent, c'est-à-dire une pose `C` telle que

```
Appliquer(C, p) = Appliquer(A, Appliquer(B, p))
```

Pour un point `p` : `A (B p) = Ra (Rb p + tb) + ta = (Ra Rb) p + (Ra tb + ta)`. On lit directement les deux composantes de `C` :

- orientation : `qa * qb` (le produit de Hamilton, qui applique `qb` d'abord puis `qa`) ;
- position : `Ra tb + ta`, soit la position de l'enfant tournée par l'orientation du parent, puis décalée de la position du parent.

C'est ce que fait `Composer(parent, enfant)`. L'ordre des arguments compte : composer n'est pas commutatif, exactement comme tourner puis avancer n'est pas avancer puis tourner.

Format d'entrée : pose `A` (7 réels : `px py pz qx qy qz qw`), pose `B` (7 réels), puis le point (3 réels).

## Code

```cpp
// Chapitre 2, exercice 5 : la composition de deux poses.
// Composer(parent, enfant) : l'enfant est exprime dans le repere du parent.
//   position    = orientation_parent * position_enfant + position_parent
//   orientation = orientation_parent * orientation_enfant
// Propriete a verifier : Appliquer(Composer(A, B), p) == Appliquer(A, Appliquer(B, p)).
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

// Produit de Hamilton : (a * b) applique b d'abord, puis a.
Quat operator*(Quat a, Quat b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

Vec3 Tourner(Quat q, Vec3 v) {
    Vec3 u = { q.x, q.y, q.z };
    return (q.w * q.w - Produit(u, u)) * v + (2.0 * Produit(u, v)) * u + (2.0 * q.w) * Vectoriel(u, v);
}

Vec3 Appliquer(const Pose& pose, Vec3 p) {
    return Tourner(pose.orientation, p) + pose.position;
}

Pose Composer(const Pose& parent, const Pose& enfant) {
    return { Tourner(parent.orientation, enfant.position) + parent.position,
             parent.orientation * enfant.orientation };
}

// Evite d'afficher "-0.000000" pour un zero negatif ou infime.
double Propre(double v) { return std::fabs(v) < 5e-7 ? 0.0 : v; }

bool LirePose(Pose& p) {
    return static_cast<bool>(std::cin >> p.position.x >> p.position.y >> p.position.z
                                      >> p.orientation.x >> p.orientation.y >> p.orientation.z >> p.orientation.w);
}

int main() {
    Pose a, b;
    Vec3 p;
    // Entree : pose A (7 reels), pose B (7 reels), point (3 reels)
    if (!LirePose(a) || !LirePose(b) || !(std::cin >> p.x >> p.y >> p.z)) {
        std::fprintf(stderr, "Il faut 17 reels : pose A (7), pose B (7), point (3)\n");
        return 1;
    }
    Vec3 direct = Appliquer(Composer(a, b), p);   // composer puis appliquer
    Vec3 pas_a_pas = Appliquer(a, Appliquer(b, p)); // appliquer B, puis A
    std::printf("composer puis appliquer : %.6f %.6f %.6f\n", Propre(direct.x), Propre(direct.y), Propre(direct.z));
    std::printf("appliquer B puis A      : %.6f %.6f %.6f\n", Propre(pas_a_pas.x), Propre(pas_a_pas.y), Propre(pas_a_pas.z));
    std::printf("ecart (norme)           : %.3e\n", Norme(direct - pas_a_pas));
    return 0;
}
```

## Vérification sur des cas

Premier cas, choisi pour qu'on puisse suivre à la main : `A` a pour position (1, 0, 0) et tourne de 90° autour de `y`, `B` a pour position (0, 2, 0) et tourne de 90° autour de `x`, le point est (1, 1, 1).
`B` envoie (1, 1, 1) sur (1, -1, 1) puis (1, 1, 1) une fois décalé de (0, 2, 0). `A` envoie (1, 1, 1) sur (1, 1, -1), puis on ajoute (1, 0, 0) : (2, 1, -1). C'est bien ce que donnent les deux chemins.

Entrée :

```
1 0 0  0 0.70710678118654752 0 0.70710678118654752   0 2 0  0.70710678118654752 0 0 0.70710678118654752   1 1 1
```

Sortie :

```
composer puis appliquer : 2.000000 1.000000 -1.000000
appliquer B puis A      : 2.000000 1.000000 -1.000000
ecart (norme)           : 4.965e-16
```

Deuxième cas avec des angles moins ronds : `A` de 45° autour de `y`, `B` de 30° autour de `x`, positions quelconques, point (0,1 ; 0 ; -1).

Entrée :

```
0.5 1 0  0 0.3826834323650898 0 0.9238795325112867   0 0.2 0.3  0.2588190451025207 0 0 0.9659258262890683   0.1 0 -1
```

Sortie :

```
composer puis appliquer : 0.170470 1.700000 -0.470951
appliquer B puis A      : 0.170470 1.700000 -0.470951
ecart (norme)           : 2.483e-16
```
