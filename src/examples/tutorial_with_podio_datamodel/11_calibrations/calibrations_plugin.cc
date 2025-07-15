
#include <JANA/JApplication.h>
#include "CalorimeterCluster_factory_filtered.h"
#include "JANA/JFactoryGenerator.h"

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new JFactoryGeneratorT<CalorimeterCluster_factory_filtered>);
}
}

