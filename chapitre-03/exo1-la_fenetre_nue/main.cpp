#include "NKWindow/NkWindow.h"
#include "NKWindow/Core/NkMain.h"

int nkmain(const nkentseu::NkEntryState& /*state*/) {
    using namespace nkentseu;

    NkAppData app;
    app.appName = "Salle";
    if (!NkInitialise(app)) return 1;

    NkWindowConfig config;
    config.title  = "Ma salle";
    config.width  = 1280;
    config.height = 720;

    Window fenetre(config);
    if (!fenetre.IsValid()) return 1;

    while (fenetre.IsOpen()) {
        EventSystem::Instance().PollEvents();
    }

    NkClose();
    return 0;
}
