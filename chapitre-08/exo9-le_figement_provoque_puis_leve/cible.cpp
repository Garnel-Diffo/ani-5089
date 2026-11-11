// Exercice 9 : le figement, provoque puis leve.
//
// Une cible hors ecran PLUS PETITE que la fenetre, branchee sur un renderer par DEUX appels :
//
//     renderer->SetFinalColorTarget(poigneeDeLaCible);              // 1. OU dessiner
//     renderer->SetRenderSizeOverride(largeurCible, hauteurCible);  // 2. EN QUELLE TAILLE
//
// Sans le second appel, le renderer garde la taille de la fenetre : la passe tourne sur une zone plus grande
// que l'image visee. D'apres le cours, le travail soumis a la carte ne se termine alors jamais, et le moteur,
// qui veut reutiliser son tampon de commandes pour l'image suivante, le trouve encore en cours et ATTEND :
// le programme se fige sans rien dire, en general a la troisieme image. Ce que ce programme observe vraiment
// depend de l'interface graphique : on l'ecrit tel quel, on ne le suppose pas.
//
// Le meme programme sert aux deux mesures, l'une avec les deux appels, l'autre avec le premier seulement :
//   CIBLE_SANS_TAILLE=1   saute le second appel (la faute) ;
//   sans la variable      les deux appels sont faits : la boucle va jusqu'au bout.
//   CIBLE_IMAGES=N        nombre d'images a dessiner (12 par defaut) ;
//   CIBLE_API=dx11|vk|gl|dx12|sw   l'interface graphique (dx11 par defaut) ;
//   CIBLE_LARGEUR, CIBLE_HAUTEUR   la taille de la cible hors ecran (320 x 180 par defaut, dans une fenetre de 640 x 360).
//
// Deux sentinelles, qui ne touchent a rien du rendu :
//   - un fil de garde qui, si le fil principal ne termine plus aucune image pendant trois secondes (apres la
//     premiere, qui compile les nuanceurs), ecrit `image : K, etat : figee` et laisse le processus en vie pour
//     qu'on puisse y attacher un debogueur (il s'arrete de lui-meme apres CIBLE_ATTENTE_MAX secondes, 300 par defaut) ;
//   - un filtre d'exception qui, si le programme plante, ecrit `image : K, etat : plantee` puis arrete le processus.
//
// Sortie : une ligne par image terminee, de la forme `image : 1, etat : dessinee`.

#include "NKWindow/NKMain.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkWindowConfig.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"

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

