// Exercice 27 : cent cubes, et le premier mur du reel.
//
// Usage : cubes.exe [software | opengl | dx11] [novsync]         (par defaut : dx11)
//         cubes.exe [api] show one|group <n>                      (scene fixe, pour regarder)
//
// Sans "show", le programme mesure tout seul (puis garde 400 cubes a l'ecran) :
//   1. la cadence de la salle vide (le point de depart) ;
//   2. le nombre de cubes soumis UN PAR UN a partir duquel la cadence passe sous
//      le seuil, avec drawCalls et cadence a ce nombre ;
//   3. les MEMES cubes soumis en UN SEUL appel (NkRender3D::SubmitInstanced, avec
//      une NkDrawCallInstanced) : drawCalls et cadence ;
//   4. le nombre de cubes groupes a partir duquel la cadence passe a nouveau
//      sous le seuil.
// Le seuil est 60 images par seconde ; si la salle seule est deja en dessous, c'est
// la moitie de sa cadence de depart.
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
#include <algorithm>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static const int WARMUP_FRAMES  = 20;     // les premieres images paient le chargement
static const int MEASURE_FRAMES = 400;

// Grille de cubes identiques posee sur le sol : 25 x 25 par couche, couches de 12 cm.
// Le moteur plafonne a 4096 objets par image, on reste en dessous.
static const float CUBE_SIZE = 0.08f;
static const float PITCH     = 0.12f;
static const int   PER_SIDE  = 25;
static const int   MAX_CUBES = 3600;

static NkVec3f CubeCenter(int i) {
    const int perLayer = PER_SIDE * PER_SIDE;
    const int layer = i / perLayer;
    const int r     = i % perLayer;
    const float half = (PER_SIDE - 1) * 0.5f * PITCH;
    return {-half + (r % PER_SIDE) * PITCH, CUBE_SIZE * 0.5f + layer * PITCH, -half + (r / PER_SIDE) * PITCH};
}

// Boite englobante MONDE d'un cube : le moteur decide de ce qui est dans le champ
// a partir d'elle, pas a partir du maillage.
static NkAABB CubeBox(NkVec3f c) {
    const float h = CUBE_SIZE * 0.5f;
    NkAABB b;
    b.min = {c.x - h, c.y - h, c.z - h};
    b.max = {c.x + h, c.y + h, c.z + h};
    return b;
}

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

struct Sample {
    double   fps       = 0.0;
    double   ms        = 0.0;
    unsigned drawCalls = 0;
};

struct Bench {
    NkWindow       *window   = nullptr;
    NkRenderer     *renderer = nullptr;
    NkMeshHandle    cube, plane;
    NkSceneContext  ctx;
    // Les cubes, prepares une fois : un NkDrawCall3D chacun pour le mode un par un.
    std::vector<NkDrawCall3D> singles;
};

// Une image : salle + n cubes, soit un par un, soit en un seul appel groupe.
static void DrawFrame(Bench &b, int n, bool grouped, const NkDrawCallInstanced *group) {
    NkEvents().PollEvents();
    b.renderer->BeginFrame();
    b.renderer->GetRender3D()->BeginScene(b.ctx);
    SubmitRoom(b.renderer, b.cube, b.plane);
    if (grouped) {
        if (group) b.renderer->GetRender3D()->SubmitInstanced(*group);
    } else {
        for (int i = 0; i < n; ++i) b.renderer->GetRender3D()->Submit(b.singles[i]);
    }
    b.renderer->Present();
    b.renderer->EndFrame();
}

// Le groupe : UNE soumission pour n cubes. La boite englobante doit contenir TOUS
// les cubes, sinon le moteur ecarte le groupe entier d'un coup.
static NkDrawCallInstanced MakeGroup(const Bench &b, int n) {
    NkDrawCallInstanced g;
    g.mesh = b.cube;
    for (int i = 0; i < n; ++i) {
        const NkVec3f c = CubeCenter(i);
        g.transforms.PushBack(NkMat4f::Translate(c) * NkMat4f::Scale({CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}));
        g.tints.PushBack({0.25f, 0.55f, 0.85f});
        g.aabb.Merge(CubeBox(c));   // NkAABB part "inversee" : la premiere fusion la remplit
    }
    return g;
}

// Une mesure : WARMUP_FRAMES images de chauffe, puis MEASURE_FRAMES images chronometrees.
static Sample MeasureOnce(Bench &b, int n, bool grouped, const NkDrawCallInstanced *group) {
    for (int i = 0; i < WARMUP_FRAMES; ++i) DrawFrame(b, n, grouped, group);

    std::chrono::duration<double> total{0};
    for (int i = 0; i < MEASURE_FRAMES && b.window->IsOpen(); ++i) {
        auto t0 = std::chrono::steady_clock::now();
        DrawFrame(b, n, grouped, group);
        total += std::chrono::steady_clock::now() - t0;
    }
    Sample s;
    s.ms        = total.count() * 1000.0 / MEASURE_FRAMES;
    s.fps       = MEASURE_FRAMES / total.count();
    s.drawCalls = b.renderer->GetStats().drawCalls;
    return s;
}

