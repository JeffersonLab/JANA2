
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "DecodeDASSource.h"
#include "ADCSample.h"

#include <iostream>
#include <vector>

//---------------------------------
// DecodeDASSource    (Constructor)
//---------------------------------
DecodeDASSource::DecodeDASSource(std::string source_name, JApplication* app) : JEventSource(source_name, app) {
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

DecodeDASSource::~DecodeDASSource() {
}

void DecodeDASSource::Open() {

    // open the file stream
    ifs.open(GetResourceName());
    if (!ifs) throw JException("Unable to open '%s'", GetResourceName().c_str());

}

void DecodeDASSource::Close() {
    // Close the file/stream here.
    std::cout << "Closing " << GetResourceName() << std::endl;
    ifs.close();
}

JEventSource::Result DecodeDASSource::Emit(JEvent& event) {

    size_t MAX_CHANNELS = 80;
    size_t MAX_SAMPLES  = 1024;
    // open the file stream and parse the data
    // adc sample data are stored as a vector of ADCSample objects
    if (ifs.is_open()) {
        if (!ifs.eof()) {
            // each iteration of this becomes one new event
            // assumes file contains no partial events
            std::vector<ADCSample*> hits;
            for (uint16_t sample = 0; sample < MAX_SAMPLES && !ifs.eof(); ++sample) {
                for (uint16_t channel = 0; channel < MAX_CHANNELS; ++channel) {
                    auto hit = new ADCSample;
                    hit->sample_id = sample;
                    hit->channel_id = channel;
                    ifs >> hit->adc_value;
                    hits.push_back(hit);
                }
            }
            // populate the jevent with the adc samples
            event.Insert(hits);
            event.SetEventNumber(current_event_nr++);
            return Result::Success;
        }
        // close file stream when the end of file is reached
        std::cout << "Reached end of file/stream " << GetResourceName() << std::endl;
    }
    return Result::FailureFinished;

}


