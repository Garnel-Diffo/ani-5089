#include <cstdio>
#include <string>
#include <vector>

struct Rappel {
    long long id;
    std::string type;
};

static void retirer(std::vector<Rappel>& registre, long long id) {
    for (std::size_t i = 0; i < registre.size(); ++i) {
        if (registre[i].id == id) {
            registre.erase(registre.begin() + static_cast<long>(i));
            return;
        }
    }
}

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 0;

    std::vector<Rappel> registre;

    for (int i = 0; i < n; ++i) {
        char buf[16];
        std::scanf("%15s", buf);
        std::string commande(buf);

        if (commande == "poser") {
            long long id;
            char type[32];
            std::scanf("%lld %31s", &id, type);
            retirer(registre, id);
            registre.push_back({id, std::string(type)});
        } else if (commande == "retirer") {
            long long id;
            std::scanf("%lld", &id);
            retirer(registre, id);
        } else if (commande == "envoyer") {
            char type[32];
            std::scanf("%31s", type);
            std::string typeStr(type);

            bool premier = true;
            bool trouve = false;
            for (const Rappel& r : registre) {
                if (r.type == typeStr) {
                    if (!premier) std::printf(" ");
                    std::printf("%lld", r.id);
                    premier = false;
                    trouve = true;
                }
            }
            if (!trouve) std::printf("AUCUN");
            std::printf("\n");
        }
    }

    return 0;
}
