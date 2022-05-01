
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JAutoActivator.h"

JAutoActivator::JAutoActivator() {
    SetTypeName("JAutoActivator");
}

bool JAutoActivator::IsRequested(std::shared_ptr <JParameterManager> params) {
    return params->Exists("AUTOACTIVATE") && (!params->GetParameterValue<string>("AUTOACTIVATE").empty());
}

void JAutoActivator::AddAutoActivatedFactory(string factory_name, string factory_tag) {
    m_auto_activated_factories.push_back({std::move(factory_name), std::move(factory_tag)});
}

void JAutoActivator::Init() {

    string autoactivate_conf;
    if (GetApplication()->GetParameter("AUTOACTIVATE", autoactivate_conf)) {
        try {
            if (!autoactivate_conf.empty()) {
                // Loop over comma separated list of factories to auto activate
                vector <string> myfactories;
                string &str = autoactivate_conf;
                unsigned int cutAt;
                while ((cutAt = str.find(",")) != (unsigned int) str.npos) {
                    if (cutAt > 0)myfactories.push_back(str.substr(0, cutAt));
                    str = str.substr(cutAt + 1);
                }
                if (str.length() > 0)myfactories.push_back(str);

                // Loop over list of factory strings (which could be in factory:tag
                // form) and parse the strings as needed in order to add them to
                // the auto activate list.
                for (unsigned int i = 0; i < myfactories.size(); i++) {
                    string nametag = myfactories[i];
                    string name = nametag;
                    string tag = "";
                    string::size_type pos = nametag.find(":");
                    if (pos != string::npos) {
                        name = nametag.substr(0, pos);
                        tag = nametag.substr(pos + 1, nametag.size() - pos);
                    }
                    AddAutoActivatedFactory(name, tag);
                }
            }
        }
        catch (...) {
            LOG << "Error parsing AUTOACTIVATE=" << autoactivate_conf << LOG_END;
        }
    }
}

void JAutoActivator::Process(const std::shared_ptr<const JEvent> &event) {
    for (const auto &pair: m_auto_activated_factories) {
        auto name = pair.first;
        auto tag = pair.second;
        auto factory = event->GetFactory(name, tag);
        if (factory != nullptr) {
            factory->Create(event, event->GetJApplication(), event->GetRunNumber());
        }
        else {
            LOG << "Warning: Could not find factory with name=" << name << ", tag=" << tag << LOG_END;
        }
    }
}

