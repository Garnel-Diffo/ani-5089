// Exercice 10 : la manette qui s'eteint.
//
// Une manette s'eteint pendant que le joueur tient un objet, et l'objet ne doit pas tomber.
//
// Ce qui est simule ici, et pourquoi : le kit n'a pas de module XR. La main (sa pose de paume),
// la gachette (le doigt reste enfonce de l'image 3 a l'image 20) et l'etat d'une action sont donc
// ecrits dans ce programme. L'extinction de la manette est simulee comme le demande l'exercice :
// l'action "saisir" n'a plus de source liee pendant dix images (de l'image 5 a l'image 14), donc
// `actif` vaut faux, puis la source revient a l'image 15. Pendant ce temps le doigt n'a pas bouge.
// La scene est dessinee par le moteur, dans une cible hors ecran qu'on capture en PNG. Le geste
// est joue image par image, a pas fixe (1/30 s) : le resultat ne depend pas de la machine.
//
// Les trois champs de l'etat d'une action booleenne (voir actions.h de l'exercice 8) :
//   courant (current) : le niveau ;  change (changed) : a bascule depuis l'image d'avant ;
//   actif (active)    : une source materielle est liee.
// Une action inactive rend `courant` faux, et `change` vaut 1 sur l'image de l'extinction, comme
// pour un relachement : rien, dans `courant` et `change`, ne distingue les deux cas. Seul `actif`
// le fait.
//
// LES DEUX VERSIONS :
//
//   VERSION FAUSSE (on ne teste pas `actif`) : on lache sur le front de relachement, tel quel.
//
//       if (!etat.courant && etat.change) {       // FAUX : vrai aussi quand la manette s'eteint
//           tenu = false;
//       }
//
//   VERSION JUSTE : on ne lache que si `actif` est vrai ET que le relachement est un vrai front.
//
//       if (etat.actif && !etat.courant && etat.change) {
//           tenu = false;
//       }
//
// Le programme joue la version juste. ACTIF_SANS_TEST=1 rejoue la version fausse, celle de
// avant.png.
//
// Ce que je choisis, et pourquoi : pendant l'extinction, l'objet reste tenu. Dix images font un
// tiers de seconde, et lacher l'objet pour une coupure aussi breve est pire que de le garder. Cette
// reponse n'est pas bonne partout : une manette eteinte pour de bon (pile vide) laisserait l'objet
// accroche a la main indefiniment, et un vrai jeu y ajouterait un delai, ou un message.
//
// Sorties : journal.txt (trois champs, une ligne par image : `image : 12, courant : 1, change :
// 0, actif : 1`) et, si on le demande, la capture.
//
// Variables d'environnement (facultatives) :
//     ACTIF_SANS_TEST=1             la version fausse
//     ACTIF_CAPTURE_IMAGE=N         l'image a capturer (14 par defaut : la derniere eteinte)
//     ACTIF_CAPTURE=chemin.png      ou ecrire la capture (aucune sinon)
//     ACTIF_JOURNAL=chemin.txt      ou ecrire le journal (journal.txt par defaut)
//     ACTIF_IMAGES=N                nombre d'images jouees (20 par defaut)

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

