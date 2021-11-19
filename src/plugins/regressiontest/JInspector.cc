
#include "JInspector.h"
#include <JANA/JEventSource.h>
#include <stack>
#include <JANA/Utils/JTablePrinter.h>


JInspector::JInspector(const JEvent* event) : m_event(event) {
    event->GetJApplication()->SetDefaultParameter("jana:enable_inspector", m_enabled);
    event->GetJApplication()->SetDefaultParameter("jana:inspector_format", m_format, "Options are 'table','json'");
}

void JInspector::SetEvent(const JEvent* event) {
    m_event = event;
    m_indexes_built = false;
    m_factory_index.clear();
}
void JInspector::BuildIndices() {
    if (m_indexes_built) return;
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    int i = 0;
    for (auto fac : facs) {
        std::pair<std::string, std::string> key = {fac->GetObjectName(), fac->GetTag()};
        std::pair<int, const JFactory*> value = {i++, fac};
        m_factory_index.insert({key, value});
    }
    m_indexes_built = true;
}

std::vector<const JObject*> JInspector::FindAllAncestors(const JObject* root) {
    std::vector<const JObject*> all_ancestors;
    std::stack<const JObject*> to_visit;
    to_visit.push(root);
    while (!to_visit.empty()) {
        auto current_obj = to_visit.top();
        to_visit.pop();
        all_ancestors.push_back(current_obj);
        std::vector<const JObject*> current_ancestors;
        current_obj->GetT<JObject>(current_ancestors);
        for (auto obj : current_ancestors) {
            to_visit.push(obj);
        }
    }
    return all_ancestors;
}

std::pair<JFactory*, size_t> JInspector::LocateObject(const JEvent& event, const JObject* obj) {
    auto objName = obj->className();
    for (auto fac : event.GetAllFactories()) {
        if (fac->GetObjectName() == objName) {
            int i = 0;
            for (auto o : fac->GetAs<JObject>()) { // Won't trigger factory creation if it hasn't already happened
                if (obj == o) return {fac, i};
                i++;
            }
        }
    }
    return {nullptr, -1};
}

void JInspector::PrintEvent() {
    ToText(m_event, m_format==Format::Json, m_out);
}
void JInspector::PrintFactories() {
    auto facs = m_event->GetAllFactories();
    ToText(facs, m_format==Format::Json, m_out);
}
void JInspector::PrintFactory(int factory_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)\n";
        return;
    }
    auto fac = facs[factory_idx];
    ToText(fac, m_format==Format::Json, m_out);
}
void JInspector::PrintObjects(int factory_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)" << std::endl;
        return;
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    ToText(objs, m_format==Format::Json, m_out);
}
void JInspector::PrintObject(int factory_idx, int object_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)" << std::endl;
        return;
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    if (object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    ToText(obj, m_format==Format::Json, m_out);
}
void JInspector::PrintHelp() {
    m_out << "  -----------------------------------------" << std::endl;
    m_out << "  Available commands" << std::endl;
    m_out << "  -----------------------------------------" << std::endl;
    m_out << "  pe   PrintEvent" << std::endl;
    m_out << "  pf   PrintFactories" << std::endl;
    m_out << "  pf   PrintFactory fac_idx" << std::endl;
    m_out << "  po   PrintObjects fac_idx" << std::endl;
    m_out << "  po   PrintObject fac_idx obj_idx" << std::endl;
    m_out << "  pfp  PrintFactoryParents fac_idx" << std::endl;
    m_out << "  pop  PrintObjectParents fac_idx obj_idx" << std::endl;
    m_out << "  poa  PrintObjectAncestors fac_idx obj_idx" << std::endl;
    m_out << "  vt   ViewAsTable" << std::endl;
    m_out << "  vj   ViewAsJson" << std::endl;
    m_out << "  n    Next [evt_nr]" << std::endl;
    m_out << "  f    Finish" << std::endl;
    m_out << "  q    Quit" << std::endl;
    m_out << "  h    Help" << std::endl;
    m_out << "  -----------------------------------------" << std::endl;
}
void JInspector::ToText(const JEvent* event, bool asJson, std::ostream& out) {
    auto className = event->GetJEventSource()->GetTypeName();
    if (className.empty()) className = "(unknown)";
    if (asJson) {
        out << "{ \"run_number\": " << event->GetRunNumber();
        out << ", \"event_number\": " << event->GetEventNumber();
        out << ", \"source\": \"" << event->GetJEventSource()->GetResourceName();
        out << "\", \"class_name\": \"" << className;
        out << "\", \"is_sequential\": \"" << event->GetSequential() << "\"}";
        out << std::endl;
    }
    else {
        out << "Run number:   " << event->GetRunNumber() << std::endl;
        out << "Event number: " << event->GetEventNumber() << std::endl;
        out << "Event source: " << event->GetJEventSource()->GetResourceName() << std::endl;
        out << "Class name:   " << className << std::endl;
        out << "Sequential:   " << event->GetSequential() << std::endl;
    }
}

