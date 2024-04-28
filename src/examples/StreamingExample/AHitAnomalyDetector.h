
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JHITCALIBRATOR_H
#define JANA2_JHITCALIBRATOR_H

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JPerfUtils.h>
#include "AHit.h"

class AHitAnomalyDetector : public JEventProcessor {

public:
    AHitAnomalyDetector(JApplication* app = nullptr, size_t delay_ms=1000)
        : JEventProcessor(app)
        , m_delay_ms(delay_ms) {
            SetCallbackStyle(CallbackStyle::ExpertMode);
        };

    void Init() override {

    }
    void Process(const JEvent& event) override {

        auto a_hits = event.Get<AHit>();
        std::stringstream ss;
        ss << "Anomaly detection: Event #" << event.GetEventNumber() << " : {";
        for (auto & hit : a_hits) {
            ss << "(" << hit->E << "," << hit->x << "), ";
        }
        ss << "}" << std::endl;
        std::cout << ss.str();
        consume_cpu_ms(m_delay_ms);
    }
    void Finish() override {
        std::cout << "Anomaly detection: Done!" << std::endl;
    }
private:
    size_t m_delay_ms;

};


#endif