#include "NKMath/NkMat.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData &d) { d.appName = "Cible"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

	// La taille de la fenetre, et celle de la cible hors ecran : la moitie, donc plus petite.
	const uint32 kLargeurFenetre = 640;
	const uint32 kHauteurFenetre = 360;
	uint32 gLargeurCible = kLargeurFenetre / 2;
	uint32 gHauteurCible = kHauteurFenetre / 2;

	// Le nombre d'images terminees, lu par le fil de garde et par le filtre d'exception.
	std::atomic<uint64> gTerminees{ 0 };

	// Si le programme plante : on ecrit dans quel etat etait l'image en cours, puis le processus s'arrete.
	LONG WINAPI SurUnPlantage(EXCEPTION_POINTERS *) {
		std::printf("image : %llu, etat : plantee\n", (unsigned long long)(gTerminees.load() + 1));
		std::fflush(stdout);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	uint64 EnvU64(const char *nom, uint64 defaut) {
		const char *t = getenv(nom);
		if (t == nullptr || *t == '\0') return defaut;
		return (uint64)strtoull(t, nullptr, 10);
	}

	// Un pave pose dans la scene.
	void Pave(NkRender3D *r3d, NkMeshSystem *meshes, const NkVec3f &centre, const NkVec3f &taille,
			  const NkVec3f &teinte) {
		NkDrawCall3D dc;
		dc.mesh = meshes->GetCube();
		dc.transform = NkMat4f::Translate(centre) * NkMat4f::Scale(taille);
		dc.aabb = { { centre.x - taille.x * 0.5f, centre.y - taille.y * 0.5f, centre.z - taille.z * 0.5f },
					{ centre.x + taille.x * 0.5f, centre.y + taille.y * 0.5f, centre.z + taille.z * 0.5f } };
		dc.tint = teinte;
		dc.metallic = 0.f;
		dc.roughness = 0.8f;
		r3d->Submit(dc);
	}

	// Une scene minimale : un sol et trois pave. Rendue sur le command buffer de l'ecran, comme dans la salle.
	void RendreLaScene(NkRenderer *renderer, NkICommandBuffer *cmd, float32 aspect, float32 temps) {
		NkRender3D *r3d = renderer->GetRender3D();
		renderer->FlushGraphRebuilds();
		r3d->ResetFrame();
		if (auto *mc = renderer->GetMaterialCollection()) {
			mc->Upload();
		}

		NkCamera3DData cd;
		cd.position = { 0.f, 1.7f, 3.f };
		cd.target = { 0.f, 0.8f, 0.f };
		cd.up = { 0.f, 1.f, 0.f };
		cd.fovY = 70.f;
		cd.aspect = aspect;
		cd.nearPlane = 0.05f;
		cd.farPlane = 100.f;

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
		{
			NkDrawCall3D dc;
			dc.mesh = renderer->GetMeshSystem()->GetPlane();
			dc.transform = NkMat4f::Scale({ 6.f, 1.f, 6.f });
			dc.aabb = { { -3.f, 0.f, -3.f }, { 3.f, 0.f, 3.f } };
			dc.castShadow = false;
			dc.tint = { 0.38f, 0.36f, 0.34f };
			dc.metallic = 0.f;
			dc.roughness = 1.f;
			r3d->Submit(dc);
		}
		Pave(r3d, renderer->GetMeshSystem(), { -1.0f, 0.5f, 0.f }, { 1.f, 1.f, 1.f }, { 0.85f, 0.80f, 0.30f });
		Pave(r3d, renderer->GetMeshSystem(), { 0.8f, 0.4f, 0.4f }, { 0.8f, 0.8f, 0.8f }, { 0.55f, 0.38f, 0.22f });
		Pave(r3d, renderer->GetMeshSystem(), { 0.2f, 0.25f, -1.f }, { 0.5f, 0.5f, 0.5f }, { 0.75f, 0.85f, 0.92f });

		if (auto *graph = renderer->GetRenderGraph()) {
			graph->Execute(cmd);
		}
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	SetUnhandledExceptionFilter(SurUnPlantage);

	const bool sansTaille = getenv("CIBLE_SANS_TAILLE") != nullptr;
	gLargeurCible = (uint32)EnvU64("CIBLE_LARGEUR", gLargeurCible);
	gHauteurCible = (uint32)EnvU64("CIBLE_HAUTEUR", gHauteurCible);
	const uint64 nombreImages = EnvU64("CIBLE_IMAGES", 12);

	NkWindowConfig cfg;
	cfg.title = "Cible hors ecran";
	cfg.width = kLargeurFenetre;
	cfg.height = kHauteurFenetre;
	cfg.centered = true;
	cfg.vsync = true;

	NkWindow fenetre(cfg);
	if (!fenetre.IsValid()) {
		std::printf("[Cible] la fenetre n'a pas pu s'ouvrir\n");
		return 1;
	}

	NkDeviceInitInfo devInit{};
	devInit.surface = fenetre.GetSurfaceDesc();
	devInit.width = fenetre.GetSize().x;
	devInit.height = fenetre.GetSize().y;
	// DirectX 11 : les nuanceurs DirectX 12 de cette machine ne compilent pas.
	devInit.api = NkGraphicsApi::NK_GFX_API_DX11;
	if (const char *api = getenv("CIBLE_API")) {
		if (std::string(api) == "gl") devInit.api = NkGraphicsApi::NK_GFX_API_OPENGL;
		if (std::string(api) == "vk") devInit.api = NkGraphicsApi::NK_GFX_API_VULKAN;
		if (std::string(api) == "dx12") devInit.api = NkGraphicsApi::NK_GFX_API_DX12;
		if (std::string(api) == "sw") devInit.api = NkGraphicsApi::NK_GFX_API_SOFTWARE;
	}

	NkIDevice *device = NkDeviceFactory::CreateForApi(devInit.api, devInit);
	if (device == nullptr) {
		std::printf("[Cible] aucune API graphique disponible\n");
		return 1;
	}

	// Le premier renderer possede l'image de l'ecran et ne dessine que du 2D.
	NkRenderer *rEcran = NkRenderer::Create(device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
	if (rEcran == nullptr || !rEcran->Initialize()) {
		std::printf("[Cible] le renderer d'ecran n'a pas pu s'initialiser\n");
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	// La cible hors ecran : PLUS PETITE que la fenetre.
	NkOffscreenDesc od;
	od.width = gLargeurCible;
	od.height = gHauteurCible;
	od.hasDepth = true;
	od.hdr = false;
	od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
	od.readable = true;
	od.name = "Cible_Reduite";
	NkOffscreenTarget *cible = rEcran->CreateOffscreen(od);
	if (cible == nullptr) {
		std::printf("[Cible] cible hors ecran refusee\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	// Le second renderer dessine la scene dans cette cible.
	NkRendererConfig cfgScene = NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
	cfgScene.Disable(NK_SS_RENDER2D);
	cfgScene.Disable(NK_SS_TEXT);
	cfgScene.Disable(NK_SS_OVERLAY);
	cfgScene.Enable(NK_SS_OFFSCREEN);
	NkRenderer *renderer = NkRenderer::Create(device, cfgScene);
	if (renderer == nullptr || !renderer->Initialize()) {
		std::printf("[Cible] le renderer de la scene n'a pas pu s'initialiser\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	// ---- LES DEUX APPELS ----
	// 1. OU dessiner : la cible hors ecran.
	renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));
	// 2. EN QUELLE TAILLE : la taille de la cible. Sans cette ligne, le renderer garde la taille de la fenetre.
	if (!sansTaille) {
		renderer->SetRenderSizeOverride(gLargeurCible, gHauteurCible);
	}

	// Le fil de garde : il regarde le compteur d'images terminees. Si le fil principal n'avance plus depuis
	// trois secondes, il l'ecrit (une seule fois) et laisse le processus en vie pour le debogueur.
	std::atomic<bool> fini{ false };
	std::thread garde([&]() {
		uint64 vu = 0;
		auto dernierProgres = std::chrono::steady_clock::now();
		bool ecrit = false;
		while (!fini.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			const uint64 maintenant = gTerminees.load();
			if (maintenant == 0) {
				continue;   // la toute premiere image compile les nuanceurs : elle peut durer plusieurs secondes
			}
			if (maintenant != vu) {
				vu = maintenant;
				dernierProgres = std::chrono::steady_clock::now();
				continue;
			}
			const auto attente = std::chrono::steady_clock::now() - dernierProgres;
			if (!ecrit && attente > std::chrono::seconds(3)) {
				std::printf("image : %llu, etat : figee\n", (unsigned long long)(vu + 1));
				ecrit = true;
			}
			// Hygiene : un processus fige ne reste pas en vie pour toujours.
			if (attente > std::chrono::seconds(EnvU64("CIBLE_ATTENTE_MAX", 300))) {
				std::_Exit(2);
			}
		}
	});

	bool tourne = true;
	for (uint64 image = 1; tourne && image <= nombreImages && fenetre.IsOpen(); ++image) {
		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			}
		}
		if (!tourne) break;

		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			RendreLaScene(renderer, cmd, (float32)gLargeurCible / (float32)gHauteurCible, (float32)image / 60.f);
			if (auto *r2d = rEcran->GetRender2D()) {
				r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float32)devInit.width, (float32)devInit.height });
			}
			rEcran->Present();
			rEcran->EndFrame();
		}
		gTerminees.store(image);
		std::printf("image : %llu, etat : dessinee\n", (unsigned long long)image);
	}

	fini.store(true);
	garde.join();

	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
