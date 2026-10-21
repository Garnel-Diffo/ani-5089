// Exercice 8 : le SDF, mesure. Le panneau "Sortie" fait deux fois : avec un atlas ordinaire et avec un atlas SDF,
// puis compare de loin, de pres et en memoire. Le programme enchaine les quatre captures (ordinaire puis sdf,
// a 3 m puis a 50 cm) et ecrit atlas.txt.
// MASALLE_SDF_S=<valeur> change la douceur du bord (s) ; MASALLE_SDF_TEST=1 n'ecrit que les images des panneaux.
#include "NKFont/NkFont.h"
#include "NKFont/Embedded/NkFontEmbedded.h"
#include <cmath>
#include <cstring>
#include "NKWindow/Core/NkMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWESystem.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkGraphicsApi.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkSceneContext.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterial.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"
#include "NKImage/Core/NkImage.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace nkentseu;
using namespace nkentseu::renderer;

// ---- La salle : 4 m sur 4, plafond a 2,50 m, murs de 10 cm. Origine au centre du sol. ----
static const float MUR_EP     = 0.10f;
static const float FACE_FOND  = -2.0f + MUR_EP / 2;   // face interieure du mur du fond : z = -1.95

// Un pave : un cube du moteur (unitaire, centre) mis a l'echelle, pose par son centre.
// La boite englobante MONDE est deduite de la meme taille.
static void Pave(NkRenderer *rendu, NkMeshHandle cube, NkVec3f centre, NkVec3f taille, NkVec3f teinte) {
    NkDrawCall3D dc;
    dc.mesh      = cube;
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    dc.tint      = teinte;
    dc.aabb.min  = {centre.x - taille.x / 2, centre.y - taille.y / 2, centre.z - taille.z / 2};
    dc.aabb.max  = {centre.x + taille.x / 2, centre.y + taille.y / 2, centre.z + taille.z / 2};
    rendu->GetRender3D()->Submit(dc);
}

// Variante de Pave qui recoit un materiau (une texture) au lieu d'une teinte.
static void PaveMateriau(NkRenderer *rendu, NkMeshHandle cube, NkVec3f centre, NkVec3f taille, NkMatInstHandle materiau) {
    NkDrawCall3D dc;
    dc.mesh      = cube;
    dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
    dc.material  = materiau;
    dc.aabb.min  = {centre.x - taille.x / 2, centre.y - taille.y / 2, centre.z - taille.z / 2};
    dc.aabb.max  = {centre.x + taille.x / 2, centre.y + taille.y / 2, centre.z + taille.z / 2};
    rendu->GetRender3D()->Submit(dc);
}

// Capture ce qui est a l'ecran dans la zone cliente de la fenetre et l'ecrit en PNG.
static bool CapturerFenetre(const char *titre, const char *chemin) {
#ifdef _WIN32
    HWND h = FindWindowA(nullptr, titre);
    if (!h) return false;
    RECT rc;
    GetClientRect(h, &rc);
    POINT origine = {0, 0};
    ClientToScreen(h, &origine);
    const int w = rc.right - rc.left, hauteur = rc.bottom - rc.top;
    HDC ecran = GetDC(nullptr);
    HDC mem   = CreateCompatibleDC(ecran);
    HBITMAP bmp = CreateCompatibleBitmap(ecran, w, hauteur);
    HGDIOBJ ancien = SelectObject(mem, bmp);
    BitBlt(mem, 0, 0, w, hauteur, ecran, origine.x, origine.y, SRCCOPY | CAPTUREBLT);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -hauteur;   // lignes de haut en bas
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> bgra(static_cast<size_t>(w) * hauteur * 4);
    GetDIBits(mem, bmp, 0, hauteur, bgra.data(), &bi, DIB_RGB_COLORS);
    SelectObject(mem, ancien);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, ecran);

    NkImage image = NkImage::Create(w, hauteur, 4, 0u);
    if (!image.IsValid()) return false;
    unsigned char *d = image.Pixels();
    for (size_t i = 0; i < static_cast<size_t>(w) * hauteur; ++i) {
        d[4 * i + 0] = bgra[4 * i + 2];
        d[4 * i + 1] = bgra[4 * i + 1];
        d[4 * i + 2] = bgra[4 * i + 0];
        d[4 * i + 3] = 255;
    }
    return image.SavePNG(chemin);
