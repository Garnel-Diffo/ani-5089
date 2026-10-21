// Exercice 7 : un panneau au-dessus de la porte. Le mot "Sortie" ecrit avec une police embarquee dans le moteur,
// compose pixel par pixel dans une NkImage a partir de l'atlas, puis pose sur un pave du mur du fond.
// MASALLE_CAM=3m (defaut) ou 50cm choisit le point de vue ; MASALLE_SHOT / MASALLE_SHOT_PATH prennent la capture.
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

int nkmain(const NkEntryState & /*state*/) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // 1. L'atlas : la police Inter, embarquee dans le moteur (aucun fichier a fournir), a 32 pixels.
    NkFontAtlas atlas;
    NkFont *police = NkFontEmbedded::AddToAtlas(atlas, NkEmbeddedFontId::Inter, 32.f);
    if (!police) { std::printf("police embarquee absente\n"); return 1; }
    if (!atlas.Build()) { std::printf("atlas.Build() a echoue\n"); return 1; }

    // Les pixels de l'atlas : un octet par pixel, qui dit de combien la lettre le couvre.
    nkft_uint8 *pixelsAtlas = nullptr;
    int largeurAtlas = 0, hauteurAtlas = 0;
    atlas.GetTexDataAsAlpha8(&pixelsAtlas, &largeurAtlas, &hauteurAtlas);
    if (!pixelsAtlas || largeurAtlas <= 0 || hauteurAtlas <= 0) { std::printf("pixels d'atlas illisibles\n"); return 1; }
    std::printf("atlas : %d x %d\n", largeurAtlas, hauteurAtlas);

    // 2. L'image : largeur = somme des advanceX des six lettres, hauteur = ascent - descent,
    //    ligne de base a `ascent` pixels du haut.
    float largeurTexte = 0.f;
    for (const char *c = MOT; *c; ++c) {
        const NkFontGlyph *g = police->FindGlyph(static_cast<NkFontCodepoint>(static_cast<unsigned char>(*c)));
        if (!g) { std::printf("lettre absente de l'atlas : %c\n", *c); return 1; }
        largeurTexte += g->advanceX;
    }
    const int largeur = static_cast<int>(std::ceil(largeurTexte));
    const int hauteur = static_cast<int>(std::ceil(police->ascent - police->descent));
    const int base    = static_cast<int>(std::lround(police->ascent));

    const unsigned char fondR = 0x0B, fondG = 0x6B, fondB = 0x3A;        // vert de panneau de sortie
    const unsigned char texteR = 0xFF, texteG = 0xFF, texteB = 0xFF;     // lettres blanches
    NkImage image = NkImage::Create(largeur, hauteur, 4, 0x0B6B3AFFu);   // 0xRRGGBBAA
    if (!image.IsValid()) { std::printf("NkImage::Create a echoue\n"); return 1; }

    // 3. Chaque lettre est lue dans l'atlas par ses coordonnees (u0, v0, u1, v1), pas copiee a la main :
    //    son rectangle dans l'image peut avoir une autre largeur que celui de l'atlas.
    float curseur = 0.f;
    for (const char *c = MOT; *c; ++c) {
        const NkFontGlyph *g = police->FindGlyph(static_cast<NkFontCodepoint>(static_cast<unsigned char>(*c)));
        const float gauche = curseur + g->x0, droite = curseur + g->x1;
        const float haut   = base + g->y0,     bas    = base + g->y1;      // y0 est negatif au-dessus de la ligne
        if (g->visible && droite > gauche && bas > haut) {
            for (int y = static_cast<int>(std::floor(haut)); y < static_cast<int>(std::ceil(bas)); ++y) {
                for (int x = static_cast<int>(std::floor(gauche)); x < static_cast<int>(std::ceil(droite)); ++x) {
                    if (x < 0 || y < 0 || x >= largeur || y >= hauteur) continue;
                    // fraction f de la largeur (et de la hauteur) du rectangle, au centre du pixel
                    const float fx = (x + 0.5f - gauche) / (droite - gauche);
                    const float fy = (y + 0.5f - haut) / (bas - haut);
                    if (fx < 0.f || fx > 1.f || fy < 0.f || fy > 1.f) continue;
                    const float u = g->u0 + fx * (g->u1 - g->u0);
                    const float v = g->v0 + fy * (g->v1 - g->v0);
                    int ax = static_cast<int>(u * largeurAtlas), ay = static_cast<int>(v * hauteurAtlas);
                    if (ax < 0) ax = 0;
                    if (ax >= largeurAtlas) ax = largeurAtlas - 1;
                    if (ay < 0) ay = 0;
                    if (ay >= hauteurAtlas) ay = hauteurAtlas - 1;
                    const float couverture = pixelsAtlas[ay * largeurAtlas + ax] / 255.f;
                    // fondu entre la couleur du texte et celle du fond, dans cette proportion
                    const unsigned char r  = static_cast<unsigned char>(fondR + (texteR - fondR) * couverture + 0.5f);
                    const unsigned char gr = static_cast<unsigned char>(fondG + (texteG - fondG) * couverture + 0.5f);
                    const unsigned char b  = static_cast<unsigned char>(fondB + (texteB - fondB) * couverture + 0.5f);
                    image.SetPixel(x, y, math::NkColor(r, gr, b, 255));
                }
            }
        }
        curseur += g->advanceX;
    }
    std::printf("image : %d x %d\n", largeur, hauteur);
    if (!image.SavePNG("panneau.png")) { std::printf("panneau.png non ecrit\n"); return 1; }

    // La face avant du pave du moteur affiche l'image en MIROIR (de gauche a droite) : sur le mur, "Sortie"
    // se lirait a l'envers. panneau.png reste dans le bon sens ; la texture posee sur le pave est une copie
    // retournee horizontalement.
    NkImage surPave = image.Copy();
    surPave.FlipHorizontal();
    if (!surPave.SavePNG("panneau-pave.png")) { std::printf("panneau-pave.png non ecrit\n"); return 1; }

    // 4. La salle.
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

    // L'image du panneau devient une texture : trois appels, une fois, avant la boucle.
    NkTexHandle texture = rendu->GetTextures()->Load("panneau-pave.png");
    NkMaterial *materiau = NkMaterial::Create(rendu->GetMaterials(), NkMaterialType::NK_PBR_METALLIC);
    materiau->SetAlbedoMap(texture);
    materiau->SetEmissiveMap(texture);          // le panneau est eclaire par lui-meme : lisible meme sur un mur peu eclaire
    materiau->SetEmissive({1.f, 1.f, 1.f}, 1.0f);

    // Le pave du panneau : 20 cm de haut, la largeur qui garde les proportions de l'image,
    // au-dessus de la porte (dont le haut est a 2 m), contre le mur, a 3 mm de sa face pour ne pas scintiller.
    const float hautPanneau = 0.20f;
    const float largPanneau = hautPanneau * static_cast<float>(largeur) / static_cast<float>(hauteur);
    const float epaisseur   = 0.02f;
    const NkVec3f centrePanneau = {-0.70f, 2.00f + hautPanneau / 2 + 0.01f, FACE_FOND + epaisseur / 2 + 0.003f};

    // Les deux points de vue demandes ; dans les deux cas la camera regarde le panneau.
    const char *vue = std::getenv("MASALLE_CAM");
    const bool pres = vue && std::strcmp(vue, "50cm") == 0;
    const NkVec3f positionCamera = pres ? NkVec3f{-0.70f, 2.20f, -1.50f} : NkVec3f{-0.70f, 1.70f, 1.00f};

    NkSceneContext ctx;
    ctx.camera = Camera(positionCamera, centrePanneau, wc.width, wc.height);
    ctx.lights.PushBack(Soleil());

    const char *envImage  = std::getenv("MASALLE_SHOT");
    const char *envChemin = std::getenv("MASALLE_SHOT_PATH");
    const int imageCapture = envImage ? std::atoi(envImage) : 0;
    int numero = 0;

    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();

        rendu->BeginFrame();
        rendu->GetRender3D()->BeginScene(ctx);
        SalleVide(rendu, cube, plan, false, NkMatInstHandle{});
        Porte(rendu, cube);
        PaveMateriau(rendu, cube, centrePanneau, {largPanneau, hautPanneau, epaisseur}, materiau->GetInstHandle());
        rendu->Present();
        rendu->EndFrame();

        ++numero;
        if (imageCapture > 0 && numero == imageCapture) {
            const bool ok = CapturerFenetre("MaSalle", envChemin ? envChemin : "capture.png");
            std::printf("capture image %d : %s\n", numero, ok ? "ecrite" : "echec");
            break;
        }
    }

    NkMaterial::Destroy(materiau);
    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
