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
    size_t m_cputime_ms = 0;
    double m_cputime_spread = 0.25;

public:

    ADCSampleFactory() : JFactoryT<ADCSample>("ADCSampleFactory") {

        // acquire the parameters
        auto params = japp->GetJParameterManager();
        params->GetParameter("streamDet:rawhit_ms",     m_cputime_ms);
        params->GetParameter("streamDet:rawhit_spread", m_cputime_spread);
        SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER); // TODO: This is a memory leak
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

        // Allocate all samples at once, fill later
        ADCSample* samples = new ADCSample[max_channels*max_samples];
        size_t i = 0;

        // decode the message and populate the associated jobject (hit) for the event
        for (uint16_t sample = 0; sample < max_samples; ++sample) {
            for (uint16_t channel = 0; channel < max_channels; ++channel) {

                uint16_t current_value = (payload_buffer[0]-48) * 1000 + (payload_buffer[1]-48) * 100 + (payload_buffer[2]-48) * 10 + (payload_buffer[3]-48);
                //assert(current_value >= 0);
                //assert(current_value <= 1024);
                payload_buffer += 5;
                ADCSample& hit = samples[i++];
                hit.source_id  = source_id;
                hit.sample_id  = sample;
                hit.channel_id = channel;
                hit.adc_value  = current_value;
                Insert(&hit);
            }
        }
    }
};

#endif  // _ADCSampleFactory_h_
