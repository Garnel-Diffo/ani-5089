// Exercice 10 : deux chaines, deux yeux, et le redimensionnement.
//
// Deux chaines d'echange independantes, une par oeil, chacune avec SON anneau, SON booleen d'emprunt et SES
// poignees. Chaque image suit les quatre temps pour chaque oeil :
//     1. AcquireImage   (prete-moi une image, laquelle ?)
//     2. le rendu       (on dessine dedans, et on est seul a le faire)
//     3. ReleaseImage   (j'ai fini, elle est a toi)
//     4. la soumission  (affiche la DERNIERE image rendue, GetLastReleasedIndex, pas celle sur laquelle l'anneau pointe)
// Les images sont des cibles hors ecran fabriquees par l'application (c'est ce que fait un simulateur : avec un vrai
// casque, ce serait le runtime) ; la chaine ne fait que retenir leurs poignees, des entiers de 64 bits sans type.
//
// Au redimensionnement de la fenetre, on ne se contente pas de redimensionner les cibles : on DETRUIT les deux
// chaines, on les RECREE, et on REENREGISTRE leurs images. Une chaine neuve a un tableau de poignees plein de zeros.
//
// Le programme ecrit lui-meme les deux fichiers rendus, dans le dossier courant :
//     trace.txt    une ligne par image, de la forme  `image : 12, gauche : 1, droit : 1`  (l'index prete a chaque oeil)
//     resize.txt   une operation par ligne, dans l'ordre : REDIMENSIONNE, DETRUIT, RECREE, REENREGISTRE
//
// Variables d'environnement (toutes facultatives) :
//     DEUX_CHAINES_IMAGES=N         nombre d'images a tracer (30 par defaut)
//     DEUX_CHAINES_REDIM_APRES=K    le redimensionnement est demande apres l'image K (13 par defaut)
//     DEUX_CHAINES_ERREUR=...       provoque les deux fautes classiques, pour les mesurer :
//                                     sans_recreation        redimensionne les cibles, ne refait pas les chaines
//                                     sans_reenregistrement  refait les chaines, oublie d'enregistrer les images
//     DEUX_CHAINES_MESURE=1         relit les pixels de ce qui est affiche et compte ceux qui ne sont pas noirs

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

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static void ConfigureAppData(NkAppData &d) { d.appName = "DeuxChaines"; }
NK_REGISTER_ENTRY_APPDATA_UPDATER(ConfigureAppData)

namespace {

	// ------------------------------------------------------------------------------------------------------------
	// La chaine d'echange (celle du banc de l'exercice 8) : un anneau d'au plus quatre images dans un tableau a
	// taille fixe (zero allocation), un booleen d'emprunt, deux methodes qui REFUSENT en retournant false.
	// ------------------------------------------------------------------------------------------------------------
	class ChaineEchange {
		public:
			static constexpr std::uint32_t kImagesMax = 4;

			bool Create(std::uint32_t nombreImages, const char *nom) noexcept {
				if (nombreImages == 0 || nombreImages > kImagesMax) {
					return false;
				}
				Destroy();
				mImageCount = nombreImages;
				mNom = nom;
				return true;
			}

			// Tout est remis a zero, poignees comprises : c'est pourquoi il faut RE-enregistrer apres une recreation.
			void Destroy() noexcept {
				for (std::uint32_t i = 0; i < kImagesMax; ++i) {
					mHandles[i] = 0;
				}
				mImageCount = 0;
				mNextImage = 0;
				mAcquise = false;
			}

			bool IsValid() const noexcept { return mImageCount != 0; }
			std::uint32_t GetImageCount() const noexcept { return mImageCount; }
			const char *GetName() const noexcept { return mNom; }

			bool RegisterImage(std::uint32_t index, std::uint64_t handle) noexcept {
				if (index >= mImageCount) {
					return false;
				}
				mHandles[index] = handle;
				return true;
			}

			std::uint64_t GetImageHandle(std::uint32_t index) const noexcept {
				return index < mImageCount ? mHandles[index] : 0;
			}

			// Temps 1 : refuse le double emprunt.
			bool AcquireImage(std::uint32_t &index) noexcept {
				if (!IsValid() || mAcquise) {
					return false;
				}
				mAcquise = true;
				index = mNextImage;
				return true;
			}

