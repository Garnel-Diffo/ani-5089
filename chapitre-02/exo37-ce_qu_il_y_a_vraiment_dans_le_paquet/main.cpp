// Chapitre 2, exercice 37 : ce qu'il y a vraiment dans le paquet.
//
// Entree :
//   architecture voulue   (ex. arm64-v8a)
//   F
//   F lignes "chemin taille"
//
// Sortie, quatre lignes :
//   la taille totale (en octets)
//   SIGNE ou NON SIGNE
//   ABI OUI ou ABI NON
//   INCONNU n
#include <iostream>
#include <string>

bool commence_par(const std::string& s, const std::string& prefixe) {
    return s.size() >= prefixe.size() && s.compare(0, prefixe.size(), prefixe) == 0;
}

bool finit_par(const std::string& s, const std::string& suffixe) {
    return s.size() >= suffixe.size() && s.compare(s.size() - suffixe.size(), suffixe.size(), suffixe) == 0;
}

int main() {
    std::string archi;
    if (!(std::cin >> archi)) return 1;

    int f;
    std::cin >> f;

    const std::string PREFIXE_LIB = "lib/";
    const std::string PREFIXE_ARCHI = PREFIXE_LIB + archi + "/";
    const std::string PREFIXE_META = "META-INF/";

    long long total = 0;
    bool signe = false;
    bool abi_ok = false;
    int inutiles = 0;

    for (int i = 0; i < f; ++i) {
        std::string chemin;
        long long taille;
        std::cin >> chemin >> taille;
        total += taille;

        if (commence_par(chemin, PREFIXE_META) &&
            (finit_par(chemin, ".RSA") || finit_par(chemin, ".DSA") || finit_par(chemin, ".EC"))) {
            signe = true;
        }
        if (commence_par(chemin, PREFIXE_ARCHI)) {
            abi_ok = true;
        } else if (commence_par(chemin, PREFIXE_LIB)) {
            ++inutiles;   // sous lib/, mais d'une autre architecture
        }
    }

    std::cout << total << "\n";
    std::cout << (signe ? "SIGNE" : "NON SIGNE") << "\n";
    std::cout << (abi_ok ? "ABI OUI" : "ABI NON") << "\n";
    std::cout << "INUTILE " << inutiles << "\n";
    return 0;
}
