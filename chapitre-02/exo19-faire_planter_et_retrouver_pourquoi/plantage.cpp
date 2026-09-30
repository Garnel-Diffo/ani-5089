// Plantage volontaire sur Android : une lecture a travers un pointeur nul.
//
// Meme bogue que sur bureau : TrouverPosition rend nullptr quand
// l'identifiant demande n'existe pas dans la scene, et l'appelant ne
// verifie jamais ce pointeur avant de le lire. C'est une faute ordinaire
// (un objet introuvable traite comme s'il existait), pas un cas limite
// invente pour l'exercice.
//
// android_main est le point d'entree d'une NativeActivity (fourni par la
// glue native du NDK, appele apres la creation de l'activite).
#include <android/log.h>
#include <android_native_app_glue.h>

struct Position { float x, y, z; };

struct Entite {
    int id;
    Position position;
};

Entite g_scene[2] = {
    { 1, { 0.0f, 1.6f, 0.0f } },
    { 2, { 2.5f, 0.0f, -1.0f } },
};

Position* TrouverPosition(int id) {
    for (Entite& e : g_scene) {
        if (e.id == id) return &e.position;
    }
    return nullptr;
}

float HauteurDeLEntite(int id) {
    Position* p = TrouverPosition(id);
    // Bogue : aucune verification que p n'est pas nullptr avant de le lire.
    return p->y;
}

extern "C" void android_main(struct android_app* app) {
    (void)app;
    __android_log_print(ANDROID_LOG_INFO, "Plantage", "Hauteur de l'entite 1 : %f", HauteurDeLEntite(1));
    __android_log_print(ANDROID_LOG_INFO, "Plantage", "Hauteur de l'entite 2 : %f", HauteurDeLEntite(2));
    // L'entite 3 n'existe pas dans g_scene : TrouverPosition rend nullptr,
    // et la ligne suivante lit a travers ce pointeur nul.
    __android_log_print(ANDROID_LOG_INFO, "Plantage", "Hauteur de l'entite 3 : %f", HauteurDeLEntite(3));
}
