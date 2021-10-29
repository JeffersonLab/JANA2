//
// Created by Nathan Brei on 10/25/21.
//

#ifndef JANA2_JCALLGRAPHRECORDER_H
#define JANA2_JCALLGRAPHRECORDER_H

#include <vector>
#include <string>
#include <sys/time.h>

class JCallGraphRecorder {
public:
    enum JDataSource {
        DATA_NOT_AVAILABLE = 1,
        DATA_FROM_CACHE,
        DATA_FROM_SOURCE,
        DATA_FROM_FACTORY
    };

    struct JCallStack {
        std::string caller_name;
        std::string caller_tag;
        std::string callee_name;
        std::string callee_tag;
        double start_time;  // TODO: Change this to std::chrono something. Why not a duration instead?
        double end_time;
        JDataSource data_source;
    };

    struct JErrorCallStack {
        const char* factory_name;
        std::string tag;
        const char* filename;
        int line;
    };

private:
    std::vector<JErrorCallStack> m_error_call_stack;
    std::vector<JCallStack> m_call_stack;
    bool m_record_call_stack;
    std::string m_caller_name;
    std::string m_caller_tag;

public:
    inline bool GetCallStackRecordingStatus(){ return m_record_call_stack; }
    inline void DisableCallStackRecording(){ m_record_call_stack = false; }
    inline void EnableCallStackRecording(){ m_record_call_stack = true; }
    inline void CallStackStart(JCallStack &cs, const std::string &caller_name, const std::string &caller_tag, const std::string callee_name, const std::string callee_tag);
    inline void CallStackEnd(JCallStack &cs);
    inline std::vector<JCallStack> GetCallStack() {return m_call_stack;} ///< Get the current factory call stack
    inline void AddToCallStack(JCallStack &cs) {if(m_record_call_stack) m_call_stack.push_back(cs);} ///< Add specified item to call stack record but only if record_call_stack is true
    inline void AddToErrorCallStack(JErrorCallStack &cs) {m_error_call_stack.push_back(cs);} ///< Add layer to the factory call stack
    inline std::vector<JErrorCallStack> GetErrorCallStack(){return m_error_call_stack;} ///< Get the current factory error call stack
    void PrintErrorCallStack(); ///< Print the current factory call stack
};



void JCallGraphRecorder::CallStackStart(JCallGraphRecorder::JCallStack &cs, const std::string &caller_name,
                                        const std::string &caller_tag, const std::string callee_name,
                                        const std::string callee_tag) {

    /// This is used to fill initial info into a call_stack_t stucture
    /// for recording the call stack. It should be matched with a call
    /// to CallStackEnd. It is normally called from the Get() method
    /// above, but may also be used by external actors to manipulate
    /// the call stack (presumably for good and not evil).

    struct itimerval tmr;
    getitimer(ITIMER_PROF, &tmr);

    cs.caller_name = this->m_caller_name;
    cs.caller_tag = this->m_caller_tag;
    this->m_caller_name = cs.callee_name = callee_name;
    this->m_caller_tag = cs.callee_tag = callee_tag;
    cs.start_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec / 1.0E6;
}


void JCallGraphRecorder::CallStackEnd(JCallGraphRecorder::JCallStack &cs) {

    /// Complete a call stack entry. This should be matched
    /// with a previous call to CallStackStart which was
    /// used to fill the cs structure.

    struct itimerval tmr;
    getitimer(ITIMER_PROF, &tmr);
    cs.end_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;
    m_caller_name = cs.caller_name;
    m_caller_tag  = cs.caller_tag;
    m_call_stack.push_back(cs);
}


#endif //JANA2_JCALLGRAPHRECORDER_H
