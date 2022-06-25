
#include "JInspector.h"
#include <JANA/JEventSource.h>
#include <stack>
#include <JANA/Utils/JTablePrinter.h>


JInspector::JInspector(const JEvent* event) : m_event(event) {}

void JInspector::BuildIndices() {
    if (m_indexes_built) return;
    m_factories.clear();
    for (auto fac: m_event->GetFactorySet()->GetAllFactories()) {
        m_factories.push_back(fac);
    }
    std::sort(m_factories.begin(), m_factories.end(), [](const JFactory* first, const JFactory* second){
        return std::make_pair(first->GetObjectName(), first->GetTag()) <
               std::make_pair(second->GetObjectName(), second->GetTag());});

    int i = 0;
    for (auto fac : m_factories) {
        std::string key = MakeFactoryKey(fac->GetObjectName(), fac->GetTag());
        std::pair<int, const JFactory*> value = {i++, fac};
        m_factory_index.insert({key, value});
        m_factory_index.insert({std::to_string(i), value});
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

std::tuple<JFactory*, size_t, size_t> JInspector::LocateObject(const JEvent& event, const JObject* obj) {
    auto objName = obj->className();
    size_t fac_idx = 0;
    for (auto fac : event.GetAllFactories()) {
        if (fac->GetObjectName() == objName) {
            size_t obj_idx = 0;
            for (auto o : fac->GetAs<JObject>()) { // Won't trigger factory creation if it hasn't already happened
                if (obj == o) {
                    return std::make_tuple(fac, fac_idx, obj_idx);
                }
                obj_idx++;
            }
        }
        fac_idx++;
    }
    return std::make_tuple(nullptr, -1, -1);
}

void JInspector::PrintEvent() {
    ToText(m_event, m_format==Format::Json, m_out);
}
void JInspector::PrintFactories(int filter_level=0) {
    auto facs = m_event->GetAllFactories();
    ToText(facs, filter_level, m_format==Format::Json, m_out);
}

void JInspector::PrintObjects(std::string factory_key) {

    auto result = m_factory_index.find(factory_key);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    auto fac = const_cast<JFactory*>(result->second.second);
    auto objs = fac->GetAs<JObject>();
    ToText(objs, m_format==Format::Json, m_out);
}

void JInspector::PrintFactoryDetails(std::string fac_key) {
    BuildIndices();
    auto result = m_factory_index.find(fac_key);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    auto fac = result->second.second;
    ToText(fac, m_format==Format::Json, m_out);
}

void JInspector::PrintObject(std::string fac_key, int object_idx) {
    BuildIndices();
    auto result = m_factory_index.find(fac_key);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    JFactory* fac = const_cast<JFactory*>(result->second.second);
    auto objs = fac->GetAs<JObject>();
    if ((size_t) object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    ToText(obj, m_format==Format::Json, m_out);
}
std::string JInspector::MakeFactoryKey(std::string name, std::string tag) {
    std::ostringstream ss;
    ss << name;
    if (!tag.empty()) {
        ss << ":" << tag;
    }
    return ss.str();
}

void JInspector::PrintHelp() {
    m_out << "  -----------------------------------------" << std::endl;
    m_out << "  Available commands" << std::endl;
    m_out << "  -----------------------------------------" << std::endl;
    m_out << "  pe   PrintEvent" << std::endl;
    m_out << "  pf   PrintFactories [filter_level <- {0,1,2,3}]" << std::endl;
    m_out << "  pfd  PrintFactoryDetails fac_idx" << std::endl;
    m_out << "  po   PrintObjects fac_idx" << std::endl;
    m_out << "  po   PrintObject fac_idx obj_idx" << std::endl;
    m_out << "  pfp  PrintFactoryParents fac_idx" << std::endl;
    m_out << "  pop  PrintObjectParents fac_idx obj_idx" << std::endl;
    m_out << "  poa  PrintObjectAncestors fac_idx obj_idx" << std::endl;
    m_out << "  vt   ViewAsTable" << std::endl;
    m_out << "  vj   ViewAsJson" << std::endl;
    m_out << "  x    Exit" << std::endl;
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

void JInspector::ToText(const JFactory* fac, bool asJson, std::ostream& out) {

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

void JInspector::ToText(const std::vector<JFactory*>& factories, int filterlevel, bool asJson, std::ostream &out) {
    size_t idx = -1;
    if (!asJson) {
        JTablePrinter t;
        t.AddColumn("Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object");
        t.AddColumn("Tag");
        t.AddColumn("Creation status");
        t.AddColumn("Object count", JTablePrinter::Justify::Right);
        for (auto fac : factories) {
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
            idx += 1;
            if (filterlevel > 0 && (fac->GetCreationStatus()==JFactory::CreationStatus::NeverCreated ||
                           fac->GetCreationStatus()==JFactory::CreationStatus::NotCreatedYet)) continue;
            if (filterlevel > 1 && (fac->GetNumObjects()== 0)) continue;
            if (filterlevel > 2 && (fac->GetCreationStatus()==JFactory::CreationStatus::Inserted ||
                                    fac->GetCreationStatus()==JFactory::CreationStatus::InsertedViaGetObjects)) continue;

            t | idx | fac->GetObjectName() | tag | creationStatus | fac->GetNumObjects();
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
    std::set<std::string> objnames;
    for (auto obj: objs) {
        objnames.insert(obj->className());
    }
    if (objnames.size() == 1) {
        out << *(objnames.begin()) << std::endl;
    }
    else {
        out << "{ ";
        for (auto name : objnames) {
            out << name << ",";
        }
        out << "}" << std::endl;
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
    out << obj->className() << std::endl;
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

void JInspector::PrintFactoryParents(std::string factory_idx) {

    bool callgraph_on = m_event->GetJCallGraphRecorder()->IsEnabled();
    if (!callgraph_on) {
        m_out << "(Error: Callgraph recording is currently disabled)" << std::endl;
    }

    BuildIndices();  // So that we can retrieve the integer index given the factory name/tag pair
    auto result = m_factory_index.find(factory_idx);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    auto fac = result->second.second;
    auto obj_name = fac->GetObjectName();
    auto fac_tag = fac->GetTag();
    if (fac_tag.empty()) {
        m_out << obj_name << std::endl;
    }
    else {
        m_out << obj_name << ":" << fac_tag << std::endl;
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
                auto idx = m_factory_index[MakeFactoryKey(node.callee_name, node.callee_tag)].first;
                auto tag = node.callee_tag;
                if (tag.empty()) tag = "(no tag)";
                t | idx | node.callee_name | tag;
            }
        }
        if (!found_anything) {
            m_out << "(No parents found)" << std::endl;
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
                auto idx = m_factory_index[MakeFactoryKey(node.callee_name, node.callee_tag)].first;
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

void JInspector::PrintObjectParents(std::string factory_key, int object_idx) {

    BuildIndices();  // So that we can retrieve the integer index given the factory name/tag pair
    auto result = m_factory_index.find(factory_key);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    auto fac = const_cast<JFactory*>(result->second.second);
    auto objs = fac->GetAs<JObject>();
    if ((size_t) object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    m_out << obj->className() << std::endl;
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
        t.AddColumn("Factory Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object Index", JTablePrinter::Justify::Right);
        t.AddColumn("Object contents");
        for (auto parent : parents) {
            JFactory* fac;
            size_t fac_idx;
            size_t obj_idx;
            std::tie(fac, fac_idx, obj_idx) = LocateObject(*m_event, parent);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            JObjectSummary summary;
            parent->Summarize(summary);

            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";
            std::ostringstream objval;
            objval << "{";
            for (auto& field : summary.get_fields()) {
                objval << field.name << ": " << field.value << ", ";
            }
            objval << "}";

            t | fac->GetObjectName() | tag | fac_idx | obj_idx | objval.str();
        }
        t.Render(m_out);
    }
    else {
        m_out << "[" << std::endl;
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t fac_idx;
            size_t obj_idx;
            std::tie(fac, fac_idx, obj_idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";

            m_out << "  " << "{ " << std::endl << "    \"object_name\": \"" << fac->GetObjectName() << "\", ";
            if (tag.empty()) {
                m_out << "\"tag\": null, ";
            }
            else {
                m_out << "\"tag\": \"" << tag << "\", ";
            }
            m_out << "\"fac_index\": " << fac_idx << ", \"obj_index\": " << obj_idx << "," << std::endl;
            m_out << "    \"object_contents\": ";

            JObjectSummary summary;
            ancestor->Summarize(summary);
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

void JInspector::PrintObjectAncestors(std::string factory_idx, int object_idx) {

    BuildIndices();  // So that we can retrieve the integer index given the factory name/tag pair
    auto result = m_factory_index.find(factory_idx);
    if (result == m_factory_index.end()) {
        m_out << "(Error: Invalid factory name or index)\n";
        return;
    }
    auto fac = const_cast<JFactory*>(result->second.second);
    auto objs = fac->GetAs<JObject>();
    if ((size_t) object_idx >= objs.size()) {
        m_out << "(Error: Object index out of range)" << std::endl;
        return;
    }
    auto obj = objs[object_idx];
    m_out << obj->className() << std::endl;
    auto ancestors = FindAllAncestors(obj);
    if (ancestors.empty()) {
         m_out << "(No ancestors found)" << std::endl;
         return;
    }

    if (m_format == Format::Table) {
        JTablePrinter t;
        t.AddColumn("Object name");
        t.AddColumn("Tag");
        t.AddColumn("Factory index", JTablePrinter::Justify::Right);
        t.AddColumn("Object index", JTablePrinter::Justify::Right);
        t.AddColumn("Object contents");
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t fac_idx, obj_idx;
            std::tie(fac, fac_idx, obj_idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            JObjectSummary summary;
            ancestor->Summarize(summary);

            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";
            std::ostringstream objval;
            objval << "{";
            for (auto& field : summary.get_fields()) {
                objval << field.name << ": " << field.value << ", ";
            }
            objval << "}";

            t | fac->GetObjectName() | tag | fac_idx | obj_idx | objval.str();
        }
        t.Render(m_out);
    }
    else {
        m_out << "[" << std::endl;
        for (auto ancestor : FindAllAncestors(obj)) {
            JFactory* fac;
            size_t fac_idx, obj_idx;
            std::tie(fac, fac_idx, obj_idx) = LocateObject(*m_event, ancestor);
            if (fac == nullptr) {
                m_out << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
                continue;
            }
            auto tag = fac->GetTag();
            if (tag.empty()) tag = "(no tag)";

            m_out << "  " << "{ " << std::endl << "    \"object_name\": \"" << fac->GetObjectName() << "\", ";
            if (tag.empty()) {
                m_out << "\"tag\": null, ";
            }
            else {
                m_out << "\"tag\": \"" << tag << "\", ";
            }
            m_out << "\"fac_index\": " << fac_idx << ", \"obj_index\": " << obj_idx << "," << std::endl;
            m_out << "    \"object_contents\": ";

            JObjectSummary summary;
            ancestor->Summarize(summary);
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

void JInspector::Loop() {
    bool stay_in_loop = true;
    m_event->GetJApplication()->SetTicker( false ); // TODO: Get the current ticker state first (requires JApplication be modified)
    m_event->GetJApplication()->SetTimeoutEnabled( false ); // TODO: Get current state and save
    m_out << std::endl;
    m_out << "--------------------------------------------------------------------------------------" << std::endl;
    m_out << "Welcome to JANA's interactive inspector! Type `Help` or `h` to see available commands." << std::endl;
    m_out << "--------------------------------------------------------------------------------------" << std::endl;
    PrintEvent();
    while (stay_in_loop) {
        std::string user_input;
        m_out << std::endl << "JANA: "; m_out.flush();
        // Obtain a single line
        std::getline(m_in, user_input);
        // Split into tokens
        std::stringstream ss(user_input);
        std::string token;
        ss >> token;
        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }
        if (token == "PrintEvent" || token == "pe") {
            PrintEvent();
        }
        else if ((token == "PrintFactories" || token == "pf") && args.empty()) {
            PrintFactories(0);
        }
        else if ((token == "PrintFactories" || token == "pf") && args.size() == 1) {
            PrintFactories(std::stoi(args[0]));
        }
        else if ((token == "PrintFactoryDetails" || token == "pfd") && (args.size() == 1)) {
            PrintFactoryDetails(args[0]);
        }
        else if ((token == "PrintObjects" || token == "po") && (args.size() == 1)) {
            PrintObjects(args[0]);
        }
        else if ((token == "PrintObject" || token == "po") && (args.size() == 2)) {
            PrintObject(args[0], std::stoi(args[1]));
        }
        else if ((token == "PrintFactoryParents" || token == "pfp") && (args.size() == 1)) {
            PrintFactoryParents(args[0]);
        }
        else if ((token == "PrintObjectParents" || token == "pop") && (args.size() == 2)) {
            PrintObjectParents(args[0], std::stoi(args[1]));
        }
        else if ((token == "PrintObjectAncestors" || token == "poa") && (args.size() == 2)) {
            PrintObjectAncestors(args[0], std::stoi(args[1]));
        }
        else if (token == "ViewAsTable" || token == "vt") {
            m_format = Format::Table;
            m_out << "(Switching to table view mode)" << std::endl;
        }
        else if (token == "ViewAsJson" || token == "vj") {
            m_format = Format::Json;
            m_out << "(Switching to JSON view mode)" << std::endl;
        }
        else if (token == "Continue" || token == "c") {
            stay_in_loop = false;
        }
        else if (token == "Exit" || token == "x") {
            stay_in_loop = false;
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

    m_event->GetJApplication()->SetTicker( true ); // TODO: Reset to what it was upon entry
    m_event->GetJApplication()->SetTimeoutEnabled( true ); // TODO: Reset to what it was upon entry
}
