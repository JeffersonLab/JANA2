// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <string>
#include <map>
#include <set>
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
    // bool m_enabled = true;
    Format m_format = Format::Table;
    const JEvent* m_event;
    bool m_indexes_built = false;
    std::map<std::string, std::pair<int, const JFactory*>> m_factory_index;
    std::vector<JFactory*> m_factories;
    std::ostream& m_out = std::cout;
    std::istream& m_in = std::cin;
    std::set<std::string> m_discrepancies;
    bool m_enable_timeout_on_exit = false;
    bool m_enable_ticker_on_exit = false;

public:
    explicit JInspector(const JEvent* event);

    void PrintEvent();
    void PrintFactories(int filter_level);
    void PrintFactoryDetails(std::string factory_key);
    void PrintObjects(std::string factory_key);
    void PrintObject(std::string factory_key, int object_idx);
    void PrintFactoryParents(std::string factory_key);
    void PrintObjectParents(std::string factory_key, int object_idx);
    void PrintObjectAncestors(std::string factory_key, int object_idx);
    void PrintHelp();
    void Loop();
    void Reset();

    void SetDiscrepancies(std::set<std::string>&& diverging_factory_keys);

    static void ToText(const JEvent* event, bool asJson=false, std::ostream& out=std::cout);
    void ToText(const std::vector<JFactory*>& factories, const std::set<std::string>& discrepancies, int filter_level, bool asJson=false, std::ostream& out=std::cout);
    static void ToText(const JFactory* factory, bool asJson=false, std::ostream& out=std::cout);
    static void ToText(std::vector<JObject*> objs, bool as_json, std::ostream& out= std::cout);
    static void ToText(const JObject* obj, bool asJson, std::ostream& out=std::cout);

private:
    void BuildIndices();
    std::string MakeFactoryKey(std::string name, std::string tag);
    static std::vector<const JObject*> FindAllAncestors(const JObject*);
    static std::tuple<JFactory*, size_t, size_t> LocateObject(const JEvent&, const JObject* obj);
};

template <>
inline std::string JParameterManager::Stringify(const JInspector::Format& value) {
    switch (value) {
        case JInspector::Format::Table: return "table";
        case JInspector::Format::Json: return "json";
        case JInspector::Format::Tsv: return "tsv";
        default: return "unknown";
    }
}

template <>
inline void JParameterManager::Parse(const std::string& value, JInspector::Format& val) {
    auto lowered = JParameterManager::ToLower(value);
    if (lowered == "table") val = JInspector::Format::Table;
    if (lowered == "json") val =  JInspector::Format::Json;
    if (lowered == "tsv") val =  JInspector::Format::Tsv;
    else val =  JInspector::Format::Table;
}

