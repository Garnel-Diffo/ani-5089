// Exercice 7 : la latence injectee, et la borne. Le simulateur du moteur garde les 64 dernieres poses de la tete dans un
// tableau circulaire, chacune avec son instant et sa vitesse, et pour simuler une latence il ne rend pas la pose la plus
// recente mais celle d'il y a "latence" secondes. La prediction doit alors combler l'ecart, et c'est ce qu'on mesure.
// Tout est ecrit ici sans fenetre, en secondes, au demarrage de la salle, et imprime par std::printf.
//
//   le geste : un quart de tour en 400 ms, commence a t = 1 s ; une pose par periode de 72 Hz, pendant trois secondes ;
//   a chaque instant t, on prend l'echantillon rendu pour une latence donnee, on predit la pose de t + periode, et on
//   mesure l'angle avec la vraie pose de cet instant, avec la prediction bornee a 100 ms, puis sans prediction ;
//   enfin la borne est retiree et on injecte une latence d'une seconde.
//
// MASALLE_CALCUL_SEUL=1 s'arrete apres le calcul, sans ouvrir la salle ; MASALLE_EXIT=<image> arrete la salle sinon.
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

	const double kPi = 3.14159265358979323846;
	const double kPeriode = 1.0 / 72.0;			// une pose par periode d'un casque a 72 Hz, en secondes

	// Un quart de tour en 400 ms, depart et arrivee doux. t en secondes depuis le debut, lacet en degres.
	double Lacet(double t) {
		const double amplitude = 90.0, duree = 0.400;
		if (t <= 0.0) return 0.0;
		if (t >= duree) return amplitude;
		return amplitude * 0.5 * (1.0 - std::cos(kPi * t / duree));
	}

	// La vraie pose de l'instant t, avec le geste commence a t = 1 s.
	NkQuatf PoseVraie(double t) { return NkQuatf::RotateY(NkAngle((float32)Lacet(t - 1.0))); }

	// L'angle entre deux orientations, en degres : atan2 de la partie vectorielle (Angle() = 2 acos |w| ne distingue plus
	// les angles de moins de 0,04 degre en float32, voir l'exercice 6).
	double AngleEntre(const NkQuatf &a, const NkQuatf &b) {
		const NkQuatf d = (b * a.Conjugate()).Normalized();
		const double v = std::sqrt((double)d.x * d.x + (double)d.y * d.y + (double)d.z * d.z);
		return 2.0 * std::atan2(v, std::fabs((double)d.w)) * 180.0 / kPi;
	}

	// Le lacet d'une rotation autour de l'axe vertical : 2 * atan2(q.y, q.w), en degres, entre -180 et 180.
	double LacetEnDegres(const NkQuatf &q) { return 2.0 * std::atan2((double)q.y, (double)q.w) * 180.0 / kPi; }

	// ---- Le simulateur : les 64 dernieres poses, en tableau circulaire ----------------------------------------------
	struct PoseDatee {
		double temps = 0.0;				// secondes
		NkQuatf orientation;
		NkVec3f vitesse;				// vitesse angulaire, rad/s
	};

	const uint32 kMaxSamples = 64;

	struct Simulateur {
		PoseDatee mSamples[kMaxSamples];
		uint32 mSampleHead = 0, mSampleCount = 0;
		float32 mLatencySeconds = 0.f;

		// Une nouvelle pose : la vitesse vient de la pose d'avant.
		void Pousser(double temps, const NkQuatf &orientation) {
			NkVec3f vitesse(0.f, 0.f, 0.f);
			if (mSampleCount > 0u) {
				const PoseDatee &avant = mSamples[mSampleHead];
				vitesse = VitesseAngulaire(avant.orientation, orientation, (float32)(temps - avant.temps));
			}
			mSampleHead = (mSampleCount == 0u) ? 0u : (mSampleHead + 1u) % kMaxSamples;
			mSamples[mSampleHead].temps = temps;
			mSamples[mSampleHead].orientation = orientation;
			mSamples[mSampleHead].vitesse = vitesse;
			if (mSampleCount < kMaxSamples) ++mSampleCount;
		}

		// L'echantillon que le simulateur rend, tel que l'enonce l'ecrit, en secondes.
		const PoseDatee &SampleAt(double now) const {
			// Latence nulle : l'echantillon le plus recent, point.
			if (mLatencySeconds <= 0.f || mSampleCount <= 1u) {
				return mSamples[mSampleHead];
			}
			// Latence simulee : on rend l'echantillon d'il y a "latence" secondes.
			const double target = now - (double)mLatencySeconds;
			uint32 index = mSampleHead;
			for (uint32 i = 0u; i < mSampleCount; ++i) {
				if (mSamples[index].temps <= target) {
					return mSamples[index];
				}
				index = (index + kMaxSamples - 1u) % kMaxSamples;
			}
			// Plus vieux disponible : l'historique ne remonte pas si loin.
			return mSamples[(mSampleHead + kMaxSamples - (mSampleCount - 1u)) % kMaxSamples];
		}
	};

	// La pose predite a l'instant demande. Avec la borne, la fenetre d'extrapolation est de 100 ms au plus (dans les deux
	// sens) ; sans la borne, dt va ou il veut.
	NkQuatf PredictAt(const PoseDatee &echantillon, double quand, bool borne) {
		float32 dt = (float32)(quand - echantillon.temps);
		if (borne) dt = math::NkClamp(dt, -0.1f, 0.1f);
		return Extrapoler(echantillon.orientation, echantillon.vitesse, dt);
	}

	struct Resultat {
		double moyAvec = 0.0, maxAvec = 0.0, moySans = 0.0, maxSans = 0.0;
		int images = 0;
	};

	// Le geste, trois secondes de poses a 72 Hz ; a chaque instant t de 1,0 a 1,9 s, avec la latence donnee :
	// l'echantillon rendu, la prediction de t + periode, l'angle avec la vraie pose, et l'angle sans prediction.
	Resultat Mesurer(float32 latenceSecondes, bool borne) {
		Simulateur sim;
		sim.mLatencySeconds = latenceSecondes;
		Resultat r;
		for (int k = 0; (double)k * kPeriode <= 3.0; ++k) {
			const double t = (double)k * kPeriode;
			sim.Pousser(t, PoseVraie(t));
			if (t < 1.0 - 1e-9 || t > 1.9 + 1e-9) continue;
			const PoseDatee &s = sim.SampleAt(t);
			const NkQuatf vraie = PoseVraie(t + kPeriode);
			const double eAvec = AngleEntre(PredictAt(s, t + kPeriode, borne), vraie);
			const double eSans = AngleEntre(s.orientation, vraie);	// sans prediction : la pose de l'echantillon, telle quelle
			r.moyAvec += eAvec;
			r.moySans += eSans;
			r.maxAvec = std::max(r.maxAvec, eAvec);
			r.maxSans = std::max(r.maxSans, eSans);
			++r.images;
		}
		r.moyAvec /= r.images;
		r.moySans /= r.images;
		return r;
	}

	// La latence d'une seconde, sans ou avec la borne : l'image dont l'echantillon est le plus proche du milieu du geste
	// (t = 1,2 s), l'instant de cet echantillon, l'horizon de la prediction, le lacet predit et le vrai.
	void LatenceUneSeconde(bool borne) {
		Simulateur sim;
		sim.mLatencySeconds = 1.0f;
		double meilleurEcart = 1e30;
		double instantEchantillon = 0.0, horizon = 0.0, predit = 0.0, vrai = 0.0;
		for (int k = 0; (double)k * kPeriode <= 3.0; ++k) {
			const double t = (double)k * kPeriode;
			sim.Pousser(t, PoseVraie(t));
			const PoseDatee &s = sim.SampleAt(t);
			if (std::fabs(s.temps - 1.2) < meilleurEcart) {
				meilleurEcart = std::fabs(s.temps - 1.2);
				instantEchantillon = s.temps;
				horizon = (t + kPeriode) - s.temps; // ce que le code a reellement enregistre
				predit = LacetEnDegres(PredictAt(s, t + kPeriode, borne));
				vrai = LacetEnDegres(PoseVraie(t + kPeriode));
			}
		}
		std::printf("%s : sample at %.3f s, horizon %.0f ms, predicted %.1f deg, true %.1f deg\n", borne ? "with bound" : "no bound",
					instantEchantillon, horizon * 1e3, predit, vrai);
	}

} // namespace

