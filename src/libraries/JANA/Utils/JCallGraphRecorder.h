//
// Created by Nathan Brei on 10/25/21.
//

#ifndef JANA2_JCALLGRAPHRECORDER_H
#define JANA2_JCALLGRAPHRECORDER_H

#include <vector>
#include <string>
#include <cassert>
#include <sys/time.h>
#include <chrono>

#include <JANA/Services/JLoggingService.h>

// Note on tracking how/where Insert() objects came from.
//
// The JEvent class has a JCallGraphRecorder member object. This object has a member called
// m_insert_dataorigin_type that is used to indicate how new inserts should be classified. This
// value is copied into the m_insert_origin member of the JFactory during the call to JEvent::Insert. This
// is set to ORIGIN_FROM_SOURCE in JEventSource::DoNext just before GetEvent is called and reset after.
// This should capture all Insert calls made by the source.
//
// The default value for m_insert_dataorigin_type is ORIGIN_FROM_FACTORY which will usually be
// correct. Note that the factory's m_insert_origin member is initialized to ORIGIN_NOT_AVAILABLE,
// but this will be overwritten iff the factory has objects inserted by an Insert() call.
//
// WARNING: The above described mechanism could fail if a more complex subevent task scheme is implemented
// that results in multiple threads inserting into the same JEvent at the same time. It would actually
// need to be one thread inserting from a factory while another inserts from a source. Multiple source
// scenarios and multiple factory scenarios would be OK, but a mixture could result in incorrect
// values for the origin type being recorded.

class JCallGraphRecorder {
public:
    enum JDataSource {
        DATA_NOT_AVAILABLE = 1,
        DATA_FROM_CACHE,
        DATA_FROM_SOURCE,
        DATA_FROM_FACTORY
    };

    enum JDataOrigin {
        ORIGIN_NOT_AVAILABLE = 1,
        ORIGIN_FROM_FACTORY,
        ORIGIN_FROM_SOURCE
    };

    struct JCallGraphNode {
        std::string caller_name;
        std::string caller_tag;
        std::string callee_name;
        std::string callee_tag;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        JDataSource data_source = DATA_NOT_AVAILABLE;
	JCallGraphNode() {}
	JCallGraphNode(std::string caller_name, std::string caller_tag, std::string callee_name, std::string callee_tag) 
	: caller_name(caller_name), caller_tag(caller_tag), callee_name(callee_name), callee_tag(callee_tag) {}
    };

    struct JCallStackFrame {
        std::string factory_name;
        std::string factory_tag;
        std::chrono::steady_clock::time_point start_time;
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
    JDataOrigin m_insert_dataorigin_type = ORIGIN_FROM_FACTORY; // See note at top of file

public:
    inline bool IsEnabled() const { return m_enabled; }
    inline void SetEnabled(bool recordingEnabled=true){ m_enabled = recordingEnabled; }
    inline void StartFactoryCall(const std::string& callee_name, const std::string& callee_tag);
    inline JDataOrigin SetInsertDataOrigin(JDataOrigin origin){ auto previous = m_insert_dataorigin_type; m_insert_dataorigin_type = origin; return previous; }
    inline JDataOrigin GetInsertDataOrigin(){ return m_insert_dataorigin_type; }
    inline void FinishFactoryCall(JDataSource data_source=JDataSource::DATA_FROM_FACTORY);
    inline std::vector<JCallGraphNode> GetCallGraph() {return m_call_graph;} ///< Get the current factory call stack
    inline void AddToCallGraph(const JCallGraphNode &cs) {if(m_enabled) m_call_graph.push_back(cs);} ///< Add specified item to call stack record but only if record_call_stack is true
    inline void AddToErrorCallStack(const JErrorCallStack &cs) {if (m_enabled) m_error_call_stack.push_back(cs);} ///< Add layer to the factory call stack
    inline std::vector<JErrorCallStack> GetErrorCallStack(){return m_error_call_stack;} ///< Get the current factory error call stack
    void PrintErrorCallStack() const; ///< Print the current factory call stack
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
    JCallStackFrame frame;
    frame.factory_name = callee_name;
    frame.factory_tag = callee_tag;
    frame.start_time = std::chrono::steady_clock::now();
    m_call_stack.push_back(frame);
}


void JCallGraphRecorder::FinishFactoryCall(JCallGraphRecorder::JDataSource data_source) {

    /// Complete a call stack entry. This should be matched
    /// with a previous call to CallStackStart which was
    /// used to fill the cs structure.

    if (!m_enabled) return;
    assert(!m_call_stack.empty());

    JCallStackFrame& callee_frame = m_call_stack.back();

    JCallGraphNode node;
    node.callee_name = callee_frame.factory_name;
    node.callee_tag = callee_frame.factory_tag;
    node.start_time = callee_frame.start_time;
    node.end_time = std::chrono::steady_clock::now();
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