#else
    (void)titre; (void)chemin;
    return false;
#endif
}

// Le peripherique : DirectX 11 d'abord, le rendu logiciel si la carte refuse. On rend l'interface retenue.
static NkIDevice *OuvrirPeripherique(NkWindow &fenetre, int largeur, int hauteur, NkGraphicsApi &api) {
    api = NkGraphicsApi::NK_GFX_API_DX11;
    NkDeviceInitInfo init;
    init.api     = api;
    init.surface = fenetre.GetSurfaceDesc();
    init.width   = largeur;
    init.height  = hauteur;
    NkIDevice *device = NkDeviceFactory::CreateForApi(api, init);
    if (!device) {
        api      = NkGraphicsApi::NK_GFX_API_SOFTWARE;
        init.api = api;
        device   = NkDeviceFactory::CreateForApi(api, init);
    }
    return device;
}

// La salle sans ses ouvertures : sol (plan), plafond et quatre murs. Le mur du fond peut recevoir un materiau.
static void SalleVide(NkRenderer *rendu, NkMeshHandle cube, NkMeshHandle plan, bool murFondMateriau, NkMatInstHandle materiau) {
    {
        NkDrawCall3D dc;
        dc.mesh      = plan;
        dc.transform = NkMat4f::Scale({4.f, 1.f, 4.f});
        dc.tint      = {0.55f, 0.52f, 0.48f};
        rendu->GetRender3D()->Submit(dc);
    }
    Pave(rendu, cube, {0.f, 2.5f, 0.f}, {4.f, 0.1f, 4.f}, {0.85f, 0.85f, 0.85f});
    const NkVec3f mur = {0.72f, 0.70f, 0.66f};
    if (murFondMateriau) PaveMateriau(rendu, cube, {0.f, 1.25f, -2.f}, {4.f, 2.5f, MUR_EP}, materiau);
    else                 Pave(rendu, cube, {0.f, 1.25f, -2.f}, {4.f, 2.5f, MUR_EP}, mur);
    Pave(rendu, cube, {0.f, 1.25f, 2.f}, {4.f, 2.5f, MUR_EP}, mur);
    Pave(rendu, cube, {-2.f, 1.25f, 0.f}, {MUR_EP, 2.5f, 4.f}, mur);
    Pave(rendu, cube, {2.f, 1.25f, 0.f}, {MUR_EP, 2.5f, 4.f}, mur);
}

// La porte du fond : 0,90 m x 2,00 m x 2 cm, au sol, centree en x = -0,70, dos contre la face interieure du mur.
static void Porte(NkRenderer *rendu, NkMeshHandle cube) {
    Pave(rendu, cube, {-0.70f, 1.00f, FACE_FOND + 0.01f}, {0.90f, 2.00f, 0.02f}, {0.35f, 0.22f, 0.12f});
}

static NkCamera3D Camera(NkVec3f position, NkVec3f cible, int largeur, int hauteur) {
    return NkCamera3D(NkCamera3DData{
        position, cible, {0.f, 1.f, 0.f},
        70.f, static_cast<float32>(largeur) / static_cast<float32>(hauteur), 0.05f, 200.f,
    });
}

static NkLightDesc Soleil() {
    NkLightDesc s;
    s.type       = NkLightType::NK_DIRECTIONAL;
    s.direction  = {-0.4f, -1.f, -0.3f};
    s.color      = {1.f, 0.95f, 0.85f};
    s.intensity  = 3.f;
    s.castShadow = false;
    return s;
}


static const char *MOT = "Sortie";

// La regle du nuanceur SDF du moteur (kNkFontFragSDF), recopiee ici. Le bord d'une lettre SDF est a 0,5 :
// une distance sous 0,5 est dehors, au-dessus elle est dedans. `s` regle la douceur du bord.
static float smoothstep(float a, float b, float x) {
    float t = (x - a) / (b - a);
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return t * t * (3.0f - 2.0f * t);
}

