# Chapitre 2, exercice 6 - Le bras, en poses

## Le modèle

Un bras à trois segments : une épaule à l'origine, un coude à un bras de distance, une main à un avant-bras du coude. Chaque articulation porte sa pose **dans le repère de la précédente**, et la pose d'une articulation dans le monde s'obtient en composant celle de son parent avec la sienne :

```
coude_monde = Composer(epaule_monde, coude_local)
main_monde  = Composer(coude_monde, main_locale)
```

Longueurs choisies (en mètres, dans l'ordre de grandeur du tableau du chapitre, où la portée d'un bras va de 0,60 à 0,75 m) :

| Segment | Longueur |
|---|---|
| bras (épaule → coude) | 0,30 m |
| avant-bras (coude → main) | 0,27 m |

Le squelette est défini le long de l'axe `x` local : le coude est en (0,30 ; 0 ; 0) dans le repère de l'épaule, la main en (0,27 ; 0 ; 0) dans le repère du coude. Le coude est fléchi par une rotation autour de l'axe vertical `y` (90° par défaut : le bras part sur le côté et l'avant-bras revient vers l'avant, comme quand on tient quelque chose devant soi, le coude écarté). L'épaule au repos est la pose identité à l'origine.

Le programme lit facultativement deux angles en degrés : la rotation de l'épaule puis la flexion du coude. Sans entrée il prend 90° et 90°.

## Code

```cpp
// Chapitre 2, exercice 6 : le bras, en poses.
// Trois segments : epaule (a l'origine), coude, main.
// Chaque articulation porte sa pose dans le repere de la precedente ;
// la pose dans le monde s'obtient en composant celle du parent avec la sienne.
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

Quat Conjugue(Quat q) { return { -q.x, -q.y, -q.z, q.w }; }

Vec3 Appliquer(const Pose& pose, Vec3 p) { return Tourner(pose.orientation, p) + pose.position; }

Pose Composer(const Pose& parent, const Pose& enfant) {
    return { Tourner(parent.orientation, enfant.position) + parent.position,
             parent.orientation * enfant.orientation };
}

Pose Inverser(const Pose& pose) {
    Quat c = Conjugue(pose.orientation);
    return { Tourner(c, -1.0 * pose.position), c };
}

// Rotation d'un angle (en degres) autour de l'axe vertical +y.
Quat AutourDeY(double degres) {
    const double pi = 3.14159265358979323846;
    double moitie = degres * pi / 180.0 / 2.0;
    return { 0.0, std::sin(moitie), 0.0, std::cos(moitie) };
}

const double LONGUEUR_BRAS = 0.30;        // epaule -> coude, en metres
const double LONGUEUR_AVANT_BRAS = 0.27;  // coude -> main, en metres

struct Bras { Pose coude_monde; Pose main_monde; };

// Le squelette ne change pas : seules la pose de l'epaule et la flexion du coude varient.
Bras Construire(const Pose& epaule_monde, double flexion_coude_deg) {
    Pose coude_local = { { LONGUEUR_BRAS, 0.0, 0.0 }, AutourDeY(flexion_coude_deg) };
    Pose main_locale = { { LONGUEUR_AVANT_BRAS, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } };
    Pose coude = Composer(epaule_monde, coude_local);
    Pose main = Composer(coude, main_locale);
    return { coude, main };
}

// Evite d'afficher "-0.0000" pour un zero negatif ou infime.
double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

void Afficher(const char* nom, Vec3 v) {
    std::printf("  %-6s : %8.4f %8.4f %8.4f\n", nom, Propre(v.x), Propre(v.y), Propre(v.z));
}

int main() {
    double rotation_epaule = 90.0;  // degres, autour de +y
    double flexion_coude = 90.0;    // degres, autour de +y
    if (std::cin >> rotation_epaule) { std::cin >> flexion_coude; }

    Pose epaule_repos = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } };
    Pose epaule_tournee = { { 0.0, 0.0, 0.0 }, AutourDeY(rotation_epaule) };

    Bras repos = Construire(epaule_repos, flexion_coude);
    Bras tourne = Construire(epaule_tournee, flexion_coude);

    std::printf("Epaule au repos :\n");
    Afficher("coude", repos.coude_monde.position);
    Afficher("main", repos.main_monde.position);
    std::printf("Epaule tournee de %.1f degres :\n", rotation_epaule);
    Afficher("coude", tourne.coude_monde.position);
    Afficher("main", tourne.main_monde.position);

    // Verification 1 : si la main suit, elle est exactement la main de repos tournee comme l'epaule.
    Vec3 attendue = Tourner(epaule_tournee.orientation, repos.main_monde.position);
    std::printf("ecart avec la main de repos tournee  : %.3e\n", Norme(tourne.main_monde.position - attendue));
    // Verification 2 : vue depuis l'epaule, la main n'a pas bouge.
    Vec3 avant = Appliquer(Inverser(epaule_repos), repos.main_monde.position);
    Vec3 apres = Appliquer(Inverser(epaule_tournee), tourne.main_monde.position);
    std::printf("deplacement de la main vue de l'epaule : %.3e\n", Norme(apres - avant));
    return 0;
}
```

## Ce que le programme affiche

Épaule au repos, puis épaule tournée de 90° autour de `y` :

Sortie :

```
Epaule au repos :
  coude  :   0.3000   0.0000   0.0000
  main   :   0.3000   0.0000  -0.2700
Epaule tournee de 90.0 degres :
  coude  :   0.0000   0.0000  -0.3000
  main   :  -0.2700   0.0000  -0.3000
ecart avec la main de repos tournee  : 1.110e-16
deplacement de la main vue de l'epaule : 2.001e-16
```

Lecture, à la main :

- au repos, le coude est à 0,30 m sur `x` ; l'avant-bras, tourné de 90° autour de `y`, part vers l'avant (-z), donc la main est en (0,30 ; 0 ; -0,27) ;
- si on tourne l'épaule de 90°, tout le bras tourne avec elle autour de l'origine : le coude passe de (0,30 ; 0 ; 0) à (0 ; 0 ; -0,30) et la main de (0,30 ; 0 ; -0,27) à (-0,27 ; 0 ; -0,30).

## La main suit-elle ?

Le programme fait deux vérifications indépendantes. Dans les deux cas on n'a rien dit à la main : on n'a changé que la pose de l'épaule.

1. La main obtenue en recomposant la chaîne est comparée à la main de repos, tournée directement par la rotation de l'épaule. Écart : 1 × 10⁻¹⁶.
2. La position de la main vue depuis l'épaule (on applique l'inverse de la pose de l'épaule) est la même avant et après la rotation. Déplacement : 2 × 10⁻¹⁶.

Ce sont des erreurs d'arrondi : la main suit exactement.
