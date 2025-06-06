// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JControlEventProcessor_h_
#define _JControlEventProcessor_h_

#include <map>
#include <set>
#include <sstream>
#include <JANA/JEventProcessor.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Utils/JStringification.h>

template <typename T>
std::string ToString(const T &t){
    std::stringstream ss;
    ss << t;
    return ss.str();
}

/// The JControlEventProcessor class is used by the janacontrol plugin, primarily
/// to help with its debugging feature. It can be used to stall event processing
/// while events are inspected. It will also grab lists of factories and objects
/// from a JEvent and encode them in forms (e.g. strings) that can be easily
/// browsed.
class JControlEventProcessor : public JEventProcessor {

    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;

    struct JFactorySummaryCompare
    {
        bool operator() (const JFactorySummary& lhs, const JFactorySummary& rhs) const
        {
            if( lhs.object_name != rhs.object_name ) return lhs.object_name < rhs.object_name;
            if( lhs.factory_tag != rhs.factory_tag ) return lhs.factory_tag < rhs.factory_tag;
            if( lhs.factory_name != rhs.factory_name ) return lhs.factory_name < rhs.factory_name;
            return lhs.plugin_name < rhs.plugin_name;
        }
    };


public:

    JControlEventProcessor();
    virtual ~JControlEventProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

    void SetDebugMode(bool debug_mode);
    bool GetDebugMode(void){return _debug_mode;}
    void SetFetchFactories(std::set<std::string> &factorytags);
    bool GetFetchFlag(void){return _fetch_flag;}
    void FetchObjects(std::shared_ptr<const JEvent> jevent);
    void FetchObjectsNow(void);
    auto GetLastFetchResult(void){return _fetch_object_summaries;}
    auto GetLastFetchMetadata(void){return _fetch_metadata;}
    void NextEvent(void);
    void GetObjectStatus( std::map<JFactorySummary, std::size_t> &factory_object_counts );
    void GetObjects(const std::string &factory_name, const std::string &factory_tag, const std::string &object_name, std::map<std::string, JObjectSummary> &objects);
    uint32_t GetRunNumber(void);
    uint64_t GetEventNumber(void);

protected:
    bool _debug_mode    = false;
    bool _wait          = true;
    bool _fetch_flag    = false;
    std::set<std::string> _fetch_factorytags;
    std::map<std::string, std::string> _fetch_metadata;
    std::map<std::string, std::map<std::string, JObjectSummary> > _fetch_object_summaries;
    std::shared_ptr<const JEvent> _jevent;
    std::shared_ptr<JStringification> jstringification;
};

// Compare function to allow JFactorySummary to be used as keys in std::map
// ( used in argument to JControlEventProcessor::GetObjectStatus() )
inline bool operator<(const JFactorySummary& lhs, const JFactorySummary& rhs){
    if( lhs.object_name != rhs.object_name ) return lhs.object_name < rhs.object_name;
    if( lhs.factory_tag != rhs.factory_tag ) return lhs.factory_tag < rhs.factory_tag;
    if( lhs.factory_name != rhs.factory_name ) return lhs.factory_name < rhs.factory_name;
    return lhs.plugin_name < rhs.plugin_name;
}

#endif // _JControlEventProcessor_h_

