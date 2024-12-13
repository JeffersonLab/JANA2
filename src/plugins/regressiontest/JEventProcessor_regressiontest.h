//
//    File: JEventProcessor_danarest.h
// Created: Mon Jul 1 09:08:37 EDT 2012
// Creator: Richard Jones
//

#ifndef _JEventProcessor_regressiontest_
#define _JEventProcessor_regressiontest_

#include <JANA/JEventProcessor.h>

#include <fstream>
#include <map>
#include <string>


class JEventProcessor_regressiontest : public JEventProcessor
{
public:
    void Init() override;
    void BeginRun(const std::shared_ptr<const JEvent>& event) override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void EndRun() override;
    void Finish() override;

private:
    std::mutex m_mutex;

    bool interactive = true;
    bool have_old_log_file = false;
    std::ifstream old_log_file;
    std::string old_log_file_name = "regression_log_old.tsv";
    std::ofstream new_log_file;
    std::string new_log_file_name = "regression_log_new.tsv";
    std::ifstream blacklist_file;
    std::string blacklist_file_name = "blacklist.tsv";
    std::set<std::string> blacklist;
    std::map<std::string, int> discrepancy_counts;

    std::vector<JFactory*> GetFactoriesTopologicallyOrdered(const JEvent& event);
    std::pair<std::string, int> ParseFactorySummary(std::string line);
};

#endif // _JEventProcessor_regressiontest_
