// Exercice 10 : la boucle qui ne reprend jamais.
//
// On oublie la fin de trame quand le casque est pose, on constate que la boucle ne reprend pas,
// puis on corrige. Sauter la fin de trame quand le casque est pose ne coute rien sur le moment :
// cela vous empeche de reprendre quand on le remet, et aucun message ne le dira.
//
// LA BOUCLE COMPLETE, dans l'ordre, six gestes :
//     1. la pompe d'evenements ;
//     2. l'attente de trame  (WaitFrame) ;
//     3. le debut de trame   (BeginFrame) ;
//     4. la synchronisation des actions ;
//     5. le rendu CONDITIONNEL (s'il faut dessiner) : localisation des vues, acquisition, rendu,
//        liberation, et remplissage de la couche avec les poses du rendu et la derniere image
//        relachee ;
//     6. la fin de trame     (EndFrame), TOUJOURS : hors du test de rendu, avec zero couche s'il
//        n'y a rien eu a dessiner.
//
// Ce qui est simule ici, et pourquoi : le kit n'a pas de module XR. La session (ses trois gestes
// et leur ordre), la pause du casque et la gachette sont donc ecrites dans ce programme, comme le
// simulateur du chapitre : il ne compose rien, il valide. Une trame commencee et jamais finie
// laisse l'attente suivante sans reponse : une vraie session bloquerait la boucle a cet endroit,
// et elle ne reprendrait jamais ; ici, pour pouvoir relever vingt trames, l'attente est refusee
// (et comptee par la session) au lieu de bloquer. La boucle ne lit aucun code de retour, comme
// beaucoup de boucles : elle ne voit donc rien, garde l'ancien etat de trame et continue de
// tourner. Le moteur dessine la scene dans une cible hors ecran qu'on capture en PNG.
//
// LA PAUSE : shouldRender vaut faux de la trame 6 a la trame 10 (cinq trames), puis revient a vrai.
//
// LES DEUX VERSIONS (le code ci-dessous contient les deux ; une seule est compilee) :
//
//   VERSION FAUTIVE : l'appel de fin de trame est a l'INTERIEUR du test de rendu.
//
//       if (etat.shouldRender) {
//           ... localisation, acquisition, rendu, liberation, couche ...
//           session.EndFrame(etat.displayTime, &couche, 1);   // FAUX : jamais appelee en pause
//       }
//
//   VERSION JUSTE : la fin de trame sort du test, et elle est appelee a chaque trame.
//
//       if (etat.shouldRender) {
//           ... localisation, acquisition, rendu, liberation, couche ...
//       }
//       session.EndFrame(etat.displayTime, nombre > 0 ? &couche : nullptr, nombre);   // TOUJOURS
//
// Compilation normale : la version juste (c'est elle qu'on rend). Compilation avec
// -DBOUCLE_FAUTIVE : la version fautive, celle de journal.txt et de avant.png.
//
// Sorties : journal.txt (une ligne par trame, `trame : 7, rend : 0, debut : 1, fin : 0`) et, si on
// le demande, la capture. Dans le journal, `rend` vaut 1 si la trame a ete dessinee, `debut` et
// `fin` valent 1 si la boucle a appele BeginFrame et EndFrame a cette trame.
//
// Variables d'environnement (facultatives) :
//     BOUCLE_CAPTURE_TRAME=N         la trame a capturer (12 par defaut)
//     BOUCLE_CAPTURE=chemin.png      ou ecrire la capture (aucune sinon)
//     BOUCLE_JOURNAL=chemin.txt      ou ecrire le journal (journal.txt par defaut)
//     BOUCLE_TRAMES=N                nombre de trames jouees (20 par defaut)

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

