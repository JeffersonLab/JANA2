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
    if (interactive) {
        event->Inspect();
        return;
    }
    auto app = GetApplication();
    auto evt_nr = event->GetEventNumber();
    auto run_nr = event->GetRunNumber();
    for (auto fac : event->GetAllFactories()) {
        fac->Create(event, app, run_nr); // Make sure all factories have run
    }
    auto facs = GetFactoriesTopologicallyOrdered(*event);


    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto fac : facs) {
        bool found_discrepancy = false;
	auto jobs = fac->GetAs<JObject>();
        auto item_ct = jobs.size();
        int old_item_ct = 0;

        // Generate line describing factory counts
        std::ostringstream os;
        os << evt_nr << "\t" << fac->GetObjectName() << "\t" << fac->GetTag() << "\t" << item_ct;
        std::string count_line = os.str();

        new_log_file << count_line << std::endl;

        if (have_old_log_file) {
            std::string old_count_line;
            std::getline(old_log_file, old_count_line);
            old_item_ct = ParseOldItemCount(old_count_line);

            if (old_count_line != count_line) {
                std::cout << "MISMATCH" << std::endl;
                std::cout << "OLD COUNT: " << old_count_line << std::endl;
                std::cout << "NEW COUNT: " << count_line << std::endl;
                found_discrepancy = true;
            }
	    else {
		    // std::cout << "MATCH " << count_line << std::endl;
	    }
        }

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
            for (int i=0; i<old_item_ct; ++i) {
                std::string old_object_line;
                std::getline(old_log_file, old_object_line);
		if ((size_t) i >= new_object_lines.size()) {
                    std::cout << "MISMATCH: " << old_item_ct << " vs " << item_ct << std::endl;
                    std::cout << "OLD OBJ: " << old_object_line << std::endl;
                    std::cout << "NEW OBJ: missing" << std::endl;
                    found_discrepancy = true;
		}
		else if (old_object_line != new_object_lines[i]) {
                    found_discrepancy = true;
                    std::cout << "MISMATCH" << std::endl;
                    std::cout << "OLD OBJ: " << old_object_line << std::endl;
                    std::cout << "NEW OBJ: " << new_object_lines[i] << std::endl;
                }
		else {
                    // std::cout << "MATCH" << old_object_line << std::endl;
		}
            }
        }
        if (found_discrepancy) {
            event->Inspect();
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

int JEventProcessor_regressiontest::ParseOldItemCount(std::string old_count_line) {

    std::istringstream iss(old_count_line);
    std::string split;
    std::getline(iss, split, '\t');
    std::getline(iss, split, '\t');
    std::getline(iss, split, '\t');
    int result;
    iss >> result;
    return result;

}
