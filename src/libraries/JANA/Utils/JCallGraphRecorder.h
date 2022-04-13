//
// Created by Nathan Brei on 10/25/21.
//

#ifndef JANA2_JCALLGRAPHRECORDER_H
#define JANA2_JCALLGRAPHRECORDER_H

#include <vector>
#include <string>
#include <cassert>
#include <sys/time.h>

class JCallGraphRecorder {
public:
    enum JDataSource {
        DATA_NOT_AVAILABLE = 1,
        DATA_FROM_CACHE,
        DATA_FROM_SOURCE,
        DATA_FROM_FACTORY
    };

    struct JCallGraphNode {
        std::string caller_name;
        std::string caller_tag;
        std::string callee_name;
        std::string callee_tag;
        double start_time = 0;
        double end_time = 0;
        JDataSource data_source = DATA_NOT_AVAILABLE;
	JCallGraphNode() {}
	JCallGraphNode(std::string caller_name, std::string caller_tag, std::string callee_name, std::string callee_tag) 
	: caller_name(caller_name), caller_tag(caller_tag), callee_name(callee_name), callee_tag(callee_tag) {}
    };

    struct JCallStackFrame {
        std::string factory_name;
        std::string factory_tag;
        double start_time = 0;
    };

    struct JErrorCallStack {
        std::string factory_name;
        std::string tag;
        const char* filename;
        int line = 0;
    };

private:
    bool m_enabled = false;
    std::vector<JCallStackFrame> m_call_stack;
    std::vector<JErrorCallStack> m_error_call_stack;
    std::vector<JCallGraphNode> m_call_graph;

public:
    inline bool IsEnabled() const { return m_enabled; }
    inline void SetEnabled(bool recordingEnabled=true){ m_enabled = recordingEnabled; }
    inline void StartFactoryCall(const std::string& callee_name, const std::string& callee_tag);
    inline void FinishFactoryCall(JDataSource data_source=JDataSource::DATA_FROM_FACTORY);
    inline std::vector<JCallGraphNode> GetCallGraph() {return m_call_graph;} ///< Get the current factory call stack
    inline void AddToCallGraph(const JCallGraphNode &cs) {if(m_enabled) m_call_graph.push_back(cs);} ///< Add specified item to call stack record but only if record_call_stack is true
    inline void AddToErrorCallStack(const JErrorCallStack &cs) {if (m_enabled) m_error_call_stack.push_back(cs);} ///< Add layer to the factory call stack
    inline std::vector<JErrorCallStack> GetErrorCallStack(){return m_error_call_stack;} ///< Get the current factory error call stack
    void PrintErrorCallStack(); ///< Print the current factory call stack
    void Reset();
    std::vector<std::pair<std::string, std::string>> TopologicalSort() const;
};



void JCallGraphRecorder::StartFactoryCall(const std::string& callee_name, const std::string& callee_tag) {

    /// This is used to fill initial info into a call_stack_t stucture
    /// for recording the call stack. It should be matched with a call
    /// to CallStackEnd. It is normally called from the Get() method
    /// above, but may also be used by external actors to manipulate
    /// the call stack (presumably for good and not evil).

    if (!m_enabled) return;
    struct itimerval tmr;
    getitimer(ITIMER_PROF, &tmr);
    double start_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec / 1.0E6;
    JCallStackFrame frame;
    frame.factory_name = callee_name;
    frame.factory_tag = callee_tag;
    frame.start_time = start_time;
    m_call_stack.push_back(frame);
}


void JCallGraphRecorder::FinishFactoryCall(JCallGraphRecorder::JDataSource data_source) {

    /// Complete a call stack entry. This should be matched
    /// with a previous call to CallStackStart which was
    /// used to fill the cs structure.

    if (!m_enabled) return;
    assert(!m_call_stack.empty());

    struct itimerval tmr;
    getitimer(ITIMER_PROF, &tmr);
    double end_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;

    JCallStackFrame& callee_frame = m_call_stack.back();

    JCallGraphNode node;
    node.callee_name = callee_frame.factory_name;
    node.callee_tag = callee_frame.factory_tag;
    node.start_time = callee_frame.start_time;
    node.end_time = end_time;
    node.data_source = data_source;

    m_call_stack.pop_back();

    if (!m_call_stack.empty()) {
        JCallStackFrame& caller_frame = m_call_stack.back();
        node.caller_name = caller_frame.factory_name;
        node.caller_tag = caller_frame.factory_tag;
        m_call_graph.push_back(node);
    }
}


#endif //JANA2_JCALLGRAPHRECORDER_H