static void ConfigureAppData(NkAppData &d) { d.appName = "Boucle"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

const uint32 kTramesDePause = 5;      // shouldRender vaut faux pendant cinq trames ...
const uint32 kPremiereDePause = 6;    // ... de la trame 6 a la trame 10
const uint32 kImages = 3;             // l'anneau d'images : comme s'il y en avait plusieurs
const int64 kPeriodeMicro = 13889;    // une trame a 72 hertz, en microsecondes

// ------------------------------------------------------------------------------------------------
// La session simulee : trois etats, trois gestes, et le refus des gestes dans le desordre.
// ------------------------------------------------------------------------------------------------
struct EtatDeTrame {
    bool shouldRender = false;   // le casque est-il porte, faut-il dessiner ?
    int64 displayTime = 0;       // l'instant d'affichage PREDIT pour CETTE trame
};

struct CoucheProjection {
    uint32 indexImage = 0;       // la derniere image relachee
    float32 poseLacet = 0.f;     // la pose du RENDU
};

class SessionSimulee {
public:
    // L'attente : refusee tant que la trame precedente est ouverte (commencee, jamais finie).
    bool WaitFrame(uint32 trame, EtatDeTrame &etat) {
        if (mOuverte) {
            ++mRefus;       // une vraie session bloquerait ici, et la boucle ne reprendrait jamais
            return false;   // etat n'est pas touche : l'appelant garde l'ancien
        }
        const bool enPause = trame >= kPremiereDePause && trame < kPremiereDePause + kTramesDePause;
        etat.shouldRender = !enPause;
        etat.displayTime = (int64)trame * kPeriodeMicro;
        return true;
    }

    bool BeginFrame() {
        if (mOuverte) {
            ++mRefus;
            return false;
        }
        mOuverte = true;
        return true;
    }

    // La fin de trame : on y soumet les couches, zero ou plus.
    bool EndFrame(int64 displayTime, const CoucheProjection *couches, int nombre) {
        (void)displayTime;
        (void)couches;
        (void)nombre;
        if (!mOuverte) {
            ++mRefus;
            return false;
        }
        mOuverte = false;
        return true;
    }

    int Refus() const { return mRefus; }   // les gestes refuses, que personne n'annonce a la boucle

private:
    bool mOuverte = false;   // une trame est commencee et pas encore finie
    int mRefus = 0;
};

// ------------------------------------------------------------------------------------------------
// Les actions : l'etat d'une action booleenne, calcule une fois par trame (geste 4).
// ------------------------------------------------------------------------------------------------
struct EtatAction {
    bool courant = false;   // current
    bool change = false;    // changed
    bool actif = false;     // active
};

struct Action {
    bool sourceLiee = true;   // une source materielle est liee
    bool presse = false;      // l'etat de la source
    EtatAction etat;
    bool precedent = false;
};

void Synchronise(Action &action) {
    action.etat.actif = action.sourceLiee;
    action.etat.courant = action.etat.actif && action.presse;
    action.etat.change = (action.etat.courant != action.precedent);
    action.precedent = action.etat.courant;
}

// L'anneau d'images : l'index n'avance que quand on RELACHE une image rendue.
struct Anneau {
    uint32 suivant = 0;
    uint32 Acquiert() const { return suivant; }
    void Relache() { suivant = (suivant + 1) % kImages; }
};

// ------------------------------------------------------------------------------------------------
// La scene : un cube dont l'angle vaut huit degres fois le numero de la trame, et une rangee de
// vingt temoins, dont les `trame` premiers sont verts. Un ecran fige se reconnait a ce qu'il ne
// change plus.
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

void RendreLaScene(NkRenderer *renderer, NkICommandBuffer *cmd, float32 aspect, uint32 trame,
                   bool gachette, uint32 trames) {
    NkRender3D *r3d = renderer->GetRender3D();
    NkMeshSystem *meshes = renderer->GetMeshSystem();
    renderer->FlushGraphRebuilds();
    r3d->ResetFrame();
    if (auto *mc = renderer->GetMaterialCollection()) {
        mc->Upload();
    }

    NkCamera3DData cd;
    cd.position = { 0.f, 1.7f, 3.4f };
    cd.target = { 0.f, 0.7f, 0.f };
    cd.up = { 0.f, 1.f, 0.f };
    cd.fovY = 50.f;
    cd.aspect = aspect;
    cd.nearPlane = 0.05f;
    cd.farPlane = 100.f;

    NkSceneContext sctx;
    sctx.camera = NkCamera3D(cd);
    sctx.time = (float32)trame / 72.f;
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
        sol.transform = NkMat4f::Scale({ 12.f, 1.f, 12.f });
        sol.aabb = { { -6.f, 0.f, -6.f }, { 6.f, 0.f, 6.f } };
        sol.castShadow = false;
        sol.tint = { 0.42f, 0.40f, 0.37f };
        sol.metallic = 0.f;
        sol.roughness = 1.f;
        r3d->Submit(sol);
    }
    Boite(r3d, meshes, En(0.f, 1.5f, -2.2f), { 9.f, 3.f, 0.2f }, { 0.56f, 0.64f, 0.78f });

    // Le cube : huit degres par numero de trame ; orange tant que la gachette est enfoncee.
    const NkVec3f orange{ 0.95f, 0.45f, 0.10f };
    const NkVec3f bleu{ 0.20f, 0.50f, 0.95f };
    const NkVec3f teinte = gachette ? orange : bleu;
    Boite(r3d, meshes, En(0.f, 0.9f, 0.f) *
          NkMat4f::RotationY(NkAngle::FromRad(0.13962634f * (float32)trame)),
          { 0.7f, 0.7f, 0.7f }, teinte);

    // La rangee de temoins : un par trame, verts jusqu'a la trame rendue.
    for (uint32 i = 1; i <= trames; ++i) {
        const float32 x = -1.9f + 0.2f * (float32)(i - 1);
        const NkVec3f vert{ 0.25f, 0.80f, 0.30f };
        const NkVec3f gris{ 0.25f, 0.25f, 0.28f };
        Boite(r3d, meshes, En(x, 0.12f, 1.2f), { 0.15f, 0.15f, 0.15f }, i <= trame ? vert : gris);
    }

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

    const uint32 nombreTrames = (uint32)EnvU64("BOUCLE_TRAMES", 20);
    const uint32 trameCapture = (uint32)EnvU64("BOUCLE_CAPTURE_TRAME", 12);
    const char *cheminCapture = getenv("BOUCLE_CAPTURE");
    const char *cheminJournal = getenv("BOUCLE_JOURNAL");
    if (cheminJournal == nullptr || *cheminJournal == '\0') cheminJournal = "journal.txt";

    NkWindowConfig cfg;
    cfg.title = "La boucle qui ne reprend jamais";
    cfg.width = 640;
    cfg.height = 360;
    cfg.centered = true;
    cfg.vsync = true;
    NkWindow fenetre(cfg);
    if (!fenetre.IsValid()) {
        std::printf("[Boucle] la fenetre n'a pas pu s'ouvrir\n");
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
        std::printf("[Boucle] aucune API graphique disponible\n");
        return 1;
    }

    // Le renderer d'ecran possede l'image ; celui de la scene dessine dans une cible hors ecran
    // qu'on peut relire et capturer. Une seule cible reelle : le code est ecrit comme s'il y avait
    // plusieurs images (l'anneau), mais il n'y en a qu'une a dessiner.
    NkRenderer *rEcran = NkRenderer::Create(
        device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (rEcran == nullptr || !rEcran->Initialize()) {
        std::printf("[Boucle] le renderer d'ecran n'a pas pu s'initialiser\n");
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
    od.name = "Boucle_Cible";
    NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
    if (cible == nullptr) {
        std::printf("[Boucle] cible hors ecran refusee\n");
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
        std::printf("[Boucle] le renderer de la scene n'a pas pu s'initialiser\n");
        NkRenderer::Destroy(rEcran);
        NkDeviceFactory::Destroy(device);
        return 1;
    }
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));

    // Le journal : une ligne par trame. Ouvert en binaire : un seul octet de fin de ligne.
    std::FILE *journal = std::fopen(cheminJournal, "wb");

    SessionSimulee session;
    Anneau anneau;
    Action saisir;   // l'action "saisir", liee a une gachette scriptee (enfoncee aux trames 3 et 4)
    EtatDeTrame etat;   // il survit d'une trame a l'autre : si l'attente echoue, on garde l'ancien
    int rendues = 0;
    int fins = 0;

#ifdef BOUCLE_FAUTIVE
    std::printf("[Boucle] version FAUTIVE (fin de trame dans le test de rendu)\n");
#else
    std::printf("[Boucle] version JUSTE (fin de trame toujours appelee)\n");
#endif

    for (uint32 trame = 1; trame <= nombreTrames && fenetre.IsOpen(); ++trame) {
        int rend = 0;
        int debut = 0;
        int fin = 0;

        // 1. La pompe d'evenements.
        NkEvent *ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                trame = nombreTrames;   // on s'arrete apres cette trame
            }
        }
        // L'image de la fenetre : on la presente a chaque tour, avec ce que la cible contient
        // (la derniere image rendue : un ecran fige si plus rien n'est dessine).
        const bool fenetreOk = rEcran->BeginFrame();
        NkICommandBuffer *cmd = fenetreOk ? rEcran->GetCmd() : nullptr;

        // 2. L'attente de trame. Le code de retour n'est lu par personne, comme dans beaucoup de
        //    boucles : un refus ne laisse aucune trace, et etat garde sa derniere valeur.
        session.WaitFrame(trame, etat);

        // 3. Le debut de trame.
        session.BeginFrame();
        debut = 1;

        // 4. La synchronisation des actions : une fois par trame, puis on interroge.
        saisir.presse = (trame == 3 || trame == 4);
        Synchronise(saisir);

        // 5. Le rendu conditionnel.
        CoucheProjection couche;
        int nombre = 0;
        if (etat.shouldRender && cmd != nullptr) {
            const float32 pose = 2.0f * (float32)trame;      // localisation des vues
            const uint32 image = anneau.Acquiert();          // acquisition
            RendreLaScene(renderer, cmd, (float32)devInit.width / (float32)devInit.height,
                          trame, saisir.etat.courant, nombreTrames);   // rendu
            anneau.Relache();                                // liberation
            couche.indexImage = image;                       // la derniere image relachee
            couche.poseLacet = pose;                         // la pose du RENDU
            nombre = 1;
            rend = 1;
            ++rendues;
#ifdef BOUCLE_FAUTIVE
            // FAUTE : la fin de trame est A L'INTERIEUR du test de rendu.
            fin = session.EndFrame(etat.displayTime, &couche, nombre) ? 1 : 0;
            fins += fin;
#endif
        }

