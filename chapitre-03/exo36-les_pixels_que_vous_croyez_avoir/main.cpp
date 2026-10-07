#include <cstdio>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 0;

    int lisible = 0;
    for (int i = 0; i < n; ++i) {
        long long width, height, scale, field;
        std::scanf("%lld %lld %lld %lld", &width, &height, &scale, &field);

        long long realWidth = width * scale / 100;
        long long realHeight = height * scale / 100;
        long long ppd = (realWidth + field / 2) / field;

        std::printf("%lld %lld %lld\n", realWidth, realHeight, ppd);
        if (ppd >= 15) ++lisible;
    }

    std::printf("LISIBLE %d\n", lisible);
    return 0;
}
