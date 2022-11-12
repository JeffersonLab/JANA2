
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include "JTestTracker.h"

class JTestPlotter : public JEventProcessor {

    size_t m_cputime_ms = 20;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestPlotter(JApplication* app) : JEventProcessor(app) {

        SetTypeName(NAME_OF_THIS);

        app->GetParameter("jtest:plotter_bytes", m_write_bytes);
        app->GetParameter("jtest:plotter_ms", m_cputime_ms);
        app->GetParameter("jtest:plotter_bytes_spread", m_write_spread);
        app->GetParameter("jtest:plotter_spread", m_cputime_spread);
    }

    virtual std::string GetType() const {
        return JTypeInfo::demangle<decltype(*this)>();
    }


    void Process(const std::shared_ptr<const JEvent>& aEvent) {

        // Read the track data
        auto td = aEvent->GetSingle<JTestTrackData>();
        read_memory(td->buffer);

        // Read the extra data objects inserted by JTestTracker
        aEvent->Get<JTestTracker::JTestTrackAuxilliaryData>();

        // Consume CPU
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        write_memory(hd->buffer, m_write_bytes, m_write_spread);
        aEvent->Insert(hd);
    }

};

#endif // JTestEventProcessor