#ifndef BOUCLE_FAUTIVE
        // 6. La fin de trame : TOUJOURS, hors du test de rendu, meme quand on n'a rien dessine
        //    (zero couche).
        fin = session.EndFrame(etat.displayTime, nombre > 0 ? &couche : nullptr, nombre) ? 1 : 0;
        fins += fin;
#endif

        if (fenetreOk) {
            if (auto *r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(),
                               { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }

        if (journal != nullptr) {
            std::fprintf(journal, "trame : %u, rend : %d, debut : %d, fin : %d\n", trame, rend,
                         debut, fin);
        }
        std::printf("[Boucle] trame %2u : rend %d, debut %d, fin %d\n", trame, rend, debut, fin);

        if (cheminCapture != nullptr && *cheminCapture != '\0' && trame == trameCapture) {
            const bool ok = cible->Capture(cheminCapture);
            std::printf("[Boucle] capture de la trame %u -> %s (%s)\n", trame, cheminCapture,
                        ok ? "ok" : "echec");
        }
    }

    std::printf("[Boucle] %d trames rendues, %d fins de trame\n", rendues, fins);
    std::printf("[Session simulee] gestes refuses : %d (la boucle n'en lit aucun)\n",
                session.Refus());
    if (journal != nullptr) std::fclose(journal);
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
