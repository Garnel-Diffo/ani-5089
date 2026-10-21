// Exercice 1 : treize formats, une image.
// Le moteur ecrit la meme photo en cinq formats, relit chaque fichier, et on note ce qui revient.
#include "NKImage/Core/NkImage.h"

#include <cstdio>

using namespace nkentseu;

// Taille d'un fichier sur le disque, en octets (-1 s'il n'existe pas).
static long TailleSurDisque(const char *chemin) {
    FILE *f = std::fopen(chemin, "rb");
    if (!f) return -1;
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fclose(f);
    return n;
}

// Ecart entre la photo et le fichier relu, sur les trois canaux : moyenne et maximum des differences absolues.
static void Ecart(const NkImage &a, const NkImage &b, double &moyenne, int &maximum) {
    double somme = 0.0;
    maximum = 0;
    const long total = static_cast<long>(a.Width()) * a.Height() * 3;
    for (int y = 0; y < a.Height(); ++y) {
        for (int x = 0; x < a.Width(); ++x) {
            const math::NkColor p = a.GetPixel(x, y), q = b.GetPixel(x, y);
            const int d[3] = {p.r > q.r ? p.r - q.r : q.r - p.r, p.g > q.g ? p.g - q.g : q.g - p.g, p.b > q.b ? p.b - q.b : q.b - p.b};
            for (int k = 0; k < 3; ++k) { somme += d[k]; if (d[k] > maximum) maximum = d[k]; }
        }
    }
    moyenne = somme / total;
}

struct Essai {
    const char *nom;
    const char *fichier;
    bool (*ecrire)(const NkImage &, const char *);
};

int main() {
    // 1. La photo, chargee dans une variable.
    NkImage photo;
    if (!photo.Load("photo.jpg")) {
        std::printf("photo.jpg introuvable ou illisible\n");
        return 1;
    }
    std::printf("photo : %d x %d\n", photo.Width(), photo.Height());

    // 2. Cinq formats au choix. Chaque ecriture est testee, chaque fichier est relu.
    const Essai essais[5] = {
        {"png",  "essai.png",  [](const NkImage &i, const char *p) { return i.SavePNG(p); }},
        {"jpeg", "essai.jpg",  [](const NkImage &i, const char *p) { return i.SaveJPEG(p); }},
        {"bmp",  "essai.bmp",  [](const NkImage &i, const char *p) { return i.SaveBMP(p); }},
        {"tga",  "essai.tga",  [](const NkImage &i, const char *p) { return i.SaveTGA(p); }},
        {"qoi",  "essai.qoi",  [](const NkImage &i, const char *p) { return i.SaveQOI(p); }},
    };

    std::FILE *sortie = std::fopen("formats.txt", "w");
    if (!sortie) return 1;
    for (const Essai &e : essais) {
        const bool ecrit = e.ecrire(photo, e.fichier);
        const long octets = TailleSurDisque(e.fichier);

        // Ce qui compte, c'est ce que la lecture rend : la taille et les dimensions viennent du fichier relu.
        NkImage relue;
        const bool lisible = ecrit && relue.Load(e.fichier) && relue.IsValid();
        if (lisible) {
            std::fprintf(sortie, "%s : %ld octets, %d x %d\n", e.nom, octets, relue.Width(), relue.Height());
            std::printf("%s : %ld octets, %d x %d  (%s la photo)\n", e.nom, octets, relue.Width(), relue.Height(),
                        (relue.Width() == photo.Width() && relue.Height() == photo.Height()) ? "memes dimensions que" : "dimensions DIFFERENTES de");
            if (relue.Width() == photo.Width() && relue.Height() == photo.Height()) {
                double moyenne;
                int maximum;
                Ecart(photo, relue, moyenne, maximum);
                std::printf("    ecart avec la photo : moyenne %.4f, maximum %d (sur 255, par canal)\n", moyenne, maximum);
            }
        } else {
            std::fprintf(sortie, "%s : %ld octets, illisible\n", e.nom, octets < 0 ? 0L : octets);
            std::printf("%s : ecrit=%d, relu illisible\n", e.nom, ecrit ? 1 : 0);
        }
    }
    std::fclose(sortie);
    return 0;
}
