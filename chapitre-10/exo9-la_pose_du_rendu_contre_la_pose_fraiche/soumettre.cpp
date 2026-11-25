// Exercice 9 : la pose du rendu, contre la pose fraiche.
//
// On soumet la couche deux fois, avec la pose du rendu puis avec la pose fraiche, et on montre la
// difference : rendu.png (la version juste) et fraiche.png (la version fautive), capturees a la
// meme trame du mouvement. La pose fraiche est plus vraie, et elle est fausse ici.
//
// Ce qui est simule ici, et pourquoi : le kit n'a pas de module XR ni de compositeur. La tete
// (un lacet qui croit de deux degres par trame), l'instant d'affichage (trois trames apres le
// rendu : la tete a donc bouge de six degres entre le calcul des matrices et la soumission) et le
// compositeur sont ecrits dans ce programme. Le moteur dessine la scene dans une cible hors
// ecran qu'on capture en PNG. Le geste est joue trame par trame : le resultat ne depend pas de
// la vitesse de la machine.
//
// Ce que fait le compositeur d'un vrai casque : il connait la VRAIE pose au moment de l'affichage
// (reel). Il compare avec la pose que la couche dit avoir utilisee au rendu (soumis), et il
// decale l'image de l'ecart, reel - soumis. L'image corrigee parait donc vue depuis
//     lacet affiche = rendu + (reel - soumis)
// et l'erreur qui reste a l'ecran vaut rendu - soumis (voir l'exercice 1). Pour une rotation
// pure, decaler l'image rendue revient exactement a la rendre depuis le lacet corrige : c'est ce
// que ce programme dessine, en une passe, a la place d'un recalage d'image.
//
// L'IMAGE TEMOIN : un poteau orange a toujours le lacet reel (la direction ou regarde la tete a
// l'affichage), et un reticule blanc est colle a la tete, au centre de l'ecran. Quand l'image est
// juste, le poteau est sous le reticule ; sinon l'ecart entre les deux est l'erreur, mesuree en
// pixels par le programme.
//
// LES DEUX VERSIONS :
//
//   VERSION JUSTE : on garde les vues employees au rendu, et on les resoumet telles quelles.
//   La couche dit d'ou cette image a ete vue.
//
//       couche.vues = vuesDuRendu;                    // la pose et le champ du RENDU
//
//   VERSION FAUTIVE : on redemande les vues juste avant de soumettre, pour l'instant courant.
//   On remplace celles du rendu, et on les demande pour l'instant courant au lieu de l'instant
//   predit. Le code est parfaitement naturel, et il marche sur le simulateur.
//
//       const Vues fraiches = LocaliserLaTete(instantCourant);
//       couche.vues = fraiches;                       // FAUX : la pose d'apres, pas celle du rendu
//
// Le programme joue la version juste. SOUMETTRE_POSE_FRAICHE=1 rejoue la version fautive, celle
// de fraiche.png.
//
// Variables d'environnement (facultatives) :
//     SOUMETTRE_POSE_FRAICHE=1       la version fautive
//     SOUMETTRE_CAPTURE_TRAME=N      la trame a capturer (30 par defaut)
//     SOUMETTRE_CAPTURE=chemin.png   ou ecrire la capture (aucune sinon)
//     SOUMETTRE_TRAMES=N             nombre de trames jouees (40 par defaut)

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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData &d) { d.appName = "Soumettre"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

// ------------------------------------------------------------------------------------------------
// La tete simulee, la couche, et le compositeur.
// ------------------------------------------------------------------------------------------------
const float32 kDegresParTrame = 2.0f;   // la tete tourne de deux degres par trame
const uint32 kTramesDeRetard = 3;       // l'affichage a lieu trois trames apres le rendu
const float32 kChampVertical = 50.0f;   // le champ de vision employe au rendu, en degres

// Les vues d'un rendu : la pose de la tete (ici son lacet, en degres) et le champ de vision.
struct Vues {
    float32 lacet = 0.f;
    float32 champ = kChampVertical;
};

// Une couche de projection, reduite a ce qui compte ici : les vues qu'elle declare avoir utilisees.
struct CoucheProjection {
    Vues vues;
};

// Ou est la tete a l'instant donne (une trame = un pas de deux degres).
Vues LocaliserLaTete(float32 instantEnTrames) {
    Vues v;
    v.lacet = kDegresParTrame * instantEnTrames;
    return v;
}

// Le compositeur : il connait la vraie pose (reel), compare avec celle de la couche (soumis), et
// decale l'image de reel - soumis. Le lacet sous lequel l'image corrigee parait vue :
float32 LacetAffiche(float32 lacetRendu, float32 lacetReel, const CoucheProjection &couche) {
    return lacetRendu + (lacetReel - couche.vues.lacet);
}

// ------------------------------------------------------------------------------------------------
// La scene : un sol, un cercle de poteaux gris, le poteau orange (le lacet reel) et le reticule.
// ------------------------------------------------------------------------------------------------
float32 Radians(float32 degres) { return degres * 3.14159265358979f / 180.f; }

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

// Un point du monde a un lacet et une distance donnes, vus depuis la tete (au centre, a 1,6 m),
// tourne pour faire face a la tete : un poteau vu de face a un contour symetrique.
NkMat4f AuLacet(float32 lacetDegres, float32 distance, float32 hauteur) {
    const float32 a = Radians(lacetDegres);
    return En(distance * std::sin(a), hauteur, -distance * std::cos(a)) *
           NkMat4f::RotationY(NkAngle::FromRad(-a));
}

void Rendre(NkRenderer *renderer, NkICommandBuffer *cmd, float32 aspect, float32 lacetAffiche,
            float32 lacetReel, float32 temps) {
    NkRender3D *r3d = renderer->GetRender3D();
    NkMeshSystem *meshes = renderer->GetMeshSystem();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    if (auto *mc = renderer->GetMaterialCollection()) {
        mc->Upload();
    }

    // La camera est la tete, a 1,6 m, tournee du lacet affiche.
    const float32 a = Radians(lacetAffiche);
    NkCamera3DData cd;
    cd.position = { 0.f, 1.6f, 0.f };
    cd.target = { 5.f * std::sin(a), 1.6f, -5.f * std::cos(a) };
    cd.up = { 0.f, 1.f, 0.f };
    cd.fovY = kChampVertical;
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
        sol.transform = NkMat4f::Scale({ 30.f, 1.f, 30.f });
        sol.aabb = { { -15.f, 0.f, -15.f }, { 15.f, 0.f, 15.f } };
        sol.castShadow = false;
        sol.tint = { 0.42f, 0.40f, 0.37f };
        sol.metallic = 0.f;
        sol.roughness = 1.f;
        r3d->Submit(sol);
    }

    // Quatre murs, loin : un fond, pour que l'horizon ne soit pas noir.
    Boite(r3d, meshes, En(0.f, 3.f, -14.f), { 30.f, 6.f, 0.3f }, { 0.50f, 0.58f, 0.72f });
    Boite(r3d, meshes, En(0.f, 3.f, 14.f), { 30.f, 6.f, 0.3f }, { 0.50f, 0.58f, 0.72f });
    Boite(r3d, meshes, En(-14.f, 3.f, 0.f), { 0.3f, 6.f, 30.f }, { 0.66f, 0.60f, 0.50f });
    Boite(r3d, meshes, En(14.f, 3.f, 0.f), { 0.3f, 6.f, 30.f }, { 0.66f, 0.60f, 0.50f });

    // Les poteaux gris, tous les quinze degres : ils fixent le monde, ils ne bougent pas.
    for (int k = -12; k < 24; ++k) {
        const float32 lacet = 15.f * (float32)k;
        const bool grand = (k % 2) == 0;
        Boite(r3d, meshes, AuLacet(lacet, 4.f, grand ? 1.0f : 0.75f),
              { 0.22f, grand ? 2.0f : 1.5f, 0.22f }, { 0.70f, 0.72f, 0.78f });
    }
    // Le poteau orange : le lacet REEL, la direction ou regarde la tete au moment de l'affichage.
    Boite(r3d, meshes, AuLacet(lacetReel, 4.f, 1.3f), { 0.40f, 2.6f, 0.40f },
          { 0.95f, 0.45f, 0.10f });
    // Le reticule : colle a la tete, il est toujours au centre de l'ecran (a 2,5 m, devant les
    // poteaux).
    Boite(r3d, meshes, AuLacet(lacetAffiche, 2.5f, 1.6f), { 0.03f, 1.8f, 0.03f },
          { 1.0f, 1.0f, 1.0f });

    if (auto *graph = renderer->GetRenderGraph()) {
        graph->Execute(cmd);
    }
}

