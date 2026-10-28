// Exercice 9 : les deux images, et l'image noire. La salle est dessinee DEUX fois, une par oeil, chacune dans sa
// propre cible hors ecran de 640 x 720, cote a cote dans la fenetre. Les deux yeux sont a la position de la tete,
// decalee d'un demi-ecart de 63 mm le long de la droite de la tete (tete droite, regard vers -Z).
//
// A l'image 30 les deux cibles sont enregistrees (oeil-gauche.png, oeil-droit.png) puis soustraites pixel a pixel :
// l'ecart d'un pixel est la plus grande difference sur ses trois canaux. difference.png montre quatre fois l'ecart
// (plafonne a 255), le noir veut dire "pareil". On recommence ensuite en OUBLIANT de decaler les yeux (les deux
// cameras a la position de la tete) : difference-vue-oubliee.png. Les deux lignes de difference.txt sortent du
// programme. Le programme s'arrete une fois les deux soustractions faites.
#include "NKImage/Core/NkImage.h"

#include <algorithm>
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

	const int32 kLargeurOeil = 640;
	const int32 kHauteurOeil = 720;
	const float32 kEcartInterpupillaire = 0.063f; // 63 mm, la valeur du simulateur du moteur

	// La droite de la tete : l'image, par son orientation, de la droite de la salle, +X.
	NkVec3f DroiteDeLaTete(const NkQuatf &orientation) { return orientation * NkVec3f(1.f, 0.f, 0.f); }

	struct Soustraction {
		bool ok = false;
		int pixels = 0;	   // nombre total de pixels
		int ecartes = 0;   // pixels dont l'ecart depasse 1
		int ecartMax = 0;  // le plus grand ecart
		float64 luminance = 0.0; // luminance moyenne de l'image gauche, sur 255 : un zero venu de deux images BLANCHES
								 // (255) ou NOIRES (0) ne prouverait rien
	};

	// Charge les deux images, les soustrait pixel a pixel et ecrit l'image de la difference. L'ecart d'un pixel est la
	// plus grande difference sur ses trois canaux ; l'image de la difference montre quatre fois cet ecart, plafonne a 255.
	Soustraction Soustraire(const char *fichierGauche, const char *fichierDroit, const char *fichierDifference) {
		Soustraction s;
		NkImage gauche, droit;
		if (!gauche.Load(fichierGauche) || !droit.Load(fichierDroit) || !gauche.IsValid() || !droit.IsValid()) {
			std::printf("[MaSalle] impossible de relire %s et %s\n", fichierGauche, fichierDroit);
			return s;
		}
		const int largeur = (int)gauche.Width(), hauteur = (int)gauche.Height();
		if (largeur != (int)droit.Width() || hauteur != (int)droit.Height()) {
			std::printf("[MaSalle] les deux images n'ont pas la meme taille\n");
			return s;
		}
		NkImage difference = NkImage::Create(largeur, hauteur, 4);
		float64 sommeLuminance = 0.0;
		for (int y = 0; y < hauteur; ++y) {
			for (int x = 0; x < largeur; ++x) {
				const math::NkColor a = gauche.GetPixel(x, y), b = droit.GetPixel(x, y);
				sommeLuminance += ((float64)a.r + (float64)a.g + (float64)a.b) / 3.0;
				const int dr = std::abs((int)a.r - (int)b.r), dg = std::abs((int)a.g - (int)b.g),
						  db = std::abs((int)a.b - (int)b.b);
				const int ecart = std::max(dr, std::max(dg, db));
				if (ecart > 1) ++s.ecartes;
				if (ecart > s.ecartMax) s.ecartMax = ecart;
				const uint8 v = (uint8)std::min(255, ecart * 4);
				difference.SetPixel(x, y, math::NkColor(v, v, v, 255));
			}
		}
		s.pixels = largeur * hauteur;
		s.luminance = sommeLuminance / (float64)s.pixels;
		s.ok = difference.SavePNG(fichierDifference);
		return s;
	}

	// Le decalage de la porte entre les deux yeux, mesure sur une ligne des deux images enregistrees. La porte est de
	// bois (le rouge depasse nettement le bleu), le mur est gris (rouge et bleu a peu pres egaux) : on cherche le premier
	// et le dernier pixel de bois de la ligne. On le compare a ce que dit la geometrie : un point a la distance z se
	// decale de f * b / z pixels d'un oeil a l'autre, f etant la focale en pixels (720 pixels de haut, 70 degres).
	void MesurerLaPorte(const char *fichierGauche, const char *fichierDroit, int ligne, float32 distance) {
		NkImage images[2];
		int premier[2] = { -1, -1 }, dernier[2] = { -1, -1 };
		const char *fichiers[2] = { fichierGauche, fichierDroit };
		for (int i = 0; i < 2; ++i) {
			if (!images[i].Load(fichiers[i]) || !images[i].IsValid()) {
				std::printf("[MaSalle] impossible de relire %s\n", fichiers[i]);
				return;
			}
			for (int x = 0; x < (int)images[i].Width(); ++x) {
				const math::NkColor c = images[i].GetPixel(x, ligne);
				if ((int)c.r - (int)c.b > 30) {
					if (premier[i] < 0) premier[i] = x;
					dernier[i] = x;
				}
			}
		}
		const float32 focale = (float32)kHauteurOeil * 0.5f / std::tan(35.f * 3.14159265358979f / 180.f);
		std::printf("[MaSalle] porte, ligne %d : pixels de bois de x = %d a %d (oeil gauche), de %d a %d (oeil droit), "
				"decalage %d pixels ; attendu f * b / z = %.1f * %.3f / %.2f = %.1f pixels\n",
				ligne, premier[0], dernier[0], premier[1], dernier[1], premier[0] - premier[1], (double)focale,
				(double)kEcartInterpupillaire, (double)distance, (double)(focale * kEcartInterpupillaire / distance));
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

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

	// Le renderer d'ecran possede l'image : il ne dessine que du 2D, c'est-a-dire les deux cibles cote a cote.
	NkRendererConfig cfgEcran = NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height);
	NkRenderer *rEcran = NkRenderer::Create(device, cfgEcran);
	if (rEcran == nullptr || !rEcran->Initialize()) {
		std::printf("[MaSalle] le renderer d'ecran n'a pas pu s'initialiser\n");
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	// ---- Deux cibles hors ecran et deux renderers de salle, un par oeil, chacun de 640 x 720 -----------------
	NkOffscreenDesc od;
	od.width = kLargeurOeil;
	od.height = kHauteurOeil;
	od.hasDepth = true;
	od.hdr = false;
	od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
	od.readable = true;
	od.readback = true; // sans quoi Capture() ne peut rien relire

	od.name = "MaSalle_OeilGauche";
	NkOffscreenTarget *cibleGauche = rEcran->CreateOffscreen(od);
	od.name = "MaSalle_OeilDroit";
	NkOffscreenTarget *cibleDroite = rEcran->CreateOffscreen(od);
	if (cibleGauche == nullptr || cibleDroite == nullptr) {
		std::printf("[MaSalle] cible hors ecran refusee\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, kLargeurOeil, kHauteurOeil);
	cfgSalle.Disable(NK_SS_RENDER2D);
	cfgSalle.Disable(NK_SS_TEXT);
	cfgSalle.Disable(NK_SS_OVERLAY);
	cfgSalle.Enable(NK_SS_OFFSCREEN);

	NkRenderer *rGauche = NkRenderer::Create(device, cfgSalle);
	NkRenderer *rDroit = NkRenderer::Create(device, cfgSalle);
	if (rGauche == nullptr || rDroit == nullptr || !rGauche->Initialize() || !rDroit->Initialize()) {
		std::printf("[MaSalle] un renderer de salle n'a pas pu s'initialiser\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}
	rGauche->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cibleGauche->GetColorResult()));
	rDroit->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cibleDroite->GetColorResult()));

	// Sans cette ligne, les cibles restent blanches sur certaines machines et le programme se fige a la
	// troisieme image sur d'autres : chaque renderer recoit la taille de SA cible.
	rGauche->SetRenderSizeOverride(kLargeurOeil, kHauteurOeil);
	rDroit->SetRenderSizeOverride(kLargeurOeil, kHauteurOeil);

	// ---- Les deux yeux : la tete droite, regard vers -Z ---------------------------------------------------
	const NkVec3f tete = { 0.30f, 1.70f, 1.20f };
	const NkQuatf orientation = NkQuatf::Identity();
	const NkVec3f regard = orientation * NkVec3f(0.f, 0.f, -1.f);
	const NkVec3f haut = orientation * NkVec3f(0.f, 1.f, 0.f);
	const NkVec3f droite = DroiteDeLaTete(orientation);
	const float32 demiEcart = kEcartInterpupillaire * 0.5f;
	const NkVec3f oeilGauche = { tete.x - droite.x * demiEcart, tete.y - droite.y * demiEcart, tete.z - droite.z * demiEcart };
	const NkVec3f oeilDroit = { tete.x + droite.x * demiEcart, tete.y + droite.y * demiEcart, tete.z + droite.z * demiEcart };
	std::printf("[MaSalle] oeil gauche (%.4f, %.4f, %.4f), oeil droit (%.4f, %.4f, %.4f)\n", (double)oeilGauche.x,
				(double)oeilGauche.y, (double)oeilGauche.z, (double)oeilDroit.x, (double)oeilDroit.y, (double)oeilDroit.z);

	const float32 aspect = (float32)kLargeurOeil / (float32)kHauteurOeil;
	bool oubli = false;			  // vrai : on a oublie de decaler les yeux, les deux cameras sont a la tete
	Soustraction deuxYeux, vueOubliee;
	bool fini = false;
	uint64 image = 0;
	bool tourne = true;

	while (tourne && !fini && fenetre.IsOpen()) {
		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			const float32 temps = (float32)image / 60.f; // le meme instant pour les deux yeux

			// Chaque oeil : meme salle, seule la position de la camera change. RendreLaSalle rejoue pour CHAQUE
			// renderer FlushGraphRebuilds, ResetFrame et Upload (sans ResetFrame, le second oeil herite de ce que
			// le premier a laisse), soumet la salle, puis execute son graphe sur le command buffer de l'ecran.
			const NkCamera3DData camGauche = CameraDe(oubli ? tete : oeilGauche, regard, haut, aspect);
			const NkCamera3DData camDroite = CameraDe(oubli ? tete : oeilDroit, regard, haut, aspect);
			RendreLaSalle(rGauche, cmd, camGauche, temps);
			RendreLaSalle(rDroit, cmd, camDroite, temps);

			// Les deux cibles, cote a cote dans la fenetre : x = 0 puis 640.
			if (auto *r2d = rEcran->GetRender2D()) {
				r2d->DrawImage(cibleGauche->GetColorResult(), { 0.f, 0.f, (float32)kLargeurOeil, (float32)kHauteurOeil });
				r2d->DrawImage(cibleDroite->GetColorResult(), { (float32)kLargeurOeil, 0.f, (float32)kLargeurOeil,
																(float32)kHauteurOeil });
			}
			rEcran->Present();
			rEcran->EndFrame();
		}
		++image;

		// A l'image 30 : les deux yeux, enregistres a la MEME image, puis soustraits.
		if (image == 30) {
			const bool a = cibleGauche->Capture("oeil-gauche.png");
			const bool b = cibleDroite->Capture("oeil-droit.png");
			std::printf("[MaSalle] image 30 : oeil-gauche.png (%s), oeil-droit.png (%s)\n", a ? "ok" : "echec", b ? "ok" : "echec");
			deuxYeux = Soustraire("oeil-gauche.png", "oeil-droit.png", "difference.png");
			std::printf("[MaSalle] luminance moyenne de l'oeil gauche : %.1f sur 255\n", deuxYeux.luminance);
			// La porte est a la distance de la tete a sa face avant : z = 1.20 - (-2 + 0.04) = 3.16 metres.
			MesurerLaPorte("oeil-gauche.png", "oeil-droit.png", 450, tete.z + kDemi - 0.04f);
			oubli = true; // la suite : on oublie de decaler les yeux
		}
		// A l'image 60, la meme chose avec les deux cameras a la position de la tete. Les deux captures ne servent
		// qu'a cette soustraction : on les efface ensuite.
		if (image == 60) {
			const bool a = cibleGauche->Capture("oeil-gauche-vue-oubliee.tmp.png");
			const bool b = cibleDroite->Capture("oeil-droit-vue-oubliee.tmp.png");
			std::printf("[MaSalle] image 60 : deux cameras a la tete (%s, %s)\n", a ? "ok" : "echec", b ? "ok" : "echec");
			vueOubliee = Soustraire("oeil-gauche-vue-oubliee.tmp.png", "oeil-droit-vue-oubliee.tmp.png",
									"difference-vue-oubliee.png");
			std::printf("[MaSalle] luminance moyenne de l'oeil gauche, vue oubliee : %.1f sur 255\n", vueOubliee.luminance);
			std::remove("oeil-gauche-vue-oubliee.tmp.png");
			std::remove("oeil-droit-vue-oubliee.tmp.png");
			fini = true;
		}
	}

	// Les deux lignes de difference.txt, dans la forme demandee.
	if (deuxYeux.ok && vueOubliee.ok) {
		if (std::FILE *f = std::fopen("difference.txt", "wb")) {
			std::fprintf(f, "two eyes : %d pixels out of %d, max gap %d\n", deuxYeux.ecartes, deuxYeux.pixels, deuxYeux.ecartMax);
			std::fprintf(f, "view forgotten : %d pixels out of %d, max gap %d\n", vueOubliee.ecartes, vueOubliee.pixels,
						 vueOubliee.ecartMax);
			std::fclose(f);
		}
		std::printf("two eyes : %d pixels out of %d, max gap %d\n", deuxYeux.ecartes, deuxYeux.pixels, deuxYeux.ecartMax);
		std::printf("view forgotten : %d pixels out of %d, max gap %d\n", vueOubliee.ecartes, vueOubliee.pixels,
					vueOubliee.ecartMax);
	} else {
		std::printf("[MaSalle] les soustractions ont echoue\n");
	}

	NkRenderer::Destroy(rDroit);
	NkRenderer::Destroy(rGauche);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
