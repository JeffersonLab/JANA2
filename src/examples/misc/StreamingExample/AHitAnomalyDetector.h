
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JHITCALIBRATOR_H
#define JANA2_JHITCALIBRATOR_H

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JBenchUtils.h>
#include "AHit.h"

class AHitAnomalyDetector : public JEventProcessor {

public:
    AHitAnomalyDetector(size_t delay_ms=1000) : m_delay_ms(delay_ms) {
            SetCallbackStyle(CallbackStyle::ExpertMode);
        };

    void Init() override {

    }
    void ProcessSequential(const JEvent& event) override {

        auto a_hits = event.Get<AHit>();
        std::stringstream ss;
        ss << "Anomaly detection: Event #" << event.GetEventNumber() << " : {";
        for (auto & hit : a_hits) {
            ss << "(" << hit->E << "," << hit->x << "), ";
        }
        ss << "}" << std::endl;
        std::cout << ss.str();
        m_bench_utils.set_seed(event.GetEventNumber(), NAME_OF_THIS);
        m_bench_utils.consume_cpu_ms(m_delay_ms);
    }
    void Finish() override {
        std::cout << "Anomaly detection: Done!" << std::endl;
    }
private:
    size_t m_delay_ms;
    JBenchUtils m_bench_utils = JBenchUtils();

};


#endif
