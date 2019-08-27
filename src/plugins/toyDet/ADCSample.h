//
// Author: Nathan Brei
//

#ifndef _ADCSample_h_
#define _ADCSample_h_

#include <JANA/JObject.h>

struct ADCSample : public JObject {

    uint16_t source_id;
    uint16_t channel_id;
    uint16_t sample_id;
    uint16_t adc_value;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(source_id,  NAME_OF(source_id), "%d");
        summary.add(sample_id,  NAME_OF(sample_id),  "%d");
        summary.add(channel_id, NAME_OF(channel_id), "%d");
        summary.add(adc_value,  NAME_OF(adc_value),  "%d");
    }
};

#endif  // _ADCSample_h_
