// Exercice 4 : les crochets de test. La tete (lacet et tangage en radians, position en metres) sait se poser
// sans la souris, par PoserTete ; son Regard() donne la direction du regard, et la camera la suit.
//
// Un test sans fenetre tourne au demarrage quand la variable d'environnement MASALLE_TEST vaut 1 : il pose la
// tete, compare le regard a des directions ECRITES A LA MAIN, affiche une ligne par verification, puis nkmain
// rend 0 si tout passe et 1 sinon. Le code de sortie est ce que lit un script, pas une ligne "OK".
//
// Pour voir le test dire non, on compile une variante dont le lacet a le signe inverse :
//     -DINVERSER_LACET
// (voir Regard()). Sans cette option, c'est le bon signe.
//
// Hors test : MASALLE_LACET=<degres> et MASALLE_TANGAGE=<degres> posent la tete (PoserTete) avant la boucle ;
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

	// La tete : deux angles en radians, une position en metres. Le lacet est un flottant nu et n'est pas borne (un
	// NkAngle le replierait dans [-180, 180] et trois tours deviendraient zero) ; le tangage serait borne a +-89 degres
	// (a 90 la verticale exacte rend le lacet indetermine). Ni la souris ni cette structure n'en ont besoin ici :
	// PoserTete pose exactement ce qu'on lui donne, c'est ce qui permet au test de viser un angle precis.
	struct Tete {
		float32 lacetRad = 0.f;		// autour de la verticale, positif = vers la gauche (sens trigonometrique vu de dessus)
		float32 tangageRad = 0.f;	// positif = vers le haut
		NkVec3f position = { 0.30f, kYeux, 1.20f };

		// Pose la tete sans passer par la souris. Le simulateur du moteur a le meme crochet, sous le nom
		// DebugSetHead(float32 yawRad, float32 pitchRad, const NkVec3f &positionStage).
		void PoserTete(float32 lacet, float32 tangage, const NkVec3f &pos) {
			lacetRad = lacet;
			tangageRad = tangage;
			position = pos;
		}

		// Direction du regard. Repere de la salle : l'avant est -Z, le haut +Y, repere direct (la droite est +X).
		// (-sin(lacet)*cos(tangage) ; sin(tangage) ; -cos(lacet)*cos(tangage))
		NkVec3f Regard() const {
#ifdef INVERSER_LACET
			const float32 lacet = -lacetRad; // variante d'essai : le signe du lacet est inverse
#else
			const float32 lacet = lacetRad;
#endif
			return { -std::sin(lacet) * std::cos(tangageRad), std::sin(tangageRad),
					 -std::cos(lacet) * std::cos(tangageRad) };
		}
	};

	// ---- Le test --------------------------------------------------------------

	// Un zero negatif ("-0.000") est un zero : on ne veut pas le lire autrement.
	float32 Propre(float32 v) { return std::fabs(v) < 0.0005f ? 0.f : v; }

	// Une verification : on pose la tete, on lit le regard, on compare a la direction ATTENDUE avec une
	// tolerance (cos(90 degres) calcule en float n'est pas exactement zero). Rend vrai si elles concordent.
	bool Verifier(Tete &tete, float32 lacetDeg, float32 tangageDeg, const NkVec3f &attendu) {
		tete.PoserTete(lacetDeg * kPi / 180.f, tangageDeg * kPi / 180.f, { 0.30f, kYeux, 1.20f });
		const NkVec3f obtenu = tete.Regard();
		const float32 tolerance = 1e-4f;
		const bool ok = std::fabs(obtenu.x - attendu.x) < tolerance && std::fabs(obtenu.y - attendu.y) < tolerance &&
						std::fabs(obtenu.z - attendu.z) < tolerance;
		std::printf("yaw %g, pitch %g : expected (%.3f, %.3f, %.3f), got (%.3f, %.3f, %.3f), %s\n", (double)lacetDeg,
					(double)tangageDeg, (double)Propre(attendu.x), (double)Propre(attendu.y), (double)Propre(attendu.z),
					(double)Propre(obtenu.x), (double)Propre(obtenu.y), (double)Propre(obtenu.z), ok ? "OK" : "FAIL");
		return ok;
	}

	// Les directions attendues viennent du RAISONNEMENT, pas de la formule de Regard() : un test qui calcule
	// l'attendu avec la formule qu'il verifie ne peut pas echouer.
	// Convention de la salle : l'avant est -Z, le haut +Y, repere direct. Un lacet positif est une rotation
	// autour de +Y selon la regle de la main droite (pouce vers le haut : de dessus, on tourne a l'envers des
	// aiguilles d'une montre), donc la tete tourne vers sa GAUCHE. A 90 degres l'avant -Z devient -X : la tete
	// regarde le mur des fenetres (x = -2). A 180 degres elle regarde +Z, a -90 degres elle regarde +X.
	// Un tangage de 30 degres leve le regard de 30 degres : y = sin(30) = 0,5 et la partie horizontale
	// (cos(30) = 0,866) reste dans la direction du lacet.
	int Epreuves() {
		Tete tete;
		int echecs = 0;
		echecs += Verifier(tete, 90.f, 0.f, { -1.f, 0.f, 0.f }) ? 0 : 1;			 // 90 degres a gauche : -X
		echecs += Verifier(tete, 0.f, 0.f, { 0.f, 0.f, -1.f }) ? 0 : 1;				 // tete droite : -Z
		echecs += Verifier(tete, -90.f, 0.f, { 1.f, 0.f, 0.f }) ? 0 : 1;			 // 90 degres a droite : +X
		echecs += Verifier(tete, 180.f, 0.f, { 0.f, 0.f, 1.f }) ? 0 : 1;			 // demi-tour : +Z
		echecs += Verifier(tete, 0.f, 30.f, { 0.f, 0.5f, -0.866025f }) ? 0 : 1;		 // yeux leves de 30 degres
		echecs += Verifier(tete, 90.f, 30.f, { -0.866025f, 0.5f, 0.f }) ? 0 : 1;	 // a gauche et leves
		return echecs == 0 ? 0 : 1;
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// Le test : avant l'ouverture de la fenetre, sans souris, et son verdict est le code de sortie.
	if (EnvU64("MASALLE_TEST", 0) == 1) {
		return Epreuves();
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


	// La tete, posee sans la souris.
	Tete tete;
	tete.PoserTete(EnvFloat("MASALLE_LACET", 0.f) * kPi / 180.f, EnvFloat("MASALLE_TANGAGE", 0.f) * kPi / 180.f,
				   { 0.30f, kYeux, 1.20f });

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

			// La camera suit la tete : cd.target = cd.position + tete.Regard().
			const NkCamera3DData cd = CameraDe(tete.position, tete.Regard(), { 0.f, 1.f, 0.f },
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