// Trois mesures de 400 images, on garde celle du milieu : une seule mesure se laisse
// fausser par un autre programme qui passe sur la machine (une mesure isolee a donne
// 150 cubes plus lents que 200).
static Sample Measure(Bench &b, int n, bool grouped) {
    NkDrawCallInstanced group;
    if (grouped) group = MakeGroup(b, n);

    Sample reps[3];
    for (int r = 0; r < 3; ++r) reps[r] = MeasureOnce(b, n, grouped, &group);
    std::sort(reps, reps + 3, [](const Sample &x, const Sample &y) { return x.fps < y.fps; });
    const Sample &s = reps[1];
    std::printf("MEASURE %-5s cubes=%-5d drawCalls=%-5u fps=%.1f ms=%.2f   (3 mesures : %.1f %.1f %.1f fps)\n",
                grouped ? "group" : "one", n, s.drawCalls, s.fps, s.ms, reps[0].fps, reps[1].fps, reps[2].fps);
    return s;
}

// Premier nombre de cubes (a 25 pres) qui passe sous le seuil : on double, puis on
// resserre par dichotomie. Renvoie -1 si la limite de 3600 cubes ne suffit pas.
static int FirstBelow(Bench &b, bool grouped, double threshold, Sample &atFound) {
    int lo = 0, hi = -1;
    for (int n = 100; hi < 0; n *= 2) {
        if (n > MAX_CUBES) n = MAX_CUBES;
        Sample s = Measure(b, n, grouped);
        if (s.fps < threshold) { hi = n; atFound = s; }
        else if (n == MAX_CUBES) return -1;
        else lo = n;
    }
    while (hi - lo > 25) {
        int mid = ((lo + hi) / 2 / 10) * 10;
        Sample s = Measure(b, mid, grouped);
        if (s.fps < threshold) { hi = mid; atFound = s; }
        else lo = mid;
    }
    return hi;
}

// Scene fixe a l'ecran jusqu'a la fermeture de la fenetre ; la cadence et drawCalls
// s'affichent toutes les 100 images. Avec "alternate", on change de facon de dessiner
// toutes les 30 secondes.
static void Showcase(Bench &b, int n, bool grouped, bool alternate) {
    NkDrawCallInstanced group = MakeGroup(b, n);
    auto tSwitch = std::chrono::steady_clock::now();
    auto t0      = tSwitch;
    int frames   = 0;
    std::printf("SHOWCASE %s cubes=%d\n", grouped ? "group" : "one", n);
    while (b.window->IsOpen()) {
        DrawFrame(b, n, grouped, &group);
        auto now = std::chrono::steady_clock::now();
        if (++frames == 100) {
            std::printf("SHOW %s cubes=%d drawCalls=%u fps=%.1f\n", grouped ? "group" : "one", n,
                        b.renderer->GetStats().drawCalls,
                        100.0 / std::chrono::duration<double>(now - t0).count());
            t0     = now;
            frames = 0;
        }
        if (alternate && std::chrono::duration<double>(now - tSwitch).count() > 30.0) {
            grouped = !grouped;
            tSwitch = t0 = now;
            frames  = 0;
            std::printf("SHOWCASE %s cubes=%d\n", grouped ? "group" : "one", n);
        }
    }
}

