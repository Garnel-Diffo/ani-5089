// Exercice 9 : le verrou de demi-cadence. La salle est rendue plus lourde jusqu'a ne plus tenir la cadence de l'ecran : elle est
// soumise plusieurs fois, chaque copie decalee de 5 metres le long de x. Puis on vise EXPRES la moitie de la cadence : a la
// fin de chaque image, le programme attend que DEUX periodes d'ecran se soient ecoulees depuis le debut de cette image
// (le verrou). On compare la regularite des deux : mille intervalles entre deux debuts d'image, sans verrou puis avec.
//
// La decision du moteur, dans un de ses commentaires : "Verrou demi-cadence, 36 images par seconde pour un Quest a 72 Hz :
// mesure 39 a 51 images par seconde instables." Et sa raison : "Un rythme stable a 72 divise par 2 vaut mieux qu'un rythme
// oscillant plus haut : l'ASW se cale et lisse." Ici on le verifie sur l'ecran de cette machine, qui tourne a 60 hertz :
// la periode est un soixantieme de seconde, deux periodes valent 33,3 ms.
//
//     MASALLE_COPIES=<K> : le nombre de copies de la salle (1 : la salle seule) ;
//     MASALLE_VERROU=1   : active le verrou (attendre deux periodes depuis le debut de l'image) ;
//     MASALLE_EXIT=1040  : arrete la salle d'elle-meme (30 images laissees de cote, 1000 mesurees, une marge).
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

	// Le decalage, en metres le long de x, de la copie de la salle en cours de soumission (exercice 9). 0 pour la salle d'origine.
	float32 gDecalageX = 0.f;

	// Un pave pose dans la salle. `centre` est le centre geometrique, `taille`
	// les trois dimensions en metres.
	void Pave(NkRender3D *r3d, NkMeshSystem *meshes, const NkVec3f &centre,
			  const NkVec3f &taille, const NkVec3f &teinte, float32 rugosite = 1.f) {
		NkDrawCall3D dc;
		dc.mesh = meshes->GetCube();
		dc.transform = NkMat4f::Translate({ centre.x + gDecalageX, centre.y, centre.z }) * NkMat4f::Scale(taille);
		dc.aabb = { { centre.x + gDecalageX - taille.x * 0.5f, centre.y - taille.y * 0.5f,
					  centre.z - taille.z * 0.5f },
					{ centre.x + gDecalageX + taille.x * 0.5f, centre.y + taille.y * 0.5f,
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
			dc.transform = NkMat4f::Translate({ gDecalageX, 0.f, 0.f }) * NkMat4f::Scale({ kPiece, 1.f, kPiece });
			dc.aabb = { { gDecalageX - kDemi, 0.f, -kDemi }, { gDecalageX + kDemi, 0.f, kDemi } };
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
					   float32 temps, int copies = 1) {
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
		// La salle, puis (exercice 9) des copies decalees de 5 metres le long de x : plus de copies, plus de travail.
		for (int i = 0; i < copies; ++i) {
			gDecalageX = 5.f * (float32)i;
			SoumettreLaSalle(r3d, renderer->GetMeshSystem());
		}
		gDecalageX = 0.f;

		if (auto *graph = renderer->GetRenderGraph()) {
			graph->Execute(cmd);
		}
	}

} // namespace


namespace {

	// La periode de l'ecran, en nanosecondes : un divise par sa cadence, 60 hertz (les parametres d'affichage de Windows).
	const float64 kPeriodeEcran = 1e9 / 60.0;

	// L'instant present, en nanosecondes, sur une horloge qui ne recule jamais (float64 : un float32 ne garde que sept chiffres).
	float64 Maintenant() { return NkChrono::Now().nanoseconds; }

	// Attendre jusqu'a un instant donne. Le commentaire de NkChrono::SleepMilliseconds le dit : sans minuterie fine (voir
	// BeginPreciseTiming dans nkmain), un sommeil de 1 a 12 ms dure environ 15,5 ms quoi qu'on lui demande, et une attente
	// cadencee par un sommeil n'est pas cadencee, elle est quantifiee. Avec la minuterie fine, 1 ms dure de 1,9 a 2 ms. Donc :
	// on dort par pas de 1 ms tant qu'il reste plus de 3 ms, puis on tourne en cedant la main (YieldThread) pour la fin :
	// la precision de la sortie est de l'ordre de la dizaine de microsecondes, pour un cout de CPU limite a ces 3 dernieres ms.
	void AttendreJusqua(float64 instantCible) {
		for (;;) {
			const float64 reste = instantCible - Maintenant();
			if (reste <= 0.0) return;
			if (reste > 3e6) {
				NkChrono::SleepMilliseconds(1);
			} else {
				NkChrono::YieldThread();
			}
		}
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	const int copies = (int)EnvU64("MASALLE_COPIES", 1);
	const bool verrou = getenv("MASALLE_VERROU") != nullptr;

	// La minuterie fine, pour que les sommeils courts durent ce qu'on leur demande. A APPARIER avec EndPreciseTiming.
	NkChrono::BeginPreciseTiming();

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


	// On regarde le long de x : toutes les copies sont dans le champ, aucune n'est ecartee par le test de visibilite.
	const NkVec3f position = { 0.30f, kYeux, 1.20f };
	const NkVec3f regard = { 1.f, -0.05f, 0.f };

	const uint64 imageDeSortie = EnvU64("MASALLE_EXIT", 0);
	const uint64 imagesLaissees = 30;		 // elles paient le demarrage
	const uint64 imagesMesurees = 1000;
	std::vector<float64> debuts;			 // le debut de chaque image, en nanosecondes
	debuts.reserve((size_t)imageDeSortie + 2);
	uint64 image = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
		// Le debut de l'image : c'est ici qu'on note l'instant, avant tout travail.
		const float64 debut = Maintenant();
		debuts.push_back(debut);

		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			NkCamera3DData cd = CameraDe(position, regard, { 0.f, 1.f, 0.f },
										 (float32)devInit.width / (float32)devInit.height);
			// La derniere copie est a 5 m * K : on repousse le plan lointain (100 m par defaut) pour qu'aucune ne soit coupee.
			cd.farPlane = 100.f + 5.f * (float32)copies;
			RendreLaSalle(renderer, cmd, cd, (float32)image / 60.f, copies);

			if (auto *r2d = rEcran->GetRender2D()) {
				r2d->DrawImage(cible->GetColorResult(),
							   { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
			}
			rEcran->Present();
			rEcran->EndFrame();
		}
		++image;

		// Le verrou : a la FIN de l'image, on attend que deux periodes d'ecran se soient ecoulees depuis son DEBUT. Si le
		// travail a deja pris plus de temps, il n'y a rien a attendre.
		if (verrou) {
			AttendreJusqua(debut + 2.0 * kPeriodeEcran);
		}

		if (imageDeSortie != 0 && image >= imageDeSortie) {
			tourne = false;
		}
	}

	// ---- La mesure : l'intervalle entre deux debuts d'image successifs, mille fois, les trente premieres images laissees ----
	if (debuts.size() >= (size_t)(imagesLaissees + imagesMesurees + 1)) {
		std::vector<float64> temps;
		temps.reserve((size_t)imagesMesurees);
		for (size_t i = (size_t)imagesLaissees; i < (size_t)(imagesLaissees + imagesMesurees); ++i) {
			temps.push_back((debuts[i + 1] - debuts[i]) * 1e-6); // nanosecondes -> millisecondes
		}
		float64 somme = 0.0, maximum = 0.0, minimum = 1e30;
		for (float64 t : temps) {
			somme += t;
			maximum = std::max(maximum, t);
			minimum = std::min(minimum, t);
		}
		const float64 moyenne = somme / (float64)temps.size();
		float64 carres = 0.0;
		for (float64 t : temps) carres += (t - moyenne) * (t - moyenne);
		std::printf("room copies : %d\n", copies);
		std::printf("%s : average %.3f ms, standard deviation %.3f ms, max %.3f ms\n", verrou ? "with lock" : "without lock", moyenne,
					std::sqrt(carres / (float64)temps.size()), maximum);
		// Pour la reponse : le minimum, la periode de l'ecran, et ce que valent les images en images par seconde.
		std::printf("[MaSalle] min %.3f ms ; periode de l'ecran %.3f ms ; deux periodes %.3f ms ; cadence moyenne %.2f images par seconde\n",
					minimum, kPeriodeEcran * 1e-6, 2.0 * kPeriodeEcran * 1e-6, 1000.0 / moyenne);
	} else {
		std::printf("[MaSalle] trop peu d'images pour mesurer (%zu) : lancer avec MASALLE_EXIT=1040\n", debuts.size());
	}

	NkChrono::EndPreciseTiming();

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
