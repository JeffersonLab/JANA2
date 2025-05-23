
#include <JANA/JApplication.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include "PodioClusteringFactory.h"
#include "PodioProtoclusteringFactory.h"

extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);

    auto cluster_gen = new JOmniFactoryGeneratorT<PodioClusteringFactory>();

    cluster_gen->AddWiring("clusterizer", 
                           {"protoclusters"},
                           {"clusters"},
                           {.offset=1000});

    app->Add(cluster_gen);



    auto protocluster_gen = new JOmniFactoryGeneratorT<PodioProtoclusteringFactory>();

    protocluster_gen->AddWiring("protoclusterizer", 
                                {"hits"}, 
                                {"protoclusters"});

    app->Add(protocluster_gen);

}
} // "C"
