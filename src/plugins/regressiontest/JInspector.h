// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JInspector_h_
#define _JInspector_h_

#include <string>
#include <vector>
#include <tuple>
#include <mutex>
#include <iostream>

#include <JANA/JEvent.h>


class JInspector {
    bool m_enabled = true;
    const JEvent* m_event;
    bool m_indexes_built = false;
    std::map<std::pair<std::string, std::string>, std::pair<int, const JFactory*>> m_factory_index;
    std::ostream& m_out = std::cout;
    std::istream& m_in = std::cin;

public:
    explicit JInspector(const JEvent* event);
    void PrintEvent();
    void PrintFactories();
    void PrintFactory(int factory_idx);
    void PrintJObjects(int factory_idx);
    void PrintJObject(int factory_idx, int object_idx);
    void PrintAncestors(int factory_idx);
    void PrintAssociations(int factory_idx, int object_idx);
    void PrintHelp();

    uint64_t DoReplLoop(uint64_t current_evt_nr);

private:
    std::string StringifyEvent();
    std::string StringifyFactories();
    std::string StringifyFactory(int factory_idx);
    std::string StringifyJObjects(int factory_idx);
    std::string StringifyJObject(int factory_idx, int object_idx);
    std::string StringifyAncestors(int factory_idx);
    std::string StringifyAssociations(int factory_idx, int object_idx);

    void BuildIndices();
    std::vector<const JObject*> FindAllAncestors(const JObject*) const;
    static std::pair<JFactory*, size_t> LocateObject(const JEvent&, const JObject* obj);
    static std::pair<std::string, std::vector<int>> Parse(const std::string&);
};


#endif // _JIntrospection_h_
