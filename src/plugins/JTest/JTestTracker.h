
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTESTTRACKER_H
#define JANA2_JTESTTRACKER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JPerfUtils.h>
#include "JTestDataObjects.h"

class JTestTracker : public JFactoryT<JTestTrackData> {

    size_t m_cputime_ms = 200;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestTracker() : JFactoryT<JTestTrackData>("JTestTracker") {};


    void Init() override {

        auto app = GetApplication();
        assert (app != nullptr);
        app->GetParameter("jtest:tracker_bytes", m_write_bytes);
        app->GetParameter("jtest:tracker_ms", m_cputime_ms);
        app->GetParameter("jtest:tracker_bytes_spread", m_write_spread);
        app->GetParameter("jtest:tracker_spread", m_cputime_spread);
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        // Read (large) event data
        auto ed = aEvent->GetSingle<JTestEventData>();
        read_memory(ed->buffer);

        // Do lots of computation
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (small) track data
        auto td = new JTestTrackData;
        write_memory(td->buffer, m_write_bytes, m_write_spread);
        Insert(td);
    }
};

#endif //JANA2_JTESTTRACKER_H