void JInspector::ToText(JFactory* fac, bool asJson, std::ostream& out) {

    auto pluginName = fac->GetPluginName();
    if (pluginName.empty()) pluginName = "(no plugin)";

    auto factoryName = fac->GetFactoryName();
    if (factoryName.empty()) factoryName = "(dummy factory)";

    auto tag = fac->GetTag();
    if (tag.empty()) tag = "(no tag)";

    std::string creationStatus;
    switch (fac->GetCreationStatus()) {
        case JFactory::CreationStatus::NotCreatedYet: creationStatus = "NotCreatedYet"; break;
        case JFactory::CreationStatus::Created: creationStatus = "Created"; break;
        case JFactory::CreationStatus::Inserted: creationStatus = "Inserted"; break;
        case JFactory::CreationStatus::InsertedViaGetObjects: creationStatus = "InsertedViaGetObjects"; break;
        case JFactory::CreationStatus::NeverCreated: creationStatus = "NeverCreated"; break;
        default: creationStatus = "Unknown";
    }

    if (!asJson) {
        out << "Plugin name          " << pluginName << std::endl;
        out << "Factory name         " << factoryName << std::endl;
        out << "Object name          " << fac->GetObjectName() << std::endl;
        out << "Tag                  " << tag << std::endl;
        out << "Creation status      " << creationStatus << std::endl;
        out << "Object count         " << fac->GetNumObjects() << std::endl;
        out << "Persistent flag      " << fac->TestFactoryFlag(JFactory::PERSISTENT) << std::endl;
        out << "NotObjectOwner flag  " << fac->TestFactoryFlag(JFactory::NOT_OBJECT_OWNER) << std::endl;
    }
    else {
        out << "{" << std::endl;
        out << "  \"plugin_name\":   \"" << pluginName << "\"," << std::endl;
        out << "  \"factory_name\":  \"" << factoryName << "\"," << std::endl;
        out << "  \"object_name\":   \"" << fac->GetObjectName() << "\"," << std::endl;
        out << "  \"tag\":           \"" << tag << "\"," << std::endl;
        out << "  \"creation\":      \"" << creationStatus << "\"," << std::endl;
        out << "  \"object_count\":  " << fac->GetNumObjects() << "," << std::endl;
        out << "  \"persistent\":    " << fac->TestFactoryFlag(JFactory::PERSISTENT) << "," << std::endl;
        out << "  \"not_obj_owner\": " << fac->TestFactoryFlag(JFactory::NOT_OBJECT_OWNER) << std::endl;
        out << "}" << std::endl;
    }
}

