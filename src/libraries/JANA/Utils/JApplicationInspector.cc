
#include "JApplicationInspector.h"
#include "JANA/Topology/JTopologyBuilder.h"
#include <JANA/JApplication.h>
#include <JANA/Engine/JArrowProcessingController.h>


void PrintMenu() {
    std::cout << "  -----------------------------------------" << std::endl;
    std::cout << "  Available commands" << std::endl;
    std::cout << "  -----------------------------------------" << std::endl;
    std::cout << "  ic   InspectComponents" << std::endl;
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
    auto engine = app->GetService<JArrowProcessingController>();
    auto result = engine->execute_arrow(arrow_id);
    std::cout << to_string(result) << std::endl;
}

void InspectApplication(JApplication* app) {
    auto engine = app->GetService<JArrowProcessingController>();
    engine->request_pause();
    engine->wait_until_paused();
    app->SetTimeoutEnabled(false);
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
        std::vector<int> args;
        std::string arg;
        try {
            while (ss >> arg) {
                args.push_back(std::stoi(arg));
            }
            if (token == "InspectComponents" || token == "ic") {
                // InspectComponents();
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
                Fire(app, args[0]);
            }
            else if (token == "Resume" || token == "r") {
                app->Run(false);
                break;
            }
            else if ((token == "Scale" || token == "s") && (args.size() == 1)) {
                app->Scale(args[0]);
                break;
            }
            else if (token == "Quit" || token == "q") {
                app->Quit(true);
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
