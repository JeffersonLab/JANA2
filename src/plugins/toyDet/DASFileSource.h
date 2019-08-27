//
//    File: toyDet/DASFileSource.h
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#ifndef _DASFileSource_h_
#define _DASFileSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

#include <fstream>
#include <string>

class DASFileSource : public JEventSource {

public:

    DASFileSource(std::string source_name, JApplication* app);
    ~DASFileSource() override;

    static std::string GetDescription() { return "ToyDet event source (direct ADC serialization mode)"; }
    void Open() final;
    void GetEvent(std::shared_ptr<JEvent>) final;

private:

    std::ifstream ifs;
    size_t current_event_nr = 0;

};

#endif // _DASFileSource_h_

