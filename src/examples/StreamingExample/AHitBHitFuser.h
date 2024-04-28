
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JHITCALIBRATOR_H
#define JANA2_JHITCALIBRATOR_H

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
#include <JANA/JPerfUtils.h>
#include "AHit.h"

/// AHitBHitFuser
class AHitBHitFuser : public JEventProcessor {

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
        auto b_hits = event.Get<BHit>();
        std::stringstream ss;
        ss << "AHit/BHit fusion: Event #" << event.GetEventNumber() << " : {";
        for (auto & hit : a_hits) {
            ss << "(" << hit->E << "," << hit->t << "), ";
        }
        ss << "}, ";
        for (auto & hit : b_hits) {
            ss << "(" << hit->E << "," << hit->t << "), ";
        }
        ss << "}" << std::endl;
        std::cout << ss.str();
        consume_cpu_ms(m_delay_ms);


        auto raw_hits = event.Get<AHit>("raw_hits");


        std::cout << "Processing event #" << event.GetEventNumber() << std::endl;
        Serializer<AHit> serializer;
        for (auto & hit : raw_hits) {
            AHit* calibrated_hit = new DetectorAHit(*hit);
            calibrated_hit->V += 7;
            std::cout << serializer.serialize(*calibrated_hit) << std::endl;
        }
        consume_cpu_ms(m_delay_ms);
    }
    void Finish() override {
        std::cout << "Done!" << std::endl;
    }
private:
    size_t m_delay_ms;

};


#endif
