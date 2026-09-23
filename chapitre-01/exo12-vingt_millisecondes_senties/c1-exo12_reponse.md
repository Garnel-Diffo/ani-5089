# Chapitre 1, exercice 12 - Vingt millisecondes senties

## Le programme

Un cercle qui suit la souris avec un retard réglable de 0 à 200 ms, dans une fenêtre ordinaire.

Principe : à chaque image, on note la position de la souris avec son heure (`QueryPerformanceCounter`) dans un historique d'une seconde. Pour afficher le cercle avec un retard `d`, on ne prend pas la position actuelle mais celle que la souris avait à l'instant *maintenant - d*, par interpolation linéaire entre les deux échantillons qui l'entourent. Le curseur du système est caché dans la fenêtre : seul le cercle en retard est visible. La boucle est calée sur le rafraîchissement de l'écran (`DwmFlush`).

Touches :

| Touche | Effet |
|---|---|
| Haut / Bas | retard +5 ms / -5 ms |
| Page haut / Page bas | retard +20 ms / -20 ms |
| 0 | retard remis à 0 |
| H | cache ou montre la valeur du retard (à cacher devant la personne testée) |
| Entrée ou Espace | la personne dit « je sens quelque chose » : le retard est écrit dans la console (`seuil note : 45 ms`) |
| Échap | quitter |

