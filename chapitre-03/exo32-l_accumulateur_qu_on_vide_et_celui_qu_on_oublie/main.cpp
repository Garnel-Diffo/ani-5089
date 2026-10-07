#include <cstdio>
#include <string>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 0;

    long long totalX = 0, totalY = 0;
    long long engineX = 0, engineY = 0;

    for (int i = 0; i < n; ++i) {
        char buf[16];
        std::scanf("%15s", buf);
        std::string commande(buf);

        if (commande == "bouge") {
            long long dx, dy;
            std::scanf("%lld %lld", &dx, &dy);
            totalX += dx;
            totalY += dy;
            engineX = dx;
            engineY = dy;
        } else if (commande == "image") {
            std::printf("%lld %lld %lld %lld\n", totalX, totalY, engineX, engineY);
            totalX = 0;
            totalY = 0;
        }
    }

    return 0;
}
