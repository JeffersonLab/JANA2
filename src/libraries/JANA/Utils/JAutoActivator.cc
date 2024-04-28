
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JAutoActivator.h"

JAutoActivator::JAutoActivator() {
    SetTypeName("JAutoActivator");
}

void JAutoActivator::AddAutoActivatedFactory(string factory_name, string factory_tag) {
    m_auto_activated_factories.push_back({std::move(factory_name), std::move(factory_tag)});
}

/// Converts "ObjName:TagName" into ("ObjName", "TagName")
///      and "name::space::ObjName:TagName" into ("name::space::ObjName", "TagName")
std::pair<std::string, std::string> JAutoActivator::Split(std::string factory_name) {

    std::string::size_type pos = -1;
    while (true) {
        // Find the next colon
        pos = factory_name.find(':', pos+1);

        if (pos == std::string::npos) {
            // There are no colons at all remaining. Everything is objname
            return std::make_pair(factory_name, "");
        }
        else if (factory_name[pos+1] == ':') {
            // Else we found a double colon, which is part of the objname namespace. Keep going.
            pos += 1;
        }
        else {
            // We found the first single colon, which has to delimit the objname from the tag. Stop.
            return std::make_pair(factory_name.substr(0, pos), factory_name.substr(pos+1));
        }
    }
}

void JAutoActivator::Init() {

    string autoactivate_conf;
    if (GetApplication()->GetParameter("autoactivate", autoactivate_conf)) {
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
                    auto pair = Split(myfactories[i]);
                    AddAutoActivatedFactory(pair.first, pair.second);
                }
            }
        }
        catch (...) {
            LOG_ERROR(GetLogger()) << "Error parsing parameter 'autoactivate'. Found: " << autoactivate_conf << LOG_END;
            throw JException("AutoActivator could not parse parameter 'autoactivate'");
        }
    }
}

void JAutoActivator::Process(const JEvent& event) {
    for (const auto &pair: m_auto_activated_factories) {
        auto name = pair.first;
        auto tag = pair.second;
        auto factory = event.GetFactory(name, tag);
        if (factory != nullptr) {
            factory->Create(event.shared_from_this()); // This will do nothing if factory is already created
        }
        else {
            LOG_ERROR(GetLogger()) << "Could not find factory with typename=" << name << ", tag=" << tag << LOG_END;
            throw JException("AutoActivator could not find factory with typename=%s, tag=%s", name.c_str(), tag.c_str());
        }
    }
}

