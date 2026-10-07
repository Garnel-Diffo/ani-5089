#include <cstdio>
#include <string>

int main() {
    long long p;
    int f;
    if (std::scanf("%lld %d", &p, &f) != 2) return 0;

    long long compteur = 0;
    int premier = 0;

    for (int tour = 1; tour <= f; ++tour) {
        char buf[16];
        std::scanf("%15s", buf);
        std::string commande(buf);

        if (commande == "releve") {
            compteur = 0;
        } else {
            ++compteur;
        }

        bool morte = (compteur >= p);
        std::printf("%lld %s\n", compteur, morte ? "MORTE" : "VIVANTE");

        if (morte && premier == 0) premier = tour;
    }

    std::printf("PREMIER %d\n", premier);
    return 0;
}
