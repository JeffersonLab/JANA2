//
//    File: streamDet/DecodeDASSource.h
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#ifndef _DecodeDASSource_h_
#define _DecodeDASSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

#include <fstream>
#include <string>

class DecodeDASSource : public JEventSource {

public:

    // constructors and destructors
    DecodeDASSource(std::string source_name, JApplication* app);
    ~DecodeDASSource() override;

    // define public methods
    static std::string GetDescription() { return "streamDet event source (direct ADC serialization mode)"; }
    void Open() final;
    void GetEvent(std::shared_ptr<JEvent>) final;

private:

    // file stream and event counter
    std::ifstream ifs;
    size_t current_event_nr = 0;

};

#endif // _DecodeDASSource_h_

