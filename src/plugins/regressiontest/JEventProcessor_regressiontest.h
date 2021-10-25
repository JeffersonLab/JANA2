//
//    File: JEventProcessor_danarest.h
// Created: Mon Jul 1 09:08:37 EDT 2012
// Creator: Richard Jones
//

#ifndef _JEventProcessor_regressiontest_
#define _JEventProcessor_regressiontest_

#include <string>
#include <map>
#include <ostream>

using std::map;
using std::tuple;
using std::string;

#include <JANA/JEventProcessor.h>


class JEventProcessor_regressiontest : public JEventProcessor
{
public:
    void Init() override;
    void BeginRun(const std::shared_ptr<const JEvent>& event) override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void EndRun() override;
    void Finish() override;

private:
    map<tuple<uint64_t, string, string>, uint64_t> counts;
    map<tuple<uint64_t, string, string, int>, string> summaries;
    map<tuple<uint64_t, string, string, int, string>, string> summaries_expanded;
    bool expand_summaries = false;
    std::ofstream counts_file;
    std::ofstream summaries_file;
    std::string counts_file_name = "objcounts.tsv";
    std::string summaries_file_name = "objsummaries.tsv";
};

#endif // _JEventProcessor_regressiontest_