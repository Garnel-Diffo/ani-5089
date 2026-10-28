// Exercice 1 : le pointeur capture. Le pointeur est cache et capture des l'ouverture de la fenetre, il ne peut
// plus en sortir, et une touche le rend (Echap), une autre le reprend (Espace). Avant de quitter, le programme
// rend toujours le pointeur, y compris quand la fenetre est fermee par sa croix.
//
// Tout le code qui touche au pointeur est dans ce fichier. Cinq variables d'environnement servent aux essais :
//   MASALLE_SANS_CONFINEMENT=1  ne confine pas : CaptureMouse tout seul (etape 3 de l'enonce) ;
//   MASALLE_SANS_GARDE=1        appelle ShowMouse a chaque appui, sans compter (etape 7) ;
//   MASALLE_SANS_RENDRE=1       ne rend pas le pointeur en quittant (etape 8) ;
//   MASALLE_SONDE_SEULE=1       n'ouvre aucune fenetre : affiche seulement ce que le systeme dit du pointeur,
//                               pour voir ce qu'un programme precedent a laisse derriere lui ;
//   MASALLE_EXIT=<image>        ferme la fenetre apres cette image, comme le ferait un clic sur la croix.
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


#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

	// ---- Le pointeur ---------------------------------------------------------

	bool gPris = false;		// ce que le programme croit : le pointeur est cache, capture et confine
	bool gConfiner = true;	// faux avec MASALLE_SANS_CONFINEMENT
	bool gGarde = true;		// faux avec MASALLE_SANS_GARDE

	void PrendreLePointeur(NkWindow &fenetre) {
		fenetre.ShowMouse(false);
		fenetre.CaptureMouse(true);
		if (gConfiner) {
			fenetre.ClipMouseToClient(true);
		}
		gPris = true;
		std::printf("[pointeur] pris : cache, capture%s\n", gConfiner ? ", confine" : ", NON confine");
	}

	void RendreLePointeur(NkWindow &fenetre) {
		// Dans l'ordre inverse de la prise. Sur Win32, ShowCursor est un COMPTEUR et non un booleen
		// (voir le commentaire de NkWindow::ShowMouse) : deux ShowMouse(false) demandent deux
		// ShowMouse(true), et un ShowMouse(true) de trop fait monter le compteur au-dessus de zero, ce
		// qui rendrait le prochain ShowMouse(false) sans effet. On ne l'appelle donc que si le pointeur
		// est vraiment pris (voir AppuiRendre et la fin de nkmain).
		fenetre.ClipMouseToClient(false);
		fenetre.CaptureMouse(false);
		fenetre.ShowMouse(true);
		gPris = false;
		std::printf("[pointeur] rendu : visible, libre\n");
	}

	// La touche qui reprend le pointeur. La garde (gGarde) ignore un second appui tant que le pointeur est
	// deja pris ; sans elle, chaque appui appelle ShowMouse(false) et le compteur s'enfonce.
	void AppuiPrendre(NkWindow &fenetre) {
		if (gGarde && gPris) {
			std::printf("[pointeur] deja pris : appui ignore\n");
			return;
		}
		PrendreLePointeur(fenetre);
	}

	void AppuiRendre(NkWindow &fenetre) {
		if (gGarde && !gPris) {
			std::printf("[pointeur] deja rendu : appui ignore\n");
			return;
		}
		RendreLePointeur(fenetre);
	}

	// ---- La sonde : ce que le SYSTEME dit du pointeur, pas ce que le programme croit ----

	void Sonde(const char *quand) {
#ifdef _WIN32
		POINT p = { 0, 0 };
		GetCursorPos(&p);
		CURSORINFO ci = {};
		ci.cbSize = sizeof(ci);
		GetCursorInfo(&ci);
		RECT clip = {};
		GetClipCursor(&clip);

		// La zone cliente de la fenetre, en coordonnees d'ecran (rien si la fenetre n'existe pas).
		const char *dedans = "pas de fenetre";
		RECT zone = {};
		if (HWND h = FindWindowA(nullptr, "Ma salle")) {
			RECT c;
			GetClientRect(h, &c);
			POINT haut = { c.left, c.top }, bas = { c.right, c.bottom };
			ClientToScreen(h, &haut);
			ClientToScreen(h, &bas);
			zone = { haut.x, haut.y, bas.x, bas.y };
			dedans = (p.x >= zone.left && p.x < zone.right && p.y >= zone.top && p.y < zone.bottom) ? "dans la fenetre"
																									  : "HORS de la fenetre";
		}
		const int ecranL = GetSystemMetrics(SM_CXSCREEN), ecranH = GetSystemMetrics(SM_CYSCREEN);
		const bool confine = !(clip.left <= 0 && clip.top <= 0 && clip.right >= ecranL && clip.bottom >= ecranH);
		std::printf("[sonde] %s : pointeur (%ld, %ld), %s, %s, %s", quand, (long)p.x, (long)p.y, dedans,
					(ci.flags & CURSOR_SHOWING) ? "visible" : "cache", confine ? "confine" : "libre");
		if (confine) {
			std::printf(" dans (%ld, %ld)-(%ld, %ld)", (long)clip.left, (long)clip.top, (long)clip.right, (long)clip.bottom);
		}
		std::printf("\n");
#else
		(void)quand;
#endif
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0); // les lignes apparaissent au fur et a mesure, meme redirigees

	gConfiner = getenv("MASALLE_SANS_CONFINEMENT") == nullptr;
	gGarde = getenv("MASALLE_SANS_GARDE") == nullptr;

	// Mode sonde : aucune fenetre, on regarde seulement l'etat laisse par un programme precedent.
	if (getenv("MASALLE_SONDE_SEULE") != nullptr) {
		Sonde("apres le programme precedent");
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

	// Juste apres l'ouverture de la fenetre : on cache le pointeur et on le capture.
	PrendreLePointeur(fenetre);

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


	std::printf("[MaSalle] Echap rend le pointeur, Espace le reprend, la croix ferme\n");
	Sonde("au depart");

	const uint64 imageDeSortie = EnvU64("MASALLE_EXIT", 0);
	NkChrono chrono;
	uint64 image = 0;
	int derniereSeconde = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
			if (NkKeyPressEvent *touche = ev->As<NkKeyPressEvent>()) {
				if (touche->GetKey() == NkKey::NK_ESCAPE) AppuiRendre(fenetre);
				if (touche->GetKey() == NkKey::NK_SPACE) AppuiPrendre(fenetre);
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();

			// Une camera fixe, a hauteur d'yeux, qui regarde la porte.
			NkCamera3DData cd;
			cd.position = { 0.30f, kYeux, 1.20f };
			cd.target = { -0.70f, 1.10f, -kDemi };
			cd.up = { 0.f, 1.f, 0.f };
			cd.fovY = 70.f;
			cd.aspect = (float32)devInit.width / (float32)devInit.height;
			cd.nearPlane = 0.05f;
			cd.farPlane = 100.f;
			RendreLaSalle(renderer, cmd, cd, (float32)image / 60.f);

			if (auto *r2d = rEcran->GetRender2D()) {
				r2d->DrawImage(cible->GetColorResult(),
							   { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
			}
			rEcran->Present();
			rEcran->EndFrame();
		}
		++image;

		// Une ligne de sonde par seconde.
		const int seconde = (int)chrono.Elapsed().ToMilliseconds() / 1000;
		if (seconde != derniereSeconde) {
			derniereSeconde = seconde;
			char quand[32];
			std::snprintf(quand, sizeof quand, "t = %d s", seconde);
			Sonde(quand);
		}

		if (imageDeSortie != 0 && image >= imageDeSortie) {
			tourne = false;
		}
	}

	// Avant de partir : on rend toujours le pointeur, que la fenetre soit fermee par sa croix, par
	// MASALLE_EXIT ou autrement. Le confinement SURVIT au processus s'il n'est pas relache.
	if (getenv("MASALLE_SANS_RENDRE") == nullptr) {
		if (gPris) {
			RendreLePointeur(fenetre);
		}
	} else {
		std::printf("[pointeur] MASALLE_SANS_RENDRE : on quitte SANS rendre le pointeur\n");
	}
	Sonde("avant de quitter");

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