int nkmain(const NkEntryState &state) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    NkGraphicsApi api = NkGraphicsApi::NK_GFX_API_DX11;
    bool vsync = true, show = false, showGrouped = false;
    int showCount = 0;
    for (int i = 0; i < static_cast<int>(state.args.Size()); ++i) {
        const char *a = state.args[i].CStr();
        if (std::strcmp(a, "software") == 0)     api = NkGraphicsApi::NK_GFX_API_SOFTWARE;
        else if (std::strcmp(a, "opengl") == 0)  api = NkGraphicsApi::NK_GFX_API_OPENGL;
        else if (std::strcmp(a, "dx11") == 0)    api = NkGraphicsApi::NK_GFX_API_DX11;
        else if (std::strcmp(a, "novsync") == 0) vsync = false;
        else if (std::strcmp(a, "show") == 0 && i + 2 < static_cast<int>(state.args.Size())) {
            show        = true;
            showGrouped = std::strcmp(state.args[i + 1].CStr(), "group") == 0;
            showCount   = std::atoi(state.args[i + 2].CStr());
        }
    }
    if (showCount > MAX_CUBES) showCount = MAX_CUBES;

    NkAppData app;
    app.appName = "Cubes";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Cubes";
    wc.width  = 1280;
    wc.height = 720;
    // Fenetre toujours visible : le systeme ralentit une fenetre masquee par une autre,
    // ce qui fausserait la cadence mesuree.
    wc.alwaysOnTop = true;
    wc.x = 0;
    wc.y = 0;
    NkWindow window(wc);
    if (!window.IsValid()) return 1;

    NkDeviceInitInfo init;
    init.api     = api;
    init.surface = window.GetSurfaceDesc();
    init.width   = wc.width;
    init.height  = wc.height;
    NkIDevice *device = NkDeviceFactory::CreateForApi(api, init);
    if (!device) {
        std::printf("RESULT %s refused\n", NkGraphicsApiName(api));
        NkClose();
        return 1;
    }

    NkRendererConfig rcfg;
    rcfg.api        = api;
    rcfg.subsystems = NK_SS_RENDER3D;
    rcfg.vsync      = vsync;
    NkRenderer *renderer = NkRenderer::Create(device, rcfg);
    if (!renderer || !renderer->Initialize()) {
        std::printf("RESULT %s renderer_init_failed\n", NkGraphicsApiName(api));
        if (renderer) NkRenderer::Destroy(renderer);
        NkDeviceFactory::Destroy(device);
        NkClose();
        return 1;
    }
    std::printf("Interface graphique : %s (vsync=%d)\n", NkGraphicsApiName(api), vsync ? 1 : 0);

    Bench b;
    b.window   = &window;
    b.renderer = renderer;
    b.cube     = renderer->GetMeshSystem()->GetCube();
    b.plane    = renderer->GetMeshSystem()->GetPlane();
    b.ctx.camera = NkCamera3D(NkCamera3DData{
        {0.f, 1.70f, 1.5f}, {0.f, 1.40f, -2.f}, {0.f, 1.f, 0.f},
        70.f, static_cast<float32>(wc.width) / static_cast<float32>(wc.height), 0.05f, 200.f,
    });
    NkLightDesc sun;
    sun.type       = NkLightType::NK_DIRECTIONAL;
    sun.direction  = {-0.6f, -0.65f, -0.45f};
    sun.color      = {1.f, 0.95f, 0.85f};
    sun.intensity  = 1.8f;
    sun.castShadow = false;
    b.ctx.lights.PushBack(sun);

    for (int i = 0; i < MAX_CUBES; ++i) {
        NkDrawCall3D dc;
        const NkVec3f c = CubeCenter(i);
        dc.mesh      = b.cube;
        dc.transform = NkMat4f::Translate(c) * NkMat4f::Scale({CUBE_SIZE, CUBE_SIZE, CUBE_SIZE});
        dc.tint      = {0.25f, 0.55f, 0.85f};
        dc.aabb      = CubeBox(c);
        b.singles.push_back(dc);
    }

    if (show) {
        Showcase(b, showCount, showGrouped, false);
    } else {
        Sample base = Measure(b, 0, false);
        const double threshold = (base.fps >= 59.0) ? 60.0 : base.fps / 2.0;
        std::printf("THRESHOLD %.1f fps (salle seule : %.1f fps)\n", threshold, base.fps);

        Sample one;
        const int nOne = FirstBelow(b, false, threshold, one);
        if (nOne < 0) std::printf("RESULT one none_below_threshold_up_to=%d\n", MAX_CUBES);
        else {
            std::printf("RESULT one cubes=%d drawCalls=%u fps=%.1f\n", nOne, one.drawCalls, one.fps);
            Sample same = Measure(b, nOne, true);
            std::printf("RESULT group_same_cubes cubes=%d drawCalls=%u fps=%.1f\n", nOne, same.drawCalls, same.fps);
        }
        Sample grp;
        const int nGrp = FirstBelow(b, true, threshold, grp);
        if (nGrp < 0) std::printf("RESULT group none_below_threshold_up_to=%d\n", MAX_CUBES);
        else std::printf("RESULT group cubes=%d drawCalls=%u fps=%.1f\n", nGrp, grp.drawCalls, grp.fps);
        // Salle vide re-mesuree a la fin : l'ecart avec le depart dit de combien la machine a derive.
        Sample again = Measure(b, 0, false);
        std::printf("BASE_AGAIN fps=%.1f (au depart : %.1f)\n", again.fps, base.fps);
        std::printf("DONE\n");
        // Les deux facons de dessiner, en alternance, jusqu'a la fermeture de la fenetre :
        // on verifie a l'oeil que le groupe montre les memes cubes.
        Showcase(b, 400, false, true);
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
