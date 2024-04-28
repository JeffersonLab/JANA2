
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include "JTestTracker.h"
#include <mutex>

class JTestPlotter : public JEventProcessor {

    size_t m_cputime_ms = 0;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;
    std::mutex m_mutex;

public:

    JTestPlotter() {
        SetPrefix("jtest:plotter");
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Init() override {
        auto app = GetApplication();
        app->SetDefaultParameter("jtest:plotter_ms", m_cputime_ms, "Time spent during plotting");
        app->SetDefaultParameter("jtest:plotter_spread", m_cputime_spread, "Spread of time spent during plotting");
        app->SetDefaultParameter("jtest:plotter_bytes", m_write_bytes, "Bytes written during plotting");
        app->SetDefaultParameter("jtest:plotter_bytes_spread", m_write_spread, "Spread of bytes written during plotting");
    }

    void Process(const JEvent& event) override {

        // Read the track data
        auto td = event.GetSingle<JTestTrackData>();
        read_memory(td->buffer);

        // Read the extra data objects inserted by JTestTracker
        event.Get<JTestTracker::JTestTrackAuxilliaryData>();

        // Everything that happens after here is in a critical section
        std::lock_guard<std::mutex> lock(m_mutex);

        // Consume CPU
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        write_memory(hd->buffer, m_write_bytes, m_write_spread);
        event.Insert(hd);
    }

};

#endif // JTestEventProcessor

