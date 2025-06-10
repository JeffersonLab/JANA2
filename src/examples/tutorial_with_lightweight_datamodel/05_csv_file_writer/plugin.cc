
#include <JANA/JApplication.h>
#include "CsvWriter.h"

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new CsvWriter);
}
}
