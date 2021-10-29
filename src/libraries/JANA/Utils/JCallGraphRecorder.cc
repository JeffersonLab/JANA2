//
// Created by Nathan Brei on 10/25/21.
//



#include "JCallGraphRecorder.h"

#include <sstream>
#include <JANA/Compatibility/JStreamLog.h>

using std::vector;
using std::string;
using std::endl;


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
