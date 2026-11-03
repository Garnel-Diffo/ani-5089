// Exercice 3 : mesurer une vitesse. La tete tourne soit avec la souris, soit toute seule (un virage programme), et le
// programme calcule la vitesse angulaire entre deux poses datees : la pose de l'image d'avant et celle de cette image,
// separees par le temps reel qui les separe. Vingt vitesses successives, une ligne par image :
//     frame N : dt = D ms, speed = (x, y, z) deg/s
//
// MASALLE_TOUR=<degres par seconde> : le lacet suit l'HORLOGE et non la souris (virage programme).
// Sans cette variable, la tete suit la souris : les mouvements bruts sont ajoutes dans un accumulateur a chaque evenement,
// l'accumulateur est lu et remis a zero UNE fois par image (voir l'exercice 3 du chapitre 6).
// MASALLE_EXIT=<image> arrete la salle d'elle-meme.
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

	// La vitesse angulaire entre deux poses datees, telle que le module de realite virtuelle du moteur l'ecrit.
	NkVec3f VitesseAngulaire(const NkQuatf &from, const NkQuatf &to, float32 dtSeconds) {
		if (dtSeconds <= 0.f) {
			return NkVec3f(0.f, 0.f, 0.f);
		}
		NkQuatf delta = (to * from.Conjugate()).Normalized();
		// Les deux hemispheres du quaternion codent la meme rotation ; on
		// force le chemin court, sinon un delta minuscule peut se lire
		// comme un tour presque complet dans l'autre sens.
		if (delta.w < 0.f) {
			delta = NkQuatf(-delta.x, -delta.y, -delta.z, -delta.w);
		}
		const float32 angleRad = delta.Angle().Rad();
		if (angleRad <= 1e-6f) {
			return NkVec3f(0.f, 0.f, 0.f);
		}
		return delta.Axis() * (angleRad / dtSeconds);
	}

	// L'extrapolation de l'orientation a vitesse constante.
	NkQuatf Extrapoler(const NkQuatf &orientation, const NkVec3f &vitesse, float32 dtSeconds) {
		const float32 speedRad = vitesse.Len();
		if (speedRad > 1e-6f) {
			const NkVec3f axis = vitesse * (1.f / speedRad);
			// Delta en espace de BASE : il se compose a GAUCHE de
			// l'orientation courante (meme convention que la mesure).
			const NkQuatf delta(NkAngle::FromRad(speedRad * dtSeconds), axis);
			return (delta * orientation).Normalized();
		}
		return orientation;
	}

} // namespace


namespace {

	const float32 kPi = 3.14159265358979f;
	const float32 kSensibilite = 0.0022f;						// radian par compte de capteur
	const float32 kTangageMax = 89.f * kPi / 180.f;				// le tangage se borne a +-89 degres (voir ci-dessous)

	// L'instant present, en nanosecondes, sur une horloge qui ne recule jamais. Un float64 : un float32 ne garde que sept
	// chiffres, et la difference de deux instants proches y perdrait toute precision.
	float64 Maintenant() { return NkChrono::Now().nanoseconds; }

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	const bool programme = getenv("MASALLE_TOUR") != nullptr;
	const float32 tourDegParSeconde = EnvFloat("MASALLE_TOUR", 0.f);

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
	// Le pointeur : cache, capture, confine (chapitre 6). Les mouvements bruts arrivent quand meme.
	fenetre.ShowMouse(false);
	fenetre.CaptureMouse(true);
	fenetre.ClipMouseToClient(true);
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


	// Le lacet n'est PAS un NkAngle : il s'accumule sans fin (trois tours valent 1080 degres) et un NkAngle le replierait.
	// Le tangage, lui, est borne a +-89 degres : a 90 la verticale exacte rend le lacet indetermine.
	float32 lacet = 0.f, tangage = 0.f;
	int32 accumX = 0, accumY = 0; // comptes bruts de la souris, ajoutes a chaque evenement, vides une fois par image

