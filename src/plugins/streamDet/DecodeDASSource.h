
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _DecodeDASSource_h_
#define _DecodeDASSource_h_

#include <JANA/JEventSource.h>

#include <fstream>
#include <string>

class DecodeDASSource : public JEventSource {

public:

    // constructors and destructors
    DecodeDASSource();
    ~DecodeDASSource() override;

    // define public methods
    static std::string GetDescription() { return "streamDet event source (direct ADC serialization mode)"; }
    void Open() final;
    void Close() final;
    Result Emit(JEvent&) final;

private:

    // file stream and event counter
    std::ifstream ifs;
    size_t current_event_nr = 0;

};

#endif // _DecodeDASSource_h_

