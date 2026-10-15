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
#include <chrono>

using namespace nkentseu;
using namespace nkentseu::renderer;

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
        dc.transform = NkMat4f::Translate({0.f, 2.5f, 0.f}) * NkMat4f::Scale({4.f, 0.1f, 4.f});
        dc.mesh = cube;
        dc.tint = {0.85f, 0.85f, 0.85f};
        renderer->GetRender3D()->Submit(dc);
    }
    NkVec3f wallPos[4] = {{0.f,1.25f,-2.f},{0.f,1.25f,2.f},{-2.f,1.25f,0.f},{2.f,1.25f,0.f}};
    NkVec3f wallScale[4] = {{4.f,2.5f,0.1f},{4.f,2.5f,0.1f},{0.1f,2.5f,4.f},{0.1f,2.5f,4.f}};
    for (int i = 0; i < 4; ++i) {
        NkDrawCall3D mur;
        mur.mesh      = cube;
        mur.transform = NkMat4f::Translate(wallPos[i]) * NkMat4f::Scale(wallScale[i]);
        mur.tint      = {0.72f, 0.70f, 0.66f};
        renderer->GetRender3D()->Submit(mur);
    }
    // Pilier central : seul objet capable de projeter une ombre visible sur
    // le sol lorsque castShadow passe a true.
    {
        NkDrawCall3D dc;
        dc.mesh      = cube;
        dc.transform = NkMat4f::Translate({0.f, 0.5f, -0.5f}) * NkMat4f::Scale({0.4f, 1.0f, 0.4f});
        dc.tint      = {0.80f, 0.20f, 0.20f};
        renderer->GetRender3D()->Submit(dc);
    }
}

int nkmain(const NkEntryState & /*state*/) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    NkAppData app;
    app.appName = "Lumiere";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Lumiere";
    wc.width  = 1280;
    wc.height = 720;
    NkWindow window(wc);
    if (!window.IsValid()) return 1;

    NkDeviceInitInfo init;
    init.api     = NkGraphicsApi::NK_GFX_API_SOFTWARE;
    init.surface = window.GetSurfaceDesc();
    init.width   = wc.width;
    init.height  = wc.height;
    NkIDevice *device = NkDeviceFactory::CreateForApi(NkGraphicsApi::NK_GFX_API_SOFTWARE, init);
    if (!device) return 1;

    NkRendererConfig rcfg;
    rcfg.subsystems = NK_SS_RENDER3D | NK_SS_SHADOW;
    NkRenderer *renderer = NkRenderer::Create(device, rcfg);
    if (!renderer || !renderer->Initialize()) return 1;
    renderer->GetRender3D()->SetIBLStrength(3.5f);

    NkMeshHandle cube  = renderer->GetMeshSystem()->GetCube();
    NkMeshHandle plane = renderer->GetMeshSystem()->GetPlane();

    NkSceneContext ctx;
    ctx.camera = NkCamera3D(NkCamera3DData{
        {0.f, 1.70f, 1.5f}, {0.f, 1.40f, -2.f}, {0.f, 1.f, 0.f},
        70.f, static_cast<float32>(wc.width) / static_cast<float32>(wc.height), 0.05f, 200.f,
    });

    // Reglages de depart : direction, intensite, pas d'ombre.
    const NkVec3f DIR_DEPART  = {-0.6f, -0.65f, -0.45f};
    const float32 INT_DEPART  = 1.8f;
    // Changement audacieux (pas une nuance) pour chaque reglage, un a la fois.
    const NkVec3f DIR_CHANGEE = {0.75f, -0.25f, 0.15f};
    const float32 INT_CHANGEE = 6.0f;

    auto t0 = std::chrono::steady_clock::now();

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        int phase = (elapsed < 180.0) ? 0 : (elapsed < 360.0) ? 1 : (elapsed < 540.0) ? 2 : 3;
        static int lastPhase = -1;
        if (phase != lastPhase) {
            const char *names[4] = {"depart", "direction", "intensite", "ombres"};
            std::printf("DIAG: phase=%s\n", names[phase]);
            lastPhase = phase;
        }

        NkLightDesc sun;
        sun.type       = NkLightType::NK_DIRECTIONAL;
        sun.direction  = (phase == 1) ? DIR_CHANGEE : DIR_DEPART;
        sun.color      = {1.f, 0.95f, 0.85f};
        sun.intensity  = (phase == 2) ? INT_CHANGEE : INT_DEPART;
        sun.castShadow = (phase == 3);
        ctx.lights.Clear();
        ctx.lights.PushBack(sun);

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);
        SubmitRoom(renderer, cube, plane);
        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
