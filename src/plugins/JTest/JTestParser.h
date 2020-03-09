#pragma once

#include <vector>
#include <memory>

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JSourceFactoryGenerator.h>

#include <JANA/Utils/JPerfUtils.h>

#include "JTestDataObjects.h"


class JTestParser : public JEventSource {

    size_t m_cputime_ms = 10;
    size_t m_write_bytes = 2000000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;
    std::shared_ptr<std::vector<char>> m_latest_entangled_buffer;

    size_t m_max_event_count = 0;
    size_t m_events_generated = 0;

public:

    JTestParser(std::string source_name, JApplication* app) : JEventSource(source_name, app)
    {
        app->GetParameter("jtest:parser_bytes", m_write_bytes);
        app->GetParameter("jtest:parser_ms", m_cputime_ms);
        app->GetParameter("jtest:parser_bytes_spread", m_write_spread);
        app->GetParameter("jtest:parser_spread", m_cputime_spread);
        app->GetParameter("jtest:nevents", m_max_event_count);

        jout << "Hello from JTestParser" << jendl;
        SetFactoryGenerator(new JSourceFactoryGenerator<JTestEntangledEventData>());
        SetTypeName(NAME_OF_THIS);
    }

    static std::string GetDescription() {
        return "JTest Fake Event Source";
    }

    void GetEvent(std::shared_ptr<JEvent> event) {
        if (m_max_event_count != 0 && m_events_generated > m_max_event_count) {
            throw RETURN_STATUS::kNO_MORE_EVENTS;
        }

        if ((m_events_generated % 40) == 0) {
            // "Read" new entangled event every 40 events
            m_latest_entangled_buffer = std::shared_ptr<std::vector<char>>(new std::vector<char>);
            write_memory(*m_latest_entangled_buffer, m_write_bytes, m_write_spread);
        }

        // Spin the CPU
        consume_cpu_ms(m_cputime_ms);

        // Emit a shared pointer to the entangled event buffer
        auto eec = new JTestEntangledEventData;
        eec->buffer = m_latest_entangled_buffer;
        event->Insert<JTestEntangledEventData>(eec);

        m_events_generated++;

        event->SetEventNumber(m_events_generated);
        event->SetRunNumber(1);
    }

};

// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JTestParser>::CheckOpenable(std::string source) {
    return 1.0E-6;
}



