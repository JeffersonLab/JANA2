
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include <JANA/Utils/JBenchUtils.h>
#include "JTestTracker.h"
#include <mutex>

class JTestPlotter : public JEventProcessor {

    size_t m_cputime_ms = 0;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;
    JBenchUtils m_bench_utils = JBenchUtils();
    std::mutex m_mutex;

public:

    JTestPlotter() {
        SetPrefix("jtest:plotter");
        SetTypeName(NAME_OF_THIS);
    }

    void Init() override {
        auto app = GetApplication();
        app->SetDefaultParameter("jtest:plotter_ms", m_cputime_ms, "Time spent during plotting");
        app->SetDefaultParameter("jtest:plotter_spread", m_cputime_spread, "Spread of time spent during plotting");
        app->SetDefaultParameter("jtest:plotter_bytes", m_write_bytes, "Bytes written during plotting");
        app->SetDefaultParameter("jtest:plotter_bytes_spread", m_write_spread, "Spread of bytes written during plotting");
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {

        m_bench_utils.set_seed(event->GetEventNumber(), typeid(*this).name());
        // Read the track data
        auto td = event->GetSingle<JTestTrackData>();
        m_bench_utils.read_memory(td->buffer);

        // Read the extra data objects inserted by JTestTracker
        event->Get<JTestTracker::JTestTrackAuxilliaryData>();

        // Everything that happens after here is in a critical section
        std::lock_guard<std::mutex> lock(m_mutex);

        // Consume CPU
        m_bench_utils.consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        m_bench_utils.write_memory(hd->buffer, m_write_bytes, m_write_spread);
        event->Insert(hd);
    }

};

#endif // JTestEventProcessor

