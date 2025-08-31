
#include "Protocluster_factory.h"
#include "Protocluster_factory_epic.h"

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);

    auto version = app->RegisterParameter<int>("protoclusterizer:version",  1,
                                                    "Version of the protoclusterizer to use");

    if (version == 1) {

        // This is the most up-to-date, recommended way of creating factories that write Podio data
        app->Add(new JFactoryGeneratorT<Protocluster_factory>());
    }
    else if (version == 2) {

        // If you are using ePIC-style JOmniFactories, you have three options:
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
    else {
        throw JException("Bad version number");
    }

} // InitPlugin
} // extern
