
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _ADCSampleFactory_h_
#define _ADCSampleFactory_h_

#include <JANA/JFactoryT.h>
#include <JANA/Utils/JPerfUtils.h>

#include "ADCSample.h"
#include "INDRAMessage.h"

#include <fstream>

class ADCSampleFactory : public JFactoryT<ADCSample> {

    // parameters to simulate a bottle neck, spread is in sigmas
    size_t m_cputime_ms = 0;
    double m_cputime_spread = 0.25;

    std::vector<ADCSample> m_samples;
    std::vector<ADCSample*> m_sample_ptrs;
    // Rather than creating and destroying lots of individual hit objects for each event,
    // we maintain a block of them and set NOT_OBJECT_OWNER. These are owned by the JFactory
    // so there won't be a memory leak.

public:

    void Init() override {
        auto app = GetApplication();
        app->GetParameter("streamDet:rawhit_ms",     m_cputime_ms);
        app->GetParameter("streamDet:rawhit_spread", m_cputime_spread);
        SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
    }

    void Process(const std::shared_ptr<const JEvent> &event) override {

        // acquire the DASEventMessage via zmq
        // each DASEventMessage corresponds to one hardware event (readout window)
        // for now we pretend that hardware events = physics events
        auto message   = event->GetSingle<DASEventMessage>();
        auto source_id = message->as_indra_message()->source_id;

        // obtain a view into our DASEventMessage payload and assign variables
        const char* payload_buffer;
        size_t payload_buffer_size;
        message->as_payload(&payload_buffer, &payload_buffer_size);
        size_t max_samples  = message->get_sample_count();
        size_t max_channels = message->get_channel_count();

        if (m_samples.size() != max_channels*max_samples) {
            // If size actually changes (hopefully not often), create or destroy new ADCSamples as needed
            // They will be initialized with garbage data though
            m_samples.resize(max_channels*max_samples);

            // Also recreate the vector of pointers into this buffer
            m_sample_ptrs.resize(max_channels*max_samples);
            for (size_t i=0; i < m_samples.size(); ++i) {
                m_sample_ptrs[i] = &m_samples[i];
            }
        }

        size_t i = 0;
        // decode the message and populate the associated jobject (hit) for the event
        for (uint16_t sample = 0; sample < max_samples; ++sample) {
            for (uint16_t channel = 0; channel < max_channels; ++channel) {

                // Parse in the simplest way possible
                uint16_t current_value = (payload_buffer[0]-48) * 1000 + (payload_buffer[1]-48) * 100 + (payload_buffer[2]-48) * 10 + (payload_buffer[3]-48);
                assert(current_value >= 0);
                assert(current_value <= 1024);
                payload_buffer += 5;
                ADCSample& hit = m_samples[i++];
                hit.source_id  = source_id;
                hit.sample_id  = sample;
                hit.channel_id = channel;
                hit.adc_value  = current_value;
            }
        }
        Set(m_sample_ptrs); // Copy all of the pointers into m_samples over in one go
    }
};

#endif  // _ADCSampleFactory_h_