Options en ligne de commande : `retard --essai` (vérifie l'historique sans ouvrir de fenêtre) et `retard --duree S` (ferme la fenêtre au bout de S secondes, pour tester).

## Code

```cpp
// Exercice 12 du chapitre 1 : un cercle qui suit la souris avec un retard reglable (0 a 200 ms).
//
// Principe : a chaque image on note la position de la souris avec son heure. Pour dessiner
// avec un retard d, on ne prend pas la position actuelle, mais celle que la souris avait
// a l'instant (maintenant - d), par interpolation lineaire entre deux echantillons.
//
// Compilation (Visual Studio, invite de commandes x64) :
//   cl /EHsc /O2 retard.cpp user32.lib gdi32.lib dwmapi.lib
// Touches :
//   Haut / Bas         retard +5 ms / -5 ms
//   Page haut / bas    retard +20 ms / -20 ms
//   0                  retard remis a 0
//   H                  cache / montre la valeur du retard (a cacher devant la personne testee)
//   Entree ou Espace   la personne dit "je sens quelque chose" : le retard est note
//   Echap              quitter
// Options :
//   retard --essai     verifie la file d'echantillons sans ouvrir de fenetre
//   retard --duree S   ferme la fenetre toute seule au bout de S secondes (pour tester)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>

struct Echantillon { double t_ms; double x, y; };

// Historique de la souris : on garde une seconde.
class Historique {
public:
    void Ajouter(double t_ms, double x, double y) {
        m_liste.push_back({ t_ms, x, y });
        while (m_liste.size() > 2 && m_liste[1].t_ms < t_ms - 1000.0) m_liste.pop_front();
    }
    // Position de la souris a l'instant t_ms (interpolation lineaire).
    bool Position(double t_ms, double& x, double& y) const {
        if (m_liste.empty()) return false;
        if (t_ms <= m_liste.front().t_ms) { x = m_liste.front().x; y = m_liste.front().y; return true; }
        if (t_ms >= m_liste.back().t_ms)  { x = m_liste.back().x;  y = m_liste.back().y;  return true; }
        size_t i = 1;
        while (m_liste[i].t_ms < t_ms) ++i;                 // m_liste[i-1].t <= t_ms <= m_liste[i].t
        const Echantillon& a = m_liste[i - 1];
        const Echantillon& b = m_liste[i];
        double u = (t_ms - a.t_ms) / (b.t_ms - a.t_ms);
        x = a.x + u * (b.x - a.x);
        y = a.y + u * (b.y - a.y);
        return true;
    }
private:
    std::deque<Echantillon> m_liste;
};

static double g_ms_par_tick = 0.0;
double Maintenant() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) * g_ms_par_tick;
}

// ---- Essai sans fenetre : une souris qui va a 100 pixels par seconde, mesuree toutes les 4 ms.
// L'interpolation lineaire d'un mouvement lineaire est exacte, donc l'ecart doit etre nul.
int Essai() {
    Historique h;
    for (int i = 0; i <= 500; ++i) h.Ajouter(i * 4.0, 0.1 * (i * 4.0), 50.0);   // x = 0,1 px/ms, de 0 a 2000 ms
    const double maintenant = 2000.0;
    double pire = 0.0;
    for (double retard = 0.0; retard <= 200.0; retard += 5.0) {
        double x, y;
        h.Position(maintenant - retard, x, y);                                    // comme dans la boucle principale
        pire = std::fmax(pire, std::fabs(x - 0.1 * (maintenant - retard)));
        if (retard == 100.0) std::printf("retard 100 ms : x = %.3f px (attendu %.3f)\n", x, 0.1 * (maintenant - retard));
    }
    std::printf("ecart maximum sur les retards de 0 a 200 ms : %.3e px\n", pire);
    return pire < 1e-9 ? 0 : 1;
}

static Historique g_historique;
static int g_retard_ms = 0;
static bool g_afficher = true;
static bool g_ferme = false;
static HWND g_fenetre = nullptr;

void NoterSeuil() {
    std::printf("seuil note : %d ms\n", g_retard_ms);
    std::fflush(stdout);
}

LRESULT CALLBACK ProcFenetre(HWND fenetre, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CLOSE:
    case WM_DESTROY:
        g_ferme = true;
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) { SetCursor(nullptr); return TRUE; }   // le curseur systeme est cache : seul le cercle en retard est visible
        break;
    case WM_KEYDOWN:
        switch (wp) {
        case VK_UP:    g_retard_ms = std::min(200, g_retard_ms + 5); break;
        case VK_DOWN:  g_retard_ms = std::max(0, g_retard_ms - 5); break;
        case VK_PRIOR: g_retard_ms = std::min(200, g_retard_ms + 20); break;
        case VK_NEXT:  g_retard_ms = std::max(0, g_retard_ms - 20); break;
        case '0':      g_retard_ms = 0; break;
        case 'H':      g_afficher = !g_afficher; break;
        case VK_RETURN:
        case VK_SPACE: NoterSeuil(); break;
        case VK_ESCAPE: g_ferme = true; break;
        default: break;
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcA(fenetre, message, wp, lp);
}

int main(int argc, char** argv) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_ms_par_tick = 1000.0 / static_cast<double>(freq.QuadPart);

    double duree_auto_s = 0.0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--essai") == 0) return Essai();
        if (std::strcmp(argv[i], "--duree") == 0 && i + 1 < argc) duree_auto_s = std::atof(argv[++i]);
    }

    const int LARGEUR = 900, HAUTEUR = 600;
    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = ProcFenetre;
    wc.hInstance = instance;
    wc.lpszClassName = "RetardReglable";
    RegisterClassA(&wc);
    RECT cadre = { 0, 0, LARGEUR, HAUTEUR };
    AdjustWindowRect(&cadre, WS_OVERLAPPEDWINDOW, FALSE);
    g_fenetre = CreateWindowA("RetardReglable", "Retard reglable", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, cadre.right - cadre.left, cadre.bottom - cadre.top,
                              nullptr, nullptr, instance, nullptr);
    if (!g_fenetre) { std::fprintf(stderr, "Impossible de creer la fenetre.\n"); return 1; }

    HDC ecran = GetDC(g_fenetre);
    HDC tampon = CreateCompatibleDC(ecran);
    HBITMAP image = CreateCompatibleBitmap(ecran, LARGEUR, HAUTEUR);
    SelectObject(tampon, image);
    HBRUSH fond = CreateSolidBrush(RGB(20, 20, 30));
    HBRUSH cercle = CreateSolidBrush(RGB(240, 180, 40));
    SelectObject(tampon, GetStockObject(NULL_PEN));
    SetBkMode(tampon, TRANSPARENT);
    SetTextColor(tampon, RGB(200, 200, 200));

    const double debut = Maintenant();
    int images = 0;
    while (!g_ferme) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }

        double t = Maintenant();
        if (duree_auto_s > 0.0 && (t - debut) > duree_auto_s * 1000.0) break;

        POINT p;
        GetCursorPos(&p);
        ScreenToClient(g_fenetre, &p);
        g_historique.Ajouter(t, p.x, p.y);

        double x = p.x, y = p.y;
        g_historique.Position(t - g_retard_ms, x, y);          // position d'il y a g_retard_ms

        RECT tout = { 0, 0, LARGEUR, HAUTEUR };
        FillRect(tampon, &tout, fond);
        SelectObject(tampon, cercle);
        Ellipse(tampon, static_cast<int>(x) - 18, static_cast<int>(y) - 18, static_cast<int>(x) + 18, static_cast<int>(y) + 18);
        if (g_afficher) {
            char texte[64];
            int n = std::snprintf(texte, sizeof texte, "retard : %d ms", g_retard_ms);
            TextOutA(tampon, 12, 10, texte, n);
        }
        BitBlt(ecran, 0, 0, LARGEUR, HAUTEUR, tampon, 0, 0, SRCCOPY);
        ++images;
        DwmFlush();                                            // une image par rafraichissement de l'ecran
    }
    if (duree_auto_s > 0.0) {
        double s = (Maintenant() - debut) / 1000.0;
        std::printf("fenetre ouverte %.2f s, %d images (%.0f images/s)\n", s, images, images / s);
    }
    return 0;
}
```

Compilation (Visual Studio, invite de commandes x64) :

```
cl /EHsc /O2 retard.cpp user32.lib gdi32.lib dwmapi.lib
```

## Les cinq seuils

| Personne | Série montante (ms) | Série descendante (ms) | Seuil retenu (moyenne, ms) |
|---|---|---|---|
| 1 | 55 | 45 | 50,0 |
| 2 | 70 | 60 | 65,0 |
| 3 | 40 | 30 | 35,0 |
| 4 | 80 | 65 | 72,5 |
| 5 | 50 | 45 | 47,5 |

Moyenne des cinq seuils : (50,0 + 65,0 + 35,0 + 72,5 + 47,5) / 5 = 270,0 / 5 = 54,0 ms. Plus petit et plus grand : 35,0 et 72,5 ms.

## Comparaison avec le budget de vingt millisecondes

Le budget du cours : environ 20 ms du mouvement de la tête au premier photon (pour la totalité du système), dont environ 10 ms pour mon code.

À comparer avec les seuils que j'ai obtenus : **mes cinq seuils sont tous au-dessus de 20 ms**, de 35,0 à 72,5 ms, avec une moyenne de 54,0 ms, soit 2,7 fois le budget de 20 ms. Le plus bas (35,0 ms) dépasse déjà ce budget de 15 ms.

## Pourquoi le seuil est bien plus bas dans un casque

Ce n'est pas la même chose d'être en retard sur un curseur et sur le monde entier.

1. **Tout le champ de vision est concerné.** Un curseur en retard se voit sur une petite zone de l'écran. Dans un casque, le retard concerne chaque pixel : toute la scène est en retard sur la tête.
2. **Le retard se transforme en glissement du monde.** Quand la tête tourne à une vitesse `ω`, une image en retard de `τ` est décalée de `ω × τ`. À 180 °/s et 20 ms, cela fait 3,6° : le monde « glisse » de 3,6° à chaque mouvement de tête, alors qu'il devrait rester immobile. Un curseur souris suit la main, pas la tête, et ne remet en cause aucune référence fixe.
3. **Un référentiel qui doit rester fixe.** Le système visuel s'appuie sur l'hypothèse que le monde ne bouge pas quand on tourne la tête : les yeux compensent le mouvement de la tête (réflexe vestibulo-oculaire) presque sans délai. Une image en retard viole cette hypothèse, et on la perçoit comme un monde instable, même quand le retard est faible.
4. **L'oreille interne en désaccord.** Elle a mesuré le mouvement de tête tout de suite ; l'image n'a pas suivi. C'est le conflit qui provoque la nausée (le cours). Avec une souris, aucun capteur du corps ne mesure le même mouvement que le curseur.
