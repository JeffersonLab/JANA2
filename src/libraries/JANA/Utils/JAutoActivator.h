
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once
#include <JANA/JEventProcessor.h>
#include <fstream>

using std::vector;
using std::string;
using std::pair;

class JAutoActivator : public JEventProcessor {
private:
    bool m_output_processed_event_numbers = false;
    std::ofstream m_processed_event_numbers_file;
    vector<pair<string,string>> m_auto_activated_factories;

public:
    JAutoActivator();
    static std::pair<std::string, std::string> Split(std::string factory_name);
    void AddAutoActivatedFactory(string factory_name, string factory_tag);
    void Init() override;
    void ProcessParallel(const JEvent&) override;
    void Process(const JEvent&) override;
    void Finish() override;

};


