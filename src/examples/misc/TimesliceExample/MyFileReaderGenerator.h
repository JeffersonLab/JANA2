
#include <JANA/JEventSourceGenerator.h>
#include "MyFileReader.h"


class MyFileReaderGenerator : public JEventSourceGenerator {

    JEventSource* MakeJEventSource(std::string resource_name) override {

        auto source = new MyFileReader;
        source->SetResourceName(resource_name);

        // Check if the string "timeslices" appears anywhere in our filename. 
        // If so, we assume the file contains timeslices, otherwise it contains physics events.
        // Another approach might be to peek at the file's contents
        if (resource_name.find("timeslices") != std::string::npos)  {
            source->SetLevel(JEventLevel::Timeslice);
        }
        else {
            source->SetLevel(JEventLevel::PhysicsEvent);
        }
        return source;
    }

    double CheckOpenable(std::string resource_name) override {
        // In theory, we should check whether PODIO can open the file and 
        // whether it contains an 'events' or 'timeslices' tree. If not, return 0.
        if (resource_name.find(".root") != std::string::npos) {
            return 0.01;
        }
        else {
            return 0;
        }
    }
};


