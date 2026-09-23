# Chapitre 2, exercice 7 - La matrice de vue

## Les deux versions

La matrice de vue est l'inverse de la matrice de la pose de l'œil : la pose dit où est l'œil dans le monde, la vue dit où est le monde par rapport à l'œil.

- **Version A, inversion générale.** On construit la matrice 4 × 4 de la pose (rotation issue du quaternion, translation dans la dernière colonne) et on l'inverse par un Gauss-Jordan avec pivot partiel. Comme dans le module décrit au cours, cette fonction rend l'identité quand le pivot est quasi nul, au lieu d'échouer : c'est le garde-fou « matrice singulière ».
- **Version B, construction directe.** On prend le conjugué du quaternion et la translation opposée tournée par ce conjugué, puis on construit la matrice de cette pose inverse. Aucune inversion de matrice n'est faite.

Les matrices sont en convention colonne (`p' = M p`, translation dans la dernière colonne). Pour la rotation j'utilise la forme développée du quaternion, `w²+x²-y²-z²` sur la diagonale, etc. Elle est identique à la forme habituelle pour un quaternion unitaire, mais elle ne suppose pas qu'il le soit, et un quaternion nul donne alors une rotation nulle plutôt qu'une identité qui masquerait le problème.

Entrée : une pose (`px py pz  qx qy qz qw`). Le programme compare les 16 coefficients des deux versions, puis refait la comparaison sur une pose dégénérée fixée dans le code.

## Code

```cpp
// Chapitre 2, exercice 7 : la matrice de vue, en deux versions.
//   version A : on inverse la matrice de la pose par une inversion generale
//               (Gauss-Jordan). Comme dans le module decrit au cours, elle
//               rend l'identite quand elle ne sait pas inverser.
//   version B : on construit directement le conjugue du quaternion et la
//               translation opposee, sans jamais inverser de matrice.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <utility>

struct Vec3 { double x, y, z; };
struct Quat { double x, y, z, w; };
struct Pose { Vec3 position; Quat orientation; };
struct Mat4 { double m[4][4]; };   // convention colonne : p' = M * p, translation dans la derniere colonne

Vec3 operator*(double s, Vec3 v) { return { s * v.x, s * v.y, s * v.z }; }
Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
double Produit(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 Vectoriel(Vec3 a, Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

// Forme developpee : valable meme si le quaternion n'est pas unitaire
// (elle rend alors |q|^2 fois la rotation), ce qui rend un quaternion nul visible.
Vec3 Tourner(Quat q, Vec3 v) {
    Vec3 u = { q.x, q.y, q.z };
    return (q.w * q.w - Produit(u, u)) * v + (2.0 * Produit(u, v)) * u + (2.0 * q.w) * Vectoriel(u, v);
}

Quat Conjugue(Quat q) { return { -q.x, -q.y, -q.z, q.w }; }

Pose Inverser(const Pose& pose) {
    Quat c = Conjugue(pose.orientation);
    return { Tourner(c, -1.0 * pose.position), c };
}

Mat4 Identite() {
    Mat4 r = {};
    for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0;
    return r;
}

Mat4 MatricePose(const Pose& p) {
    double x = p.orientation.x, y = p.orientation.y, z = p.orientation.z, w = p.orientation.w;
    Mat4 r = Identite();
    r.m[0][0] = w * w + x * x - y * y - z * z;  r.m[0][1] = 2.0 * (x * y - w * z);      r.m[0][2] = 2.0 * (x * z + w * y);
    r.m[1][0] = 2.0 * (x * y + w * z);          r.m[1][1] = w * w - x * x + y * y - z * z;  r.m[1][2] = 2.0 * (y * z - w * x);
    r.m[2][0] = 2.0 * (x * z - w * y);          r.m[2][1] = 2.0 * (y * z + w * x);      r.m[2][2] = w * w - x * x - y * y + z * z;
    r.m[0][3] = p.position.x;
    r.m[1][3] = p.position.y;
    r.m[2][3] = p.position.z;
    return r;
}

// Version A : inversion generale par Gauss-Jordan avec pivot partiel.
// Si le pivot est quasi nul, elle rend l'identite, sans erreur ni message.
Mat4 InverseGenerale(const Mat4& a) {
    double t[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) { t[i][j] = a.m[i][j]; t[i][j + 4] = (i == j) ? 1.0 : 0.0; }

    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int i = col + 1; i < 4; ++i)
            if (std::fabs(t[i][col]) > std::fabs(t[pivot][col])) pivot = i;
        if (std::fabs(t[pivot][col]) < 1e-12) return Identite();   // le garde-fou "singuliere"
        if (pivot != col) for (int j = 0; j < 8; ++j) std::swap(t[pivot][j], t[col][j]);
        double p = t[col][col];
        for (int j = 0; j < 8; ++j) t[col][j] /= p;
        for (int i = 0; i < 4; ++i) {
            if (i == col) continue;
            double f = t[i][col];
            for (int j = 0; j < 8; ++j) t[i][j] -= f * t[col][j];
        }
    }
    Mat4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) r.m[i][j] = t[i][j + 4];
    return r;
}

Mat4 VueParInversion(const Pose& pose) { return InverseGenerale(MatricePose(pose)); }
Mat4 VueAnalytique(const Pose& pose) { return MatricePose(Inverser(pose)); }

double EcartMax(const Mat4& a, const Mat4& b) {
    double e = 0.0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) e = std::fmax(e, std::fabs(a.m[i][j] - b.m[i][j]));
    return e;
}

void Afficher(const char* titre, const Mat4& a) {
    std::printf("%s\n", titre);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double v = std::fabs(a.m[i][j]) < 0.00005 ? 0.0 : a.m[i][j];
            std::printf(" %9.4f", v);
        }
        std::printf("\n");
    }
}

void Comparer(const Pose& pose) {
    Mat4 a = VueParInversion(pose);
    Mat4 b = VueAnalytique(pose);
    Afficher("A - inversion generale :", a);
    Afficher("B - conjugue + translation opposee :", b);
    std::printf("ecart maximum sur les 16 coefficients : %.3e\n", EcartMax(a, b));
}

int main() {
    Pose pose;
    // Entree : px py pz  qx qy qz qw
    if (!(std::cin >> pose.position.x >> pose.position.y >> pose.position.z
                    >> pose.orientation.x >> pose.orientation.y >> pose.orientation.z >> pose.orientation.w)) {
        std::fprintf(stderr, "Il faut 7 reels : px py pz qx qy qz qw\n");
        return 1;
    }
    std::printf("=== Pose valide ===\n");
    Comparer(pose);

    // Pose degeneree : orientation nulle, ce que rend un runtime dont le suivi est perdu.
    Pose degeneree = { { 1.0, 1.7, -2.0 }, { 0.0, 0.0, 0.0, 0.0 } };
    std::printf("\n=== Pose degeneree : position (1, 1.7, -2), quaternion nul ===\n");
    Comparer(degeneree);
    return 0;
}
```

