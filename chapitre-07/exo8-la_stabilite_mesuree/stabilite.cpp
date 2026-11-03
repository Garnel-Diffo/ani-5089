// Exercice 8 : la stabilite, mesuree. Le temps de chaque image de la salle (l'intervalle entre deux DEBUTS d'image
// successifs) pendant exactement UNE minute, les trente premieres images laissees de cote, puis le nombre d'images,
// la moyenne, l'ecart-type, le maximum, et le nombre d'images au-dela du budget a 90 hertz (un quatre-vingt-dixieme de
// seconde, 11,111 ms). Deux mesures : la salle telle quelle, qui attend l'ecran avant chaque image, puis sans cette attente.
//
//     screen : N frames, average M ms, standard deviation E ms, max X ms, over budget H
//     no wait : ... (meme forme)
//
// Sans attente : MASALLE_SANS_ATTENTE=1. Sous Vulkan on utiliserait NK_VK_MAILBOX=1, mais cette variable ne marche que sous
// Vulkan ; la salle tourne ici sous DirectX 11 (logs/app.log ecrit api=4, la valeur de NK_GFX_API_DX11 : NONE 0, OPENGL 1, ..., VULKAN 3, DX11 4), ou la meme chose se demande a l'appareil par le champ
// context.dx11.vsync (intervalle de synchronisation 0 au lieu de 1). La salle s'arrete d'elle-meme apres la minute.
#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkKeyboardEvent.h"
#include "NKEvent/NkMouseEvent.h"
#include "NKEvent/NkEventDispatcher.h"

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

