# Chapitre 2, exercice 9 - Le chemin court

## Le calcul

On a deux orientations `q0` et `q1`, séparées de `dt` secondes. La rotation qui mène de l'une à l'autre (dans l'espace de la pose) est

```
d = q1 * conjugué(q0)
```

et une rotation d'angle `θ` autour d'un axe unitaire `a` s'écrit `d = (a · sin(θ/2), cos(θ/2))`. On récupère donc :

```
sin(θ/2) = norme de la partie vectorielle de d      (notée s)
θ        = 2 · atan2(s, w)
vitesse angulaire moyenne  ω = (partie vectorielle de d / s) · θ / dt
```

Quand `s` tend vers 0, `θ / s` tend vers 2, et le code prend `2 / dt` comme facteur pour ne pas diviser par zéro.

## Le forçage du chemin court

Le quaternion `-d` décrit la même rotation que `d`. Si `d.w` est négatif, `θ = 2·atan2(s, w)` dépasse 180° : le programme lit alors un tour presque complet dans l'autre sens. Le forçage tient en trois lignes :

```cpp
if (forcer_chemin_court && d.w < 0.0) {
    d = { -d.x, -d.y, -d.z, -d.w };   // -d est la même rotation, par le chemin court
}
```

Avec `w ≥ 0` l'angle reste dans `[0, π]`, et la rotation moyenne est toujours la plus courte des deux.

Le programme affiche les deux résultats, avec et sans forçage, en radians par seconde. Entrée : `q0` (4 réels `x y z w`), `q1` (4 réels), `dt`.

## Code

```cpp
// Chapitre 2, exercice 9 : le chemin court.
// Vitesse angulaire moyenne entre deux orientations separees de dt secondes.
// q et -q decrivent la meme rotation : si on ne force pas le chemin court,
// le delta peut se lire comme un tour presque complet dans l'autre sens.
#include <cmath>
#include <cstdio>
#include <iostream>

struct Vec3 { double x, y, z; };
struct Quat { double x, y, z, w; };

Quat operator*(Quat a, Quat b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

Quat Conjugue(Quat q) { return { -q.x, -q.y, -q.z, q.w }; }

// Delta exprime dans l'espace : q1 = delta * q0.
// Retourne la vitesse angulaire en rad/s. Si forcer_chemin_court est faux,
// on ne touche pas au signe du delta.
Vec3 VitesseAngulaire(Quat q0, Quat q1, double dt, bool forcer_chemin_court) {
    if (dt <= 0.0) return { 0.0, 0.0, 0.0 };
    Quat d = q1 * Conjugue(q0);

    // Les trois lignes du forcage :
    if (forcer_chemin_court && d.w < 0.0) {
        d = { -d.x, -d.y, -d.z, -d.w };   // -d est la meme rotation, par le chemin court
    }

    double s = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);   // sin(angle / 2)
    double angle = 2.0 * std::atan2(s, d.w);                    // dans [0, pi] si w >= 0, jusqu'a 2 pi sinon
    double facteur = (s < 1e-12) ? 2.0 / dt : angle / (s * dt); // omega = (v / s) * angle / dt ; limite si s -> 0
    return { d.x * facteur, d.y * facteur, d.z * facteur };
}

double Propre(double v) { return std::fabs(v) < 0.00005 ? 0.0 : v; }

int main() {
    Quat q0, q1;
    double dt;
    // Entree : q0 (x y z w), q1 (x y z w), dt en secondes
    if (!(std::cin >> q0.x >> q0.y >> q0.z >> q0.w >> q1.x >> q1.y >> q1.z >> q1.w >> dt)) {
        std::fprintf(stderr, "Il faut 9 reels : q0 (4), q1 (4), dt\n");
        return 1;
    }
    Vec3 avec = VitesseAngulaire(q0, q1, dt, true);
    Vec3 sans = VitesseAngulaire(q0, q1, dt, false);
    std::printf("avec forcage : %.4f %.4f %.4f  (rad/s)\n", Propre(avec.x), Propre(avec.y), Propre(avec.z));
    std::printf("sans forcage : %.4f %.4f %.4f  (rad/s)\n", Propre(sans.x), Propre(sans.y), Propre(sans.z));
    return 0;
}
```

## Cas normal

`q0` est l'identité, `q1` est une rotation de 2° autour de `z` (`sin 1° = 0,0174524`, `cos 1° = 0,9998477`), `dt = 0,01 s`. Il faut 2° en 10 ms, soit 200 °/s = 3,4907 rad/s. Avec ou sans forçage on trouve la même chose, puisque `d.w` est positif :

Entrée :

```
0 0 0 1   0 0 0.017452406437283512 0.99984769515639127   0.01
```

Sortie :

```
avec forcage : 0.0000 0.0000 3.4907  (rad/s)
sans forcage : 0.0000 0.0000 3.4907  (rad/s)
```

## Deux quaternions pour lesquels le résultat devient absurde

Même `q0`, mais `q1` est **l'opposé** du précédent : `q1 = (0 ; 0 ; -0,0174524 ; -0,9998477)`. C'est encore exactement la même orientation (la rotation de 2° autour de `z`) : un runtime peut très bien rendre `q` ou `-q` d'un échantillon à l'autre, les deux sont valides. Mais ici `d.w` est négatif.

Entrée :

```
0 0 0 1   0 0 -0.017452406437283512 -0.99984769515639127   0.01
```

Sortie :

```
avec forcage : 0.0000 0.0000 3.4907  (rad/s)
sans forcage : 0.0000 0.0000 -624.8279  (rad/s)
```

| | vitesse angulaire | en degrés par seconde |
|---|---|---|
| avec forçage | +3,4907 rad/s autour de `z` | +200 °/s |
| **sans forçage** | **-624,83 rad/s** autour de `z` | **-35 800 °/s** |

Sans le forçage, l'angle lu est 358° dans le sens inverse au lieu de 2° dans le bon sens : une vitesse 179 fois trop grande et de signe contraire. Si on extrapolait la pose avec cette vitesse, la tête ferait presque un tour complet à l'envers en dix millisecondes, alors que la tête a bougé de deux degrés.

Autre exemple, celui que donne le cours avec des angles : aller de 350° à 10° autour de `y`. Les quaternions sont `q0 = (0 ; sin 175° ; 0 ; cos 175°)` (avec `w` négatif) et `q1 = (0 ; sin 5° ; 0 ; cos 5°)`, `dt = 1 s`. La bonne réponse est 20° en une seconde (via 0°). La soustraction naïve prend l'autre chemin, 340° :

Entrée :

```
0 0.087155742747658174 0 -0.99619469809174555   0 0.087155742747658174 0 0.99619469809174555   1
```

Sortie :

```
avec forcage : 0.0000 0.3491 0.0000  (rad/s)
sans forcage : 0.0000 -5.9341 0.0000  (rad/s)
```

Avec forçage : 0,3491 rad/s = 20 °/s. Sans forçage : -5,9341 rad/s = -340 °/s.

Dernier cas, le plus dur : `q1 = -q0` exactement (deux quaternions opposés, aucune rotation entre eux). Le résultat est nul dans les deux versions, mais seulement parce que le garde-fou `s < 10⁻¹²` donne alors `ω = 0` (sans lui, on aurait `0/0`). Dès que l'écart n'est plus exactement nul, la version sans forçage retombe dans l'absurde du cas précédent.

Entrée :

```
0 0 0 1   0 0 0 -1   0.01
```

Sortie :

```
avec forcage : 0.0000 0.0000 0.0000  (rad/s)
sans forcage : 0.0000 0.0000 0.0000  (rad/s)
```
