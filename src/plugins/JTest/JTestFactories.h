#ifndef JTestFactories_h
#define JTestFactories_h

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/JPerfUtils.h>
#include "JTestDataObjects.h"

#include <random>
#include <chrono>
#include <iostream>


class JTestDisentangler : public JFactoryT<JTestEventData> {

    size_t m_cputime_ms = 200;
    size_t m_write_bytes = 100;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestDisentangler() : JFactoryT<JTestEventData>("JTestDisentangler") {
        auto params = japp->GetJParameterManager();
        params->SetDefaultParameter("jtest:disentangler_bytes", m_write_bytes, "");
        params->SetDefaultParameter("jtest:disentangler_ms", m_cputime_ms, "");
        params->SetDefaultParameter("jtest:disentangler_bytes_spread", m_write_spread, "");
        params->SetDefaultParameter("jtest:disentangler_spread", m_cputime_spread, "");
    };

    void Process(const std::shared_ptr<const JEvent> &aEvent) {

        // Read (large) entangled event data
        auto eed = aEvent->GetSingle<JTestEntangledEventData>();
        auto sum = read_memory(*eed->buffer);

        // Do a little bit of computation
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (large) event data
        auto ed = new JTestEventData;
        write_memory(ed->buffer, m_write_bytes, m_write_spread);
        Insert(ed);
    }
};



class JTestTracker : public JFactoryT<JTestTrackData> {

    size_t m_cputime_ms = 200;
    size_t m_write_bytes = 100;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestTracker() : JFactoryT<JTestTrackData>("JTestTracker") {
        auto params = japp->GetJParameterManager();
        params->SetDefaultParameter("jtest:tracker_bytes", m_write_bytes, "");
        params->SetDefaultParameter("jtest:tracker_ms", m_cputime_ms, "");
        params->SetDefaultParameter("jtest:tracker_bytes_spread", m_write_spread, "");
        params->SetDefaultParameter("jtest:tracker_spread", m_cputime_spread, "");
    };

    void Process(const std::shared_ptr<const JEvent> &aEvent) {

        // Read (large) event data
        auto ed = aEvent->GetSingle<JTestEventData>();
        auto sum = read_memory(ed->buffer);

        // Do lots of computation
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (small) track data
        auto td = new JTestTrackData;
        write_memory(td->buffer, m_write_bytes, m_write_spread);
        Insert(td);
    }
};

#endif // JTestFactories_h