#include "NKTime/NkChrono.h"
#include "NKMath/NkQuat.h"
#include "NKMath/NkMat.h"
#include "NKMath/NkAngle.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData &d) { d.appName = "MaSalle"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

	// Les dimensions de la salle, en un seul endroit, en METRES (celles du kit).
	const float32 kPiece = 4.00f;	  // cote interieur du sol
	const float32 kPlafond = 2.50f;	  // hauteur sous plafond
	const float32 kMur = 0.10f;		  // epaisseur d'un mur
	const float32 kPorteL = 0.90f;	  // porte : largeur
	const float32 kPorteH = 2.00f;	  //         hauteur
	const float32 kFenL = 1.20f;	  // fenetre : largeur
	const float32 kFenH = 1.00f;	  //           hauteur
	const float32 kFenY = 1.40f;	  //           hauteur de son centre
	const float32 kTableL = 1.20f;	  // table : longueur
	const float32 kTableP = 0.80f;	  //         profondeur
	const float32 kTableH = 0.75f;	  //         hauteur du plateau
	const float32 kYeux = 1.70f;	  // hauteur des yeux, debout

	const float32 kDemi = kPiece * 0.5f;

	// Un pave pose dans la salle. `centre` est le centre geometrique, `taille`
	// les trois dimensions en metres.
	void Pave(NkRender3D *r3d, NkMeshSystem *meshes, const NkVec3f &centre,
			  const NkVec3f &taille, const NkVec3f &teinte, float32 rugosite = 1.f) {
		NkDrawCall3D dc;
		dc.mesh = meshes->GetCube();
		dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
		dc.aabb = { { centre.x - taille.x * 0.5f, centre.y - taille.y * 0.5f,
					  centre.z - taille.z * 0.5f },
					{ centre.x + taille.x * 0.5f, centre.y + taille.y * 0.5f,
					  centre.z + taille.z * 0.5f } };
		dc.tint = teinte;
		dc.metallic = 0.f;
		dc.roughness = rugosite;
		r3d->Submit(dc);
	}

	void SoumettreLaSalle(NkRender3D *r3d, NkMeshSystem *meshes) {
		const NkVec3f grisMur = { 0.72f, 0.70f, 0.66f };
		const NkVec3f grisSol = { 0.38f, 0.36f, 0.34f };
		const NkVec3f bois = { 0.55f, 0.38f, 0.22f };
		const NkVec3f vitre = { 0.75f, 0.85f, 0.92f };

		{ // le sol : le plan est unitaire, une echelle S donne un cote S
			NkDrawCall3D dc;
			dc.mesh = meshes->GetPlane();
			dc.transform = NkMat4f::Scale({ kPiece, 1.f, kPiece });
			dc.aabb = { { -kDemi, 0.f, -kDemi }, { kDemi, 0.f, kDemi } };
			dc.castShadow = false;
			dc.tint = grisSol;
			dc.metallic = 0.f;
			dc.roughness = 1.f;
			r3d->Submit(dc);
		}

		const float32 yMur = kPlafond * 0.5f;
		const float32 bord = kDemi + kMur * 0.5f;

		// Quatre murs. La porte est dans celui du fond, les fenetres a gauche.
		Pave(r3d, meshes, { 0.f, yMur, -bord }, { kPiece, kPlafond, kMur }, grisMur);
		Pave(r3d, meshes, { 0.f, yMur, bord }, { kPiece, kPlafond, kMur }, grisMur);
		Pave(r3d, meshes, { -bord, yMur, 0.f }, { kMur, kPlafond, kPiece }, grisMur);
		Pave(r3d, meshes, { bord, yMur, 0.f }, { kMur, kPlafond, kPiece }, grisMur);

		// Le plafond.
		Pave(r3d, meshes, { 0.f, kPlafond + kMur * 0.5f, 0.f },
			 { kPiece, kMur, kPiece }, grisMur);

		// La porte, fermee : un panneau plaque contre le mur du fond.
		Pave(r3d, meshes, { -0.70f, kPorteH * 0.5f, -kDemi + 0.02f },
			 { kPorteL, kPorteH, 0.04f }, bois, 0.6f);

		// Deux fenetres sur le mur de gauche.
		Pave(r3d, meshes, { -kDemi + 0.02f, kFenY, -0.90f },
			 { 0.04f, kFenH, kFenL }, vitre, 0.15f);
		Pave(r3d, meshes, { -kDemi + 0.02f, kFenY, 0.90f },
			 { 0.04f, kFenH, kFenL }, vitre, 0.15f);

		// La table : un plateau et quatre pieds.
		const float32 tx = 0.60f, tz = 0.70f;
		const float32 ep = 0.05f;
		Pave(r3d, meshes, { tx, kTableH - ep * 0.5f, tz },
			 { kTableL, ep, kTableP }, bois, 0.5f);

		const float32 pied = 0.06f;
		const float32 hp = kTableH - ep;
		for (int i = 0; i < 4; ++i) {
			const float32 dx = ((i & 1) ? 1.f : -1.f) * (kTableL * 0.5f - pied);
			const float32 dz = ((i & 2) ? 1.f : -1.f) * (kTableP * 0.5f - pied);
			Pave(r3d, meshes, { tx + dx, hp * 0.5f, tz + dz },
				 { pied, hp, pied }, bois, 0.6f);
		}

		// Le repere d'un metre : un cube d'un metre de cote, pose au sol.
		Pave(r3d, meshes, { -1.10f, 0.50f, 1.20f }, { 1.f, 1.f, 1.f },
			 { 0.85f, 0.80f, 0.30f }, 0.8f);
	}

	uint64 EnvU64(const char *nom, uint64 defaut) {
		const char *t = getenv(nom);
		if (t == nullptr || *t == '\0') return defaut;
		return (uint64)strtoull(t, nullptr, 10);
	}

	float32 EnvFloat(const char *nom, float32 defaut) {
		const char *t = getenv(nom);
		if (t == nullptr || *t == '\0') return defaut;
		return (float32)strtod(t, nullptr);
	}

	// Une camera a hauteur d'yeux : position, direction du regard et haut.
	NkCamera3DData CameraDe(const NkVec3f &position, const NkVec3f &regard, const NkVec3f &haut,
							float32 aspect) {
		NkCamera3DData cd;
		cd.position = position;
		cd.target = { position.x + regard.x, position.y + regard.y, position.z + regard.z };
		cd.up = haut;
		cd.fovY = 70.f;
		cd.aspect = aspect;
		cd.nearPlane = 0.05f;
		cd.farPlane = 100.f;
		return cd;
	}

	// Ce que BeginFrame ferait si ce renderer possedait l'image : en mode partage on le
	// rejoue a la main (oublier ResetFrame pietine les tampons uniformes d'une image a
	// l'autre), puis on soumet la salle et on execute le graphe sur le command buffer de l'ecran.
	void RendreLaSalle(NkRenderer *renderer, NkICommandBuffer *cmd, const NkCamera3DData &cd,
					   float32 temps) {
		NkRender3D *r3d = renderer->GetRender3D();
		renderer->FlushGraphRebuilds();
		r3d->ResetFrame();
		if (auto *mc = renderer->GetMaterialCollection()) {
			mc->Upload();
		}

		NkSceneContext sctx;
		sctx.camera = NkCamera3D(cd);
		sctx.time = temps;
		r3d->SetIBLStrength(0.35f);

		NkLightDesc soleil;
		soleil.type = NkLightType::NK_DIRECTIONAL;
		soleil.direction = { -0.5f, -1.f, -0.35f };
		soleil.color = { 1.f, 0.96f, 0.88f };
		soleil.intensity = 3.f;
		soleil.castShadow = false;
		sctx.lights.PushBack(soleil);

		r3d->BeginScene(sctx);
		SoumettreLaSalle(r3d, renderer->GetMeshSystem());

		if (auto *graph = renderer->GetRenderGraph()) {
			graph->Execute(cmd);
		}
	}

} // namespace


