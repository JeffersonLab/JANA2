//
//    File: toyDet/DASFileSource.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#include "DASFileSource.h"
#include "ADCSample.h"

#include <iostream>
#include <vector>

//---------------------------------
// DASFileSource    (Constructor)
//---------------------------------
DASFileSource::DASFileSource(std::string source_name, JApplication* app) : JEventSource(source_name, app) {
    // Don't open the file/stream here. Do it in the Open() method below.
}

DASFileSource::~DASFileSource() {
    // Delete JFactoryGenerator if we created one
    if (mFactoryGenerator != nullptr) delete mFactoryGenerator;

    // Close the file/stream here.
    std::cout << "Closing " << mName << std::endl;
    ifs.close();
}

void DASFileSource::Open() {
    ifs.open(mName);
    if (!ifs) {
        throw JException("Unable to open '%s'", mName.c_str());
    }
}

void DASFileSource::GetEvent(std::shared_ptr<JEvent> event) {

    // TODO: Put these somewhere that makes sense
    size_t current_event_nr = 0;
    size_t MAX_CHANNELS     = 80;
    size_t MAX_SAMPLES      = 1024;

    if (ifs.is_open()) {

        if (!ifs.eof()) {
            // Each iteration of this becomes one new event
            // Assumes file contains no partial events

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
            event->Insert(hits);
            event->SetEventNumber(current_event_nr++);
            return;
        }

        std::cout << "Reached end of file/stream " << mName << std::endl;
        ifs.close();
    }
    throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
}


