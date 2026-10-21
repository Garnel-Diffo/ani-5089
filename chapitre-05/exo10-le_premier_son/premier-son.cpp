// Exercice 10 : le premier son. Un son joue au demarrage de la salle, sans etre place dans l'espace.
// MASALLE_DUREE=<secondes> ferme la salle toute seule apres ce temps (sinon : fermer la fenetre).
#include "NKAudio/NkAudio.h"
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


using namespace nkentseu::audio;

int nkmain(const NkEntryState & /*state*/) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

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

    NkSceneContext ctx;
    ctx.camera = Camera({0.f, 1.70f, 1.5f}, {0.f, 1.40f, -2.f}, wc.width, wc.height);
    ctx.lights.PushBack(Soleil());

    // --- Le son : initialise UNE fois, avant la boucle ; ferme UNE fois, apres. ---
    AudioEngineConfig cfg;
    cfg.sampleRate = 48000;
    if (!AudioEngine::Instance().Initialize(cfg)) { std::printf("moteur sonore non initialise\n"); return 1; }
    std::printf("dorsal : %s\n", AudioEngine::Instance().GetBackendName());
    std::printf("frequence : %d\n", AudioEngine::Instance().GetSampleRate());   // lue apres Initialize : pas forcement les 48000 demandes
    std::printf("tampon : %d\n", AudioEngine::Instance().GetBufferSize());
    std::printf("latence : %.2f\n", AudioEngine::Instance().GetLatencyMs());
    AudioSample son = AudioGenerator::GenerateTone(440.f, 1.f);
    AudioEngine::Instance().Play(son);      // une fois, au demarrage, sans position : le son est pour les deux oreilles

    const char *envDuree = std::getenv("MASALLE_DUREE");
    const double duree = envDuree ? std::atof(envDuree) : 0.0;
    int numero = 0;
    const double imagesParSeconde = 30.0;   // ordre de grandeur de la salle ici : sert seulement a couper la boucle

    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();

        rendu->BeginFrame();
        rendu->GetRender3D()->BeginScene(ctx);
        SalleVide(rendu, cube, plan, false, NkMatInstHandle{});
        Porte(rendu, cube);
        rendu->Present();
        rendu->EndFrame();

        ++numero;
        if (duree > 0.0 && numero >= duree * imagesParSeconde) break;
    }

    AudioEngine::Instance().Shutdown();     // le moteur d'abord...
    AudioLoader::Free(son);                 // ...puis l'echantillon : une voix qui joue encore lit ses donnees
    // --- fin du son ---

    NkRenderer::Destroy(rendu);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