namespace {

	// Le budget d'une image a 90 hertz, en millisecondes : un divise par la cadence.
	const float64 kBudgetMs = 1000.0 / 90.0;

	// L'instant present, en nanosecondes, sur une horloge qui ne recule jamais (float64 : un float32 ne garde que sept chiffres).
	float64 Maintenant() { return NkChrono::Now().nanoseconds; }

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	const bool sansAttente = getenv("MASALLE_SANS_ATTENTE") != nullptr;

	// La taille de la fenetre : 640 x 360 par defaut. A 1280 x 720, la salle du kit met de 40 a 100 ms par image sur cette
	// machine (une carte Intel HD 5500, bien au-dela d'une periode d'ecran de 16,7 ms) ; a 640 x 360 elle se cale sur
	// l'ecran, ce que les mesures de ce sprint supposent. MASALLE_LARGEUR et MASALLE_HAUTEUR redonnent la taille du kit.
	NkWindowConfig cfg;
	cfg.title = "Ma salle";
	cfg.width = (uint32)EnvU64("MASALLE_LARGEUR", 640);
	cfg.height = (uint32)EnvU64("MASALLE_HAUTEUR", 360);
	cfg.centered = true;
	cfg.vsync = true;

	NkWindow fenetre(cfg);
	if (!fenetre.IsValid()) {
		std::printf("[MaSalle] la fenetre n'a pas pu s'ouvrir\n");
		return 1;
	}
	NkDeviceInitInfo devInit{};
	devInit.surface = fenetre.GetSurfaceDesc();
	devInit.width = fenetre.GetSize().x;
	devInit.height = fenetre.GetSize().y;

	// CreateAutoDetect choisit DirectX 12 sur cette machine, et ses nuanceurs n'y compilent pas
	// (ps_5_1 : maximum sampler register index (16) exceeded) : on demande DirectX 11.
	devInit.api = NkGraphicsApi::NK_GFX_API_DX11;
	// Sans attente de l'ecran : intervalle de synchronisation 0 sous DirectX 11 (NK_VK_MAILBOX ne vaut que sous Vulkan).
	if (sansAttente) {
		devInit.context.dx11.vsync = false;
	}
	NkIDevice *device = NkDeviceFactory::CreateForApi(devInit.api, devInit);
	if (device == nullptr) {
		std::printf("[MaSalle] aucune API graphique disponible\n");
		return 1;
	}

