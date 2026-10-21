// Exercice 12 : les quatre attenuations, mesurees avec le moteur lui-meme.
// Pas de fenetre, pas de sortie sonore (AudioBackendType::NULL_OUTPUT) : on rend le melange avec RenderToBuffer.
// L'auditeur est a l'origine et regarde vers -Z. La source est devant lui, a la distance d.
#include "NKAudio/NkAudio.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::audio;

// Valeur efficace des DEUX oreilles ensemble, sur 48 000 images rendues par blocs de 256.
static double Mesurer(const AudioSample &son, const VoiceParams &params) {
    AudioEngine &moteur = AudioEngine::Instance();
    const AudioHandle voix = moteur.Play(son, params);

    const int TOTAL = 48000, BLOC = 256;
    std::vector<float> tampon(static_cast<size_t>(BLOC) * 2);
    double somme = 0.0;
    long n = 0;
    for (int fait = 0; fait < TOTAL; fait += BLOC) {
        const int images = std::min(BLOC, TOTAL - fait);
        moteur.RenderToBuffer(tampon.data(), images, 2);
        for (int i = 0; i < 2 * images; ++i) somme += static_cast<double>(tampon[i]) * tampon[i];
        n += 2L * images;   // gauche et droite comptent ensemble
    }
    moteur.Stop(voix);      // la source ne bouge pas pendant une mesure : arretee avant d'etre replacee
    return std::sqrt(somme / n);
}

int main() {
    AudioEngineConfig cfg;
    cfg.backend    = AudioBackendType::NULL_OUTPUT;
    cfg.sampleRate = 48000;
    if (!AudioEngine::Instance().Initialize(cfg)) {
        std::printf("moteur sonore non initialise\n");
        return 1;
    }
    AudioEngine::Instance().SetListenerPosition(0.f, 0.f, 0.f);
    AudioEngine::Instance().SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);

    // Un sinus doux : l'amplitude reste basse, car le moteur adoucit les valeurs fortes en sortie.
    AudioSample son = AudioGenerator::GenerateTone(440.f, 2.f, WaveformType::SINE, 48000, 0.1f);

    const AttenuationModel modeles[4] = {AttenuationModel::NONE, AttenuationModel::INVERSE,
                                         AttenuationModel::LINEAR, AttenuationModel::EXPONENTIAL};
    const char *noms[4] = {"NONE", "INVERSE", "LINEAR", "EXPONENTIAL"};
    const float distances[5] = {0.5f, 1.f, 2.f, 4.f, 8.f};
    const float rolloffs[2] = {1.f, 2.f};

    std::FILE *sortie = std::fopen("mesures.txt", "w");
    if (!sortie) return 1;
    for (float rolloff : rolloffs) {
        // Les quatre modeles a chaque distance ; le gain est la mesure divisee par celle du modele NONE
        // a la meme distance.
        double mesure[4][5];
        for (int m = 0; m < 4; ++m) {
            for (int d = 0; d < 5; ++d) {
                VoiceParams params;
                params.looping                   = true;
                params.source3d.positional       = true;
                params.source3d.minDistance      = 1.f;
                params.source3d.maxDistance      = 100.f;
                params.source3d.rolloffFactor    = rolloff;
                params.source3d.attenuationModel = modeles[m];
                params.source3d.position[0]      = 0.f;
                params.source3d.position[1]      = 0.f;
                params.source3d.position[2]      = -distances[d];
                mesure[m][d] = Mesurer(son, params);
            }
        }
        for (int m = 0; m < 4; ++m) {
            for (int d = 0; d < 5; ++d) {
                const double gain = mesure[m][d] / mesure[0][d];
                char distance[16];
                std::snprintf(distance, sizeof distance, "%g", static_cast<double>(distances[d]));
                std::fprintf(sortie, "%s, rolloff %g, %s m : %.3f\n", noms[m], static_cast<double>(rolloff), distance, gain);
                std::printf("%s, rolloff %g, %s m : %.3f   (RMS %.5f, NONE %.5f)\n", noms[m], static_cast<double>(rolloff),
                            distance, gain, mesure[m][d], mesure[0][d]);
            }
        }
    }
    std::fclose(sortie);

    AudioEngine::Instance().Shutdown();
    AudioLoader::Free(son);
    return 0;
}
