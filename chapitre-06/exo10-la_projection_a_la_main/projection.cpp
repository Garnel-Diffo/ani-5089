// Exercice 10 : la projection a la main. La matrice de projection DECENTREE est ecrite a la main, a partir de la
// matrice NULLE, avec quatre angles (gauche, droite, haut, bas) en radians, signes : gauche et bas sont negatifs.
// Elle est comparee coefficient par coefficient :
//   symmetric : a NkMat4f::Perspective(NkAngle(70), 1280/720, 0.05, 100), pour le champ symetrique de la camera
//               de la salle (70 degres de haut, rapport 1280/720, plans a 0,05 et 100 metres) ;
//   quest     : a la matrice de la camera du moteur, NkCamera3D avec SetNearFar puis SetFovAsym, pour l'oeil gauche
//               d'un vrai Quest 2 (gauche -52, droite 45, haut 48, bas -50 degres).
// Le calcul se fait au demarrage, avant l'ouverture de la fenetre, et s'affiche avec std::printf : quatre lignes
// d'ecarts par comparaison (une par ligne de la matrice), puis le plus grand ecart de chacune.
// NkMat4f range ses coefficients colonne par colonne : mat[colonne][ligne]. Une table imprimee ligne par ligne lit
// donc mat[0][r], mat[1][r], mat[2][r], mat[3][r].
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

	const float32 kPi = 3.14159265358979f;

	// La projection decentree, telle que le module de realite virtuelle du moteur l'ecrit. Les angles sont en
	// RADIANS et signes (gauche et bas negatifs) ; NkAngle se construit en degres, std::tan attend des radians.
	// `NkMat4f resultat;` est la matrice NULLE, pas l'identite.
	NkMat4f Projection(float32 gauche, float32 droite, float32 haut, float32 bas, float32 nearPlane, float32 farPlane) {
		const float32 tanGauche = std::tan(gauche);
		const float32 tanDroite = std::tan(droite);
		const float32 tanHaut = std::tan(haut);
		const float32 tanBas = std::tan(bas);
		const float32 tanLargeur = tanDroite - tanGauche;
		const float32 tanHauteur = tanHaut - tanBas;

		NkMat4f resultat; // la matrice NULLE
		resultat.mat[0][0] = 2.f / tanLargeur;
		resultat.mat[1][1] = 2.f / tanHauteur;
		resultat.mat[2][0] = (tanDroite + tanGauche) / tanLargeur; // le decentrage horizontal
		resultat.mat[2][1] = (tanHaut + tanBas) / tanHauteur;		// le decentrage vertical
		resultat.mat[2][3] = -1.f;
		// La profondeur, entre -1 et 1 : deux coefficients de plus, les memes que ceux de NkMat4f::Perspective.
		resultat.mat[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
		resultat.mat[3][2] = 2.f * farPlane * nearPlane / (nearPlane - farPlane);
		return resultat;
	}

	// Compare deux matrices coefficient par coefficient, ecrit quatre lignes (une par ligne de la matrice, quatre
	// ecarts par ligne, un par colonne) puis rend le plus grand ecart.
	float32 Comparer(const char *nom, const NkMat4f &mienne, const NkMat4f &moteur) {
		float32 plusGrand = 0.f;
		for (int ligne = 0; ligne < 4; ++ligne) {
			float32 e[4];
			for (int colonne = 0; colonne < 4; ++colonne) {
				e[colonne] = std::fabs(mienne.mat[colonne][ligne] - moteur.mat[colonne][ligne]);
				plusGrand = std::fmax(plusGrand, e[colonne]);
			}
			std::printf("%s, row %d : %.3e %.3e %.3e %.3e\n", nom, ligne, (double)e[0], (double)e[1], (double)e[2],
						(double)e[3]);
		}
		return plusGrand;
	}

	void AfficherCoefficients(const char *nom, const NkMat4f &m) {
		std::printf("%s : mat[0][0] = %.6f, mat[1][1] = %.6f, mat[2][0] = %.6f, mat[2][1] = %.6f, mat[2][2] = %.6f, "
					"mat[3][2] = %.6f, mat[2][3] = %.1f\n",
					nom, (double)m.mat[0][0], (double)m.mat[1][1], (double)m.mat[2][0], (double)m.mat[2][1],
					(double)m.mat[2][2], (double)m.mat[3][2], (double)m.mat[2][3]);
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	const float32 proche = 0.05f, loin = 100.f;
	const float32 rad = kPi / 180.f;

	// ---- Comparaison 1 : le champ symetrique de la salle -----------------------------------------------
	// Les quatre angles se deduisent des reglages de la camera : 70 degres de haut, rapport 1280 / 720.
	//   haut = +35 degres, bas = -35 degres ;  tan(droite) = tan(35 degres) * 1280 / 720, gauche = -droite.
	const float32 rapport = 1280.f / 720.f;
	const float32 haut = 35.f * rad;
	const float32 droite = std::atan(std::tan(haut) * rapport);
	const NkMat4f miennesSymetrique = Projection(-droite, droite, haut, -haut, proche, loin);
	const NkMat4f perspective = NkMat4f::Perspective(NkAngle(70.f), rapport, proche, loin);
	const float32 ecartSymetrique = Comparer("symmetric", miennesSymetrique, perspective);

	// ---- Comparaison 2 : l'oeil gauche d'un Quest 2, lu sur un vrai casque ----------------------------------
	const float32 gaucheQ = -52.f * rad, droiteQ = 45.f * rad, hautQ = 48.f * rad, basQ = -50.f * rad;
	const NkMat4f miennesQuest = Projection(gaucheQ, droiteQ, hautQ, basQ, proche, loin);
	NkCamera3D camera;
	camera.SetNearFar(proche, loin);
	camera.SetFovAsym(gaucheQ, droiteQ, hautQ, basQ);
	const NkMat4f moteurQuest = camera.GetProj();
	const float32 ecartQuest = Comparer("quest", miennesQuest, moteurQuest);

	std::printf("symmetric : largest gap %.3e\n", (double)ecartSymetrique);
	std::printf("quest : largest gap %.3e\n", (double)ecartQuest);

	// Les coefficients eux-memes, pour lire ce que le decentrage y met (lignes de controle, pas de comparaison).
	AfficherCoefficients("coefficients symmetric (ma matrice)", miennesSymetrique);
	AfficherCoefficients("coefficients quest (ma matrice)", miennesQuest);
	AfficherCoefficients("coefficients quest (camera du moteur)", moteurQuest);

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


	const NkVec3f position = { 0.30f, kYeux, 1.20f };
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
			const NkCamera3DData cd = CameraDe(position, { 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.f },
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