void JInspector::ToText(const std::vector<JFactory*>& factories, bool asJson, std::ostream &out) {
    size_t idx = 0;
    if (!asJson) {
        JTablePrinter t;
        t.AddColumn("Index", JTablePrinter::Justify::Right);
        t.AddColumn("Factory name");
        t.AddColumn("Object name");
        t.AddColumn("Tag");
        t.AddColumn("Object count", JTablePrinter::Justify::Right);
        for (auto fac : factories) {
            auto facName = fac->GetFactoryName();
            if (facName.empty()) facName = "(no factory name)";
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";
            t | idx++ | facName | fac->GetObjectName() | tag | fac->GetNumObjects();
        }
        t.Render(out);
    }
    else {
        out << "{" << std::endl;
        for (auto fac : factories) {
            auto facName = fac->GetFactoryName();
            if (facName.empty()) facName = "null";
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "null";
            out << "  " << idx++ << ": { \"factory_name\": ";
            if (fac->GetFactoryName().empty()) {
                out << "null, ";
            }
            else {
                out << "\"" << fac->GetFactoryName() << "\", ";
            }
            out << "\"object_name\": \"" << fac->GetObjectName() << "\", \"tag\": ";
            if (fac->GetTag().empty()) {
                out << "null, ";
            }
            else {
                out << "\"" << fac->GetTag() << "\", ";
            }
            out << "\"object_count\": " << fac->GetNumObjects();
            out << "}," << std::endl;
        }
        out << "}" << std::endl;
    }
}

void JInspector::ToText(std::vector<JObject*> objs, bool as_json, std::ostream& out) {

    if (objs.empty()) {
        out << "(No objects found)" << std::endl;
        return;
    }
    if (as_json) {
        out << "{" << std::endl;
        for (size_t i = 0; i < objs.size(); ++i) {
            auto obj = objs[i];
            JObjectSummary summary;
            obj->Summarize(summary);
            out << "  " << i << ":  {";
            for (auto& field : summary.get_fields()) {
                out << "\"" << field.name << "\": \"" << field.value << "\", ";
            }
            out << "}" << std::endl;
        }
        out << "}" << std::endl;
    }
    else {
        JTablePrinter t;
        std::set<std::string> fieldnames_seen;
        std::vector<std::string> fieldnames_in_order;
        for (auto obj : objs) {
            JObjectSummary summary;
            obj->Summarize(summary);
            for (auto field : summary.get_fields()) {
                if (fieldnames_seen.find(field.name) == fieldnames_seen.end()) {
                    fieldnames_in_order.push_back(field.name);
                    fieldnames_seen.insert(field.name);
                }
            }
        }
        t.AddColumn("Index");
        for (auto fieldname : fieldnames_in_order) {
            t.AddColumn(fieldname);
        }
        for (size_t i = 0; i < objs.size(); ++i) {
            auto obj = objs[i];
            t | i;
            std::map<std::string, std::string> summary_map;
            JObjectSummary summary;
            obj->Summarize(summary);
            for (auto& field : summary.get_fields()) {
                summary_map[field.name] = field.value;
            }
            for (auto& fieldname : fieldnames_in_order) {
                auto result = summary_map.find(fieldname);
                if (result == summary_map.end()) {
                    t | "(missing)";
                }
                else {
                    t | result->second;
                }
            }
        }
        t.Render(out);
    }
}

void JInspector::ToText(const JObject* obj, bool asJson, std::ostream& out) {
    JObjectSummary summary;
    obj->Summarize(summary);
    if (asJson) {
        out << "[" << std::endl;
        for (auto& field : summary.get_fields()) {
            out << "  { \"name\": \"" << field.name << "\", ";
            out << "\"value\": \"" << field.value << "\", ";
            out << "\"description\": \"" << field.description << "\" }" << std::endl;
        }
        out << "]" << std::endl;
    }
    else {
        JTablePrinter t;
        t.AddColumn("Field name");
        t.AddColumn("Value");
        t.AddColumn("Description");
        for (auto& field : summary.get_fields()) {
            t | field.name | field.value | field.description;
        }
        t.Render(out);
    }
}

