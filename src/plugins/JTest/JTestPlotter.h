#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JTestPlotter : public JEventProcessor {

    size_t m_cputime_ms = 20;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestPlotter(JApplication* app) : JEventProcessor(app) {

        auto params = app->GetJParameterManager();

        params->GetParameter("jtest:plotter_bytes", m_write_bytes);
        params->GetParameter("jtest:plotter_ms", m_cputime_ms);
        params->GetParameter("jtest:plotter_bytes_spread", m_write_spread);
        params->GetParameter("jtest:plotter_spread", m_cputime_spread);
    }

    virtual std::string GetType() const {
        return JTypeInfo::demangle<decltype(*this)>();
    }


    void Process(const std::shared_ptr<const JEvent>& aEvent) {

        // Read the track data
        auto td = aEvent->GetSingle<JTestTrackData>();
        read_memory(td->buffer);

        // Consume CPU
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        write_memory(hd->buffer, m_write_bytes, m_write_spread);
        aEvent->Insert(hd);
    }

};

#endif // JTestEventProcessor