	// Deux renderers : le premier possede l'image et ne dessine que du 2D, le second dessine
	// la salle dans une cible hors ecran qu'on peut relire et enregistrer.
	NkRendererConfig cfgEcran =
		NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
	NkRenderer *rEcran = NkRenderer::Create(device, cfgEcran);
	if (rEcran == nullptr || !rEcran->Initialize()) {
		std::printf("[MaSalle] le renderer d'ecran n'a pas pu s'initialiser\n");
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
	od.readback = true; // sans quoi Capture() ne peut rien relire
	od.name = "MaSalle_Cible";

	NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
	if (cible == nullptr) {
		std::printf("[MaSalle] cible hors ecran refusee\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	NkRendererConfig cfgSalle =
		NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
	cfgSalle.Disable(NK_SS_RENDER2D);
	cfgSalle.Disable(NK_SS_TEXT);
	cfgSalle.Disable(NK_SS_OVERLAY);
	cfgSalle.Enable(NK_SS_OFFSCREEN);

	NkRenderer *renderer = NkRenderer::Create(device, cfgSalle);
	if (renderer == nullptr || !renderer->Initialize()) {
		std::printf("[MaSalle] le renderer de la salle n'a pas pu s'initialiser\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}
	renderer->SetFinalColorTarget(
		rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));


	const uint64 imagesLaissees = 30;		 // elles paient le demarrage : la toute premiere peut durer plusieurs secondes
	const float64 minute = 60.0e9;			 // exactement une minute, en nanosecondes
	std::vector<float64> temps;				 // le temps de chaque image mesuree, en millisecondes
	temps.reserve(8192);
	float64 debutMesure = 0.0, instantAvant = 0.0, dernierInstant = 0.0;
	uint64 image = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
		// Le debut de l'image : c'est ici qu'on note l'instant.
		const float64 instant = Maintenant();
		if (image == imagesLaissees) {
			debutMesure = instant; // la minute commence au debut de l'image 30
		} else if (image > imagesLaissees) {
			temps.push_back((instant - instantAvant) * 1e-6); // l'image d'avant a dure ca
			dernierInstant = instant;
			if (instant - debutMesure >= minute) {
				tourne = false; // la minute est ecoulee : on arrete la salle
			}
		}
		instantAvant = instant;
		if (!tourne) break;

		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			const NkCamera3DData cd = CameraDe({ 0.30f, kYeux, 1.20f }, { -1.f, -0.6f, -3.2f }, { 0.f, 1.f, 0.f },
											   (float32)devInit.width / (float32)devInit.height);
			RendreLaSalle(renderer, cmd, cd, (float32)image / 60.f);

			if (auto *r2d = rEcran->GetRender2D()) {
				r2d->DrawImage(cible->GetColorResult(),
							   { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
			}
			rEcran->Present();
			rEcran->EndFrame();
		}
		++image;
	}

	// ---- La mesure : nombre d'images, moyenne, ecart-type (racine de la moyenne des carres des ecarts a la moyenne), max ----
	if (!temps.empty()) {
		float64 somme = 0.0, maximum = 0.0;
		size_t auDela = 0;
		for (float64 t : temps) {
			somme += t;
			maximum = std::max(maximum, t);
			if (t > kBudgetMs) ++auDela;
		}
		const float64 moyenne = somme / (float64)temps.size();
		float64 carres = 0.0;
		for (float64 t : temps) carres += (t - moyenne) * (t - moyenne);
		const float64 ecartType = std::sqrt(carres / (float64)temps.size());
		std::printf("%s : %zu frames, average %.3f ms, standard deviation %.3f ms, max %.3f ms, over budget %zu\n",
					sansAttente ? "no wait" : "screen", temps.size(), moyenne, ecartType, maximum, auDela);
		// Pour la reponse : la duree reellement mesuree, et le budget (90 Hz).
		std::printf("[MaSalle] duree mesuree : %.3f s ; budget a 90 Hz : %.3f ms\n", (dernierInstant - debutMesure) * 1e-9, kBudgetMs);
		// Pour la reponse : la moyenne par tranche de dix secondes, pour voir si la salle ralentit au fil de la minute.
		std::printf("[MaSalle] moyenne par tranche de 10 s :");
		float64 cumul = 0.0, sommeTranche = 0.0;
		size_t nombreTranche = 0;
		for (float64 t : temps) {
			cumul += t;
			sommeTranche += t;
			++nombreTranche;
			if (cumul >= 10000.0) {
				std::printf(" %.2f", sommeTranche / (float64)nombreTranche);
				cumul = 0.0;
				sommeTranche = 0.0;
				nombreTranche = 0;
			}
		}
		std::printf(" ms\n");
	} else {
		std::printf("[MaSalle] aucune image mesuree\n");
	}

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
