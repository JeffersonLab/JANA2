
#include "JApplicationInspector.h"
#include "JANA/Components/JComponentSummary.h"
#include "JANA/Topology/JTopologyBuilder.h"
#include <JANA/JApplication.h>
#include <JANA/Engine/JExecutionEngine.h>


void PrintMenu() {
    std::cout << "  -----------------------------------------" << std::endl;
    std::cout << "  Available commands" << std::endl;
    std::cout << "  -----------------------------------------" << std::endl;
    std::cout << "  icm  InspectComponents" << std::endl;
    std::cout << "  icm  InspectComponent component_name" << std::endl;
    std::cout << "  icl  InspectCollections" << std::endl;
    std::cout << "  icl  InspectCollection collection_name" << std::endl;
    std::cout << "  it   InspectTopology" << std::endl;
    std::cout << "  ip   InspectPlace arrow_id place_id" << std::endl;
    std::cout << "  ie   InspectEvent arrow_id place_id slot_id" << std::endl;
    std::cout << "  f    Fire arrow_id" << std::endl;
    std::cout << "  r    Resume" << std::endl;
    std::cout << "  s    Scale nthreads" << std::endl;
    std::cout << "  q    Quit" << std::endl;
    std::cout << "  h    Help" << std::endl;
    std::cout << "  -----------------------------------------" << std::endl;
}

void InspectTopology(JApplication* app) {
    auto topology = app->GetService<JTopologyBuilder>();
    std::cout << topology->print_topology() << std::endl;
}

void Fire(JApplication* app, int arrow_id) {
    auto engine = app->GetService<JExecutionEngine>();
    auto result = engine->Fire(arrow_id, 0);
    std::cout << to_string(result);
}

void InspectComponents(JApplication* app) {
    auto& summary = app->GetComponentSummary();
    PrintComponentTable(std::cout, summary);
}

void InspectComponent(JApplication* app, std::string component_name) {
    const auto& summary = app->GetComponentSummary();
    auto lookup = summary.FindComponents(component_name);
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

void InspectCollections(JApplication* app) {
    const auto& summary = app->GetComponentSummary();
    PrintCollectionTable(std::cout, summary);
}

void InspectCollection(JApplication* app, std::string collection_name) {
    const auto& summary = app->GetComponentSummary();
    auto lookup = summary.FindCollections(collection_name);
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


void InspectApplication(JApplication* app) {
    auto engine = app->GetService<JExecutionEngine>();
    PrintMenu();

    while (true) {

        std::string user_input;
        std::cout << std::endl << "JANA: "; std::cout.flush();
        // obtain a single line
        std::getline(std::cin, user_input);
        // split into tokens
        std::stringstream ss(user_input);
        std::string token;
        ss >> token;
        std::vector<std::string> args;
        std::string arg;
        try {
            while (ss >> arg) {
                args.push_back(arg);
            }
            if ((token == "InspectComponents" || token == "icm") && args.empty()) {
                InspectComponents(app);
            }
            else if ((token == "InspectComponent" || token == "icm") && (args.size() == 1)) {
                InspectComponent(app, args[0]);
            }
            else if ((token == "InspectCollections" || token == "icl") && args.empty()) {
                InspectCollections(app);
            }
            else if ((token == "InspectCollection" || token == "icl") && (args.size() == 1)) {
                InspectCollection(app, args[0]);
            }
            else if ((token == "InspectTopology" || token == "it") && args.empty()) {
                InspectTopology(app);
            }
            else if ((token == "InspectPlace" || token == "ip") && args.size() == 2) {
                // InspectPlace(std::stoi(args[0]), std::stoi(args[1]));
            }
            else if ((token == "InspectEvent" || token == "ie") && (args.size() == 3)) {
                // InspectEvent(std::stoi(args[0])
            }
            else if ((token == "Fire" || token == "f") && (args.size() == 1)) {
                Fire(app, std::stoi(args[0]));
            }
            else if (token == "Resume" || token == "r") {
                engine->RunTopology();
                break;
            }
            else if ((token == "Scale" || token == "s") && (args.size() == 1)) {
                engine->ScaleWorkers(std::stoi(args[0]));
                engine->RunTopology();
                break;
            }
            else if (token == "Quit" || token == "q") {
                engine->DrainTopology();
                break;
            }
            else if (token == "Help" || token == "h") {
                PrintMenu();
            }
            else if (token == "") {
                // Do nothing
            }
            else {
            std::cout << "(Error: Unrecognized command, or wrong argument count)" << std::endl;
            }
        }
        catch (JException& ex) {
            std::cout << "(JException: " << ex.GetMessage() << ")" << std::endl;
        }
        catch (std::invalid_argument&) {
            std::cout << "(Parse error: Maybe an argument needs to be an int)" << std::endl;
        }
        catch (...) {
            std::cout << "(Unknown error)" << std::endl;
        }

    }

}