// Lecture de l'atlas par interpolation bilineaire entre les quatre pixels voisins.
// (u, v) sont les coordonnees dans l'atlas, entre 0 et 1 ; le centre du pixel (i, j) est en (i + 0,5 ; j + 0,5).
static float LireAtlas(const nkft_uint8 *pixels, int largeur, int hauteur, float u, float v) {
    const float x = u * largeur - 0.5f, y = v * hauteur - 0.5f;
    int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
    const float fx = x - x0, fy = y - y0;
    auto pixel = [&](int px, int py) -> float {
        if (px < 0) px = 0;
        if (px >= largeur) px = largeur - 1;
        if (py < 0) py = 0;
        if (py >= hauteur) py = hauteur - 1;
        return static_cast<float>(pixels[py * largeur + px]);
    };
    const float haut = pixel(x0, y0) * (1.f - fx) + pixel(x0 + 1, y0) * fx;
    const float bas  = pixel(x0, y0 + 1) * (1.f - fx) + pixel(x0 + 1, y0 + 1) * fx;
    return haut * (1.f - fy) + bas * fy;   // entre 0 et 255
}

// Compose l'image du panneau a `echelle` fois la taille des lettres de l'atlas. Memes dimensions pour les deux
// methodes : on compare la methode, pas la taille.
static NkImage ComposerPanneau(NkFontAtlas &atlas, NkFont *police, float echelle, bool sdf, float s) {
    nkft_uint8 *pixels = nullptr;
    int largeurAtlas = 0, hauteurAtlas = 0;
    atlas.GetTexDataAsAlpha8(&pixels, &largeurAtlas, &hauteurAtlas);

    float largeurTexte = 0.f;
    for (const char *c = MOT; *c; ++c)
        largeurTexte += police->FindGlyph(static_cast<NkFontCodepoint>(static_cast<unsigned char>(*c)))->advanceX;
    const int largeur = static_cast<int>(std::ceil(largeurTexte * echelle));
    const int hauteur = static_cast<int>(std::ceil((police->ascent - police->descent) * echelle));   // 8 * (ascent - descent)
    const float base  = police->ascent * echelle;

    NkImage image = NkImage::Create(largeur, hauteur, 4, 0x0B6B3AFFu);
    float curseur = 0.f;
    for (const char *c = MOT; *c; ++c) {
        const NkFontGlyph *g = police->FindGlyph(static_cast<NkFontCodepoint>(static_cast<unsigned char>(*c)));
        const float gauche = (curseur + g->x0) * echelle, droite = (curseur + g->x1) * echelle;
        const float haut   = base + g->y0 * echelle,      bas    = base + g->y1 * echelle;
        if (g->visible && droite > gauche && bas > haut) {
            for (int y = static_cast<int>(std::floor(haut)); y < static_cast<int>(std::ceil(bas)); ++y) {
                for (int x = static_cast<int>(std::floor(gauche)); x < static_cast<int>(std::ceil(droite)); ++x) {
                    if (x < 0 || y < 0 || x >= largeur || y >= hauteur) continue;
                    const float fx = (x + 0.5f - gauche) / (droite - gauche);
                    const float fy = (y + 0.5f - haut) / (bas - haut);
                    if (fx < 0.f || fx > 1.f || fy < 0.f || fy > 1.f) continue;
                    const float valeur = LireAtlas(pixels, largeurAtlas, hauteurAtlas,
                                                   g->u0 + fx * (g->u1 - g->u0), g->v0 + fy * (g->v1 - g->v0));
                    float alpha;
                    if (sdf) {
                        const float dist = valeur / 255.0f;                      // la valeur lue est une distance
                        alpha = smoothstep(0.5f - s, 0.5f + s, dist);            // bord a 0,5, doux sur +/- s
                    } else {
                        alpha = valeur / 255.0f;                                  // la valeur lue est une couverture
                    }
                    const float fond[3]  = {0x0B, 0x6B, 0x3A};
                    const float texte[3] = {255, 255, 255};
                    unsigned char rgb[3];
                    for (int k = 0; k < 3; ++k) rgb[k] = static_cast<unsigned char>(fond[k] + (texte[k] - fond[k]) * alpha + 0.5f);
                    image.SetPixel(x, y, math::NkColor(rgb[0], rgb[1], rgb[2], 255));
                }
            }
        }
        curseur += g->advanceX;
    }
    return image;
}

