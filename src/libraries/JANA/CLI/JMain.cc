
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JMain.h"

#include <JANA/CLI/JBenchmarker.h>
#include <JANA/CLI/JSignalHandler.h>


namespace jana {

void PrintUsage() {
    /// Prints jana.cc command-line options to stdout, for use by the CLI.
    /// This does not include JANA parameters, which come from
    /// JParameterManager::PrintParameters() instead.

    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    jana [options] source1 source2 ..." << std::endl << std::endl;

    std::cout << "Description:" << std::endl;
    std::cout << "    Command-line interface for running JANA plugins. This can be used to" << std::endl;
    std::cout << "    read in events and process them. Command-line flags control configuration" << std::endl;
    std::cout << "    while additional arguments denote input files, which are to be loaded and" << std::endl;
    std::cout << "    processed by the appropriate EventSource plugin." << std::endl << std::endl;

    std::cout << "Options:" << std::endl;
    std::cout << "   -h   --help                  Display this message" << std::endl;
    PrintUsageOptions();
    std::cout << "Example:" << std::endl;
    std::cout << "    jana -Pplugins=plugin1,plugin2,plugin3 -Pnthreads=8 inputfile1.txt" << std::endl << std::endl;

}

void PrintUsageOptions() {
    std::cout << "   -Pkey=value                        Specify a configuration parameter" << std::endl << std::endl;
    std::cout << "   -v   --version                     Display version information" << std::endl;
    std::cout << "   -c   --configs                     Display configuration parameters" << std::endl;
    std::cout << "   -l   --loadconfigs <file>          Load configuration parameters from file" << std::endl;
    std::cout << "   -d   --dumpconfigs <file>          Dump configuration parameters to file" << std::endl;
    std::cout << "   -b   --benchmark                   Run in benchmark mode" << std::endl;
    std::cout << "        --inspect-collection <name>   Inspect a collection" << std::endl;
    std::cout << "        --inspect-component <name>    Inspect a component" << std::endl;
}


JApplication* CreateJApplication(UserOptions& options) {

    auto params = new JParameterManager(); // JApplication owns params_copy, does not own eventSources
    for (auto pair : options.params) {
        params->SetParameter(pair.first, pair.second);
    }

    if (options.flags[LoadConfigs]) {
        // If the user specified an external config file, we should definitely use that
        try {
            params->ReadConfigFile(options.load_config_file);
        }
        catch (JException &e) {
            std::cout << "Problem loading config file '" << options.load_config_file << "'. Exiting." << std::endl
                      << std::endl;
            exit(-1);
        }
        std::cout << "Loaded config file '" << options.load_config_file << "'." << std::endl << std::endl;
    }

    auto app = new JApplication(params);

    for (auto event_src : options.eventSources) {
        app->Add(event_src);
    }
    return app;
}

int Execute(JApplication* app, UserOptions &options) {

    JSignalHandler::register_handlers(app);

    if (options.flags[ShowConfigs]) {
        // Load all plugins, collect all parameters, exit without running anything
        app->Initialize();
        if (options.flags[Benchmark]) {
            JBenchmarker benchmarker(app);  // Show benchmarking configs only if benchmarking mode specified
        }
        app->GetJParameterManager()->PrintParameters(2, 1);
    }
    else if (options.flags[DumpConfigs]) {
        // Load all plugins, dump parameters to file, exit without running anything
        app->Initialize();
        std::cout << std::endl << "Writing configuration options to file: " << options.dump_config_file
                  << std::endl;
        app->GetJParameterManager()->WriteConfigFile(options.dump_config_file);
    }
    else if (options.flags[InspectCollection]) {
        app->Initialize();
        const auto& summary = app->GetComponentSummary();
        std::cout << "----------------------------------------------------------" << std::endl;
        if (options.collection_query.empty()) {
            PrintCollectionTable(std::cout, summary);
        }
        else {
            auto lookup = summary.FindCollections(options.collection_query);
            std::cout << "Collection query: '" << options.collection_query << "'" << std::endl;
            if (lookup.empty()) {
                std::cout << "Collection not found!" << std::endl;
            }
            else {
                std::cout << "----------------------------------------------------------" << std::endl;
                for (auto* item : lookup) {
                    std::cout << *item;
                    std::cout << "----------------------------------------------------------" << std::endl;
                }
            }
        }
    }
    else if (options.flags[InspectComponent]) {
        app->Initialize();
        const auto& summary = app->GetComponentSummary();
        std::cout << "----------------------------------------------------------" << std::endl;
        if (options.component_query.empty()) {
            PrintComponentTable(std::cout, summary);
        }
        else {
            auto lookup = summary.FindComponents(options.component_query);
            std::cout << "Component query: '" << options.component_query << "'" << std::endl;
            if (lookup.empty()) {
                std::cout << "Component not found!" << std::endl;
            }
            else {
                std::cout << "----------------------------------------------------------" << std::endl;
                for (auto* item : lookup) {
                    std::cout << *item;
                    std::cout << "----------------------------------------------------------" << std::endl;
                }
            }
        }
    }
    else if (options.flags[Benchmark]) {
        // Run JANA in benchmark mode
        JBenchmarker benchmarker(app); // Benchmarking params override default params
        benchmarker.RunUntilFinished(); // Benchmarker will control JApp Run/Stop
    }
    else {
        // Run JANA in normal mode
        try {
            app->Run(true, true);
        }
        catch (JException& e) {
            std::cout << "----------------------------------------------------------" << std::endl;
            std::cout << e << std::endl;
            app->SetExitCode((int) JApplication::ExitCode::UnhandledException);
        }
        catch (std::exception& e) {
            std::cout << "----------------------------------------------------------" << std::endl;
            std::cout << "Exception: " << e.what() << std::endl;
            app->SetExitCode((int) JApplication::ExitCode::UnhandledException);
        }
        catch (...) {
            std::cout << "----------------------------------------------------------" << std::endl;
            std::cout << "Unknown exception" << std::endl;
            app->SetExitCode((int) JApplication::ExitCode::UnhandledException);
        }
    }
    return (int) app->GetExitCode();
}


UserOptions ParseCommandLineOptions(int nargs, char *argv[], bool expect_extra) {

    UserOptions options;

    std::map<std::string, Flag> tokenizer;
    tokenizer["-h"] = ShowUsage;
    tokenizer["--help"] = ShowUsage;
    tokenizer["-v"] = ShowVersion;
    tokenizer["--version"] = ShowVersion;
    tokenizer["-c"] = ShowConfigs;
    tokenizer["--configs"] = ShowConfigs;
    tokenizer["-l"] = LoadConfigs;
    tokenizer["--loadconfigs"] = LoadConfigs;
    tokenizer["-d"] = DumpConfigs;
    tokenizer["--dumpconfigs"] = DumpConfigs;
    tokenizer["-b"] = Benchmark;
    tokenizer["--benchmark"] = Benchmark;
    tokenizer["--inspect-collection"] = InspectCollection;
    tokenizer["--inspect-component"] = InspectComponent;

    if (nargs == 1) {
        options.flags[ShowUsage] = true;
    }

    for (int i = 1; i < nargs; i++) {

        std::string arg = argv[i];
        //std::cout << "Found arg " << arg << std::endl;

        if (argv[i][0] != '-') {
            options.eventSources.push_back(arg);
            continue;
        }

        switch (tokenizer[arg]) {

            case Benchmark:
                options.flags[Benchmark] = true;
                break;

            case ShowUsage:
                options.flags[ShowUsage] = true;
                break;

            case ShowVersion:
                options.flags[ShowVersion] = true;
                break;

            case ShowConfigs:
                options.flags[ShowConfigs] = true;
                break;

            case LoadConfigs:
                options.flags[LoadConfigs] = true;
                if (i + 1 < nargs && argv[i + 1][0] != '-') {
                    options.load_config_file = argv[i + 1];
                    i += 1;
                } else {
                    options.load_config_file = "jana.config";
                }
                break;

            case DumpConfigs:
                options.flags[DumpConfigs] = true;
                if (i + 1 < nargs && argv[i + 1][0] != '-') {
                    options.dump_config_file = argv[i + 1];
                    i += 1;
                } else {
                    options.dump_config_file = "jana.config";
                }
                break;
 
            case InspectCollection:
                options.flags[InspectCollection] = true;
                if (i + 1 < nargs && argv[i + 1][0] != '-') {
                    options.collection_query = argv[i + 1];
                    i += 1;
                } 
                break;

            case InspectComponent:
                options.flags[InspectComponent] = true;
                if (i + 1 < nargs && argv[i + 1][0] != '-') {
                    options.component_query = argv[i + 1];
                    i += 1;
                } 
                break;

            case Interactive:
                options.flags[Interactive] = true;
                break;

            case Unknown:
                if (argv[i][0] == '-' && argv[i][1] == 'P') {

                    size_t pos = arg.find("=");
                    if ((pos != std::string::npos) && (pos > 2)) {
                        std::string key = arg.substr(2, pos - 2);
                        std::string val = arg.substr(pos + 1);
                        options.params.insert({key, val});
                    } else {
                        std::cout << "Invalid JANA parameter '" << arg
                                  << "': Expected format -Pkey=value" << std::endl;
                        options.flags[ShowConfigs] = true;
                    }
                } else {
                    if (!expect_extra) {
                        std::cout << "Invalid command line flag '" << arg << "'" << std::endl;
                        options.flags[ShowUsage] = true;
                    }
                }
        }
    }
    return options;
}
}
