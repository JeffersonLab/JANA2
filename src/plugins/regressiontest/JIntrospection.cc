
#include "JIntrospection.h"
#include <JANA/JEventSource.h>
#include <sstream>
#include <stack>
#include <JANA/Utils/JTablePrinter.h>


JIntrospection::JIntrospection(const JEvent* event) : m_event(event) {}

void JIntrospection::BuildIndices() {
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

std::vector<const JObject*> JIntrospection::FindAllAncestors(const JObject* root) const {
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

std::pair<JFactory*, size_t> JIntrospection::LocateObject(const JEvent& event, const JObject* obj) {
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

void JIntrospection::PrintEvent() {
    m_out << StringifyEvent() << std::endl;
}
void JIntrospection::PrintFactories() {
    m_out << StringifyFactories() << std::endl;
}
void JIntrospection::PrintFactory(int factory_idx) {
    m_out << StringifyFactory(factory_idx) << std::endl;
}
void JIntrospection::PrintJObjects(int factory_idx) {
    m_out << StringifyJObjects(factory_idx) << std::endl;
}
void JIntrospection::PrintJObject(int factory_idx, int object_idx) {
    m_out << StringifyJObject(factory_idx, object_idx) << std::endl;
}
void JIntrospection::PrintAncestors(int factory_idx) {
    m_out << StringifyAncestors(factory_idx) << std::endl;
}
void JIntrospection::PrintAssociations(int factory_idx, int object_idx) {
    m_out << StringifyAssociations(factory_idx, object_idx) << std::endl;
}
void JIntrospection::PrintHelp() {
    m_out << "----------------------" << std::endl;
    m_out << "Available commands" << std::endl;
    m_out << "----------------------" << std::endl;
    m_out << "PrintEvent" << std::endl;
    m_out << "PrintFactories" << std::endl;
    m_out << "PrintFactory fac_idx" << std::endl;
    m_out << "PrintJObjects fac_idx" << std::endl;
    m_out << "PrintJObject fac_idx obj_idx" << std::endl;
    m_out << "PrintAncestors fac_idx" << std::endl;
    m_out << "PrintAssociations fac_idx obj_idx" << std::endl;
    m_out << "Next [evt_nr]" << std::endl;
    m_out << "Finish" << std::endl;
    m_out << "Quit" << std::endl;
    m_out << "----------------------" << std::endl;
}
std::string JIntrospection::StringifyEvent() {
    auto className = m_event->GetJEventSource()->GetTypeName();
    if (className.empty()) className = "(unknown)";
    std::ostringstream ss;
    ss << "Run number:   " << m_event->GetRunNumber() << std::endl;
    ss << "Event number: " << m_event->GetEventNumber() << std::endl;
    ss << "Event source: " << m_event->GetJEventSource()->GetResourceName() << std::endl;
    ss << "Class name:   " << className << std::endl;
    return ss.str();
}
std::string JIntrospection::StringifyFactories() {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    std::ostringstream ss;
    size_t idx = 0;
    JTablePrinter t;
    t.AddColumn("Index", JTablePrinter::Justify::Right);
    t.AddColumn("Factory name");
    t.AddColumn("Object name");
    t.AddColumn("Tag");
    t.AddColumn("Object count", JTablePrinter::Justify::Right);
    for (auto fac : facs) {
        auto facName = fac->GetFactoryName();
        if (facName.empty()) facName = "(no factory name)";
        auto tag = fac->GetTag();
        if (tag.empty()) tag = "(no tag)";
        t | idx++ | facName | fac->GetObjectName() | tag | fac->GetNumObjects();
    }
    t.Render(ss);
    return ss.str();
}
std::string JIntrospection::StringifyFactory(int factory_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        return "(Error: Factory index out of range)\n";
    }
    auto fac = facs[factory_idx];

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


    std::ostringstream ss;
    ss << "Plugin name          " << pluginName << std::endl;
    ss << "Factory name         " << factoryName << std::endl;
    ss << "Object name          " << fac->GetObjectName() << std::endl;
    ss << "Tag                  " << tag << std::endl;
    ss << "Creation status      " << creationStatus << std::endl;
    ss << "Object count         " << fac->GetNumObjects() << std::endl;
    ss << "Persistent flag      " << fac->TestFactoryFlag(JFactory::PERSISTENT) << std::endl;
    ss << "NotObjectOwner flag  " << fac->TestFactoryFlag(JFactory::NOT_OBJECT_OWNER) << std::endl;

    return ss.str();
}
std::string JIntrospection::StringifyJObjects(int factory_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        return "(Error: Factory index out of range)\n";
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    std::ostringstream ss;

    for (size_t i = 0; i < objs.size(); ++i) {
        auto obj = objs[i];
        JObjectSummary summary;
        obj->Summarize(summary);
        ss << i << ":\t {";
        for (auto& field : summary.get_fields()) {
            ss << field.name << ": " << field.value << ", ";
        }
        ss << "}" << std::endl;
    }
    return ss.str();
}
std::string JIntrospection::StringifyJObject(int factory_idx, int object_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        return "(Error: Factory index out of range)\n";
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    if (object_idx >= objs.size()) {
        return "(Error: Object index out of range)\n";
    }
    auto obj = objs[object_idx];
    JObjectSummary summary;
    obj->Summarize(summary);
    std::ostringstream ss;
    for (auto& field : summary.get_fields()) {
        ss << field.name << "\t" << field.value << "\t"  << field.description << std::endl;
    }
    return ss.str();
}
std::string JIntrospection::StringifyAncestors(int factory_idx) {
    BuildIndices();  // So that we can retrieve the integer index given the factory name/tag pair
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        return "(Error: Factory index out of range)\n";
    }
    auto fac = facs[factory_idx];
    auto obj_name = fac->GetObjectName();
    auto fac_tag = fac->GetTag();

    std::ostringstream ss;
    bool callgraph_on = m_event->GetJCallGraphRecorder()->IsEnabled();
    if (!callgraph_on) {
        ss << "(Error: Callgraph recording is currently disabled)" << std::endl;
    }
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
        ss << "(No ancestors found)" << std::endl;
        return ss.str();
    }
    t.Render(ss);
    return ss.str();
}
std::string JIntrospection::StringifyAssociations(int factory_idx, int object_idx) {
    auto facs = m_event->GetFactorySet()->GetAllFactories();
    if (factory_idx >= facs.size()) {
        return "(Error: Factory index out of range)\n";
    }
    auto objs = facs[factory_idx]->GetAs<JObject>();
    if (object_idx >= facs.size()) {
        return "(Error: Object index out of range)\n";
    }
    auto obj = objs[object_idx];
    auto ancestors = FindAllAncestors(obj);
    if (ancestors.empty()) return "(No associations found)\n";

    JTablePrinter t;
    t.AddColumn("Object name");
    t.AddColumn("Tag");
    t.AddColumn("Index", JTablePrinter::Justify::Right);
    t.AddColumn("Object contents");
    std::ostringstream ss;
    for (auto ancestor : FindAllAncestors(obj)) {
        JFactory* fac;
        size_t idx;
        std::tie(fac, idx) = LocateObject(*m_event, obj);
        if (fac == nullptr) {
            ss << "(Error: Unable to find factory containing object with classname '" << obj->className() << "')" << std::endl;
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
    t.Render(ss);
    return ss.str();
}

std::pair<std::string, std::vector<int>> JIntrospection::Parse(const std::string& user_input) {
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

uint64_t JIntrospection::DoReplLoop(uint64_t next_evt_nr) {
    if (!m_enabled) {
        return 0;
    }
    bool stay_in_loop = true;
    next_evt_nr = 0;  // 0 denotes that Repl stops at the next event number by default
    PrintEvent();
    while (stay_in_loop) {
        std::string user_input;
        m_out << "JANA: ";
        std::getline(m_in, user_input);
        auto result = Parse(user_input);
        auto token = result.first;
        auto args = result.second;
        if (token == "PrintEvent") {
            PrintEvent();
        }
        else if (token == "PrintFactories" && args.empty()) {
            PrintFactories();
        }
        else if (token == "PrintFactory" && (args.size() == 1)) {
            PrintFactory(args[0]);
        }
        else if (token == "PrintJObjects" && (args.size() == 1)) {
            PrintJObjects(args[0]);
        }
        else if (token == "PrintJObject" && (args.size() == 2)) {
            PrintJObject(args[0], args[1]);
        }
        else if (token == "PrintAncestors" && (args.size() == 1)) {
            PrintAncestors(args[0]);
        }
        else if (token == "PrintAssociations" && (args.size() == 2)) {
            PrintAssociations(args[0], args[1]);
        }
        else if (token == "Next") {
            stay_in_loop = false;
            if (args.size() > 0) {
                next_evt_nr = args[0];
            }
        }
        else if (token == "Finish") {
            stay_in_loop = false;
            m_enabled = false;
        }
        else if (token == "Quit") {
            stay_in_loop = false;
            m_enabled = false;
            m_event->GetJApplication()->Quit(false);
        }
        else if (token == "PrintHelp") {
            PrintHelp();
        }
        else if (token == "") {
            // Do nothing
        }
        else {
            m_out << "Error: Unrecognized command, or wrong argument count" << std::endl;
            PrintHelp();
        }
    }
    return next_evt_nr;
}
