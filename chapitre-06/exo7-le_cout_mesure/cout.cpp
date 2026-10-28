// Exercice 7 : le cout, mesure. On chronometre, sur un million d'iterations, deux facons de construire
// l'orientation d'une tete (lacet puis tangage) :
//   matrices    : le chemin du simulateur du moteur, deux matrices puis une conversion en quaternion ;
//   quaternions : le produit direct des deux quaternions RotateY et RotateX, dans l'ordre qui donne le meme regard.
// Chaque version est mesuree trois fois (le premier tour paie des choses que les suivants ne paient pas), et les
// lignes sortent dans la forme `Debug, matrices : T ms`. La configuration s'appelle Release quand NDEBUG est
// defini (MaSalle.jenga le definit avec optimize("Speed") dans la configuration Release), Debug sinon.
// La mesure se fait avant l'ouverture de la fenetre ; MASALLE_COUT_SEUL=1 s'arrete apres elle, sans fenetre.
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

#ifdef NDEBUG
	const char *const kConfig = "Release";
#else
	const char *const kConfig = "Debug";
#endif

	const int kIterations = 1000000;

	// Version 1 : la voie du simulateur, deux matrices et une conversion.
	NkQuatf ParMatrices(float32 lacetDeg, float32 tangageDeg) {
		const NkMat4f rotation = NkMat4f::RotationY(NkAngle(lacetDeg)) * NkMat4f::RotationX(NkAngle(tangageDeg));
		return NkQuatf(rotation).Normalized();
	}

	// Version 2 : le produit direct des deux quaternions. (a * b) * v == a * (b * v), donc lacet * tangage donne
	// le meme regard que RotationY * RotationX.
	NkQuatf ParQuaternions(float32 lacetDeg, float32 tangageDeg) {
		return (NkQuatf::RotateY(NkAngle(lacetDeg)) * NkQuatf::RotateX(NkAngle(tangageDeg))).Normalized();
	}

	// Un million d'iterations. Les angles changent a chaque tour (lacet de 0 a 359,9 degres, tangage de -30 a +30)
	// et les quatre composantes du resultat sont ajoutees a une somme, que l'appelant affiche : sans cela le
	// compilateur est libre de supprimer une boucle dont personne ne lit le resultat.
	template <typename Construire> float64 Chronometrer(Construire construire, float64 &somme) {
		NkChrono c;
		for (int i = 0; i < kIterations; ++i) {
			const float32 lacetDeg = (float32)(i % 3600) * 0.1f;
			const float32 tangageDeg = (float32)(i % 1200) * 0.05f - 30.f;
			const NkQuatf q = construire(lacetDeg, tangageDeg);
			somme += (float64)q.x + (float64)q.y + (float64)q.z + (float64)q.w;
		}
		return c.Elapsed().ToMilliseconds();
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// ---- Les deux versions donnent-elles le meme quaternion ? Controle sur un angle : 45 / 30 degres. ----------
	const NkQuatf qm = ParMatrices(45.f, 30.f);
	const NkQuatf qq = ParQuaternions(45.f, 30.f);
	// Un quaternion et son oppose representent la meme rotation : on aligne le signe avant de comparer.
	const float32 signe = (qm.x * qq.x + qm.y * qq.y + qm.z * qq.z + qm.w * qq.w) < 0.f ? -1.f : 1.f;
	const float32 ecart = std::fmax(std::fmax(std::fabs(qm.x - signe * qq.x), std::fabs(qm.y - signe * qq.y)),
									std::fmax(std::fabs(qm.z - signe * qq.z), std::fabs(qm.w - signe * qq.w)));
	std::printf("controle a 45 / 30 degres : matrices (%.6f, %.6f, %.6f, %.6f), quaternions (%.6f, %.6f, %.6f, %.6f), "
				"ecart max %.2e\n",
				(double)qm.x, (double)qm.y, (double)qm.z, (double)qm.w, (double)(signe * qq.x), (double)(signe * qq.y),
				(double)(signe * qq.z), (double)(signe * qq.w), (double)ecart);

	// Le meme controle sur TOUS les angles de la boucle (hors chronometre). Les deux versions ne choisissent pas
	// toujours le meme signe pour une meme rotation : c'est pourquoi les deux sommes lues plus bas different, sans
	// que les rotations different.
	float32 ecartMax = 0.f;
	int signesOpposes = 0;
	for (int i = 0; i < kIterations; ++i) {
		const float32 lacetDeg = (float32)(i % 3600) * 0.1f;
		const float32 tangageDeg = (float32)(i % 1200) * 0.05f - 30.f;
		const NkQuatf a = ParMatrices(lacetDeg, tangageDeg);
		const NkQuatf b = ParQuaternions(lacetDeg, tangageDeg);
		const float32 s = (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w) < 0.f ? -1.f : 1.f;
		if (s < 0.f) ++signesOpposes;
		ecartMax = std::fmax(ecartMax, std::fmax(std::fmax(std::fabs(a.x - s * b.x), std::fabs(a.y - s * b.y)),
												 std::fmax(std::fabs(a.z - s * b.z), std::fabs(a.w - s * b.w))));
	}
	std::printf("controle sur les %d angles de la boucle : ecart max %.2e au signe pres, %d quaternions de signe oppose\n",
				kIterations, (double)ecartMax, signesOpposes);

	// ---- La mesure : trois fois chaque version, les deux versions a tour de role -----------------------------
	float64 sommeMatrices = 0.0, sommeQuaternions = 0.0;
	for (int k = 0; k < 3; ++k) {
		const float64 tm = Chronometrer(ParMatrices, sommeMatrices);
		std::printf("%s, matrices : %.3f ms\n", kConfig, tm);
		const float64 tq = Chronometrer(ParQuaternions, sommeQuaternions);
		std::printf("%s, quaternions : %.3f ms\n", kConfig, tq);
	}
	// Les sommes, pour que les boucles comptent : elles ne servent qu'a etre lues.
	std::printf("sommes lues : matrices %.6f, quaternions %.6f\n", sommeMatrices, sommeQuaternions);

	if (getenv("MASALLE_COUT_SEUL") != nullptr) {
		return 0;
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
			const NkCamera3DData cd = CameraDe(position, qm * NkVec3f(0.f, 0.f, -1.f), qm * NkVec3f(0.f, 1.f, 0.f),
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
