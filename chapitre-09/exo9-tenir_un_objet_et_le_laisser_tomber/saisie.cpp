// Exercice 9 : tenir un objet, et le laisser tomber.
//
// On attache un cube a la POSE DE LA PAUME de la main droite quand l'action "saisir" est vraie,
// d'abord de la mauvaise facon (sur le front), puis de la bonne (sur le niveau), et on capture
// la meme image du geste dans les deux cas : front.png et niveau.png.
//
// Ce qui est simule ici, et pourquoi : le kit n'a pas de module XR. La main (sa pose de paume,
// image par image), la gachette (enfoncee de l'image 10 a l'image 46) et l'etat d'une action
// (les trois champs courant, change, actif, comme dans actions.h de l'exercice 8) sont donc
// ecrits dans ce programme. La scene, elle, est dessinee par le moteur, dans une cible hors
// ecran qu'on capture en PNG. Le geste est joue image par image, a pas fixe (1/60 s) : le resultat
// ne depend pas de la vitesse de la machine.
//
// LA LIGNE QUI COMPTE : l'objet est attache dans le repere de la main, UNE fois, a la saisie,
//     attache = paume.Inverse() * cube;      // a la saisie, une fois
//     cube    = paume * attache;             // a chaque image, tant qu'il est tenu
// Sans elle, l'objet saute au centre de la main au lieu d'etre tenu la ou on l'a pris.
//
// LES DEUX VERSIONS :
//
//   VERSION FAUSSE (tenir sur le front) : on teste `current && changed` a CHAQUE image pour
//   savoir si l'objet est tenu. C'est vrai sur l'image de l'appui, puis plus jamais tant que le
//   doigt reste pose : l'objet echappe a la main des l'image suivante.
//
//       const bool tenu = grab.current && grab.changed;      // FAUX : un front dure une image
//       if (tenu) {
//           cube = paume * attache;
//       }
//
//   VERSION JUSTE (tenir sur le niveau) : le front sert a saisir, le niveau sert a tenir, le front
//   de relachement sert a lacher.
//
//       if (grab.current && grab.changed)  { saisir : attache = paume.Inverse() * cube; }
//       if (!grab.current && grab.changed) { lacher : tenu = false; }
//       if (tenu) { cube = paume * attache; }
//
// Le programme joue la version juste. SAISIE_TENIR_SUR_LE_FRONT=1 rejoue la version fausse, celle
// de front.png.
//
// Variables d'environnement (facultatives) :
//     SAISIE_TENIR_SUR_LE_FRONT=1   la version fausse
//     SAISIE_CAPTURE_IMAGE=N        l'image a capturer (28 par defaut)
//     SAISIE_CAPTURE=chemin.png     ou ecrire la capture (aucune sinon)
//     SAISIE_IMAGES=N               nombre d'images jouees (60 par defaut)

#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"

#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkGraphicsApi.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Core/NkCamera.h"
#include "NKRenderer/Core/NkRenderGraph.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterialCollection.h"
#include "NKRenderer/Mesh/NkMeshSystem.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include "NKRenderer/Tools/Render2D/NkRender2D.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"

#include "NKMath/NkAngle.h"
#include "NKMath/NkMat.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData &d) { d.appName = "Saisie"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

// ------------------------------------------------------------------------------------------------
// L'etat d'une action booleenne : les trois champs, et la synchronisation qui les calcule.
// ------------------------------------------------------------------------------------------------
struct EtatActionBool {
    bool current = false;   // courant : le niveau
    bool changed = false;   // change  : a bascule depuis la synchronisation precedente
    bool active = false;    // actif   : une source materielle est liee
};

struct Touche {
    bool pressee = false;   // la gachette simulee
};

struct ActionBool {
    const Touche *source = nullptr;   // nullptr : rien n'est lie
    EtatActionBool etat;
    bool precedent = false;           // le `current` de la synchronisation precedente
};

void Synchronise(ActionBool &action) {
    action.etat.active = (action.source != nullptr);
    action.etat.current = action.etat.active && action.source->pressee;
    action.etat.changed = (action.etat.current != action.precedent);
    action.precedent = action.etat.current;
}

// ------------------------------------------------------------------------------------------------
// Le geste simule : la gachette, et la pose de la paume de la main droite, image par image.
// ------------------------------------------------------------------------------------------------
const uint32 kImageSaisie = 10;      // la gachette est enfoncee a l'image 10 (le front) ...
const uint32 kImageRelache = 47;     // ... tenue jusqu'a l'image 46, relachee a l'image 47
const uint32 kImageArrivee = 40;     // la main arrive a destination a l'image 40
const float32 kPas = 1.f / 60.f;     // un pas de simulation : 1/60 s
const float32 kCote = 0.18f;         // le cube : dix-huit centimetres de cote

