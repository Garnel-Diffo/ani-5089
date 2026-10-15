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

// Pose un objet de taille `taille` au sol (sa face inferieure touche y=0),
// centre en (x, z).
static NkMat4f PoserAuSol(NkVec3f taille, float32 x, float32 z) {
    return NkMat4f::Translate({x, taille.y * 0.5f, z}) * NkMat4f::Scale(taille);
}

// Pose un objet de taille `taille` sur une surface horizontale situee a
// hauteurSurface (sa face inferieure touche hauteurSurface), centre en (x, z).
static NkMat4f PoserSurTable(NkVec3f taille, float32 x, float32 z, float32 hauteurSurface) {
    return NkMat4f::Translate({x, hauteurSurface + taille.y * 0.5f, z}) * NkMat4f::Scale(taille);
}

int nkmain(const NkEntryState & /*state*/) {
    NkAppData app;
    app.appName = "Poser";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Poser";
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

    const float32 HAUTEUR_MUR = 2.5f;

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);

        // --- Debut du bloc recrit avec PoserAuSol / PoserSurTable ---

        // Sol : plan sans epaisseur verticale, deja a y=0 par nature du mesh.
        {
            NkDrawCall3D dc;
            dc.mesh      = plane;
            dc.transform = NkMat4f::Scale({4.f, 1.f, 4.f});
            dc.tint      = {0.55f, 0.52f, 0.48f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Plafond : pose sur le dessus des murs.
        {
            NkDrawCall3D dc;
            dc.mesh      = cube;
            dc.transform = PoserSurTable({4.f, 0.1f, 4.f}, 0.f, 0.f, HAUTEUR_MUR);
            dc.tint      = {0.85f, 0.85f, 0.85f};
            renderer->GetRender3D()->Submit(dc);
        }
        // Quatre murs : chacun pose au sol.
        struct Mur { NkVec3f taille; float32 x, z; };
        Mur murs[4] = {
            {{4.f, HAUTEUR_MUR, 0.1f},  0.f, -2.f},
            {{4.f, HAUTEUR_MUR, 0.1f},  0.f,  2.f},
            {{0.1f, HAUTEUR_MUR, 4.f}, -2.f,  0.f},
            {{0.1f, HAUTEUR_MUR, 4.f},  2.f,  0.f},
        };
        for (const Mur &m : murs) {
            NkDrawCall3D dc;
            dc.mesh      = cube;
            dc.transform = PoserAuSol(m.taille, m.x, m.z);
            dc.tint      = {0.72f, 0.70f, 0.66f};
            renderer->GetRender3D()->Submit(dc);
        }

        // --- Fin du bloc recrit ---

        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
