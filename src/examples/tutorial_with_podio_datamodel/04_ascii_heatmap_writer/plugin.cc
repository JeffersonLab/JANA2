
#include <JANA/JApplication.h>
#include "HeatmapProcessor.h"

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new HeatmapProcessor);
}
}
