
#include <JANA/JApplication.h>
#include "AsciiHeatmap_writer.h"
#include "AsciiHeatmap_writer_legacy.h"

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);

    app->Add(new AsciiHeatmap_writer);
    //app->Add(new AsciiHeatmap_writer_legacy);
}
}
