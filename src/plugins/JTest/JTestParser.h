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

    std::size_t mNumEventsToGenerate = 5000;
    std::size_t mNumEventsGenerated = 0;

public:

    JTestParser(string source_name, JApplication* app) : JEventSource(source_name, app)
    {
        app->GetParameter("nevents", mNumEventsToGenerate);
        app->GetParameter("jtest:parser_bytes", m_write_bytes);
        app->GetParameter("jtest:parser_ms", m_cputime_ms);
        app->GetParameter("jtest:parser_bytes_spread", m_write_spread);
        app->GetParameter("jtest:parser_spread", m_cputime_spread);

        SetFactoryGenerator(new JSourceFactoryGenerator<JTestEntangledEventData>());
        SetTypeName(NAME_OF_THIS);
    }

    static std::string GetDescription() {
        return "JTest Fake Event Source";
    }

    void GetEvent(std::shared_ptr<JEvent> event) {

        if ((mNumEventsGenerated % 40) == 0) {
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

        // Terminate when we've reached our limit
        if (mNumEventsToGenerate != 0 && mNumEventsToGenerate <= mNumEventsGenerated) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        mNumEventsGenerated++;

        event->SetEventNumber(mNumEventsGenerated);
        event->SetRunNumber(1);
    }

};

// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JTestParser>::CheckOpenable(std::string source) {
    return 1.0E-6;
}



