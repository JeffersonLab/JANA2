//
// Author: Nathan Brei
//

#ifndef _ADCSampleFactory_h_
#define _ADCSampleFactory_h_

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JPerfUtils.h>

#include "ADCSample.h"
#include "INDRAMessage.h"

#include <fstream>

class ADCSampleFactory : public JFactoryT<ADCSample> {

    // parameters to simulate a bottle neck, spread is in sigmas
    size_t m_cputime_ms = 200;
    double m_cputime_spread = 0.25;

public:

    ADCSampleFactory() : JFactoryT<ADCSample>("ADCSampleFactory") {

        // acquire the parameters
        auto params = japp->GetJParameterManager();
        params->GetParameter("streamDet:rawhit_ms",     m_cputime_ms);
        params->GetParameter("streamDet:rawhit_spread", m_cputime_spread);
    };

    // process the message/event and construct the jobject
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

        // decode the message and populate the associated jobject (hit) for the event
        for (uint16_t sample = 0; sample < max_samples; ++sample) {
            for (uint16_t channel = 0; channel < max_channels; ++channel) {
                uint16_t current_value;
                int offset;
                sscanf(payload_buffer, "%hu%n", &current_value, &offset);
                payload_buffer += offset;
                auto hit = new ADCSample;
                hit->source_id  = source_id;
                hit->sample_id  = sample;
                hit->channel_id = channel;
                hit->adc_value  = current_value;
                Insert(hit);
            }
        }
        // do some throwaway work in order to simulate a bottleneck
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);
    }
};

#endif  // _ADCSampleFactory_h_