bool GachettePressee(uint32 image) { return image >= kImageSaisie && image < kImageRelache; }

float32 Lisse(float32 s) { return s * s * (3.f - 2.f * s); }

NkVec3f Melange(const NkVec3f &a, const NkVec3f &b, float32 t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// La pose de la paume a l'image donnee : la main part du cote, atteint le cube a l'image 10, puis
// l'emporte vers la gauche, plus haut et vers l'avant, en tournant. Elle est demandee POUR UN
// INSTANT DONNE : ici, celui de l'image.
NkMat4f PosePaume(uint32 image) {
    const NkVec3f depart = { 0.55f, 0.95f, 0.65f };
    const NkVec3f saisie = { 0.15f, 1.10f, 0.12f };   // la paume contre le cube, au front
    const NkVec3f arrivee = { -0.45f, 1.50f, 0.50f };
    NkVec3f position = depart;
    float32 lacet = 0.f;   // l'angle autour de la verticale, en radians
    if (image <= kImageSaisie) {
        position = Melange(depart, saisie, Lisse((float32)image / (float32)kImageSaisie));
    } else {
        const float32 s = std::min(1.f, (float32)(image - kImageSaisie) /
                                            (float32)(kImageArrivee - kImageSaisie));
        position = Melange(saisie, arrivee, Lisse(s));
        lacet = 0.9f * Lisse(s);
    }
    return NkMat4f::Translate(position) * NkMat4f::RotationY(NkAngle::FromRad(lacet));
}

// ------------------------------------------------------------------------------------------------
// La scene : un sol, deux murs, la main (une paume et un avant-bras) et le cube.
// ------------------------------------------------------------------------------------------------
void Boite(NkRender3D *r3d, NkMeshSystem *meshes, const NkMat4f &pose, const NkVec3f &taille,
           const NkVec3f &teinte) {
    NkDrawCall3D dc;
    dc.mesh = meshes->GetCube();
    dc.transform = pose * NkMat4f::Scale(taille);
    const NkVec3f c = pose.TransformPoint({ 0.f, 0.f, 0.f });
    const float32 r = std::max(taille.x, std::max(taille.y, taille.z));
    dc.aabb = { { c.x - r, c.y - r, c.z - r }, { c.x + r, c.y + r, c.z + r } };
    dc.castShadow = false;
    dc.tint = teinte;
    dc.metallic = 0.f;
    dc.roughness = 0.8f;
    r3d->Submit(dc);
}

NkMat4f En(float32 x, float32 y, float32 z) { return NkMat4f::Translate({ x, y, z }); }

void Rendre(NkRenderer *renderer, NkICommandBuffer *cmd, float32 aspect, const NkMat4f &paume,
            bool gachette, const NkMat4f &cube, float32 temps) {
    NkRender3D *r3d = renderer->GetRender3D();
    NkMeshSystem *meshes = renderer->GetMeshSystem();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    if (auto *mc = renderer->GetMaterialCollection()) {
        mc->Upload();
    }

    NkCamera3DData cd;
    cd.position = { 0.00f, 1.80f, 1.95f };
    cd.target = { -0.05f, 0.85f, 0.10f };
    cd.up = { 0.f, 1.f, 0.f };
    cd.fovY = 55.f;
    cd.aspect = aspect;
    cd.nearPlane = 0.05f;
    cd.farPlane = 100.f;

    NkSceneContext sctx;
    sctx.camera = NkCamera3D(cd);
    sctx.time = temps;
    r3d->SetIBLStrength(0.45f);

    NkLightDesc soleil;
    soleil.type = NkLightType::NK_DIRECTIONAL;
    soleil.direction = { -0.4f, -1.f, -0.5f };
    soleil.color = { 1.f, 0.96f, 0.88f };
    soleil.intensity = 3.2f;
    soleil.castShadow = false;
    sctx.lights.PushBack(soleil);

    r3d->BeginScene(sctx);
    {
        NkDrawCall3D sol;
        sol.mesh = meshes->GetPlane();
        sol.transform = NkMat4f::Scale({ 6.f, 1.f, 6.f });
        sol.aabb = { { -3.f, 0.f, -3.f }, { 3.f, 0.f, 3.f } };
        sol.castShadow = false;
        sol.tint = { 0.42f, 0.40f, 0.37f };
        sol.metallic = 0.f;
        sol.roughness = 1.f;
        r3d->Submit(sol);
    }
    // Deux murs, pour qu'on lise la profondeur et la hauteur.
    Boite(r3d, meshes, En(0.f, 1.4f, -1.25f), { 5.f, 2.8f, 0.1f }, { 0.56f, 0.64f, 0.78f });
    Boite(r3d, meshes, En(-2.5f, 1.4f, 0.f), { 0.1f, 2.8f, 5.f }, { 0.74f, 0.68f, 0.56f });
    Boite(r3d, meshes, En(2.5f, 1.4f, 0.f), { 0.1f, 2.8f, 5.f }, { 0.74f, 0.68f, 0.56f });

    // La main : la paume, et l'avant-bras qui part vers l'arriere (vers le joueur).
    // La paume est verte tant que la gachette est enfoncee (l'action saisir est vraie).
    const NkVec3f peau =
        gachette ? NkVec3f{ 0.35f, 0.80f, 0.35f } : NkVec3f{ 0.86f, 0.66f, 0.52f };
    Boite(r3d, meshes, paume, { 0.14f, 0.05f, 0.16f }, peau);
    Boite(r3d, meshes, paume * En(0.f, 0.f, 0.30f), { 0.08f, 0.07f, 0.40f },
          { 0.93f, 0.78f, 0.64f });

    // Le cube, et son ombre portee au sol : un pave plat et sombre sous lui, qui aide a lire sa
    // hauteur sur une image fixe.
    Boite(r3d, meshes, cube, { kCote, kCote, kCote }, { 0.95f, 0.45f, 0.10f });
    const NkVec3f c = cube.TransformPoint({ 0.f, 0.f, 0.f });
    Boite(r3d, meshes, En(c.x, 0.003f, c.z), { 0.20f, 0.004f, 0.20f }, { 0.10f, 0.10f, 0.10f });

    if (auto *graph = renderer->GetRenderGraph()) {
        graph->Execute(cmd);
    }
}

uint64 EnvU64(const char *nom, uint64 defaut) {
    const char *t = getenv(nom);
    if (t == nullptr || *t == '\0') return defaut;
    return (uint64)strtoull(t, nullptr, 10);
}

} // namespace

