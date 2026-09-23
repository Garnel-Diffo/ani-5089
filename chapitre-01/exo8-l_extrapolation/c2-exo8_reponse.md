# Chapitre 2, exercice 8 - L'extrapolation

## Ce que fait la fonction

`Extrapoler(pose, vitesse_lineaire, vitesse_angulaire, dt)` avance une pose de `dt` secondes en gardant les deux vitesses constantes.

- **Position** : `p += v × dt`, en mètres, avec `v` en mètres par seconde.
- **Orientation** : on compose l'orientation actuelle avec la rotation faite pendant `dt`. La vitesse angulaire `ω` (en radians par seconde) est un vecteur dont la direction est l'axe et la norme la vitesse de rotation. Pendant `dt` la tête tourne de l'angle `θ = |ω| dt` autour de l'axe `ω / |ω|`, ce qui donne le quaternion

```
Δq = ( ω · sin(θ/2) / |ω| ,  cos(θ/2) )
q' = Δq * q          (puis renormalisé)
```

La vitesse angulaire est exprimée dans l'espace de la pose (comme dans OpenXR), donc `Δq` se multiplie **à gauche** de `q`. C'est aussi l'intégration d'Euler du cours, la vitesse d'abord, la position ensuite ; comme les vitesses sont constantes, elles n'ont pas à être mises à jour entre les deux.

## Vitesse angulaire nulle, sans diviser par zéro

Le quaternion contient `ω · sin(θ/2) / |ω|`. Quand `|ω|` tend vers 0 c'est une forme `0/0`, et le programme naïf divise par zéro (et rend des `NaN`). Mais la limite existe : `sin(θ/2) ≈ θ/2 = |ω| dt / 2`, donc

```
sin(θ/2) / |ω|  →  dt / 2      quand |ω| → 0
```

Le code teste `|θ| < 10⁻⁹` et prend alors `dt/2` pour le facteur devant `ω` et `1` pour le cosinus. Le test porte sur `|θ|` et non sur `|ω|`, pour que le cas `dt` très petit soit couvert de la même façon, et il utilise la valeur absolue pour qu'un `dt` négatif (extrapoler vers le passé) passe par le bon chemin.

Je n'ai mis aucune borne à `dt` dans cette fonction. La borne de cent millisecondes du cours est une règle d'usage qui appartient à l'appelant : on la retrouve à l'exercice 12, où l'on regarde justement où elle se justifie.

Format d'entrée (14 réels) :

```
px py pz  qx qy qz qw   vx vy vz   wx wy wz   dt
```

La sortie est la pose extrapolée : position puis quaternion, quatre décimales.

## Code

