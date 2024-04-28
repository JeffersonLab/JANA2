
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JCSVEVENTPROCESSOR_H
#define JANA2_JCSVEVENTPROCESSOR_H


#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include <fstream>

template <typename T>
class JCsvWriter : public JEventProcessor {
private:
    std::string m_tag;
    std::string m_dest_dir = ".";
    std::fstream m_dest_file;
    std::mutex m_mutex;
    bool m_header_written = false;

public:

    JCsvWriter(std::string tag = "") : m_tag(std::move(tag)) {
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    };

    void Init() override {

        GetApplication()->SetDefaultParameter("csv:dest_dir", m_dest_dir, "Location where CSV files get written");

        std::string filename;
        if (m_tag == "") {
            filename = m_dest_dir + "/" + JTypeInfo::demangle<T>() + ".csv";
        }
        else {
            filename = m_dest_dir + "/" + JTypeInfo::demangle<T>() + "_" + m_tag + ".csv";
        }
        m_dest_file.open(filename, std::fstream::out);
    }

    void Process(const JEvent& event) override {

        auto event_nr = event.GetEventNumber();
        auto jobjs = event.Get<T>(m_tag);

        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_header_written) {
            if (jobjs.size() > 0) {
                JObjectSummary summary;
                jobjs[0]->Summarize(summary);
                m_dest_file << "EventNr";
                for (auto& field : summary.get_fields()) {
                    m_dest_file << ", " << field.name;
                }
                m_dest_file << std::endl;
                m_header_written = true;
            }
        }

        for (auto obj : jobjs) {

            JObjectSummary summary;
            obj->Summarize(summary);
            m_dest_file << event_nr;
            for (auto& field : summary.get_fields()) {
                m_dest_file << ", " << field.value;
            }
            m_dest_file << std::endl;
        }

    }

    void Finish(void) override {
        m_dest_file.close();
    }

};


#endif //JANA2_JCSVEVENTPROCESSOR_H