int nkmain(const NkEntryState &state) {
    (void)state;
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    const bool tenirSurLeFront = getenv("SAISIE_TENIR_SUR_LE_FRONT") != nullptr;
    const uint32 nombreImages = (uint32)EnvU64("SAISIE_IMAGES", 60);
    const uint32 imageCapture = (uint32)EnvU64("SAISIE_CAPTURE_IMAGE", 28);
    const char *cheminCapture = getenv("SAISIE_CAPTURE");

    NkWindowConfig cfg;
    cfg.title = "Tenir un objet";
    cfg.width = 640;
    cfg.height = 360;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[Saisie] la fenetre n'a pas pu s'ouvrir\n");
        return 1;
    }

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    // DirectX 11 : les nuanceurs DirectX 12 de cette machine ne compilent pas.
    devInit.api = NkGraphicsApi::NK_GFX_API_DX11;
    NkIDevice *device = NkDeviceFactory::CreateForApi(devInit.api, devInit);
    if (device == nullptr) {
        std::printf("[Saisie] aucune API graphique disponible\n");
        return 1;
    }

    // Le renderer d'ecran possede l'image ; celui de la scene dessine dans une cible hors ecran
    // qu'on peut relire et capturer.
    NkRenderer *rEcran = NkRenderer::Create(
        device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[Saisie] le renderer d'ecran n'a pas pu s'initialiser\n");
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    NkOffscreenDesc od;
    od.width = devInit.width;
    od.height = devInit.height;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;   // sans quoi Capture() ne peut rien relire
    od.name = "Saisie_Cible";
    NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
    if (cible == nullptr) {
        std::printf("[Saisie] cible hors ecran refusee\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    NkRendererConfig cfgScene =
        NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
    cfgScene.Disable(NK_SS_RENDER2D);
    cfgScene.Disable(NK_SS_TEXT);
    cfgScene.Disable(NK_SS_OVERLAY);
    cfgScene.Enable(NK_SS_OFFSCREEN);
    cfgScene.Disable(NK_SS_POST_PROCESS);   // des captures nettes : ni flou ni bloom
    NkRenderer *renderer = NkRenderer::Create(device, cfgScene);
    if (renderer == nullptr || !renderer->Initialize()) {
        std::printf("[Saisie] le renderer de la scene n'a pas pu s'initialiser\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));

    // ---- L'etat du monde simule ----
    Touche gachette;
    ActionBool saisir;
    saisir.source = &gachette;               // l'action "saisir" est liee a la gachette

    // Le cube flotte a hauteur de main, comme un objet a prendre.
    NkMat4f cube = En(0.15f, 1.15f, 0.0f);
    NkMat4f attache = NkMat4f::Identity();   // la pose du cube dans le repere de la main
    bool tenu = false;                       // l'objet est-il tenu ?
    bool tombe = false;                      // l'objet est-il en chute libre ?
    float32 vitesse = 0.f;                   // sa vitesse verticale pendant la chute, en m/s

    std::printf("[Saisie] version %s\n", tenirSurLeFront ? "FAUSSE (tenir sur le front)"
                                                          : "JUSTE (tenir sur le niveau)");

    for (uint32 image = 1; image <= nombreImages && fenetre.IsOpen(); ++image) {
        NkEvent *ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                image = nombreImages;   // on s'arrete apres cette image
            }
        }

        // 1. Le geste de cette image : la gachette, puis la pose de la paume, demandee pour cet
        //    instant.
        gachette.pressee = GachettePressee(image);
        const NkMat4f paume = PosePaume(image);

        // 2. On synchronise l'action UNE fois, puis on l'interroge.
        Synchronise(saisir);
        const EtatActionBool &grab = saisir.etat;

        // 3. Ce que fait le jeu de cet etat.
        const NkVec3f ecart = [&]() {
            const NkVec3f p = paume.TransformPoint({ 0.f, 0.f, 0.f });
            const NkVec3f c = cube.TransformPoint({ 0.f, 0.f, 0.f });
            return NkVec3f{ c.x - p.x, c.y - p.y, c.z - p.z };
        }();
        const float32 distance2 = ecart.x * ecart.x + ecart.y * ecart.y + ecart.z * ecart.z;
        const bool pres = distance2 < 0.35f * 0.35f;   // a moins de 35 cm de la paume

        if (grab.active && grab.current && grab.changed && pres && !tenu && !tombe) {
            attache = paume.Inverse() * cube;   // la ligne qui compte : une fois, a la saisie
            tenu = true;
        }
        if (tenirSurLeFront) {
            // VERSION FAUSSE : tenu seulement sur l'image du front.
            const bool tenuCetteImage = grab.current && grab.changed;
            if (tenuCetteImage) {
                cube = paume * attache;
            } else if (tenu) {
                tenu = false;   // l'objet a echappe a la main : il tombe
                tombe = true;
            }
        } else {
            // VERSION JUSTE : tenu sur le niveau, lache sur le front de relachement.
            if (tenu && !grab.current && grab.changed) {
                tenu = false;
                tombe = true;
            }
            if (tenu) {
                cube = paume * attache;   // a chaque image, tant qu'il est tenu
            }
        }
        if (tombe) {
            const float32 hauteur = cube.TransformPoint({ 0.f, 0.f, 0.f }).y;
            vitesse -= 9.81f * kPas;
            float32 dy = vitesse * kPas;
            if (hauteur + dy < kCote * 0.5f) {
                dy = kCote * 0.5f - hauteur;   // le sol arrete le cube
                vitesse = 0.f;
            }
            cube = NkMat4f::Translate({ 0.f, dy, 0.f }) * cube;
        }

        std::printf("[Saisie] image %2u : courant %d change %d actif %d, tenu %d, "
                    "hauteur du cube %.3f m\n",
                    image, grab.current ? 1 : 0, grab.changed ? 1 : 0, grab.active ? 1 : 0,
                    tenu ? 1 : 0, (double)cube.TransformPoint({ 0.f, 0.f, 0.f }).y);

        // 4. Le rendu, puis la capture a l'image demandee.
        if (rEcran->BeginFrame()) {
            NkICommandBuffer *cmd = rEcran->GetCmd();
            Rendre(renderer, cmd, (float32)devInit.width / (float32)devInit.height, paume,
                   grab.current, cube, (float32)image * kPas);
            if (auto *r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(),
                               { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        if (cheminCapture != nullptr && *cheminCapture != '\0' && image == imageCapture) {
            const bool ok = cible->Capture(cheminCapture);
            std::printf("[Saisie] capture de l'image %u -> %s (%s)\n", image, cheminCapture,
                        ok ? "ok" : "echec");
        }
    }

    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