static void ConfigureAppData(NkAppData &d) { d.appName = "Actif"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

// ------------------------------------------------------------------------------------------------
// L'etat d'une action booleenne : les trois champs, et la synchronisation qui les calcule.
// ------------------------------------------------------------------------------------------------
struct EtatAction {
    bool courant = false;   // current : le niveau
    bool change = false;    // changed : a bascule depuis la synchronisation precedente
    bool actif = false;     // active  : une source materielle est liee
};

struct Touche {
    bool pressee = false;   // la gachette simulee
};

struct Action {
    const Touche *source = nullptr;   // nullptr : rien n'est lie (la manette est eteinte)
    EtatAction etat;
    bool precedent = false;           // le `courant` de la synchronisation precedente
};

void Synchronise(Action &action) {
    action.etat.actif = (action.source != nullptr);
    action.etat.courant = action.etat.actif && action.source->pressee;
    action.etat.change = (action.etat.courant != action.precedent);
    action.precedent = action.etat.courant;
}

// ------------------------------------------------------------------------------------------------
// Le geste simule.
// ------------------------------------------------------------------------------------------------
const uint32 kImageAppui = 3;          // le doigt s'enfonce a l'image 3 ...
const uint32 kImageEteinte = 5;        // la manette s'eteint a l'image 5 ...
const uint32 kImageRallumee = 15;      // ... et revient a l'image 15 : dix images eteinte (5 a 14)
const uint32 kImageArrivee = 18;       // la main arrive a destination a l'image 18
const float32 kPas = 1.f / 30.f;       // un pas de simulation : 1/30 s
const float32 kCote = 0.18f;           // le cube : dix-huit centimetres de cote

// Le doigt reste enfonce jusqu'au bout : il ne bouge pas, c'est la manette qui s'eteint.
bool DoigtEnfonce(uint32 image) { return image >= kImageAppui; }
bool ManetteAllumee(uint32 image) { return image < kImageEteinte || image >= kImageRallumee; }

float32 Lisse(float32 s) { return s * s * (3.f - 2.f * s); }

NkVec3f Melange(const NkVec3f &a, const NkVec3f &b, float32 t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// La pose de la paume a l'image donnee : la main atteint le cube a l'image 3, puis l'emporte
// doucement vers la gauche et vers le haut, en tournant. Elle est demandee POUR UN INSTANT DONNE.
NkMat4f PosePaume(uint32 image) {
    const NkVec3f depart = { 0.50f, 0.50f, 0.60f };
    const NkVec3f saisie = { 0.15f, 0.50f, 0.12f };   // la paume contre le cube, au front
    const NkVec3f arrivee = { -0.35f, 0.72f, 0.40f };
    NkVec3f position = depart;
    float32 lacet = 0.f;   // l'angle autour de la verticale, en radians
    if (image <= kImageAppui) {
        position = Melange(depart, saisie, Lisse((float32)image / (float32)kImageAppui));
    } else {
        const float32 s = std::min(1.f, (float32)(image - kImageAppui) /
                                            (float32)(kImageArrivee - kImageAppui));
        position = Melange(saisie, arrivee, Lisse(s));
        lacet = 0.9f * Lisse(s);
    }
    return NkMat4f::Translate(position) * NkMat4f::RotationY(NkAngle::FromRad(lacet));
}

// ------------------------------------------------------------------------------------------------
// La scene : un sol, trois murs, la main (une paume et un avant-bras) et le cube.
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
            const EtatAction &etat, const NkMat4f &cube, float32 temps) {
    NkRender3D *r3d = renderer->GetRender3D();
    NkMeshSystem *meshes = renderer->GetMeshSystem();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    if (auto *mc = renderer->GetMaterialCollection()) {
        mc->Upload();
    }

    NkCamera3DData cd;
    cd.position = { 0.60f, 1.90f, 1.60f };
    cd.target = { -0.05f, 0.40f, 0.10f };
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
    // Trois murs, pour qu'on lise la profondeur et la hauteur.
    Boite(r3d, meshes, En(0.f, 1.4f, -1.25f), { 5.f, 2.8f, 0.1f }, { 0.56f, 0.64f, 0.78f });
    Boite(r3d, meshes, En(-2.5f, 1.4f, 0.f), { 0.1f, 2.8f, 5.f }, { 0.74f, 0.68f, 0.56f });
    Boite(r3d, meshes, En(2.5f, 1.4f, 0.f), { 0.1f, 2.8f, 5.f }, { 0.74f, 0.68f, 0.56f });

    // La main : la paume, et l'avant-bras qui part vers l'arriere (vers le joueur). La paume est
    // verte tant que l'action est vraie, grise quand la manette est eteinte, couleur peau sinon.
    NkVec3f peau = { 0.86f, 0.66f, 0.52f };
    if (!etat.actif) {
        peau = { 0.45f, 0.45f, 0.48f };
    } else if (etat.courant) {
        peau = { 0.35f, 0.80f, 0.35f };
    }
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

    const bool sansTest = getenv("ACTIF_SANS_TEST") != nullptr;
    const uint32 nombreImages = (uint32)EnvU64("ACTIF_IMAGES", 20);
    const uint32 imageCapture = (uint32)EnvU64("ACTIF_CAPTURE_IMAGE", 14);
    const char *cheminCapture = getenv("ACTIF_CAPTURE");
    const char *cheminJournal = getenv("ACTIF_JOURNAL");
    if (cheminJournal == nullptr || *cheminJournal == '\0') cheminJournal = "journal.txt";

    NkWindowConfig cfg;
    cfg.title = "La manette qui s'eteint";
    cfg.width = 640;
    cfg.height = 360;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[Actif] la fenetre n'a pas pu s'ouvrir\n");
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
        std::printf("[Actif] aucune API graphique disponible\n");
        return 1;
    }

    // Le renderer d'ecran possede l'image ; celui de la scene dessine dans une cible hors ecran
    // qu'on peut relire et capturer.
    NkRenderer *rEcran = NkRenderer::Create(
        device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[Actif] le renderer d'ecran n'a pas pu s'initialiser\n");
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
    od.name = "Actif_Cible";
    NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
    if (cible == nullptr) {
        std::printf("[Actif] cible hors ecran refusee\n");
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
        std::printf("[Actif] le renderer de la scene n'a pas pu s'initialiser\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));

    // Le journal : les trois champs, une ligne par image. Ouvert en binaire : un seul octet de fin
    // de ligne, le meme partout.
    std::FILE *journal = std::fopen(cheminJournal, "wb");

    // ---- L'etat du monde simule ----
    Touche gachette;
    Action saisir;   // l'action "saisir" : sa source est liee a la gachette, sauf manette eteinte

    // Le cube flotte a hauteur de main, comme un objet a prendre.
    NkMat4f cube = En(0.15f, 0.55f, 0.0f);
    NkMat4f attache = NkMat4f::Identity();   // la pose du cube dans le repere de la main
    bool tenu = false;                       // l'objet est-il tenu ?
    bool tombe = false;                      // l'objet est-il en chute libre ?
    float32 vitesse = 0.f;                   // sa vitesse verticale pendant la chute, en m/s

    std::printf("[Actif] version %s\n", sansTest ? "FAUSSE (sans tester actif)"
                                                  : "JUSTE (actif ET front de relachement)");

    for (uint32 image = 1; image <= nombreImages && fenetre.IsOpen(); ++image) {
        NkEvent *ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                image = nombreImages;   // on s'arrete apres cette image
            }
        }

        // 1. Le geste de cette image : le doigt, la manette (allumee ou eteinte), la paume.
        gachette.pressee = DoigtEnfonce(image);
        saisir.source = ManetteAllumee(image) ? &gachette : nullptr;   // eteinte : rien n'est lie
        const NkMat4f paume = PosePaume(image);

        // 2. On synchronise l'action UNE fois, puis on l'interroge.
        Synchronise(saisir);
        const EtatAction &etat = saisir.etat;

        if (journal != nullptr) {
            std::fprintf(journal, "image : %u, courant : %d, change : %d, actif : %d\n", image,
                         etat.courant ? 1 : 0, etat.change ? 1 : 0, etat.actif ? 1 : 0);
        }

        // 3. Ce que fait le jeu de cet etat.
        const NkVec3f p = paume.TransformPoint({ 0.f, 0.f, 0.f });
        const NkVec3f c0 = cube.TransformPoint({ 0.f, 0.f, 0.f });
        const float32 distance2 = (c0.x - p.x) * (c0.x - p.x) + (c0.y - p.y) * (c0.y - p.y) +
                                  (c0.z - p.z) * (c0.z - p.z);
        const bool pres = distance2 < 0.35f * 0.35f;   // a moins de 35 cm de la paume

        if (etat.courant && etat.change && pres && !tenu && !tombe) {
            attache = paume.Inverse() * cube;   // saisir : une fois, sur le front
            tenu = true;
        }
        if (tenu) {
            const bool relachement = !etat.courant && etat.change;   // le front de relachement
            // Lacher : seulement si la manette est allumee. Sans ce test, l'extinction (courant
            // faux, change vrai) est prise pour un relachement et l'objet tombe.
            const bool lache = sansTest ? relachement : (etat.actif && relachement);
            if (lache) {
                tenu = false;
                tombe = true;
            }
        }
        if (tenu) {
            cube = paume * attache;   // a chaque image, tant qu'il est tenu
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

        std::printf("[Actif] image %2u : courant %d change %d actif %d, tenu %d, "
                    "hauteur du cube %.3f m\n",
                    image, etat.courant ? 1 : 0, etat.change ? 1 : 0, etat.actif ? 1 : 0,
                    tenu ? 1 : 0, (double)cube.TransformPoint({ 0.f, 0.f, 0.f }).y);

        // 4. Le rendu, puis la capture a l'image demandee.
        if (rEcran->BeginFrame()) {
            NkICommandBuffer *cmd = rEcran->GetCmd();
            Rendre(renderer, cmd, (float32)devInit.width / (float32)devInit.height, paume, etat,
                   cube, (float32)image * kPas);
            if (auto *r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(),
                               { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        if (cheminCapture != nullptr && *cheminCapture != '\0' && image == imageCapture) {
            const bool ok = cible->Capture(cheminCapture);
            std::printf("[Actif] capture de l'image %u -> %s (%s)\n", image, cheminCapture,
                        ok ? "ok" : "echec");
        }
    }

    if (journal != nullptr) std::fclose(journal);
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
