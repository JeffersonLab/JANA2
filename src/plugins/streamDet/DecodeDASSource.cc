
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

}

DecodeDASSource::~DecodeDASSource() {

    // Close the file/stream here.
    std::cout << "Closing " << GetName() << std::endl;
    ifs.close();

}

void DecodeDASSource::Open() {

    // open the file stream
    ifs.open(GetName());
    if (!ifs) throw JException("Unable to open '%s'", GetName().c_str());

}

void DecodeDASSource::GetEvent(std::shared_ptr<JEvent> event) {

    // TODO: Put these somewhere that makes sense
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
            event->Insert(hits);
            event->SetEventNumber(current_event_nr++);
            return;
        }
        // close file stream when the end of file is reached
        std::cout << "Reached end of file/stream " << GetName() << std::endl;
        ifs.close();
    }
    // signal jana to terminate
    throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;

}


