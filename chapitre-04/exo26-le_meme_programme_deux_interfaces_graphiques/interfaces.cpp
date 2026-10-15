// Exercice 26 : la meme salle sur plusieurs interfaces graphiques imposees.
//
// Usage : interfaces.exe                       essaie le rendu logiciel, DirectX 11
//                                              (avec puis sans synchro verticale), OpenGL
//         interfaces.exe software|opengl|dx11 [novsync] [ibl=<force>]
//                                              n'essaie que celle-la
//
// L'interface n'est plus choisie par la detection automatique : on la demande
// a NkDeviceFactory::CreateForApi, puis on donne la MEME valeur a la config du
// renderer (CreateForApi ne la recopie pas, et le defaut de NkRendererConfig::api
// est OpenGL).
//
// "novsync" coupe la synchronisation verticale : avec elle, le temps d'une image
// est arrondi a un multiple de 16,7 ms (ecran a 60 Hz) et ne mesure plus le moteur.
// "ibl=<force>" regle l'eclairage ambiant ; sans l'option on garde le reglage du
// moteur, donc exactement le meme programme sur toutes les interfaces.
#include "NKWindow/Core/NkMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWESystem.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkGraphicsApi.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkSceneContext.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace nkentseu;
using namespace nkentseu::renderer;

static const int WARMUP_FRAMES  = 5;    // les premieres images paient le chargement
static const int MEASURE_FRAMES = 400;
static const int HOLD_SECONDS   = 20;   // la salle reste affichee apres la mesure (capture d'ecran)

// La salle de l'exercice 8 : sol (plan), plafond et quatre murs (cubes).
static void SubmitRoom(NkRenderer *renderer, NkMeshHandle cube, NkMeshHandle plane) {
    {
        NkDrawCall3D dc;
        dc.mesh      = plane;
        dc.transform = NkMat4f::Scale({4.f, 1.f, 4.f});
        dc.tint      = {0.55f, 0.52f, 0.48f};
        renderer->GetRender3D()->Submit(dc);
    }
    {
        NkDrawCall3D dc;
        dc.mesh      = cube;
        dc.transform = NkMat4f::Translate({0.f, 2.5f, 0.f}) * NkMat4f::Scale({4.f, 0.1f, 4.f});
        dc.tint      = {0.85f, 0.85f, 0.85f};
        renderer->GetRender3D()->Submit(dc);
    }
    NkVec3f wallPos[4]   = {{0.f, 1.25f, -2.f}, {0.f, 1.25f, 2.f}, {-2.f, 1.25f, 0.f}, {2.f, 1.25f, 0.f}};
    NkVec3f wallScale[4] = {{4.f, 2.5f, 0.1f}, {4.f, 2.5f, 0.1f}, {0.1f, 2.5f, 4.f}, {0.1f, 2.5f, 4.f}};
    for (int i = 0; i < 4; ++i) {
        NkDrawCall3D mur;
        mur.mesh      = cube;
        mur.transform = NkMat4f::Translate(wallPos[i]) * NkMat4f::Scale(wallScale[i]);
        mur.tint      = {0.72f, 0.70f, 0.66f};
        renderer->GetRender3D()->Submit(mur);
    }
}

struct Options {
    bool  hasIbl = false;
    float ibl    = 0.f;
};

