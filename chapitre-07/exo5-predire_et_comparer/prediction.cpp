// Exercice 5 : predire, et comparer. A chaque image, la salle predit l'orientation de la tete pour l'instant ou l'image
// sera VUE (une periode d'ecran plus tard) en supposant que la vitesse reste constante. A l'image suivante, elle mesure
// l'angle entre cette prediction et l'orientation mesuree, en degres, et l'angle entre l'orientation de l'image d'avant
// (prise telle quelle, sans prediction) et l'orientation mesuree. Mille images de la tete tournee a la souris.
//
// La periode est un divise par la cadence de l'ecran, lue dans MASALLE_HZ (60 : la valeur des parametres d'affichage
// de Windows pour cet ecran). MASALLE_EXIT=1060 arrete la salle d'elle-meme : 30 images laissees de cote, 1000 comparees.
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
	const float32 kSensibilite = 0.0022f;				// radian par compte de capteur
	const float32 kTangageMax = 89.f * kPi / 180.f;	// le tangage se borne a +-89 degres

	// L'instant present, en nanosecondes (float64 : un float32 ne garde que sept chiffres).
	float64 Maintenant() { return NkChrono::Now().nanoseconds; }

	// L'angle entre deux orientations, en degres, comme l'enonce : (b * a.Conjugate()).Normalized().Angle().Deg()
	float32 AngleEnonce(const NkQuatf &a, const NkQuatf &b) { return (b * a.Conjugate()).Normalized().Angle().Deg(); }

	// Le meme angle avec atan2 de la partie vectorielle. Angle() vaut 2 * acos(|w|), et pour un petit angle w vaut 1 a
	// l'arrondi pres d'un float32 (1 - 6e-8) : acos y perd tout, l'angle ne prend que des paliers de 0,04 degre, et une
	// erreur de quelques centiemes de degre est arrondie a zero ou a un palier. atan2 n'a pas ce defaut. C'est la mesure
	// rendue ; la formule de l'enonce est calculee a cote, pour montrer l'ecart.
	double AngleEntre(const NkQuatf &a, const NkQuatf &b) {
		const NkQuatf d = (b * a.Conjugate()).Normalized();
		const double v = std::sqrt((double)d.x * d.x + (double)d.y * d.y + (double)d.z * d.z);
		return 2.0 * std::atan2(v, std::fabs((double)d.w)) * 180.0 / 3.14159265358979323846;
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// Le temps qui reste avant l'affichage : une periode de l'ecran, un divise par sa cadence.
	const float32 hz = (float32)EnvU64("MASALLE_HZ", 60);
	const float32 periode = 1.f / hz;

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


	// Le lacet n'est PAS un NkAngle : il s'accumule sans fin. Le tangage se borne a +-89 degres.
	float32 lacet = 0.f, tangage = 0.f;
	int32 accumX = 0, accumY = 0; // comptes bruts, ajoutes a chaque evenement, vides une fois par image

	const uint64 imageDeSortie = EnvU64("MASALLE_EXIT", 0);
	const uint64 imagesLaissees = 30, imagesComparees = 1000;
	NkQuatf orientationAvant = NkQuatf::Identity();
	NkQuatf preditAvant = NkQuatf::Identity();	 // la prediction faite a l'image d'avant, pour CETTE image
	float64 instantAvant = 0.0;
	double sommeAvec = 0.0, sommeSans = 0.0, maxAvec = 0.0, maxSans = 0.0;
	double enonceAvec = 0.0, enonceSans = 0.0; // les memes sommes avec la formule de l'enonce
	uint64 comparees = 0, immobiles = 0;
	uint64 image = 0;
	bool tourne = true;

	while (tourne && fenetre.IsOpen()) {
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

		// Une fois par image : on LIT les totaux et on VIDE. Souris vers la droite = lacet qui diminue.
		const bool sansMouvement = (accumX == 0 && accumY == 0);
		lacet -= (float32)accumX * kSensibilite;
		tangage -= (float32)accumY * kSensibilite;
		tangage = math::NkClamp(tangage, -kTangageMax, kTangageMax);
		accumX = 0;
		accumY = 0;

		// L'orientation mesuree, comme le simulateur du moteur : deux matrices, puis une conversion en quaternion.
		const NkMat4f rotation = NkMat4f::RotationY(NkAngle::FromRad(lacet)) * NkMat4f::RotationX(NkAngle::FromRad(tangage));
		const NkQuatf orientation = NkQuatf(rotation).Normalized();

		// 1. On compare, a CETTE image, avec la prediction faite a l'image d'avant : la prediction de l'image n est
		// comparee a l'orientation de l'image n + 1, jamais a celle de la meme image (l'erreur y serait nulle).
		if (image > imagesLaissees && comparees < imagesComparees) {
			const double eAvec = AngleEntre(preditAvant, orientation);		// avec prediction
			const double eSans = AngleEntre(orientationAvant, orientation);	// sans : la pose d'avant, telle quelle
			sommeAvec += eAvec;
			sommeSans += eSans;
			maxAvec = std::max(maxAvec, eAvec);
			maxSans = std::max(maxSans, eSans);
			enonceAvec += AngleEnonce(preditAvant, orientation);
			enonceSans += AngleEnonce(orientationAvant, orientation);
			if (sansMouvement) ++immobiles;
			++comparees;
		}

		// 2. On predit pour l'image suivante : la vitesse entre les deux dernieres poses, puis une periode d'extrapolation.
		const float32 dt = (float32)((instant - instantAvant) * 1e-9);
		const NkVec3f vitesse = VitesseAngulaire(orientationAvant, orientation, image > 0 ? dt : 0.f);
		preditAvant = Extrapoler(orientation, vitesse, periode);
		orientationAvant = orientation;
		instantAvant = instant;

		if (image == imagesLaissees - 20) {
			std::printf("[MaSalle] prete : la comparaison commence a l'image %llu\n", (unsigned long long)imagesLaissees);
		}

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

	// ---- Les deux lignes demandees ------------------------------------------------------------------------------
	if (comparees > 0) {
		std::printf("with prediction : average %.4f deg, max %.4f deg, over %llu frames\n", sommeAvec / (double)comparees,
					maxAvec, (unsigned long long)comparees);
		std::printf("without prediction : average %.4f deg, max %.4f deg, over %llu frames\n", sommeSans / (double)comparees,
					maxSans, (unsigned long long)comparees);
		// Pour la reponse : le rapport des deux moyennes, des deux maxima, et le nombre d'images ou la tete est immobile.
		std::printf("[MaSalle] rapport sans / avec : moyennes %.2f, maxima %.2f ; images sans mouvement de souris : %llu sur %llu ; "
					"periode %.3f ms\n",
					sommeSans / sommeAvec, maxSans / maxAvec, (unsigned long long)immobiles, (unsigned long long)comparees,
					(double)periode * 1e3);
		std::printf("[MaSalle] avec la formule de l'enonce (Angle() = 2 acos |w|) : moyenne avec %.4f deg, sans %.4f deg, rapport %.2f\n",
					enonceAvec / (double)comparees, enonceSans / (double)comparees, enonceSans / enonceAvec);
	} else {
		std::printf("[MaSalle] aucune image comparee : lancer avec MASALLE_EXIT=1060\n");
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
