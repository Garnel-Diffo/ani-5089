// Exercice 5 : quatre filtres, une mesure.
// Reduit la meme image de 2048 a 256 pixels de cote avec les quatre filtres du moteur,
// chronometre chacun (moyenne de vingt reductions), puis compare les quatre resultats au
// resultat bilineaire, pixel par pixel.
#include "NKImage/Core/NkImage.h"
#include "NKTime/NkChrono.h"

#include <cstdio>

using namespace nkentseu;

// Nombre de pixels qui different, sur au moins un canal, entre deux images de meme taille.
static long Differences(const NkImage &a, const NkImage &b) {
    long n = 0;
    for (int y = 0; y < a.Height(); ++y) {
        for (int x = 0; x < a.Width(); ++x) {
            const math::NkColor p = a.GetPixel(x, y);
            const math::NkColor q = b.GetPixel(x, y);
            if (p.r != q.r || p.g != q.g || p.b != q.b || p.a != q.a) ++n;
        }
    }
    return n;
}

struct Filtre {
    const char    *nom;
    const char    *fichier;
    NkResizeFilter filtre;
};

int main() {
    // 1. La photo, reduite une fois a 2048 x 2048 : c'est la source de toutes les mesures.
    NkImage photo;
    if (!photo.Load("photo.jpg")) {
        std::printf("photo.jpg introuvable ou illisible\n");
        return 1;
    }
    const NkImage source = photo.Resize(2048, 2048);
    if (!source.IsValid()) return 1;
    std::printf("photo %d x %d, source %d x %d\n", photo.Width(), photo.Height(), source.Width(), source.Height());

    const Filtre filtres[4] = {
        {"NK_NEAREST",  "nearest.png",  NkResizeFilter::NK_NEAREST},
        {"NK_BILINEAR", "bilinear.png", NkResizeFilter::NK_BILINEAR},
        {"NK_BICUBIC",  "bicubic.png",  NkResizeFilter::NK_BICUBIC},
        {"NK_LANCZOS3", "lanczos3.png", NkResizeFilter::NK_LANCZOS3},
    };
    const int REPETITIONS = 20;

    NkImage resultats[4];
    double  moyennes[4] = {};

    for (int f = 0; f < 4; ++f) {
        // La premiere reduction paie ce que les suivantes ne paient pas (allocations, caches) :
        // on la fait une fois sans la compter, puis on chronometre vingt reductions.
        NkImage chauffe = source.Resize(256, 256, filtres[f].filtre);
        (void)chauffe;

        double total = 0.0;
        for (int i = 0; i < REPETITIONS; ++i) {
            NkChrono chrono;
            NkImage  reduite = source.Resize(256, 256, filtres[f].filtre);
            total += chrono.Elapsed().ToMicroseconds();
            if (i == REPETITIONS - 1) resultats[f] = static_cast<NkImage &&>(reduite);
        }
        moyennes[f] = total / REPETITIONS;
        if (!resultats[f].IsValid() || !resultats[f].SavePNG(filtres[f].fichier)) {
            std::printf("echec avec %s\n", filtres[f].nom);
            return 1;
        }
    }

    // 2. Chaque resultat compare au resultat bilineaire (indice 1), pixel par pixel.
    std::FILE *sortie = std::fopen("mesures.txt", "w");
    if (!sortie) return 1;
    for (int f = 0; f < 4; ++f) {
        const long d = Differences(resultats[f], resultats[1]);
        std::fprintf(sortie, "%s : %.0f us, differences : %ld\n", filtres[f].nom, moyennes[f], d);
        std::printf("%s : %.1f us, differences : %ld sur %d pixels\n", filtres[f].nom, moyennes[f], d,
                    resultats[f].Width() * resultats[f].Height());
    }
    std::fclose(sortie);
    return 0;
}