void JInspector::PrintFactoryParents(int factory_idx) {

    BuildIndices();  // So that we can retrieve the integer index given the factory name/tag pair
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)" << std::endl;
        return;
    }
    auto fac = facs[factory_idx];
    auto obj_name = fac->GetObjectName();
    auto fac_tag = fac->GetTag();

    bool callgraph_on = m_event->GetJCallGraphRecorder()->IsEnabled();
    if (!callgraph_on) {
        m_out << "(Error: Callgraph recording is currently disabled)" << std::endl;
    }

    if (m_format != Format::Json) {
        JTablePrinter t;
        t.AddColumn("Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object name");
        t.AddColumn("Tag");
        auto callgraph = m_event->GetJCallGraphRecorder()->GetCallGraph();
        bool found_anything = false;
        for (const auto& node : callgraph) {
            if ((node.caller_name == obj_name) && (node.caller_tag == fac_tag)) {
                found_anything = true;
                auto idx = m_factory_index[{node.callee_name, node.callee_tag}].first;
                auto tag = node.callee_tag;
                if (tag.empty()) tag = "(no tag)";
                t | idx | node.callee_name | tag;
            }
        }
        if (!found_anything) {
            m_out << "(No ancestors found)" << std::endl;
            return;
        }
        t.Render(m_out);
    }
    else {
        auto callgraph = m_event->GetJCallGraphRecorder()->GetCallGraph();
        bool found_anything = false;
        m_out << "[" << std::endl;
        for (const auto& node : callgraph) {
            if ((node.caller_name == obj_name) && (node.caller_tag == fac_tag)) {
                found_anything = true;
                auto idx = m_factory_index[{node.callee_name, node.callee_tag}].first;
                auto tag = node.callee_tag;
                m_out << "  { \"index\": " << idx << ", \"object_name\": \"" << node.callee_name << "\", \"tag\": ";
                if (tag.empty()) {
                    m_out << "null }," << std::endl;
                }
                else {
                    m_out << "\"" << tag << "\" }," << std::endl;
                }
            }
        }
        m_out << "]" << std::endl;
        if (!found_anything) {
            m_out << "(No ancestors found)" << std::endl;
            return;
        }
    }
}

void JInspector::PrintObjectParents(int factory_idx, int object_idx) {

    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)" << std::endl;
        return;
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    if (object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    std::vector<const JObject*> parents;
    obj->GetT<JObject>(parents);
    if (parents.empty()) {
        m_out << "(No parents found)" << std::endl;
        return;
    }

    if (m_format == Format::Table) {
        JTablePrinter t;
        t.AddColumn("Object name");
        t.AddColumn("Tag");
        t.AddColumn("Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object contents");
        for (auto parent : parents) {
            JFactory* fac;
            size_t idx;
            std::tie(fac, idx) = LocateObject(*m_event, parent);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            JObjectSummary summary;
            obj->Summarize(summary);

            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";
            std::ostringstream objval;
            objval << "{";
            for (auto& field : summary.get_fields()) {
                objval << field.name << ": " << field.value << ", ";
            }
            objval << "}";

            t | fac->GetObjectName() | tag | idx | objval.str();
        }
        t.Render(m_out);
    }
    else {
        m_out << "[" << std::endl;
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t idx;
            std::tie(fac, idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";

            m_out << "  " << "{ " << std::endl << "    \"object_name\": \"" << fac->GetObjectName() << "\", ";
            if (tag.empty()) {
                m_out << "\"tag\": null, \"index\": " << idx << ", " << std::endl << "    \"object_contents\": ";
            }
            else {
                m_out << "\"tag\": \"" << tag << "\", \"index\": " << idx << ", " << std::endl << "    \"object_contents\": ";
            }

            JObjectSummary summary;
            obj->Summarize(summary);
            m_out << "{";
            for (auto& field : summary.get_fields()) {
                m_out << "\"" << field.name << "\": \"" << field.value << "\", ";
            }
            m_out << "}" << std::endl;
            m_out << "  }, " << std::endl;
        }
        m_out << "]" << std::endl;
    }
}

