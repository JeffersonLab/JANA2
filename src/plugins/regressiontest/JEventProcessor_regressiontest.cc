//
// JEventProcessor_regressiontest.cc
//
// JANA event processor plugin writes out rest events to a file
//
// Richard Jones, 1-July-2012

#include "JEventProcessor_regressiontest.h"
#include "JInspector.h"

#include <tuple>

// Make us a plugin
// for initializing plugins
extern "C" {
void InitPlugin(JApplication *app) {
    InitJANAPlugin(app);
    app->Add(new JEventProcessor_regressiontest());
    app->SetParameterValue("RECORD_CALL_STACK", true);
}
} // "extern C"

//-------------------------------
// Init
//-------------------------------
void JEventProcessor_regressiontest::Init()
{
    auto app = GetApplication();
    app->SetTicker(false);
    app->SetDefaultParameter("regressiontest:counts_file_name", counts_file_name);
    app->SetDefaultParameter("regressiontest:summaries_file_name", summaries_file_name);
    app->SetDefaultParameter("regressiontest:expand_summaries", expand_summaries);
    app->SetDefaultParameter("regressiontest:expand_summaries", expand_summaries);


    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "Welcome to JANA's interactive inspector! Type `PrintHelp` to see available commands." << std::endl;
}

//-------------------------------
// BeginRun
//-------------------------------
void JEventProcessor_regressiontest::BeginRun(const std::shared_ptr<const JEvent>&)
{
}

//-------------------------------
// Process
//-------------------------------
void JEventProcessor_regressiontest::Process(const std::shared_ptr<const JEvent>& event)
{
    auto app = GetApplication();
    auto evt_nr = event->GetEventNumber();
    auto run_nr = event->GetRunNumber();
    auto facs = event->GetAllFactories();

    std::lock_guard<std::mutex> lock(m_mutex);
    JInspector introspection (event.get());
    if ((evt_nr == m_next_event_nr) || (m_next_event_nr == 0)) {
        m_next_event_nr = introspection.DoReplLoop(event->GetEventNumber());
    }

    for (auto fac : facs) {
        auto item_ct = fac->Create(event, app, run_nr);
        auto key = make_tuple(evt_nr, fac->GetObjectName(), fac->GetTag());
        counts.insert({key, item_ct});

        int i = 0;
        for (auto obj : fac->GetAs<JObject>()) {

            JObjectSummary summary;
            obj->Summarize(summary);

            if (expand_summaries) {
                for (auto& field : summary.get_fields()) {
                    auto key = make_tuple(evt_nr, fac->GetObjectName(), fac->GetTag(), i++, field.name);
                    summaries_expanded.insert({key, field.value});
                }
            }
            else {
                std::stringstream ss;
                ss << "{";
                for (auto& field : summary.get_fields()) {
                    ss << field.name << ": " << field.value << ", ";
                }
                ss << "}";
                auto key = make_tuple(evt_nr, fac->GetObjectName(), fac->GetTag(), i++);
                summaries.insert({key, ss.str()});
            }
        }
    }
}

//-------------------------------
// EndRun
//-------------------------------
void JEventProcessor_regressiontest::EndRun()
{
}

//-------------------------------
// Finish
//-------------------------------
void JEventProcessor_regressiontest::Finish()
{
    counts_file.open(counts_file_name);

    for (auto pair : counts) {
        string fac_name, fac_tag;
        uint64_t evt_nr, obj_count;
        std::tie(evt_nr, fac_name, fac_tag) = pair.first;
        obj_count = pair.second;
        counts_file << evt_nr << "\t" << fac_name << "\t" << fac_tag << "\t" << obj_count << std::endl;
    }
    counts_file.close();

    summaries_file.open(summaries_file_name);

    if (!expand_summaries) {
        for (auto pair : summaries) {
            string fac_name, fac_tag, obj_summary;
            uint64_t evt_nr;
            int obj_idx;
            std::tie(evt_nr, fac_name, fac_tag, obj_idx) = pair.first;
            obj_summary = pair.second;
            summaries_file << evt_nr << "\t" << fac_name << "\t" << fac_tag << "\t" << obj_idx << "\t" << obj_summary << std::endl;
        }
    }
    else {
        for (auto pair : summaries_expanded) {
            string fac_name, fac_tag, field_name, field_val;
            uint64_t evt_nr;
            int obj_idx;
            std::tie(evt_nr, fac_name, fac_tag, obj_idx, field_name) = pair.first;
            field_val = pair.second;
            summaries_file << evt_nr << "\t" << fac_name << "\t" << fac_tag << "\t" << obj_idx << "\t" << field_name << "\t" << field_val << std::endl;
        }
    }
    summaries_file.close();
}