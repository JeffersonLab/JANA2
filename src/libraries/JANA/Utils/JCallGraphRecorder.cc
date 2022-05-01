//
// Created by Nathan Brei on 10/25/21.
//



#include "JCallGraphRecorder.h"

#include <sstream>
#include <JANA/Compatibility/JStreamLog.h>
#include <queue>
#include <algorithm>

using std::vector;
using std::string;
using std::endl;

void JCallGraphRecorder::Reset() {
    m_call_graph.clear();
    m_call_stack.clear();
    m_error_call_stack.clear();
}

void JCallGraphRecorder::PrintErrorCallStack() {

    // Create a list of the call strings while finding the longest one
    vector<string> routines;
    unsigned int max_length = 0;
    for(unsigned int i=0; i<m_error_call_stack.size(); i++){
        string routine = m_error_call_stack[i].factory_name;
        if(m_error_call_stack[i].tag.length()){
            routine = routine + ":" + m_error_call_stack[i].tag;
        }
        if(routine.size()>max_length) max_length = routine.size();
        routines.push_back(routine);
    }

    std::stringstream sstr;
    sstr<<" Factory Call Stack"<<endl;
    sstr<<"============================"<<endl;
    for(unsigned int i=0; i<m_error_call_stack.size(); i++){
        string routine = routines[i];
        sstr<<" "<<routine<<string(max_length+2 - routine.size(),' ');
        if(m_error_call_stack[i].filename){
            sstr<<"--  "<<" line:"<<m_error_call_stack[i].line<<"  "<<m_error_call_stack[i].filename;
        }
        sstr<<endl;
    }
    sstr<<"----------------------------"<<endl;

    jout<<sstr.str();

}

std::vector<std::pair<std::string, std::string>> JCallGraphRecorder::TopologicalSort() const {
    // The JCallGraphNodes are _probably_ already in topological order, but we cannot assume
    // that because the user is allowed to add whatever JCallGraphNodes they wish.
    // This uses Kahn's algorithm because it is simple. Furthermore, it uses string comparisons instead of int
    // comparisons, which are slow. We can make this faster if we find it is necessary.

    using FacName = std::pair<std::string, std::string>;
    struct FacEdges {
        std::vector<FacName> incoming;
        std::vector<FacName> outgoing;
    };

    // Build adjacency matrix
    std::map<FacName, FacEdges> adjacency;
    for (const JCallGraphNode& node : m_call_graph) {

        adjacency[{node.caller_name, node.caller_tag}].incoming.emplace_back(node.callee_name, node.callee_tag);
        adjacency[{node.callee_name, node.callee_tag}].outgoing.emplace_back(node.caller_name, node.caller_tag);
    }

    std::vector<FacName> sorted;
    std::queue<FacName> ready;

    // Populate frontier of "ready" elements with no incoming edges
    for (auto& p : adjacency) {
        if (p.second.incoming.empty()) ready.push(p.first);
    }

    // Process each ready element
    while (!ready.empty()) {
        auto n = ready.front();
        ready.pop();
        sorted.push_back(n);
        for (auto& m : adjacency[n].outgoing) {
            auto& incoming = adjacency[m].incoming;
            incoming.erase(std::remove(incoming.begin(), incoming.end(), n), incoming.end());
            if (incoming.empty()) {
                ready.push(m);
            }
        }
    }
    return sorted;
}
