#include <cstdio>
#include <string>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 0;

    long long sansGarde = 0;
    for (int i = 0; i < n; ++i) {
        char buf[16];
        std::scanf("%15s", buf);
        std::string evenement(buf);

        if (evenement == "enfonce") {
            std::printf("SAISIR\n");
            ++sansGarde;
        } else if (evenement == "repete") {
            std::printf("RIEN\n");
            ++sansGarde;
        } else if (evenement == "relache") {
            std::printf("LACHER\n");
        }
    }

    std::printf("SANS_GARDE %lld\n", sansGarde);
    return 0;
}
