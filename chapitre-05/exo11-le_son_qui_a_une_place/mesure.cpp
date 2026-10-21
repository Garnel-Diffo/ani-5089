// Exercice 11 (mesure) : ce que le moteur envoie a chaque oreille. Ni fenetre, ni sortie sonore :
// le mixeur tourne dans un programme sans haut-parleur (AudioBackendType::NULL_OUTPUT) et on lit ce qu'il produit.
#include "NKAudio/NkAudio.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::audio;

struct Oreilles {
    double gauche;
    double droite;
};

// Joue la voix, rend 48 000 images (une seconde) par blocs de 256 avec RenderToBuffer, et rend la valeur
// efficace (racine de la moyenne des carres) de l'oreille gauche puis de la droite.
// Les echantillons sont ranges gauche, droite, gauche, droite...
static Oreilles Mesurer(const AudioSample &son, const VoiceParams &params) {
    AudioEngine &moteur = AudioEngine::Instance();
    const AudioHandle voix = moteur.Play(son, params);

    const int TOTAL = 48000, BLOC = 256;
    std::vector<float> tampon(static_cast<size_t>(BLOC) * 2);
    double sommeG = 0.0, sommeD = 0.0;
    long n = 0;
    for (int fait = 0; fait < TOTAL; fait += BLOC) {
        const int images = std::min(BLOC, TOTAL - fait);
        moteur.RenderToBuffer(tampon.data(), images, 2);
        for (int i = 0; i < images; ++i) {
            const double g = tampon[2 * i], d = tampon[2 * i + 1];
            sommeG += g * g;
            sommeD += d * d;
        }
        n += images;
    }
    moteur.Stop(voix);   // la source ne bouge pas pendant une mesure : on l'arrete avant de la placer ailleurs
    return {std::sqrt(sommeG / n), std::sqrt(sommeD / n)};
}

int main() {
    AudioEngineConfig cfg;
    cfg.backend    = AudioBackendType::NULL_OUTPUT;   // pas de sortie sonore
    cfg.sampleRate = 48000;
    if (!AudioEngine::Instance().Initialize(cfg)) {
        std::printf("moteur sonore non initialise\n");
        return 1;
    }
    // L'auditeur est a l'origine et regarde vers -Z : devant (0,0,-1), haut (0,1,0).
    AudioEngine::Instance().SetListenerPosition(0.f, 0.f, 0.f);
    AudioEngine::Instance().SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);

    AudioSample pluie = AudioGenerator::GenerateNoise(4.f, WaveformType::NOISE_PINK);

    // La pluie a deux metres : a gauche, devant, a droite.
    struct Place { const char *nom; float x, y, z; };
    const Place places[3] = {{"gauche", -2.f, 0.f, 0.f}, {"devant", 0.f, 0.f, -2.f}, {"droite", 2.f, 0.f, 0.f}};

    std::FILE *sortie = std::fopen("mesures.txt", "w");
    if (!sortie) return 1;
    // D'abord avec `positional` a vrai (le chiffre apres le nom vaut 1), puis a faux (0).
    for (int positional = 1; positional >= 0; --positional) {
        for (const Place &p : places) {
            VoiceParams params;
            params.looping                  = true;
            params.source3d.position[0]     = p.x;
            params.source3d.position[1]     = p.y;
            params.source3d.position[2]     = p.z;
            params.source3d.positional      = (positional == 1);
            params.source3d.minDistance     = 1.0f;
            params.source3d.maxDistance     = 6.0f;
            const Oreilles o = Mesurer(pluie, params);
            std::fprintf(sortie, "%s %d : G %.4f D %.4f\n", p.nom, positional, o.gauche, o.droite);
            std::printf("%s %d : G %.4f D %.4f\n", p.nom, positional, o.gauche, o.droite);
        }
    }
    std::fclose(sortie);

    AudioEngine::Instance().Shutdown();
    AudioLoader::Free(pluie);
    return 0;
}