int nkmain(const NkEntryState & /*state*/) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // 1. Trois atlas : un ordinaire a 32 pixels, un SDF a 32 pixels, un ordinaire a 128 pixels.
    NkFontAtlas ordinaire32;
    NkFont *police32 = NkFontEmbedded::AddToAtlas(ordinaire32, NkEmbeddedFontId::Inter, 32.f);
    NkFontAtlas sdf32;
    sdf32.sdfMode   = true;    // avant Build
    sdf32.sdfSpread = 6;       // rayon en pixels
    NkFont *policeSdf = NkFontEmbedded::AddToAtlas(sdf32, NkEmbeddedFontId::Inter, 32.f);
    NkFontAtlas ordinaire128;
    NkFont *police128 = NkFontEmbedded::AddToAtlas(ordinaire128, NkEmbeddedFontId::Inter, 128.f);
    if (!police32 || !policeSdf || !police128) { std::printf("police embarquee absente\n"); return 1; }
    if (!ordinaire32.Build() || !sdf32.Build() || !ordinaire128.Build()) { std::printf("Build a echoue\n"); return 1; }

    // Les trois tailles, telles que le moteur les donne.
    if (std::FILE *f = std::fopen("atlas.txt", "w")) {
        std::fprintf(f, "ordinaire 32 : %d x %d\n", ordinaire32.texWidth, ordinaire32.texHeight);
        std::fprintf(f, "sdf 32 : %d x %d\n", sdf32.texWidth, sdf32.texHeight);
        std::fprintf(f, "ordinaire 128 : %d x %d\n", ordinaire128.texWidth, ordinaire128.texHeight);
        std::fclose(f);
    }
    std::printf("ordinaire 32 : %d x %d\nsdf 32 : %d x %d\nordinaire 128 : %d x %d\n", ordinaire32.texWidth, ordinaire32.texHeight,
                sdf32.texWidth, sdf32.texHeight, ordinaire128.texWidth, ordinaire128.texHeight);

    // 2. Les deux images du panneau, a huit fois la taille des lettres de l'atlas, memes dimensions.
    // Choix de s, mesure sur l'image du panneau SDF (largeur moyenne de la transition au bord des lettres) :
    //   s = 0,005 : 0,12 px   s = 0,01 : 0,21 px   (bord binaire, crenele)
    //   s = 0,02  : 1,18 px   s = 0,05 : 1,84 px
    //   s = 0,1   : 4,50 px   (bord flou)
    // 0,05 est le bord le plus net qui reste lisse. L'escalier ne disparait pour aucune valeur de s : ses marches
    // font un pixel d'atlas et viennent des distances de l'atlas SDF du moteur, pas de la regle.
    const float DOUCEUR = std::getenv("MASALLE_SDF_S") ? static_cast<float>(std::atof(std::getenv("MASALLE_SDF_S"))) : 0.05f;
    NkImage imgOrdinaire = ComposerPanneau(ordinaire32, police32, 8.f, false, 0.f);
    NkImage imgSdf       = ComposerPanneau(sdf32, policeSdf, 8.f, true, DOUCEUR);
    std::printf("image ordinaire : %d x %d, image sdf : %d x %d (s = %g)\n", imgOrdinaire.Width(), imgOrdinaire.Height(),
                imgSdf.Width(), imgSdf.Height(), static_cast<double>(DOUCEUR));
    // La face avant du pave du moteur affiche l'image en miroir (voir l'exercice precedent) : les images
    // posees sur le pave sont retournees horizontalement.
    imgOrdinaire.FlipHorizontal();
    imgSdf.FlipHorizontal();
    if (!imgOrdinaire.SavePNG("panneau-ordinaire.png") || !imgSdf.SavePNG("panneau-sdf.png")) return 1;
    if (std::getenv("MASALLE_SDF_TEST")) return 0;

    // 3. La salle.
    NkAppData app;
    app.appName = "MaSalle";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title       = "MaSalle";
    wc.width       = 1280;
    wc.height      = 720;
    wc.alwaysOnTop = true;
    wc.x           = 0;
    wc.y           = 0;
    NkWindow fenetre(wc);
    if (!fenetre.IsValid()) return 1;

    NkGraphicsApi api;
    NkIDevice *device = OuvrirPeripherique(fenetre, wc.width, wc.height, api);
    if (!device) return 1;
    std::printf("Interface graphique : %s\n", NkGraphicsApiName(api));

    NkRendererConfig rcfg;
    rcfg.api        = api;
    rcfg.subsystems = NK_SS_RENDER3D;
    NkRenderer *rendu = NkRenderer::Create(device, rcfg);
    if (!rendu || !rendu->Initialize()) return 1;

    NkMeshHandle cube = rendu->GetMeshSystem()->GetCube();
    NkMeshHandle plan = rendu->GetMeshSystem()->GetPlane();

    // Une texture se pose en trois appels ; deux fois ici, une par panneau.
    NkMaterial *materiaux[2];
    const char *fichiers[2] = {"panneau-ordinaire.png", "panneau-sdf.png"};
    for (int i = 0; i < 2; ++i) {
        NkTexHandle texture = rendu->GetTextures()->Load(fichiers[i]);
        materiaux[i] = NkMaterial::Create(rendu->GetMaterials(), NkMaterialType::NK_PBR_METALLIC);
        materiaux[i]->SetAlbedoMap(texture);
        materiaux[i]->SetEmissiveMap(texture);      // le panneau est eclaire par lui-meme : lisible meme sur un mur peu eclaire
        materiaux[i]->SetEmissive({1.f, 1.f, 1.f}, 1.0f);
    }

    const float hautPanneau = 0.20f;
    const float largPanneau = hautPanneau * static_cast<float>(imgOrdinaire.Width()) / static_cast<float>(imgOrdinaire.Height());
    const float epaisseur   = 0.02f;
    const NkVec3f centrePanneau = {-0.70f, 2.00f + hautPanneau / 2 + 0.01f, FACE_FOND + epaisseur / 2 + 0.003f};

    // Les quatre prises : (panneau, point de vue). Memes points de vue deux par deux.
    const char *noms[4]     = {"ordinaire-3m.png", "ordinaire-50cm.png", "sdf-3m.png", "sdf-50cm.png"};
    const int   panneau[4]  = {0, 0, 1, 1};
    const NkVec3f camera[4] = {{-0.70f, 1.70f, 1.00f}, {-0.70f, 2.20f, -1.50f}, {-0.70f, 1.70f, 1.00f}, {-0.70f, 2.20f, -1.50f}};

    NkSceneContext ctx;
    ctx.lights.PushBack(Soleil());

    int prise = 0, numero = 0;
    while (fenetre.IsOpen() && prise < 4) {
        NkEvents().PollEvents();
        ctx.camera = Camera(camera[prise], centrePanneau, wc.width, wc.height);

        rendu->BeginFrame();
        rendu->GetRender3D()->BeginScene(ctx);
        SalleVide(rendu, cube, plan, false, NkMatInstHandle{});
        Porte(rendu, cube);
        PaveMateriau(rendu, cube, centrePanneau, {largPanneau, hautPanneau, epaisseur}, materiaux[panneau[prise]]->GetInstHandle());
        rendu->Present();
        rendu->EndFrame();

        if (++numero == 120) {    // l'image 120 de chaque prise
            const bool ok = CapturerFenetre("MaSalle", noms[prise]);
            std::printf("capture %s : %s\n", noms[prise], ok ? "ecrite" : "echec");
            ++prise;
            numero = 0;
        }
    }

    NkMaterial::Destroy(materiaux[0]);
    NkMaterial::Destroy(materiaux[1]);
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