			// Temps 3 : refuse de rendre ce qui n'a pas ete emprunte ; rend l'image ET avance l'anneau.
			bool ReleaseImage() noexcept {
				if (!IsValid() || !mAcquise) {
					return false;
				}
				mAcquise = false;
				mNextImage = (mNextImage + 1u) % mImageCount;
				return true;
			}

			// Temps 4 : ReleaseImage a deja avance, il faut reculer d'un cran.
			std::uint32_t GetLastReleasedIndex() const noexcept {
				return IsValid() ? (mNextImage + mImageCount - 1u) % mImageCount : 0u;
			}

		private:
			std::uint64_t mHandles[kImagesMax] = { 0, 0, 0, 0 };
			std::uint32_t mImageCount = 0;
			std::uint32_t mNextImage = 0;
			bool mAcquise = false;
			const char *mNom = "";
	};

	// Un oeil : SA chaine, et les images que l'application a fabriquees pour elle. Rien n'est partage avec l'autre oeil.
	struct Oeil {
		const char *nom = "";				  // "oeil gauche" ou "oeil droit" : le nom ne sert pas au programme, il sert aux outils de mise au point
		float decalage = 0.f;				  // position de l'oeil par rapport au centre de la tete, en metres
		ChaineEchange chaine;
		NkOffscreenTarget *cibles[ChaineEchange::kImagesMax] = { nullptr, nullptr, nullptr, nullptr };
		std::uint32_t indexPrete = 0;		  // l'index prete a l'image courante
	};

	const std::uint32_t kNombreImagesParChaine = 3;	  // un anneau de trois images, comme dans les exercices precedents

	uint64 EnvU64(const char *nom, uint64 defaut) {
		const char *t = getenv(nom);
		if (t == nullptr || *t == '\0') return defaut;
		return (uint64)strtoull(t, nullptr, 10);
	}

	bool EnvEgale(const char *nom, const char *valeur) {
		const char *t = getenv(nom);
		return t != nullptr && std::strcmp(t, valeur) == 0;
	}

	// ------------------------------------------------------------------------------------------------------------
	// La scene : un sol et trois paves, vue depuis un oeil decale de `decalage` metres.
	// ------------------------------------------------------------------------------------------------------------
	void Pave(NkRender3D *r3d, NkMeshSystem *meshes, const NkVec3f &centre, const NkVec3f &taille, const NkVec3f &teinte) {
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

	// Le rendu d'un oeil sur le command buffer de l'ecran, dans la cible que le graphe vise (SetFinalColorTarget).
	void RendreLaScene(NkRenderer *renderer, NkICommandBuffer *cmd, float32 aspect, float32 temps, float32 decalage) {
		NkRender3D *r3d = renderer->GetRender3D();
		renderer->FlushGraphRebuilds();
		r3d->ResetFrame();
		if (auto *mc = renderer->GetMaterialCollection()) {
			mc->Upload();
		}

		NkCamera3DData cd;
		cd.position = { decalage, 1.7f, 3.f };
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

	// ------------------------------------------------------------------------------------------------------------
	// Fabrication et destruction des images et des chaines. Chaque operation ecrit sa ligne dans resize.txt.
	// ------------------------------------------------------------------------------------------------------------
	void Ecrire(std::FILE *f, const char *format, ...) __attribute__((format(printf, 2, 3)));
	void Ecrire(std::FILE *f, const char *format, ...) {
		if (f == nullptr) return;
		va_list args;
		va_start(args, format);
		std::vfprintf(f, format, args);
		va_end(args);
		std::fputc('\n', f);
	}

	void DetruireLaChaine(Oeil &oeil, std::FILE *journal) {
		oeil.chaine.Destroy();
		Ecrire(journal, "DETRUIT chaine %s", oeil.nom);
	}

	void DetruireLesImages(NkRenderer *rEcran, Oeil &oeil, std::FILE *journal) {
		for (NkOffscreenTarget *&cible : oeil.cibles) {
			if (cible != nullptr) {
				rEcran->DestroyOffscreen(cible);
			}
		}
		Ecrire(journal, "DETRUIT images %s", oeil.nom);
	}

	bool CreerLesImages(NkRenderer *rEcran, Oeil &oeil, uint32 largeur, uint32 hauteur, bool relecture, std::FILE *journal) {
		for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
			NkOffscreenDesc od;
			od.width = largeur;
			od.height = hauteur;
			od.hasDepth = true;
			od.hdr = false;
			od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
			od.readable = true;
			od.readback = relecture;
			od.name = oeil.nom;
			oeil.cibles[i] = rEcran->CreateOffscreen(od);
			if (oeil.cibles[i] == nullptr) {
				return false;
			}
		}
		Ecrire(journal, "RECREE images %s : %u de %u x %u", oeil.nom, (unsigned)kNombreImagesParChaine, (unsigned)largeur,
			   (unsigned)hauteur);
		return true;
	}

	bool CreerLaChaine(Oeil &oeil, std::FILE *journal) {
		const bool ok = oeil.chaine.Create(kNombreImagesParChaine, oeil.nom);
		Ecrire(journal, "RECREE chaine %s : %u images", oeil.nom, (unsigned)kNombreImagesParChaine);
		return ok;
	}

	// La poignee d'une image, telle que le moteur la connait : un entier de 64 bits (0 = invalide), que la chaine range
	// sans jamais l'interpreter.
	std::uint64_t PoigneeDe(NkRenderer *rEcran, NkOffscreenTarget *cible) {
		return rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()).id;
	}