// Une interface, une fenetre, un renderer : la salle, 400 images chronometrees,
// puis maintien a l'ecran. Renvoie false si l'interface refuse de demarrer.
static bool RunOne(NkGraphicsApi api, bool vsync, const Options &opt) {
    const char *name = NkGraphicsApiName(api);

    NkWindowConfig wc;
    wc.title  = "Interfaces";
    wc.width  = 1280;
    wc.height = 720;
    // Fenetre toujours visible : le systeme ralentit une fenetre masquee par une autre,
    // ce qui fausserait le temps mesure.
    wc.alwaysOnTop = true;
    wc.x = 0;
    wc.y = 0;
    NkWindow window(wc);
    if (!window.IsValid()) return false;

    // 1. On impose l'interface. Un peripherique peut ne pas exister : on le verifie.
    NkDeviceInitInfo init;
    init.api     = api;
    init.surface = window.GetSurfaceDesc();
    init.width   = wc.width;
    init.height  = wc.height;
    NkIDevice *device = NkDeviceFactory::CreateForApi(api, init);
    if (!device) {
        std::printf("RESULT %s vsync=%d refused\n", name, vsync ? 1 : 0);
        return false;
    }

    // 2. Le renderer recoit la meme interface que le peripherique.
    NkRendererConfig rcfg;
    rcfg.api        = api;
    rcfg.subsystems = NK_SS_RENDER3D;
    rcfg.vsync      = vsync;
    NkRenderer *renderer = NkRenderer::Create(device, rcfg);
    if (!renderer || !renderer->Initialize()) {
        std::printf("RESULT %s vsync=%d renderer_init_failed\n", name, vsync ? 1 : 0);
        if (renderer) NkRenderer::Destroy(renderer);
        NkDeviceFactory::Destroy(device);
        return false;
    }

    // 3. Le nom de l'interface retenue, affiche au demarrage.
    std::printf("Interface graphique : %s\n", name);

    if (opt.hasIbl) renderer->GetRender3D()->SetIBLStrength(opt.ibl);
    NkMeshHandle cube  = renderer->GetMeshSystem()->GetCube();
    NkMeshHandle plane = renderer->GetMeshSystem()->GetPlane();

    NkSceneContext ctx;
    ctx.camera = NkCamera3D(NkCamera3DData{
        {0.f, 1.70f, 1.5f}, {0.f, 1.40f, -2.f}, {0.f, 1.f, 0.f},
        70.f, static_cast<float32>(wc.width) / static_cast<float32>(wc.height), 0.05f, 200.f,
    });
    NkLightDesc sun;
    sun.type       = NkLightType::NK_DIRECTIONAL;
    sun.direction  = {-0.6f, -0.65f, -0.45f};
    sun.color      = {1.f, 0.95f, 0.85f};
    sun.intensity  = 1.8f;
    sun.castShadow = false;
    ctx.lights.PushBack(sun);

    auto drawFrame = [&]() {
        NkEvents().PollEvents();
        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);
        SubmitRoom(renderer, cube, plane);
        renderer->Present();
        renderer->EndFrame();
    };

    // 4. Temps moyen d'une image : on laisse passer les images de chauffe, puis
    //    on chronometre MEASURE_FRAMES images entieres (de BeginFrame a EndFrame).
    for (int i = 0; i < WARMUP_FRAMES; ++i) drawFrame();

    std::vector<double> ms;
    for (int i = 0; i < MEASURE_FRAMES && window.IsOpen(); ++i) {
        auto t0 = std::chrono::steady_clock::now();
        drawFrame();
        ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    }
    double sum = 0.0;
    for (double v : ms) sum += v;
    std::vector<double> sorted = ms;
    std::sort(sorted.begin(), sorted.end());
    std::printf("RESULT %s vsync=%d mean_ms=%.2f median_ms=%.2f frames=%d\n", name, vsync ? 1 : 0,
                sum / ms.size(), sorted[sorted.size() / 2], static_cast<int>(ms.size()));

    // La salle reste a l'ecran un moment (capture), puis on libere tout.
    std::printf("HOLD %s vsync=%d\n", name, vsync ? 1 : 0);
    auto tHold = std::chrono::steady_clock::now();
    while (window.IsOpen() &&
           std::chrono::duration<double>(std::chrono::steady_clock::now() - tHold).count() < HOLD_SECONDS)
        drawFrame();

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    return true;
}

int nkmain(const NkEntryState &state) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    Options opt;
    bool vsync = true, single = false;
    NkGraphicsApi api = NkGraphicsApi::NK_GFX_API_SOFTWARE;
    for (int i = 0; i < static_cast<int>(state.args.Size()); ++i) {
        const char *a = state.args[i].CStr();
        if (std::strcmp(a, "opengl") == 0)        { api = NkGraphicsApi::NK_GFX_API_OPENGL;   single = true; }
        else if (std::strcmp(a, "dx11") == 0)     { api = NkGraphicsApi::NK_GFX_API_DX11;     single = true; }
        else if (std::strcmp(a, "software") == 0) { api = NkGraphicsApi::NK_GFX_API_SOFTWARE; single = true; }
        else if (std::strcmp(a, "novsync") == 0)  vsync = false;
        else if (std::strncmp(a, "ibl=", 4) == 0) { opt.hasIbl = true; opt.ibl = static_cast<float>(std::atof(a + 4)); }
    }

    NkAppData app;
    app.appName = "Interfaces";
    if (!NkInitialise(app)) return 1;

    if (single) {
        RunOne(api, vsync, opt);
    } else {
        // OpenGL en dernier : c'est le seul qui risque de refuser de demarrer.
        RunOne(NkGraphicsApi::NK_GFX_API_SOFTWARE, true, opt);
        RunOne(NkGraphicsApi::NK_GFX_API_DX11, true, opt);
        RunOne(NkGraphicsApi::NK_GFX_API_DX11, false, opt);
        RunOne(NkGraphicsApi::NK_GFX_API_OPENGL, true, opt);
    }

    NkClose();
    return 0;
}
