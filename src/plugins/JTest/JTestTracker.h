
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTESTTRACKER_H
#define JANA2_JTESTTRACKER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JBenchUtils.h>
#include "JTestDataObjects.h"

class JTestTracker : public JFactoryT<JTestTrackData> {

    Parameter<bool> m_except {this, "except", false, "Event #7 always excepts" };
    Parameter<bool> m_segfault {this, "segfault", false, "Event #7 always segfaults" };
    Parameter<bool> m_timeout {this, "timeout", false, "Event #7 always times out" };
    Parameter<size_t> m_cputime_ms {this, "cputime_ms", 200, "Time spent during tracking" };
    Parameter<size_t> m_write_bytes {this, "bytes", 1000, "Bytes written during tracking"};
    Parameter<double> m_cputime_spread {this, "cputime_spread", 0.25, "Spread of time spent during tracking"};
    Parameter<double> m_write_spread {this, "bytes_spread", 0.25, "Spread of bytes written during tracking"};

    JBenchUtils m_bench_utils;

public:
    JTestTracker() {
        SetPrefix("jtest:tracker");
        SetTypeName(NAME_OF_THIS);
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        m_bench_utils.set_seed(aEvent->GetEventNumber(), typeid(*this).name());
        // Read (large) event data
        auto ed = aEvent->GetSingle<JTestEventData>();
        m_bench_utils.read_memory(ed->buffer);

        // Do lots of computation
        m_bench_utils.consume_cpu_ms(*m_cputime_ms, *m_cputime_spread);

        // Optionally trigger failure scenarios
        if(aEvent->GetEventNumber() == 7) {
            // Only one of these can happen, so in principle the param should be an enum
            if (*m_except) {
                throw std::runtime_error("Something went wrong");
            }
            if (*m_segfault) {
                // Trigger a segfault on purpose
                JTestTrackData* d = nullptr;
                d->buffer[0] = 22;
            }
            if (*m_timeout) {
                m_bench_utils.consume_cpu_ms(1000000);
            }
        }

        // Write (small) track data
        auto td = new JTestTrackData;
        m_bench_utils.write_memory(td->buffer, *m_write_bytes, *m_write_spread);
        Insert(td);

        // Insert some additional objects
        std::vector<JTestTrackAuxilliaryData*> auxObjs;
        for(int i=0;i<4;i++) auxObjs.push_back( new JTestTrackAuxilliaryData);
        aEvent->Insert( auxObjs );
    }
};

#endif //JANA2_JTESTTRACKER_H
