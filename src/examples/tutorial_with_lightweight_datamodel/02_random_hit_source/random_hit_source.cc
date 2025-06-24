
#include <JANA/JApplication.h>
#include "RandomHitSource.h"

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new RandomHitSource);
}
}
