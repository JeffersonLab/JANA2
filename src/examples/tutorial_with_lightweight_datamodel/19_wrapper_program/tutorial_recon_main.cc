
#include <iostream>

#include <JANA/JApplication.h>
#include <JANA/CLI/JMain.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JFactoryGenerator.h>

#include <RandomHitSource.h>
#include <Protocluster_factory.h>
#include <AsciiHeatmap_writer.h>


int main(int argc, char* argv[]) {

    std::cout << "Welcome to lw-tutorial-recon" << std::endl;

    auto options = jana::ParseCommandLineOptions(argc, argv);
    auto params = new JParameterManager();

    for(auto pair: options.params) {
        params->SetParameter(pair.first, pair.second);
    }
    // Instantiate the JApplication with the parameter manager    
    JApplication app(params);

    // Add the event source filenames from command line  
    for(auto event_source : options.eventSources) {  
        app.Add(event_source);  
    }

    // Register components
    app.Add(new JEventSourceGeneratorT<RandomHitSource>());
    app.Add(new JFactoryGeneratorT<Protocluster_factory>());
    app.Add(new AsciiHeatmap_writer);

    // Initialize and run the application
    app.Initialize();
    app.Run();

    // Retrieve and return the exit code
    return app.GetExitCode();
}