```cpp
// Chapitre 2, exercice 8 : l'extrapolation d'une pose.
// On avance une pose de dt secondes en gardant les vitesses constantes :
//   position    += vitesse_lineaire * dt
//   orientation  = exp(vitesse_angulaire * dt) * orientation
// La vitesse angulaire est exprimee dans l'espace de la pose (comme dans OpenXR),
// donc la rotation elementaire se multiplie a gauche.
// Aucune borne sur dt ici : la borne de 100 ms appartient a l'appelant (exercice 12).
#include <cmath>
#include <cstdio>
#include <iostream>

struct Vec3 { double x, y, z; };
struct Quat { double x, y, z, w; };
struct Pose { Vec3 position; Quat orientation; };

Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator*(double s, Vec3 v) { return { s * v.x, s * v.y, s * v.z }; }
double Norme(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

Quat operator*(Quat a, Quat b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

Quat Normaliser(Quat q) {
    double n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (n < 1e-12) return { 0.0, 0.0, 0.0, 1.0 };
    return { q.x / n, q.y / n, q.z / n, q.w / n };
}

// Quaternion de la rotation d'angle |omega|*dt autour de l'axe omega/|omega|.
// axe * sin(angle/2) = omega * sin(angle/2) / |omega| : on ne divise jamais
// par |omega| quand il est (presque) nul, on prend la limite dt/2.
Quat Increment(Vec3 omega, double dt) {
    double norme = Norme(omega);
    double angle = norme * dt;
    double k, c;
    if (std::fabs(angle) < 1e-9) {
        k = 0.5 * dt;      // limite de sin(angle/2) / |omega| quand |omega| tend vers 0
        c = 1.0;
    } else {
        k = std::sin(0.5 * angle) / norme;
        c = std::cos(0.5 * angle);
    }
    return { omega.x * k, omega.y * k, omega.z * k, c };
}

Pose Extrapoler(const Pose& pose, Vec3 vitesse_lineaire, Vec3 vitesse_angulaire, double dt) {
    Pose r;
    r.position = pose.position + dt * vitesse_lineaire;
    r.orientation = Normaliser(Increment(vitesse_angulaire, dt) * pose.orientation);
    return r;
}

double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

int main() {
    Pose pose;
    Vec3 vl, va;
    double dt;
    // Entree : pose (7), vitesse lineaire m/s (3), vitesse angulaire rad/s (3), duree s (1)
    if (!(std::cin >> pose.position.x >> pose.position.y >> pose.position.z
                    >> pose.orientation.x >> pose.orientation.y >> pose.orientation.z >> pose.orientation.w
                    >> vl.x >> vl.y >> vl.z >> va.x >> va.y >> va.z >> dt)) {
        std::fprintf(stderr, "Il faut 14 reels : pose (7), v lineaire (3), v angulaire (3), dt\n");
        return 1;
    }
    Pose r = Extrapoler(pose, vl, va, dt);
    std::printf("%.4f %.4f %.4f  %.4f %.4f %.4f %.4f\n",
                Propre(r.position.x), Propre(r.position.y), Propre(r.position.z),
                Propre(r.orientation.x), Propre(r.orientation.y), Propre(r.orientation.z), Propre(r.orientation.w));
    return 0;
}
```

## Essais

**Une vitesse angulaire de π/2 rad/s pendant 1 s, avec 1 m/s vers `x`.** Une rotation de 90° autour de `y` et un mètre parcouru :

Entrée :

```
0 0 0  0 0 0 1   1 0 0   0 1.5707963267948966 0   1
```

Sortie :

```
1.0000 0.0000 0.0000  0.0000 0.7071 0.0000 0.7071
```

**Vitesse angulaire exactement nulle** (la pose est tournée de 90° autour de `y`, elle avance de 2 m/s vers l'avant pendant 50 ms) : l'orientation ne change pas, et on obtient (0 ; 1 ; -0,1) sans aucun `NaN` :

Entrée :

```
0 1 0  0 0.70710678118654752 0 0.70710678118654752   0 0 -2   0 0 0   0.05
```

Sortie :

```
0.0000 1.0000 -0.1000  0.0000 0.7071 0.0000 0.7071
```

**Vitesse angulaire minuscule** (10⁻¹² rad/s, non nulle) : la version naïve s'en sortirait ici (elle ne casse que pour un `ω` exactement nul), mais le code prend la branche « limite » et rend l'orientation inchangée. Les deux branches donnent la même valeur au voisinage du seuil, donc il n'y a pas de saut :

Entrée :

```
0 0 0  0 0 0 1   0 0 0   1e-12 0 0   0.05
```

Sortie :

```
0.0000 0.0000 0.0000  0.0000 0.0000 0.0000 1.0000
```

**Un cas de casque** : une tête à 1,60 m qui tourne à 180 °/s (π rad/s) autour de `y` et avance à 1 m/s vers l'avant, extrapolée de 10 ms. La tête tourne de 1,8°, soit `sin(0,9°) = 0,0157` sur le `y` du quaternion, et avance d'un centimètre :

Entrée :

```
0 1.6 0  0 0 0 1   0 0 -1   0 3.141592653589793 0   0.01
```

Sortie :

```
0.0000 1.6000 -0.0100  0.0000 0.0157 0.0000 0.9999
```

**`dt` négatif** (extrapoler vers le passé d'une seconde, avec π/2 rad/s) : la rotation se fait dans l'autre sens.

Entrée :

```
0 0 0  0 0 0 1   0 0 0   0 1.5707963267948966 0   -1
```

Sortie :

```
0.0000 0.0000 0.0000  0.0000 -0.7071 0.0000 0.7071
```