	void EnregistrerLesImages(NkRenderer *rEcran, Oeil &oeil, std::FILE *journal) {
		for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
			const std::uint64_t poignee = PoigneeDe(rEcran, oeil.cibles[i]);
			oeil.chaine.RegisterImage(i, poignee);
			Ecrire(journal, "REENREGISTRE chaine %s image %u : poignee %llu", oeil.nom, (unsigned)i, (unsigned long long)poignee);
		}
	}

	// Compte les pixels qui ne sont pas noirs dans l'image `index` de l'oeil (relecture depuis la carte).
	bool CompterLesPixels(Oeil &oeil, std::uint32_t index, std::uint64_t &allumes, std::uint64_t &total) {
		NkOffscreenTarget *cible = oeil.cibles[index];
		if (cible == nullptr) return false;
		const uint32 l = cible->GetWidth();
		const uint32 h = cible->GetHeight();
		std::vector<uint8> pixels((size_t)l * h * 4, 0);
		if (!cible->ReadbackPixels(pixels.data(), l * 4)) return false;
		allumes = 0;
		total = (std::uint64_t)l * h;
		for (size_t i = 0; i < pixels.size(); i += 4) {
			if (pixels[i] > 8 || pixels[i + 1] > 8 || pixels[i + 2] > 8) ++allumes;
		}
		return true;
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	const uint64 nombreImages = EnvU64("DEUX_CHAINES_IMAGES", 30);
	const uint64 redimApres = EnvU64("DEUX_CHAINES_REDIM_APRES", 13);
	const bool sansRecreation = EnvEgale("DEUX_CHAINES_ERREUR", "sans_recreation");
	const bool sansReenregistrement = EnvEgale("DEUX_CHAINES_ERREUR", "sans_reenregistrement");
	const bool mesure = getenv("DEUX_CHAINES_MESURE") != nullptr;

	// Les deux fichiers rendus. Ouverts en binaire : un seul octet de fin de ligne, le meme partout.
	std::FILE *trace = std::fopen("trace.txt", "wb");
	std::FILE *journal = std::fopen("resize.txt", "wb");

	// La fenetre : 640 x 360 au depart ; chaque oeil en recoit la moitie, soit 320 x 360.
	uint32 largeurFenetre = 640;
	uint32 hauteurFenetre = 360;
	const uint32 largeurApres = 800;
	const uint32 hauteurApres = 450;

	NkWindowConfig cfg;
	cfg.title = "Deux chaines, deux yeux";
	cfg.width = largeurFenetre;
	cfg.height = hauteurFenetre;
	cfg.centered = true;
	cfg.vsync = true;
	NkWindow fenetre(cfg);
	if (!fenetre.IsValid()) {
		std::printf("[DeuxChaines] la fenetre n'a pas pu s'ouvrir\n");
		return 1;
	}

	NkDeviceInitInfo devInit{};
	devInit.surface = fenetre.GetSurfaceDesc();
	devInit.width = fenetre.GetSize().x;
	devInit.height = fenetre.GetSize().y;
	// DirectX 11 : les nuanceurs DirectX 12 de cette machine ne compilent pas (voir les exercices precedents).
	devInit.api = NkGraphicsApi::NK_GFX_API_DX11;

	NkIDevice *device = NkDeviceFactory::CreateForApi(devInit.api, devInit);
	if (device == nullptr) {
		std::printf("[DeuxChaines] aucune API graphique disponible\n");
		return 1;
	}
	NkRenderer *rEcran = NkRenderer::Create(device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
	if (rEcran == nullptr || !rEcran->Initialize()) {
		std::printf("[DeuxChaines] le renderer d'ecran n'a pas pu s'initialiser\n");
		NkDeviceFactory::Destroy(device);
		return 1;
	}
	NkRendererConfig cfgScene = NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
	cfgScene.Disable(NK_SS_RENDER2D);
	cfgScene.Disable(NK_SS_TEXT);
	cfgScene.Disable(NK_SS_OVERLAY);
	cfgScene.Enable(NK_SS_OFFSCREEN);
	NkRenderer *renderer = NkRenderer::Create(device, cfgScene);
	if (renderer == nullptr || !renderer->Initialize()) {
		std::printf("[DeuxChaines] le renderer de la scene n'a pas pu s'initialiser\n");
		NkRenderer::Destroy(rEcran);
		NkDeviceFactory::Destroy(device);
		return 1;
	}

	// ---- Deux chaines, une par oeil : chacune avec son anneau, ses poignees, ses images ----
	Oeil yeux[2];
	yeux[0].nom = "oeil gauche";
	yeux[0].decalage = -0.032f;
	yeux[1].nom = "oeil droit";
	yeux[1].decalage = +0.032f;

	for (Oeil &oeil : yeux) {
		if (!CreerLesImages(rEcran, oeil, largeurFenetre / 2, hauteurFenetre, mesure, nullptr) ||
			!oeil.chaine.Create(kNombreImagesParChaine, oeil.nom)) {
			std::printf("[DeuxChaines] impossible de creer la chaine de l'oeil %s\n", oeil.nom);
			return 1;
		}
		for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
			oeil.chaine.RegisterImage(i, PoigneeDe(rEcran, oeil.cibles[i]));
		}
	}
	// Les deux chaines sont-elles vraiment separees ? Aucune poignee en commun.
	bool separees = true;
	for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
		for (std::uint32_t j = 0; j < kNombreImagesParChaine; ++j) {
			if (yeux[0].chaine.GetImageHandle(i) == yeux[1].chaine.GetImageHandle(j)) separees = false;
		}
	}
	for (const Oeil &oeil : yeux) {
		std::printf("[DeuxChaines] chaine %s :", oeil.chaine.GetName());
		for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
			std::printf(" %llu", (unsigned long long)oeil.chaine.GetImageHandle(i));
		}
		std::printf("\n");
	}
	std::printf("[DeuxChaines] poignees en commun entre les deux chaines : %s\n", separees ? "aucune" : "OUI (faute)");

	// ---- La boucle : trente images, le redimensionnement demande apres l'image `redimApres` ----
	bool tourne = true;
	bool refaire = false;
	uint32 largeurVoulue = largeurFenetre;
	uint32 hauteurVoulue = hauteurFenetre;
	bool redimDemande = false;

	for (uint64 image = 1; tourne && image <= nombreImages && fenetre.IsOpen(); ++image) {
		NkEvent *ev = nullptr;
		while (NkEvents().PollEvent(ev)) {
			if (ev->As<NkWindowCloseEvent>() != nullptr) {
				tourne = false;
			} else if (auto *re = ev->As<NkWindowResizeEvent>()) {
				const uint32 l = re->GetWidth();
				const uint32 h = re->GetHeight();
				if (l > 1 && h > 1 && (l != largeurFenetre || h != hauteurFenetre)) {
					largeurVoulue = l;
					hauteurVoulue = h;
					refaire = true;   // on ne refait pas tout au milieu de la pompe : au debut de l'image suivante
				}
			}
		}
		if (!tourne) break;

		// ---- Le redimensionnement : ce qu'il faut refaire ----
		if (refaire) {
			refaire = false;
			Ecrire(journal, "REDIMENSIONNE fenetre : %u x %u -> %u x %u", (unsigned)largeurFenetre, (unsigned)hauteurFenetre,
				   (unsigned)largeurVoulue, (unsigned)hauteurVoulue);
			device->WaitIdle();   // la carte ne doit plus toucher aux images qu'on va detruire
			rEcran->OnResize(largeurVoulue, hauteurVoulue);
			renderer->OnResize(largeurVoulue, hauteurVoulue);
			largeurFenetre = largeurVoulue;
			hauteurFenetre = hauteurVoulue;
			const uint32 largeurOeil = largeurFenetre / 2;

			if (sansRecreation) {
				// LA FAUTE : on redimensionne les cibles, on ne refait pas les chaines. Les chaines gardent leurs poignees.
				for (Oeil &oeil : yeux) {
					for (std::uint32_t i = 0; i < kNombreImagesParChaine; ++i) {
						const std::uint64_t avant = oeil.chaine.GetImageHandle(i);
						oeil.cibles[i]->Resize(largeurOeil, hauteurFenetre);
						const std::uint64_t apres = PoigneeDe(rEcran, oeil.cibles[i]);
						std::printf("[mesure] sans recreation : oeil %s image %u : poignee de la chaine %llu, poignee de la cible %llu (%s)\n",
									oeil.nom, (unsigned)i, (unsigned long long)avant, (unsigned long long)apres,
									avant == apres ? "meme" : "DIFFERENTE");
					}
				}
			} else {
				for (Oeil &oeil : yeux) DetruireLaChaine(oeil, journal);
				for (Oeil &oeil : yeux) DetruireLesImages(rEcran, oeil, journal);
				for (Oeil &oeil : yeux) CreerLesImages(rEcran, oeil, largeurOeil, hauteurFenetre, mesure, journal);
				for (Oeil &oeil : yeux) CreerLaChaine(oeil, journal);
				if (!sansReenregistrement) {
					for (Oeil &oeil : yeux) EnregistrerLesImages(rEcran, oeil, journal);
				}
			}
		}

		// ---- L'image : les quatre temps pour chaque oeil ----
		if (rEcran->BeginFrame()) {
			NkICommandBuffer *cmd = rEcran->GetCmd();
			const uint32 largeurOeil = largeurFenetre / 2;
			for (Oeil &oeil : yeux) {
				if (!oeil.chaine.AcquireImage(oeil.indexPrete)) {									// temps 1
					std::printf("[DeuxChaines] image %llu : AcquireImage refuse pour l'oeil %s\n", (unsigned long long)image, oeil.nom);
					continue;
				}
				NkTextureHandle cible;
				cible.id = oeil.chaine.GetImageHandle(oeil.indexPrete);   // la poignee que la chaine a retenue (0 si oubliee)
				renderer->SetFinalColorTarget(cible);						// les DEUX appels : OU...
				renderer->SetRenderSizeOverride(largeurOeil, hauteurFenetre);	// ... et EN QUELLE TAILLE
				RendreLaScene(renderer, cmd, (float32)largeurOeil / (float32)hauteurFenetre, (float32)image / 60.f, oeil.decalage); // temps 2
				oeil.chaine.ReleaseImage();									// temps 3
			}
			if (auto *r2d = rEcran->GetRender2D()) {												// temps 4 : la soumission
				for (int e = 0; e < 2; ++e) {
					Oeil &oeil = yeux[e];
					NkOffscreenTarget *derniere = oeil.cibles[oeil.chaine.GetLastReleasedIndex()];
					r2d->DrawImage(derniere->GetColorResult(),
								   { (float32)(e * largeurOeil), 0.f, (float32)largeurOeil, (float32)hauteurFenetre });
				}
			}
			rEcran->Present();
			rEcran->EndFrame();
		}

		Ecrire(trace, "image : %llu, gauche : %u, droit : %u", (unsigned long long)image, (unsigned)yeux[0].indexPrete,
			   (unsigned)yeux[1].indexPrete);

		if (mesure) {
			for (Oeil &oeil : yeux) {
				std::uint64_t allumes = 0, total = 0;
				if (CompterLesPixels(oeil, oeil.chaine.GetLastReleasedIndex(), allumes, total)) {
					std::printf("[mesure] image %llu : oeil %s : %llu pixels non noirs sur %llu\n", (unsigned long long)image, oeil.nom,
								(unsigned long long)allumes, (unsigned long long)total);
				}
			}
		}

		// Le redimensionnement est DEMANDE ici, pendant que le programme tourne : la fenetre change de taille, le
		// systeme envoie l'evenement, et la boucle le lira au debut de l'image suivante.
		if (image == redimApres && !redimDemande) {
			redimDemande = true;
			fenetre.SetSize(largeurApres, hauteurApres);
		}
	}

	if (trace != nullptr) std::fclose(trace);
	if (journal != nullptr) std::fclose(journal);

	device->WaitIdle();
	for (Oeil &oeil : yeux) DetruireLesImages(rEcran, oeil, nullptr);
	NkRenderer::Destroy(renderer);
	NkRenderer::Destroy(rEcran);
	NkDeviceFactory::Destroy(device);
	return 0;
}
