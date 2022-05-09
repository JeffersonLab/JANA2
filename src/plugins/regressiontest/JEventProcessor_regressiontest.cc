//
// JEventProcessor_regressiontest.cc
//
// JANA event processor plugin writes out rest events to a file
//
// Richard Jones, 1-July-2012

#include "JEventProcessor_regressiontest.h"
#include "JANA/Utils/JInspector.h"

#include <tuple>

// Make us a plugin
// for initializing plugins
extern "C" {
void InitPlugin(JApplication *app) {
    InitJANAPlugin(app);
    app->Add(new JEventProcessor_regressiontest());
    app->SetParameterValue("record_call_stack", true);
}
} // "extern C"

//-------------------------------
// Init
//-------------------------------
void JEventProcessor_regressiontest::Init()
{
    auto app = GetApplication();
    app->SetTicker(false);
    app->SetTimeoutEnabled(false);
    app->SetDefaultParameter("regressiontest:interactive", interactive);
    app->SetDefaultParameter("regressiontest:old_log", old_log_file_name);
    app->SetDefaultParameter("regressiontest:new_log", new_log_file_name);
    LOG << "Running regressiontest plugin" << LOG_END;
    old_log_file.open(old_log_file_name);
    if (old_log_file.good()) {
        have_old_log_file = true;
    }
    new_log_file.open(new_log_file_name);

    blacklist_file.open(blacklist_file_name);
    std::string line;
    while (std::getline(blacklist_file, line)) {
        blacklist.insert(line);
    }
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
    for (auto fac : event->GetAllFactories()) {
        fac->Create(event, app, run_nr); // Make sure all factories have run
    }
    auto facs = GetFactoriesTopologicallyOrdered(*event);

    std::lock_guard<std::mutex> lock(m_mutex);

    std::map<std::string, std::vector<std::string>> old_event_log;
    std::set<std::string> event_discrepancies;
    if (have_old_log_file) {
        // Read entire event from log and store
        std::string line = "throwaway";
        while (!line.empty()) {
            // Empty line demarcates next event
            // Assume events are in the same order
            std::getline(old_log_file, line);
            int item_count;
            std::string fac_key;
            auto pair = ParseFactorySummary(line);
            fac_key = pair.first;
            item_count = pair.second;

            std::vector<std::string> jobjs_summaries;
            for (int i=0; i<item_count; ++i) {
                std::getline(old_log_file, line);
                jobjs_summaries.push_back(line);
            }
            old_event_log[fac_key] = std::move(jobjs_summaries);
        }
    }

    for (auto fac : facs) {
        bool found_discrepancy = false;
	auto jobs = fac->GetAs<JObject>();
        auto item_ct = jobs.size();
        int old_item_ct = 0;

        // Generate line describing factory counts
        std::ostringstream os;
        std::string fac_key = fac->GetObjectName();
        if (!fac->GetTag().empty()) fac_key += ":" + fac->GetTag();

        if (fac->GetTag().empty()) {
            os << evt_nr << "\t" << fac->GetObjectName() << "\t" << item_ct;
        }
        else {
            os << evt_nr << "\t" << fac->GetObjectName() << ":" << fac->GetTag() << "\t" << item_ct;
        }
        std::string count_line = os.str();

        new_log_file << count_line << std::endl;

        std::vector<std::string> new_object_lines;

        for (auto obj : jobs) {

            JObjectSummary summary;
            obj->Summarize(summary);

            std::stringstream ss;
            ss << evt_nr << "\t" << fac->GetObjectName() << "\t" << fac->GetTag() << "\t";
            ss << "{";
            for (auto& field : summary.get_fields()) {
                std::string blacklist_entry = fac->GetObjectName() + "\t" + fac->GetTag() + "\t" + field.name;
                if (blacklist.find(blacklist_entry) == blacklist.end()) {
                    ss << field.name << ": " << field.value << ", ";
                }
            }
            ss << "}";
            new_object_lines.push_back(ss.str());
        }

        std::sort(new_object_lines.begin(), new_object_lines.end());
        for (const auto& s : new_object_lines) {
            new_log_file << s << std::endl;
        }

        if (have_old_log_file) {
            const std::vector<std::string>& old_object_lines = old_event_log[fac_key];
            if (item_ct != old_object_lines.size()) {
                found_discrepancy = true;
                event_discrepancies.insert(fac_key);
                std::cout << "MISCOUNT: " << fac_key << ", old=" << old_object_lines.size() << ", new=" << item_ct << std::endl;
            }
            else {
                for (size_t i=0; i<item_ct; ++i) {
                    if (old_object_lines[i] != new_object_lines[i]) {
                        found_discrepancy = true;
                        event_discrepancies.insert(fac_key);
                        std::cout << "MISMATCH: " << fac_key << std::endl;
                        std::cout << "OLD OBJ: " << old_object_lines[i] << std::endl;
                        std::cout << "NEW OBJ: " << new_object_lines[i] << std::endl;
                    }
                }
            }
        }
        if (found_discrepancy) discrepancy_counts[fac_key] += 1;
    } // for each factory
    new_log_file << std::endl; // Include a blank line to indicate end-of-event

    if (interactive) {
        auto inspector = event->GetJInspector();
        inspector->SetDiscrepancies(std::move(event_discrepancies));
        inspector->Loop();
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
    std::cout << "OVERALL DISCREPANCIES" << std::endl;
    for (auto p : discrepancy_counts) {
        std::cout << p.first << "\t" << p.second;
    }
    new_log_file.close();
    if (have_old_log_file) {
        old_log_file.close();
    }
}


std::vector<JFactory*> JEventProcessor_regressiontest::GetFactoriesTopologicallyOrdered(const JEvent& event) {

    std::vector<JFactory*> sorted_factories;
    auto topologicalOrdering = event.GetJCallGraphRecorder()->TopologicalSort();
    for (auto pair : topologicalOrdering) {
        auto fac_name = pair.first;
        auto fac_tag = pair.second;
        JFactory* fac = event.GetFactory(fac_name, fac_tag);
        sorted_factories.push_back(fac);
    }
    return sorted_factories;
}

std::pair<std::string, int> JEventProcessor_regressiontest::ParseFactorySummary(std::string line) {

    std::istringstream iss(line);
    std::string facname;
    std::string split;
    std::getline(iss, split, '\t');  // Event number
    std::getline(iss, facname, '\t'); // Factory key
    int count;
    iss >> count;
    return std::make_pair(facname, count);

}
