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
    app.appName = "Cube";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Cube";
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

    NkMeshHandle cube = renderer->GetMeshSystem()->GetCube();

    // Camera a l'origine, regardant vers -Z. Le cube est a 2m devant elle.
    NkSceneContext ctx;
    ctx.camera = NkCamera3D(NkCamera3DData{
        {0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f},
        60.f, static_cast<float32>(wc.width) / static_cast<float32>(wc.height), 0.05f, 200.f,
    });

    NkLightDesc sun;
    sun.type       = NkLightType::NK_DIRECTIONAL;
    sun.direction  = {-0.4f, -0.6f, -0.6f};
    sun.color      = {1.f, 1.f, 1.f};
    sun.intensity  = 2.2f;
    sun.castShadow = false;
    ctx.lights.PushBack(sun);

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);

        NkDrawCall3D dc;
        dc.mesh      = cube;
        dc.transform = NkMat4f::Translate({0.f, 0.f, -2.f});
        dc.tint      = {1.f, 1.f, 1.f};
        renderer->GetRender3D()->Submit(dc);

        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