void JInspector::PrintObjectAncestors(int factory_idx, int object_idx) {

    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        m_out << "(Error: Factory index out of range)" << std::endl;
        return;
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    if (object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    auto ancestors = FindAllAncestors(obj);
    if (ancestors.empty()) {
         m_out << "(No ancestors found)" << std::endl;
         return;
    }

    if (m_format == Format::Table) {
        JTablePrinter t;
        t.AddColumn("Object name");
        t.AddColumn("Tag");
        t.AddColumn("Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object contents");
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t idx;
            std::tie(fac, idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            JObjectSummary summary;
            obj->Summarize(summary);

            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";
            std::ostringstream objval;
            objval << "{";
            for (auto& field : summary.get_fields()) {
                objval << field.name << ": " << field.value << ", ";
            }
            objval << "}";

            t | fac->GetObjectName() | tag | idx | objval.str();
        }
        t.Render(m_out);
    }
    else {
        m_out << "[" << std::endl;
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t idx;
            std::tie(fac, idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";

            m_out << "  " << "{ " << std::endl << "    \"object_name\": \"" << fac->GetObjectName() << "\", ";
            if (tag.empty()) {
                m_out << "\"tag\": null, \"index\": " << idx << ", " << std::endl << "    \"object_contents\": ";
            }
            else {
                m_out << "\"tag\": \"" << tag << "\", \"index\": " << idx << ", " << std::endl << "    \"object_contents\": ";
            }

            JObjectSummary summary;
            obj->Summarize(summary);
            m_out << "{";
            for (auto& field : summary.get_fields()) {
                m_out << "\"" << field.name << "\": \"" << field.value << "\", ";
            }
            m_out << "}" << std::endl;
            m_out << "  }, " << std::endl;
        }
        m_out << "]" << std::endl;
    }
}

std::pair<std::string, std::vector<int>> JInspector::Parse(const std::string& user_input) {
    std::string token;
    std::stringstream ss(user_input);
    ss >> token;
    std::vector<int> args;
    int arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    return {token, args};
}

uint64_t JInspector::DoReplLoop(uint64_t next_evt_nr) {
    if (!m_enabled) {
        return 0;
    }
    bool stay_in_loop = true;
    next_evt_nr = 0;  // 0 denotes that Repl stops at the next event number by default
    PrintEvent();
    while (stay_in_loop) {
        std::string user_input;
        m_out << std::endl << "JANA: ";
        std::getline(m_in, user_input);
        auto result = Parse(user_input);
        auto token = result.first;
        auto args = result.second;
        if (token == "PrintEvent" || token == "pe") {
            PrintEvent();
        }
        else if ((token == "PrintFactories" || token == "pf") && args.empty()) {
            PrintFactories();
        }
        else if ((token == "PrintFactory" || token == "pf") && (args.size() == 1)) {
            PrintFactory(args[0]);
        }
        else if ((token == "PrintObjects" || token == "po") && (args.size() == 1)) {
            PrintObjects(args[0]);
        }
        else if ((token == "PrintObject" || token == "po") && (args.size() == 2)) {
            PrintObject(args[0], args[1]);
        }
        else if ((token == "PrintFactoryParents" || token == "pfp") && (args.size() == 1)) {
            PrintFactoryParents(args[0]);
        }
        else if ((token == "PrintObjectParents" || token == "pop") && (args.size() == 2)) {
            PrintObjectParents(args[0], args[1]);
        }
        else if ((token == "PrintObjectAncestors" || token == "poa") && (args.size() == 2)) {
            PrintObjectAncestors(args[0], args[1]);
        }
        else if (token == "ViewAsTable" || token == "vt") {
            m_format = Format::Table;
            m_out << "(Switching to table view mode)" << std::endl;
        }
        else if (token == "ViewAsJson" || token == "vj") {
            m_format = Format::Json;
            m_out << "(Switching to JSON view mode)" << std::endl;
        }
        else if (token == "Next" || token == "n") {
            stay_in_loop = false;
            m_enabled = true;
            if (args.size() > 0) {
                next_evt_nr = args[0];
            }
        }
        else if (token == "Finish" || token == "f") {
            stay_in_loop = false;
            m_enabled = false;
        }
        else if (token == "Quit" || token == "q") {
            stay_in_loop = false;
            m_enabled = false;
            m_event->GetJApplication()->Quit(false);
        }
        else if (token == "Help" || token == "h") {
            PrintHelp();
        }
        else if (token == "") {
            // Do nothing
        }
        else {
            m_out << "(Error: Unrecognized command, or wrong argument count)" << std::endl;
            PrintHelp();
        }
    }
    return next_evt_nr;
}
