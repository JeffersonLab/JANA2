
#include "Protocluster_factory.h"
#include "Protocluster_factory_gluex.h"
#include "Protocluster_factory_epic.h"

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);

    auto version = app->RegisterParameter<int>("protoclusterizer:version", 3,
                                                    "Version of the protoclusterizer to use");

    if (version == 1) {

        // If you are using GlueX-style JFactoryT's, you have to use JFactoryGeneratorT.
        // This means that the factory prefix, input and output databundle names, and
        // default parameter values are hard-coded in the factory class itself.

        app->Add(new JFactoryGeneratorT<Protocluster_factory_gluex>());
    }
    else if (version == 2) {

        // If you are using ePIC-style JOmniFactory's, you have three options:
        // - JFactoryGeneratorT           which uses the default values you specified in the factory class
        // - JOmniFactoryGeneratorT       which uses values you specify right here
        // - JWiredFactoryGeneratorT      which uses values you specify dynamically in a TOML wiring file (see wired example)

        app->Add(new JFactoryGeneratorT<Protocluster_factory_epic>());

        app->Add(new JOmniFactoryGeneratorT<Protocluster_factory_epic>({
            .tag = "refined_protoclusterizer",
            .input_names = {"raw"},
            .output_names = {"proto_refined"},
            .configs = {.log_weight_energy = 8.0}
        }));

        // Some notes regarding JOmniFactoryGeneratorT:
        // - Input and output names are positional, following the order you declared them
        // - The Config struct is passed directly
    }
    else if (version == 3) {

        app->Add(new JFactoryGeneratorT<Protocluster_factory>());

        // TODO: Configure a refined_protoclusterizer

    }
    else {
        throw JException("Bad version number");
    }

} // InitPlugin
} // extern
