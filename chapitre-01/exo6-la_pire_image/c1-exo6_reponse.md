# Chapitre 1, exercice 6 - La pire image

## Les deux chiffres

Programme : un banc de mesure écrit pour cet exercice (code plus bas), 6000 carrés qui rebondissent dans une fenêtre de 800 × 600, mille images mesurées.

Mesure sur mille images :

| | Valeur |
|---|---|
| **Durée de la plus longue image** | **30,34 ms** (l'image 994 sur 1000) |
| **Nombre d'images de plus de 11 ms** | **49 sur 1000** |

**Mon programme ne tiendrait pas dans un casque.** La moyenne (6 ms) est sous les 11,1 ms d'une image à 90 Hz, et pourtant la plus longue image dure près de trois fois ce budget.

## Le programme

Une petite scène : N carrés (6000 par défaut) qui se déplacent et rebondissent, avec une interaction légère entre eux (chaque carré est repoussé par huit voisins de la liste). À chaque image :

1. **la logique** avance tous les carrés ;
2. **le rendu** efface un tampon en mémoire, y dessine les carrés pixel par pixel (une petite rastérisation logicielle, sans GPU), puis le recopie dans la fenêtre par un `BitBlt` ;
3. l'image est terminée, on passe à la suivante **sans attendre** : la boucle tourne à la vitesse maximale, sans synchronisation avec l'écran.

La **durée d'une image** est l'intervalle entre le début de cette image et le début de l'image précédente, mesuré avec `QueryPerformanceCounter`. Comme la boucle n'attend rien, c'est le temps de travail de l'image plus ce que le système prend au passage (messages de la fenêtre, tâches d'autres processus). C'est bien ce qu'il faut regarder : une image qui dépasse, quelle qu'en soit la cause, se verra.

## Code

```cpp
// Banc de mesure pour les exercices 6 et 7 du chapitre 1.
// Une petite scene : N carres qui rebondissent dans une fenetre.
//   - logique : on avance les carres, on gere les rebonds, un peu d'interaction
//   - rendu   : on efface l'image, on dessine les N carres pixel par pixel dans un
//               tampon memoire (rasterisation logicielle), on recopie a l'ecran (BitBlt)
// La boucle tourne sans limite de cadence. Pour chaque image on note l'intervalle
// depuis le debut de l'image precedente, ainsi que le temps de logique et de rendu.
//
// Compilation (Visual Studio, invite de commandes x64) :
//   cl /EHsc /O2 banc.cpp user32.lib gdi32.lib
// Utilisation : banc [N] [mode] [images] [echauffement]
//   N       nombre de carres (defaut 6000)
//   mode    complet (defaut) | rendu | logique
//   images  nombre d'images mesurees (defaut 1000)
//   echauffement  images jouees avant de commencer a mesurer, non comptees (defaut 0)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

struct Carre { float x, y, vx, vy; int couleur; };

const int LARGEUR = 800;
const int HAUTEUR = 600;
const int COTE = 14;
const int NB_COULEURS = 32;

static double g_ms_par_tick = 0.0;
static bool g_ferme = false;

double Maintenant() {   // en millisecondes
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) * g_ms_par_tick;
}

LRESULT CALLBACK ProcFenetre(HWND fenetre, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_CLOSE || message == WM_DESTROY) { g_ferme = true; return 0; }
    if (message == WM_ERASEBKGND) return 1;   // on efface nous-memes
    return DefWindowProcA(fenetre, message, wp, lp);
}

void Logique(std::vector<Carre>& carres, float dt) {
    const size_t n = carres.size();
    for (size_t i = 0; i < n; ++i) {
        Carre& c = carres[i];
        // un peu d'interaction : on est legerement repousse par 8 voisins de la liste
        float ax = 0.0f, ay = 0.0f;
        for (size_t k = 1; k <= 8; ++k) {
            const Carre& v = carres[(i + k * 37) % n];
            float dx = c.x - v.x, dy = c.y - v.y;
            float d2 = dx * dx + dy * dy + 25.0f;
            ax += dx / d2;
            ay += dy / d2;
        }
        c.vx += 300.0f * ax * dt;
        c.vy += 300.0f * ay * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        if (c.x < 0.0f)               { c.x = 0.0f;               c.vx = std::fabs(c.vx); }
        if (c.x > LARGEUR - COTE)     { c.x = LARGEUR - COTE;     c.vx = -std::fabs(c.vx); }
        if (c.y < 0.0f)               { c.y = 0.0f;               c.vy = std::fabs(c.vy); }
        if (c.y > HAUTEUR - COTE)     { c.y = HAUTEUR - COTE;     c.vy = -std::fabs(c.vy); }
        float v2 = c.vx * c.vx + c.vy * c.vy;    // freinage doux pour rester stable
        if (v2 > 90000.0f) { float f = 300.0f / std::sqrt(v2); c.vx *= f; c.vy *= f; }
    }
}

// Le rendu rend deux durees : le dessin dans le tampon memoire, puis la presentation
// (recopie a l'ecran). La seconde peut attendre le compositeur ou l'ecran.
void Rendu(uint32_t* pixels, const std::vector<Carre>& carres, const uint32_t* palette, HDC tampon, HDC ecran,
           double& t_dessin, double& t_presentation) {
    double a = Maintenant();
    std::fill(pixels, pixels + LARGEUR * HAUTEUR, 0x000A0A14u);          // efface
    for (const Carre& c : carres) {
        int x0 = static_cast<int>(c.x), y0 = static_cast<int>(c.y);
        uint32_t couleur = palette[c.couleur];
        for (int y = y0; y < y0 + COTE; ++y) {
            uint32_t* ligne = pixels + y * LARGEUR;
            for (int x = x0; x < x0 + COTE; ++x) ligne[x] = couleur;
        }
    }
    double b = Maintenant();
    BitBlt(ecran, 0, 0, LARGEUR, HAUTEUR, tampon, 0, 0, SRCCOPY);       // "presente"
    GdiFlush();
    double c2 = Maintenant();
    t_dessin = b - a;
    t_presentation = c2 - b;
}

int main(int argc, char** argv) {
    int n = (argc > 1) ? std::atoi(argv[1]) : 6000;
    const char* mode = (argc > 2) ? argv[2] : "complet";
    int nb_images = (argc > 3) ? std::atoi(argv[3]) : 1000;
    int echauffement = (argc > 4) ? std::atoi(argv[4]) : 0;
    if (n < 1) n = 1;
    if (nb_images < 1) nb_images = 1;
    if (echauffement < 0) echauffement = 0;
    bool fait_logique = std::strcmp(mode, "rendu") != 0;
    bool fait_rendu = std::strcmp(mode, "logique") != 0;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_ms_par_tick = 1000.0 / static_cast<double>(freq.QuadPart);

    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = ProcFenetre;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = "BancMesure";
    RegisterClassA(&wc);

    RECT cadre = { 0, 0, LARGEUR, HAUTEUR };
    AdjustWindowRect(&cadre, WS_OVERLAPPEDWINDOW, FALSE);
    HWND fenetre = CreateWindowA("BancMesure", "Banc de mesure", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 100, 100, cadre.right - cadre.left, cadre.bottom - cadre.top,
                                 nullptr, nullptr, instance, nullptr);
    if (!fenetre) { std::fprintf(stderr, "Impossible de creer la fenetre.\n"); return 1; }

    HDC ecran = GetDC(fenetre);
    HDC tampon = CreateCompatibleDC(ecran);
    // Tampon en memoire systeme (DIB 32 bits) : le dessin ne depend pas du pilote graphique.
    BITMAPINFO infos = {};
    infos.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    infos.bmiHeader.biWidth = LARGEUR;
    infos.bmiHeader.biHeight = -HAUTEUR;        // origine en haut a gauche
    infos.bmiHeader.biPlanes = 1;
    infos.bmiHeader.biBitCount = 32;
    infos.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP image = CreateDIBSection(ecran, &infos, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!image) { std::fprintf(stderr, "Impossible de creer le tampon.\n"); return 1; }
    SelectObject(tampon, image);
    uint32_t palette[NB_COULEURS];
    for (int i = 0; i < NB_COULEURS; ++i) {
        double a = 6.2831853 * i / NB_COULEURS;
        uint32_t r = static_cast<uint32_t>(127 + 120 * std::sin(a));
        uint32_t g = static_cast<uint32_t>(127 + 120 * std::sin(a + 2.09));
        uint32_t b = static_cast<uint32_t>(127 + 120 * std::sin(a + 4.19));
        palette[i] = (r << 16) | (g << 8) | b;
    }

    std::vector<Carre> carres(n);
    std::srand(12345);   // meme scene a chaque lancement
    for (Carre& c : carres) {
        c.x = static_cast<float>(std::rand() % (LARGEUR - COTE));
        c.y = static_cast<float>(std::rand() % (HAUTEUR - COTE));
        c.vx = static_cast<float>(std::rand() % 200) - 100.0f;
        c.vy = static_cast<float>(std::rand() % 200) - 100.0f;
        c.couleur = std::rand() % NB_COULEURS;
    }

    std::vector<double> intervalle, t_logique, t_rendu, t_dessin, t_presentation;
    intervalle.reserve(nb_images);
    double debut_precedent = -1.0;
    int images_faites = 0;

    while (!g_ferme && images_faites < echauffement + nb_images + 1) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }

        double debut = Maintenant();
        double apres_logique = debut;
        double dessin = 0.0, presentation = 0.0;
        if (fait_logique) { Logique(carres, 1.0f / 90.0f); apres_logique = Maintenant(); }
        if (fait_rendu) Rendu(static_cast<uint32_t*>(pixels), carres, palette, tampon, ecran, dessin, presentation);
        double fin = Maintenant();

        if (images_faites > echauffement && debut_precedent >= 0.0) {   // la premiere image mesuree n'a pas d'intervalle avant elle
            intervalle.push_back(debut - debut_precedent);
            t_logique.push_back(apres_logique - debut);
            t_rendu.push_back(fin - apres_logique);
            t_dessin.push_back(dessin);
            t_presentation.push_back(presentation);
        }
        debut_precedent = debut;
        ++images_faites;
    }

    // ---- resultats ----
    size_t m = intervalle.size();
    if (m == 0) { std::fprintf(stderr, "Aucune image mesuree.\n"); return 1; }
    std::vector<double> tri = intervalle;
    std::sort(tri.begin(), tri.end());
    double somme = 0.0, pire = 0.0;
    size_t i_pire = 0, au_dessus = 0;
    for (size_t i = 0; i < m; ++i) {
        somme += intervalle[i];
        if (intervalle[i] > pire) { pire = intervalle[i]; i_pire = i; }
        if (intervalle[i] > 11.0) ++au_dessus;
    }
    auto moyenne = [](const std::vector<double>& v) { double s = 0; for (double x : v) s += x; return s / v.size(); };
    auto maximum = [](const std::vector<double>& v) { return *std::max_element(v.begin(), v.end()); };

    std::printf("mode=%s  N=%d  images mesurees=%zu\n", mode, n, m);
    std::printf("intervalle moyen         : %8.3f ms  (%.0f images/s)\n", somme / m, 1000.0 * m / somme);
    std::printf("intervalle median        : %8.3f ms\n", tri[m / 2]);
    std::printf("intervalle au 99e centile: %8.3f ms\n", tri[static_cast<size_t>(m * 0.99)]);
    std::printf("PLUS LONGUE IMAGE        : %8.3f ms  (image numero %zu)\n", pire, i_pire + 1);
    std::printf("images > 11 ms           : %8zu sur %zu\n", au_dessus, m);
    std::printf("logique : moyenne %.4f ms, maximum %.4f ms\n", moyenne(t_logique), maximum(t_logique));
    std::printf("rendu   : moyenne %.4f ms, maximum %.4f ms\n", moyenne(t_rendu), maximum(t_rendu));
    std::printf("  dont dessin (tampon memoire) : moyenne %.4f ms, maximum %.4f ms\n", moyenne(t_dessin), maximum(t_dessin));
    std::printf("  dont presentation (ecran)    : moyenne %.4f ms, maximum %.4f ms\n", moyenne(t_presentation), maximum(t_presentation));
    return 0;
}
```

Compilation et lancement (Visual Studio, invite de commandes x64) :

```
cl /EHsc /O2 banc.cpp user32.lib gdi32.lib
banc 6000 complet 1000
```

## Tiendrait-il dans un casque ?

**Non**, pour trois raisons.

1. **La pire image dépasse le budget de trois fois.** À 90 Hz, une image de 30 ms, ce sont près de trois images d'affichage où le casque doit réafficher l'ancienne image ou la corriger par reprojection. Dans un casque, la pire image donne le mal au cœur, pas la moyenne.
2. **Les images de plus de 11 ms sont fréquentes** : 49 sur 1000, soit environ une image sur vingt.
3. **Ce banc ne fait qu'un œil.** Un casque demande de dessiner la scène deux fois (exercice 7), ce qui double au moins le temps de dessin, avec la même variabilité en plus.
