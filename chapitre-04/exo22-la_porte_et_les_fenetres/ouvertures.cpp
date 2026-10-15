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
    app.appName = "Ouvertures";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig wc;
    wc.title  = "Ouvertures";
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

    while (window.IsOpen()) {
        NkEvents().PollEvents();

        renderer->BeginFrame();
        renderer->GetRender3D()->BeginScene(ctx);

        // Sol, plafond, 4 murs : la salle de l'exercice 8.
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

        // Porte et fenetres : panneaux inseres dans le mur avant (z = -2),
        // dont la face interieure est a z = -1.95. On les decale de 0.10m
        // vers l'interieur de la piece (z = -1.85) pour eviter que leur
        // face ne soit exactement coplanaire avec celle du mur : deux
        // triangles a la MEME profondeur produisent un clignotement
        // (z-fighting) car le rasteriseur alterne de maniere instable
        // entre les deux surfaces d'une image a l'autre. Un petit decalage
        // le long de la normale du mur leve l'ambiguite.
        const float32 DECALAGE = 0.10f;

        // Porte : sombre, au sol, decentree.
        {
            NkDrawCall3D porte;
            porte.mesh      = cube;
            porte.transform = NkMat4f::Translate({-0.8f, 1.0f, -2.f + DECALAGE}) * NkMat4f::Scale({0.8f, 2.0f, 0.05f});
            porte.tint      = {0.25f, 0.18f, 0.12f};
            renderer->GetRender3D()->Submit(porte);
        }
        // Deux fenetres : teinte bleutee, plus haut sur le mur.
        {
            NkDrawCall3D fenetre;
            fenetre.mesh      = cube;
            fenetre.transform = NkMat4f::Translate({0.5f, 1.6f, -2.f + DECALAGE}) * NkMat4f::Scale({0.6f, 0.6f, 0.05f});
            fenetre.tint      = {0.55f, 0.75f, 0.85f};
            renderer->GetRender3D()->Submit(fenetre);
        }
        {
            NkDrawCall3D fenetre;
            fenetre.mesh      = cube;
            fenetre.transform = NkMat4f::Translate({1.5f, 1.6f, -2.f + DECALAGE}) * NkMat4f::Scale({0.6f, 0.6f, 0.05f});
            fenetre.tint      = {0.55f, 0.75f, 0.85f};
            renderer->GetRender3D()->Submit(fenetre);
        }

        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(device);
    NkClose();
    return 0;
}
