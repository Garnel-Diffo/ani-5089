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

using namespace nkentseu;
using namespace nkentseu::renderer;

int nkmain(const NkEntryState & /*state*/) {
    NkAppData app;
    app.appName = "Salle";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Salle";
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

    std::printf("Interface graphique : %s\n", NkGraphicsApiName(NkGraphicsApi::NK_GFX_API_SOFTWARE));

    NkRendererConfig rcfg;
    rcfg.subsystems = NK_SS_RENDER3D;
    NkRenderer *renderer = NkRenderer::Create(device, rcfg);
    if (!renderer || !renderer->Initialize()) return 1;
    renderer->GetRender3D()->SetIBLStrength(3.5f);

    NkMeshHandle cube  = renderer->GetMeshSystem()->GetCube();
    NkMeshHandle plane = renderer->GetMeshSystem()->GetPlane();

    NkSceneContext ctx;
    ctx.camera = NkCamera3D(NkCamera3DData{
        /*position*/ {0.f, 1.70f, 1.5f},
        /*target*/   {0.f, 1.40f, -2.f},
        /*up*/       {0.f, 1.f, 0.f},
        /*fovY*/     70.f,
        /*aspect*/   static_cast<float32>(wc.width) / static_cast<float32>(wc.height),
        /*nearPlane*/0.05f,
        /*farPlane*/ 200.f,
    });

    NkLightDesc sun;
    sun.type       = NkLightType::NK_DIRECTIONAL;
    sun.direction  = {-0.6f, -0.65f, -0.45f};
    sun.color      = {1.f, 0.95f, 0.85f};
    sun.intensity  = 1.8f;
    sun.castShadow = false; // NK_SS_SHADOW non demande pour cet exercice
    ctx.lights.PushBack(sun);

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);

        // Sol : un plan, pose a y=0, sans demi-hauteur.
        {
            NkDrawCall3D dc;
            dc.mesh      = plane;
            dc.transform = NkMat4f::Scale({4.f, 1.f, 4.f});
            dc.tint      = {0.55f, 0.52f, 0.48f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Plafond : un cube aplati, pose au sommet des murs (y = 2.5).
        {
            NkDrawCall3D dc;
            dc.mesh      = cube;
            dc.transform = NkMat4f::Translate({0.f, 2.5f, 0.f}) * NkMat4f::Scale({4.f, 0.1f, 4.f});
            dc.tint      = {0.85f, 0.85f, 0.85f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Quatre murs, centres a mi-hauteur (y = 1.25).
        {
            NkDrawCall3D mur;
            mur.mesh      = cube;
            mur.transform = NkMat4f::Translate({0.f, 1.25f, -2.f}) * NkMat4f::Scale({4.f, 2.5f, 0.1f});
            mur.tint      = {0.72f, 0.70f, 0.66f};
            renderer->GetRender3D()->Submit(mur);
        }
        {
            NkDrawCall3D mur;
            mur.mesh      = cube;
            mur.transform = NkMat4f::Translate({0.f, 1.25f, 2.f}) * NkMat4f::Scale({4.f, 2.5f, 0.1f});
            mur.tint      = {0.72f, 0.70f, 0.66f};
            renderer->GetRender3D()->Submit(mur);
        }
        {
            NkDrawCall3D mur;
            mur.mesh      = cube;
            mur.transform = NkMat4f::Translate({-2.f, 1.25f, 0.f}) * NkMat4f::Scale({0.1f, 2.5f, 4.f});
            mur.tint      = {0.72f, 0.70f, 0.66f};
            renderer->GetRender3D()->Submit(mur);
        }
        {
            NkDrawCall3D mur;
            mur.mesh      = cube;
            mur.transform = NkMat4f::Translate({2.f, 1.25f, 0.f}) * NkMat4f::Scale({0.1f, 2.5f, 4.f});
            mur.tint      = {0.72f, 0.70f, 0.66f};
            renderer->GetRender3D()->Submit(mur);
        }

        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