uint64 EnvU64(const char *nom, uint64 defaut) {
    const char *t = getenv(nom);
    if (t == nullptr || *t == '\0') return defaut;
    return (uint64)strtoull(t, nullptr, 10);
}

// Mesure l'ecart horizontal, en pixels, entre le poteau orange et le centre de l'ecran : le
// centre de gravite des pixels orange, moins la colonne centrale. Rend false si rien d'orange.
bool MesurerLEcart(NkOffscreenTarget *cible, float64 &ecart, uint32 &colonnes) {
    const uint32 l = cible->GetWidth();
    const uint32 h = cible->GetHeight();
    std::vector<uint8> pixels((size_t)l * h * 4, 0);
    if (!cible->ReadbackPixels(pixels.data(), l * 4)) return false;
    float64 somme = 0.0;
    uint64 nombre = 0;
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < l; ++x) {
            const uint8 *p = &pixels[((size_t)y * l + x) * 4];
            // L'orange du poteau, quelle que soit la lumiere : rouge >> vert >> bleu.
            if (p[0] > 60 && p[0] * 10 > p[1] * 18 && p[1] * 2 > p[2] * 3) {
                somme += x;
                ++nombre;
            }
        }
    }
    if (nombre == 0) return false;
    ecart = somme / (float64)nombre - (float64)(l - 1) / 2.0;
    colonnes = l;
    return true;
}

} // namespace