	const uint64 imageDeSortie = EnvU64("MASALLE_EXIT", 0);
	const uint64 imagesLaissees = 40, imagesImprimees = 20;
	NkQuatf orientationAvant = NkQuatf::Identity();
	float64 instantAvant = 0.0, debutDuVirage = 0.0;
	std::vector<float32> vitessesY;	 // la composante y des vingt vitesses imprimees
	std::vector<int32> comptes;		 // les comptes de souris lus a chacune de ces images
	uint64 image = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
		// Le debut de l'image : l'instant present, en nanosecondes.
		const float64 instant = Maintenant();

		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
			// A chaque evenement brut : on AJOUTE. On ne lit rien et on ne vide rien ici.
			if (NkMouseRawEvent *brut = ev->As<NkMouseRawEvent>()) {
				accumX += brut->GetDeltaX();
				accumY += brut->GetDeltaY();
			}
		}
		if (!tourne) break;

		// Une fois par image : on LIT les totaux et on VIDE. Souris vers la droite = la tete tourne a droite = le lacet
		// diminue (un lacet positif tourne vers la gauche), d'ou le signe moins.
		const int32 lusX = accumX;
		if (programme) {
			// Virage programme : le lacet suit l'horloge, a la vitesse de la variable, sans la souris.
			if (image == imagesLaissees - 10) debutDuVirage = instant;
			if (image >= imagesLaissees - 10) {
				lacet = tourDegParSeconde * (kPi / 180.f) * (float32)((instant - debutDuVirage) * 1e-9);
			}
		} else {
			lacet -= (float32)accumX * kSensibilite;
			tangage -= (float32)accumY * kSensibilite;
			tangage = math::NkClamp(tangage, -kTangageMax, kTangageMax);
		}
		accumX = 0;
		accumY = 0;

		// L'orientation, comme le simulateur du moteur : deux matrices, puis une conversion en quaternion.
		const NkMat4f rotation = NkMat4f::RotationY(NkAngle::FromRad(lacet)) * NkMat4f::RotationX(NkAngle::FromRad(tangage));
		const NkQuatf orientation = NkQuatf(rotation).Normalized();

		// La vitesse entre l'orientation de l'image d'avant et celle-ci, pour le temps reel qui les separe.
		const float32 dt = (float32)((instant - instantAvant) * 1e-9);
		const NkVec3f vitesse = VitesseAngulaire(orientationAvant, orientation, image > 0 ? dt : 0.f);
		const float32 versDegres = 180.f / kPi;

		if (image >= imagesLaissees && image < imagesLaissees + imagesImprimees) {
			std::printf("frame %llu : dt = %.3f ms, speed = (%.3f, %.3f, %.3f) deg/s\n", (unsigned long long)image,
						(double)dt * 1e3, (double)(vitesse.x * versDegres), (double)(vitesse.y * versDegres),
						(double)(vitesse.z * versDegres));
			vitessesY.push_back(vitesse.y * versDegres);
			comptes.push_back(lusX);
		}
		if (image == imagesLaissees - 20) {
			std::printf("[MaSalle] prete : la mesure commence a l'image %llu\n", (unsigned long long)imagesLaissees);
		}
		orientationAvant = orientation;
		instantAvant = instant;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			const NkVec3f position = { 0.30f, kYeux, 1.20f };
			const NkCamera3DData cd = CameraDe(position, orientation * NkVec3f(0.f, 0.f, -1.f), { 0.f, 1.f, 0.f },
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

		if (imageDeSortie != 0 && image >= imageDeSortie) {
			tourne = false;
		}
	}

	// ---- Pour la reponse : la dispersion de la composante y, et les comptes de souris par image ---------------------
	if (!vitessesY.empty()) {
		float64 somme = 0.0, mini = 1e30, maxi = -1e30;
		for (float32 v : vitessesY) {
			somme += v;
			mini = std::min(mini, (float64)v);
			maxi = std::max(maxi, (float64)v);
		}
		const float64 moyenne = somme / (float64)vitessesY.size();
		float64 carres = 0.0;
		for (float32 v : vitessesY) carres += ((float64)v - moyenne) * ((float64)v - moyenne);
		std::printf("[MaSalle] vitesse y sur %zu images : moyenne %.3f, ecart-type %.3f, min %.3f, max %.3f deg/s\n",
					vitessesY.size(), moyenne, std::sqrt(carres / (float64)vitessesY.size()), mini, maxi);
		if (!programme) {
			std::printf("[MaSalle] comptes de souris lus a chaque image :");
			for (int32 c : comptes) std::printf(" %d", (int)c);
			std::printf("\n");
		}
	}

	// Le pointeur est rendu avant de quitter.
	fenetre.ClipMouseToClient(false);
	fenetre.CaptureMouse(false);
	fenetre.ShowMouse(true);

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
