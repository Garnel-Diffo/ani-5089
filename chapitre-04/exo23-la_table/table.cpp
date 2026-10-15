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
    app.appName = "Table";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Table";
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
    rcfg.subsystems = NK_SS_RENDER3D;
    NkRenderer *renderer = NkRenderer::Create(device, rcfg);
    if (!renderer || !renderer->Initialize()) return 1;
    renderer->GetRender3D()->SetIBLStrength(3.5f);

    NkMeshHandle cube  = renderer->GetMeshSystem()->GetCube();
    NkMeshHandle plane = renderer->GetMeshSystem()->GetPlane();

    NkSceneContext ctx;
    ctx.camera = NkCamera3D(NkCamera3DData{
        {0.f, 1.2f, 2.6f}, {0.f, 0.4f, 0.f}, {0.f, 1.f, 0.f},
        60.f, static_cast<float32>(wc.width) / static_cast<float32>(wc.height), 0.05f, 200.f,
    });

    NkLightDesc sun;
    sun.type       = NkLightType::NK_DIRECTIONAL;
    sun.direction  = {-0.5f, -0.75f, -0.4f};
    sun.color      = {1.f, 0.95f, 0.85f};
    sun.intensity  = 2.0f;
    sun.castShadow = false;
    ctx.lights.PushBack(sun);

    // Dimensions de la table (metres).
    const float32 HAUTEUR_TABLE   = 0.75f; // hauteur totale, du sol au-dessus du plateau
    const float32 EP_PLATEAU      = 0.05f; // epaisseur du plateau
    const float32 LONGUEUR_PLATEAU = 1.20f;
    const float32 LARGEUR_PLATEAU  = 0.80f;
    const float32 SECTION_PIED     = 0.05f;

    // Calcul : le plateau occupe le dessus de la table, donc son centre est
    // a HAUTEUR_TABLE - EP_PLATEAU/2 (et non a HAUTEUR_TABLE, qui serait son
    // bord superieur). Les pieds comblent l'espace entre le sol et le dessous
    // du plateau : leur hauteur est donc HAUTEUR_TABLE - EP_PLATEAU (pas
    // HAUTEUR_TABLE), et leur centre est a mi-hauteur de cette valeur.
    const float32 hauteurPied = HAUTEUR_TABLE - EP_PLATEAU;
    const float32 centreYPied = hauteurPied * 0.5f;
    const float32 centreYPlateau = HAUTEUR_TABLE - EP_PLATEAU * 0.5f;

    std::printf("CALCUL hauteurPied=%.4f centreYPied=%.4f centreYPlateau=%.4f\n",
                hauteurPied, centreYPied, centreYPlateau);

    const float32 insetX = LONGUEUR_PLATEAU * 0.5f - SECTION_PIED;
    const float32 insetZ = LARGEUR_PLATEAU * 0.5f - SECTION_PIED;
    NkVec3f piedsXZ[4] = {
        { insetX, 0.f, insetZ}, { insetX, 0.f,-insetZ}, {-insetX, 0.f, insetZ}, {-insetX, 0.f,-insetZ},
    };

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);

        // Sol.
        {
            NkDrawCall3D dc;
            dc.mesh      = plane;
            dc.transform = NkMat4f::Scale({3.f, 1.f, 3.f});
            dc.tint      = {0.55f, 0.52f, 0.48f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Plateau.
        {
            NkDrawCall3D dc;
            dc.mesh      = cube;
            dc.transform = NkMat4f::Translate({0.f, centreYPlateau, 0.f})
                          * NkMat4f::Scale({LONGUEUR_PLATEAU, EP_PLATEAU, LARGEUR_PLATEAU});
            dc.tint      = {0.55f, 0.35f, 0.20f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Quatre pieds.
        for (int i = 0; i < 4; ++i) {
            NkDrawCall3D dc;
            dc.mesh      = cube;
            dc.transform = NkMat4f::Translate({piedsXZ[i].x, centreYPied, piedsXZ[i].z})
                          * NkMat4f::Scale({SECTION_PIED, hauteurPied, SECTION_PIED});
            dc.tint      = {0.35f, 0.22f, 0.12f};
            renderer->GetRender3D()->Submit(dc);
        }

        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