int nkmain(const NkEntryState &state) {
    (void)state;
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    const bool poseFraiche = getenv("SOUMETTRE_POSE_FRAICHE") != nullptr;
    const uint32 nombreTrames = (uint32)EnvU64("SOUMETTRE_TRAMES", 40);
    const uint32 trameCapture = (uint32)EnvU64("SOUMETTRE_CAPTURE_TRAME", 30);
    const char *cheminCapture = getenv("SOUMETTRE_CAPTURE");

    NkWindowConfig cfg;
    cfg.title = "La pose du rendu";
    cfg.width = 640;
    cfg.height = 360;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[Soumettre] la fenetre n'a pas pu s'ouvrir\n");
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
        std::printf("[Soumettre] aucune API graphique disponible\n");
        return 1;
    }

    // Le renderer d'ecran possede l'image ; celui de la scene dessine dans une cible hors ecran
    // qu'on peut relire et capturer.
    NkRenderer *rEcran = NkRenderer::Create(
        device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[Soumettre] le renderer d'ecran n'a pas pu s'initialiser\n");
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
    od.name = "Soumettre_Cible";
    NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
    if (cible == nullptr) {
        std::printf("[Soumettre] cible hors ecran refusee\n");
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
        std::printf("[Soumettre] le renderer de la scene n'a pas pu s'initialiser\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));

    std::printf("[Soumettre] version %s\n", poseFraiche ? "FAUTIVE (pose fraiche)"
                                                         : "JUSTE (pose du rendu)");

    for (uint32 trame = 1; trame <= nombreTrames && fenetre.IsOpen(); ++trame) {
        NkEvent *ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                trame = nombreTrames;   // on s'arrete apres cette trame
            }
        }

        // 1. Le rendu : les vues sont demandees pour l'instant de la trame, et ce sont elles que
        //    le rendu emploie.
        const Vues vuesDuRendu = LocaliserLaTete((float32)trame);

        // 2. La tete bouge pendant le rendu et l'attente : a l'affichage, trois trames plus tard,
        //    elle est ailleurs. C'est la vraie pose, que seul le compositeur connait.
        const Vues vuesReelles = LocaliserLaTete((float32)(trame + kTramesDeRetard));

        // 3. La soumission : la couche declare les vues qu'on a utilisees.
        CoucheProjection couche;
        couche.vues = vuesDuRendu;   // VERSION JUSTE : les vues du rendu, telles quelles
        if (poseFraiche) {
            // VERSION FAUTIVE : on les redemande juste avant de soumettre, pour l'instant courant.
            couche.vues = LocaliserLaTete((float32)(trame + kTramesDeRetard));
        }

        // 4. Ce que fait le compositeur de cette couche.
        const float32 lacetAffiche = LacetAffiche(vuesDuRendu.lacet, vuesReelles.lacet, couche);
        const float32 sans = vuesDuRendu.lacet - vuesReelles.lacet;   // l'erreur sans reprojection
        const float32 avec = vuesDuRendu.lacet - couche.vues.lacet;   // l'erreur qui reste

        if (rEcran->BeginFrame()) {
            NkICommandBuffer *cmd = rEcran->GetCmd();
            Rendre(renderer, cmd, (float32)devInit.width / (float32)devInit.height, lacetAffiche,
                   vuesReelles.lacet, (float32)trame / 72.f);
            if (auto *r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(),
                               { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }

        if (trame % 10 == 0 || trame == trameCapture) {
            std::printf("[Soumettre] trame %2u : rendu %.2f deg, reel %.2f deg, soumis %.2f deg ; "
                        "SANS %.2f AVEC %.2f ; lacet affiche %.2f deg\n",
                        trame, (double)vuesDuRendu.lacet, (double)vuesReelles.lacet,
                        (double)couche.vues.lacet, (double)sans, (double)avec,
                        (double)lacetAffiche);
        }
        if (trame == trameCapture) {
            float64 ecart = 0.0;
            uint32 largeur = 0;
            if (MesurerLEcart(cible, ecart, largeur)) {
                std::printf("[Soumettre] trame %u : le poteau orange est a %.1f pixels du centre "
                            "de l'ecran (largeur %u)\n", trame, ecart, largeur);
            } else {
                std::printf("[Soumettre] trame %u : aucun pixel orange dans l'image\n", trame);
            }
            if (cheminCapture != nullptr && *cheminCapture != '\0') {
                const bool ok = cible->Capture(cheminCapture);
                std::printf("[Soumettre] capture de la trame %u -> %s (%s)\n", trame,
                            cheminCapture, ok ? "ok" : "echec");
            }
        }
    }

    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