## Comparaison des seize coefficients

Pose : position (1 ; 1,7 ; -2), rotation de 45° autour de `y`.
À la main, la rotation inverse est la transposée, donc les lignes 1 et 3 de la sous-matrice 3 × 3 sont (0,7071 ; 0 ; -0,7071) et (0,7071 ; 0 ; 0,7071), et la translation vaut -Rᵀ t = (-2,1213 ; -1,7 ; 0,7071). C'est ce que donnent les deux versions.

```
=== Pose valide ===
A - inversion generale :
    0.7071    0.0000   -0.7071   -2.1213
    0.0000    1.0000    0.0000   -1.7000
    0.7071    0.0000    0.7071    0.7071
    0.0000    0.0000    0.0000    1.0000
B - conjugue + translation opposee :
    0.7071    0.0000   -0.7071   -2.1213
    0.0000    1.0000    0.0000   -1.7000
    0.7071    0.0000    0.7071    0.7071
    0.0000    0.0000    0.0000    1.0000
ecart maximum sur les 16 coefficients : 2.220e-16
```

Avec un quaternion quelconque (0,2 ; 0,3 ; 0,1 ; 0,9274…), pas de coefficient « rond » à deviner :

```
=== Pose valide ===
A - inversion generale :
    0.8000    0.3055   -0.5164   -1.2830
   -0.0655    0.9000    0.4309   -0.6163
    0.5964   -0.3109    0.7400    0.8149
    0.0000    0.0000    0.0000    1.0000
B - conjugue + translation opposee :
    0.8000    0.3055   -0.5164   -1.2830
   -0.0655    0.9000    0.4309   -0.6163
    0.5964   -0.3109    0.7400    0.8149
    0.0000    0.0000    0.0000    1.0000
ecart maximum sur les 16 coefficients : 2.220e-16
```

L'écart maximum sur les seize coefficients est de 2 × 10⁻¹⁶ dans les deux cas : les deux méthodes donnent la même matrice, aux arrondis près.

## Une pose dégénérée

Pose : position (1 ; 1,7 ; -2), quaternion nul (0, 0, 0, 0). C'est ce qu'un runtime peut rendre quand le suivi est perdu et que l'orientation n'est pas valide.

```
=== Pose degeneree : position (1, 1.7, -2), quaternion nul ===
A - inversion generale :
    1.0000    0.0000    0.0000    0.0000
    0.0000    1.0000    0.0000    0.0000
    0.0000    0.0000    1.0000    0.0000
    0.0000    0.0000    0.0000    1.0000
B - conjugue + translation opposee :
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    0.0000
    0.0000    0.0000    0.0000    1.0000
ecart maximum sur les 16 coefficients : 1.000e+00
```

Ce qu'on voit :

- **A rend l'identité.** Aucune erreur, aucun message, la sortie a l'air d'une matrice de vue tout à fait ordinaire. Dans un casque, la caméra se retrouve à l'origine, sans rotation, et rien ne l'explique : la pose (1 ; 1,7 ; -2) est simplement perdue.
- **B rend une matrice écrasée** (rotation nulle, translation nulle) : tout le monde s'effondre sur un point. C'est laid, mais ça se remarque tout de suite et ça se teste (la norme du quaternion vaut 0). La faute ne se cache pas derrière une valeur plausible.

C'est exactement le sens de la phrase du module : l'inverse analytique plutôt que l'inversion générale, « exacte, et sans le garde-fou “singulier” qui rendrait silencieusement l'identité en cas de bug amont ». Le bug (un quaternion nul) est en amont, l'inversion générale le transforme en image fausse mais calme, alors que l'inverse analytique le laisse visible.
