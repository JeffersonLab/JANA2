
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTESTTRACKER_H
#define JANA2_JTESTTRACKER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JBenchUtils.h>
#include "JTestDataObjects.h"

class JTestTracker : public JFactoryT<JTestTrackData> {

    size_t m_cputime_ms = 200;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;
    JBenchUtils m_bench_utils = JBenchUtils();

public:

    struct JTestTrackAuxilliaryData{
        int something = 1;
        float something2 = 2;
    };

    void Init() override {

        auto app = GetApplication();
        app->SetDefaultParameter("jtest:tracker_ms", m_cputime_ms, "Time spent during tracking");
        app->SetDefaultParameter("jtest:tracker_spread", m_cputime_spread, "Spread of time spent during tracking");
        app->SetDefaultParameter("jtest:tracker_bytes", m_write_bytes, "Bytes written during tracking");
        app->SetDefaultParameter("jtest:tracker_bytes_spread", m_write_spread, "Spread of bytes written during tracking");
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        m_bench_utils.set_seed(aEvent->GetEventNumber(), typeid(*this).name());
        // Read (large) event data
        auto ed = aEvent->GetSingle<JTestEventData>();
        m_bench_utils.read_memory(ed->buffer);

        // Do lots of computation
        m_bench_utils.consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (small) track data
        auto td = new JTestTrackData;
        m_bench_utils.write_memory(td->buffer, m_write_bytes, m_write_spread);
        Insert(td);

        // Insert some additional objects
        std::vector<JTestTrackAuxilliaryData*> auxObjs;
        for(int i=0;i<4;i++) auxObjs.push_back( new JTestTrackAuxilliaryData);
        aEvent->Insert( auxObjs );
    }
};

#endif //JANA2_JTESTTRACKER_H
