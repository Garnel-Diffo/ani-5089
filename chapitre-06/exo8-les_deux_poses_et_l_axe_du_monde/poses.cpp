// Exercice 8 : les deux poses et l'axe du monde. La position des deux yeux se calcule a partir de la pose de la
// tete, decalee d'un DEMI ecart interpupillaire de part et d'autre, de deux facons :
//   head  : le long de la DROITE DE LA TETE, c'est-a-dire l'image de la droite de la salle (+X) par l'orientation ;
//   world : le long de l'axe x du MONDE, sans regarder l'orientation.
// Trois poses : neutral (tete droite), roll 30 (tete penchee de 30 degres sur le cote), yaw 90 (tete tournee de 90
// degres). Le calcul se fait au demarrage, avant l'ouverture de la fenetre, et s'affiche avec std::printf :
// six positions en metres (quatre decimales), puis l'ecart de l'oeil gauche entre les deux methodes, en millimetres.
// Attention : NkQuatf::Right() existe mais tourne -X (voir NKMath/NkQuat.h), pas la droite de la salle : on applique
// l'orientation a (1, 0, 0).
// MASALLE_IPD_MM=<mm> change l'ecart interpupillaire (63 par defaut, la valeur du simulateur du moteur) ;
// MASALLE_EXIT=<image> ferme la fenetre apres cette image.
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

	// La droite de la tete : l'image, par son orientation, de la droite de la salle, +X.
	NkVec3f DroiteDeLaTete(const NkQuatf &orientation) { return orientation * NkVec3f(1.f, 0.f, 0.f); }

	struct Yeux {
		NkVec3f gauche;
		NkVec3f droit;
	};

	// Facon 1 : chaque oeil se decale depuis la tete, de la moitie de l'ecart, le long de la droite de la tete.
	Yeux YeuxSelonLaTete(const NkVec3f &tete, const NkQuatf &orientation, float32 ipd) {
		const float32 demiEcart = ipd * 0.5f;
		const NkVec3f droite = DroiteDeLaTete(orientation);
		return { { tete.x - droite.x * demiEcart, tete.y - droite.y * demiEcart, tete.z - droite.z * demiEcart },
				 { tete.x + droite.x * demiEcart, tete.y + droite.y * demiEcart, tete.z + droite.z * demiEcart } };
	}

	// Facon 2 : le meme decalage, mais le long de l'axe x du MONDE, quelle que soit l'orientation.
	Yeux YeuxSelonLeMonde(const NkVec3f &tete, float32 ipd) {
		const float32 demiEcart = ipd * 0.5f;
		return { { tete.x - demiEcart, tete.y, tete.z }, { tete.x + demiEcart, tete.y, tete.z } };
	}

	float32 Propre(float32 v) { return std::fabs(v) < 0.00005f ? 0.f : v; }

	void AfficherYeux(const char *pose, const char *facon, const Yeux &y) {
		std::printf("%s, %s : L (%.4f, %.4f, %.4f) R (%.4f, %.4f, %.4f)\n", pose, facon, (double)Propre(y.gauche.x),
					(double)Propre(y.gauche.y), (double)Propre(y.gauche.z), (double)Propre(y.droit.x),
					(double)Propre(y.droit.y), (double)Propre(y.droit.z));
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// ---- Le calcul, avant l'ouverture de la fenetre -------------------------------------------------------
	const NkVec3f tete = { 0.30f, 1.70f, 1.20f }; // la tete, la ou la camera de la salle se tient
	const float32 ipd = EnvFloat("MASALLE_IPD_MM", 63.f) * 0.001f; // en metres

	const char *noms[3] = { "neutral", "roll 30", "yaw 90" };
	const NkQuatf poses[3] = {
		NkQuatf::Identity(),					  // neutral : la tete droite
		NkQuatf::RotateZ(NkAngle(30.f)),		  // roll 30 : penchee de 30 degres sur le cote
		NkQuatf::RotateY(NkAngle(90.f)),		  // yaw 90 : tournee de 90 degres
	};

	Yeux selonLaTete[3], selonLeMonde[3];
	for (int i = 0; i < 3; ++i) {
		selonLaTete[i] = YeuxSelonLaTete(tete, poses[i], ipd);
		selonLeMonde[i] = YeuxSelonLeMonde(tete, ipd);
		AfficherYeux(noms[i], "head", selonLaTete[i]);
		AfficherYeux(noms[i], "world", selonLeMonde[i]);
	}

	// L'ecart de l'oeil gauche entre les deux methodes, en MILLIMETRES (les positions sont en metres : un facteur
	// 1000 oublie rendrait l'erreur mille fois trop grande).
	for (int i = 0; i < 3; ++i) {
		const NkVec3f a = selonLaTete[i].gauche, b = selonLeMonde[i].gauche;
		const float32 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
		const float32 ecartMm = std::sqrt(dx * dx + dy * dy + dz * dz) * 1000.f;
		std::printf("%s : gap %.1f mm\n", noms[i], (double)ecartMm);
	}

	// Le meme ecart, ecrit a la main : les deux decalages ont la meme longueur (le demi-ecart) et different par
	// l'angle entre la droite de la tete et +X ; pour un angle a, la corde vaut 2 * demiEcart * sin(a / 2).
	//   roll 30 : a = 30 degres -> 2 * 31,5 mm * sin(15 degres) = 16,3 mm ;
	//   yaw 90  : a = 90 degres -> 2 * 31,5 mm * sin(45 degres) = 44,5 mm ;
	//   neutral : a = 0 -> 0 mm.
	{
		const float32 pi = 3.14159265358979f;
		auto corde = [&](float32 degres) { return 2.f * ipd * 0.5f * std::sin(degres * 0.5f * pi / 180.f) * 1000.f; };
		std::printf("a la main : neutral %.1f mm, roll 30 %.1f mm, yaw 90 %.1f mm\n", (double)corde(0.f),
					(double)corde(30.f), (double)corde(90.f));
	}

	NkWindowConfig cfg;
	cfg.title = "Ma salle";
	cfg.width = 1280;
	cfg.height = 720;
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


	const uint64 imageDeSortie = EnvU64("MASALLE_EXIT", 0);
	uint64 image = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			const NkCamera3DData cd = CameraDe(tete, { 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.f },
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

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