int nkmain(const NkEntryState &state) {
	(void)state;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// ---- Les cinq latences, avec la borne de cent millisecondes -----------------------------------------------------
	const int latencesMs[5] = { 0, 10, 20, 40, 80 };
	for (int ms : latencesMs) {
		const Resultat r = Mesurer((float32)ms * 1e-3f, true);
		std::printf("latency %d ms : with %.4f deg (max %.4f), without %.4f deg (max %.4f)\n", ms, r.moyAvec, r.maxAvec, r.moySans,
					r.maxSans);
	}

	// ---- La latence d'une seconde, sans puis avec la borne ------------------------------------------------------------
	LatenceUneSeconde(false);
	LatenceUneSeconde(true);

	// ---- Pour la reponse : ou la prediction cesse de rattraper la latence --------------------------------------------
	// Pour chaque latence de 0 a 400 ms, par pas de 10 : l'erreur moyenne sans prediction, avec la borne et sans la borne.
	// Le critere : la prediction "rattrape" tant que son erreur moyenne reste sous celle de la pose telle quelle.
	std::printf("[MaSalle] balayage (latence : moyenne sans prediction, avec borne, sans borne), %d images par latence :\n",
				Mesurer(0.f, true).images);
	// Deux criteres, nommes : (A) la prediction ne rattrape plus que la MOITIE ou moins de l'erreur de la pose telle quelle
	// (erreur avec >= la moitie de l'erreur sans) ; (B) elle est PIRE que rien (erreur avec >= erreur sans). -1 : jamais
	// jusqu'a 400 ms.
	int moitieBorne = -1, moitieLibre = -1, pireBorne = -1, pireLibre = -1;
	for (int ms = 0; ms <= 400; ms += 10) {
		const Resultat borne = Mesurer((float32)ms * 1e-3f, true), libre = Mesurer((float32)ms * 1e-3f, false);
		if (moitieBorne < 0 && borne.moyAvec >= 0.5 * borne.moySans) moitieBorne = ms;
		if (moitieLibre < 0 && libre.moyAvec >= 0.5 * libre.moySans) moitieLibre = ms;
		if (pireBorne < 0 && borne.moyAvec >= borne.moySans) pireBorne = ms;
		if (pireLibre < 0 && libre.moyAvec >= libre.moySans) pireLibre = ms;
		if (ms % 20 == 0 || ms <= 100) {
			std::printf("[MaSalle]   %3d ms : %.4f  %.4f  %.4f deg\n", ms, borne.moySans, borne.moyAvec, libre.moyAvec);
		}
	}
	std::printf("[MaSalle] critere A (la prediction ne rattrape plus que la moitie ou moins) : des %d ms avec la borne, des %d ms "
				"sans la borne\n", moitieBorne, moitieLibre);
	std::printf("[MaSalle] critere B (la prediction est pire que rien) : des %d ms avec la borne (-1 : jamais jusqu'a 400 ms), "
				"des %d ms sans la borne\n", pireBorne, pireLibre);
	// Et la latence a partir de laquelle l'horizon depasse la borne de 100 ms pour au moins une image.
	for (int ms = 0; ms <= 400; ms += 10) {
		Simulateur sim;
		sim.mLatencySeconds = (float32)ms * 1e-3f;
		double horizonMax = 0.0;
		for (int k = 0; (double)k * kPeriode <= 3.0; ++k) {
			const double t = (double)k * kPeriode;
			sim.Pousser(t, PoseVraie(t));
			if (t < 1.0 - 1e-9 || t > 1.9 + 1e-9) continue;
			horizonMax = std::max(horizonMax, (t + kPeriode) - sim.SampleAt(t).temps);
		}
		if (horizonMax > 0.1) {
			std::printf("[MaSalle] l'horizon depasse 100 ms (la borne) des %d ms de latence : %.1f ms au plus\n", ms, horizonMax * 1e3);
			break;
		}
	}

	if (getenv("MASALLE_CALCUL_SEUL") != nullptr) {
		return 0;
	}

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
			const NkCamera3DData cd = CameraDe({ 0.30f, kYeux, 1.20f }, { -1.f, -0.6f, -3.2f }, { 0.f, 1.f, 0.f },
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
