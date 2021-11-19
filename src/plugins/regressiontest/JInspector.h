// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JInspector_h_
#define _JInspector_h_

#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <mutex>
#include <iostream>

#include <JANA/Services/JParameterManager.h>
class JEvent;
class JFactory;
class JObject;

class JInspector {

public:
    enum class Format {Table, Json, Tsv};

private:
    bool m_enabled = true;
    Format m_format = Format::Table;
    const JEvent* m_event;
    bool m_indexes_built = false;
    std::map<std::pair<std::string, std::string>, std::pair<int, const JFactory*>> m_factory_index;
    std::ostream& m_out = std::cout;
    std::istream& m_in = std::cin;

public:
    explicit JInspector(const JEvent* event);
    void SetEvent(const JEvent* event);

    void PrintEvent();
    void PrintFactories(int filter_level);
    void PrintFactory(int factory_idx);
    void PrintObjects(int factory_idx);
    void PrintObject(int factory_idx, int object_idx);
    void PrintFactoryParents(int factory_idx);
    void PrintObjectParents(int factory_idx, int object_idx);
    void PrintObjectAncestors(int factory_idx, int object_idx);
    void PrintHelp();

    uint64_t DoReplLoop(uint64_t current_evt_nr);

    static void ToText(const JEvent* event, bool asJson=false, std::ostream& out=std::cout);
    static void ToText(const std::vector<JFactory*>& factories, int filter_level, bool asJson=false, std::ostream& out=std::cout);
    static void ToText(JFactory* factory, bool asJson=false, std::ostream& out=std::cout);
    static void ToText(std::vector<JObject*> objs, bool as_json, std::ostream& out= std::cout);
    static void ToText(const JObject* obj, bool asJson, std::ostream& out=std::cout);

private:
    void BuildIndices();
    static std::vector<const JObject*> FindAllAncestors(const JObject*);
    static std::tuple<JFactory*, size_t, size_t> LocateObject(const JEvent&, const JObject* obj);
    static std::pair<std::string, std::vector<int>> Parse(const std::string&);
};

template <>
inline std::string JParameterManager::stringify(JInspector::Format value) {
    switch (value) {
        case JInspector::Format::Table: return "table";
        case JInspector::Format::Json: return "json";
        case JInspector::Format::Tsv: return "tsv";
        default: return "unknown";
    }
}

template <>
inline JInspector::Format JParameterManager::parse(const std::string& value) {
    auto lowered = JParameterManager::to_lower(value);
    if (lowered == "table") return JInspector::Format::Table;
    if (lowered == "json") return JInspector::Format::Json;
    if (lowered == "tsv") return JInspector::Format::Tsv;
    else return JInspector::Format::Table;
}

#endif // _JIntrospection_h_
