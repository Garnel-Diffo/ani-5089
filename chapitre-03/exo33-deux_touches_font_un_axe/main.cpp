#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main() {
    int c;
    if (std::scanf("%d", &c) != 1) return 0;

    std::vector<long long> scales(c), thresholds(c);
    for (int i = 0; i < c; ++i) {
        char buf[32];
        std::scanf("%31s %lld %lld", buf, &scales[i], &thresholds[i]);
    }

    int t;
    std::scanf("%d", &t);

    for (int turn = 0; turn < t; ++turn) {
        long long axe = 0;
        for (int i = 0; i < c; ++i) {
            long long raw;
            std::scanf("%lld", &raw);
            long long contribution = raw * scales[i] / 1000;
            if (std::llabs(contribution) < thresholds[i]) continue;
            axe += contribution;
        }
        std::printf("%lld\n", axe);
    }

    return 0;
}
