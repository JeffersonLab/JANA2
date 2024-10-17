//
// Created by davidl on 11/9/22.
//

#pragma once

#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/JFactory.h>

/// Stack object to handle recording entry to call graph
///
/// This is used to add a new entry to a JCallGraph, recording the time when it is created
/// and then again when it is deleted. The intent is to use it as a local stack variable
/// so if the call being timed throws an exception, the destructor of this will still get
/// run, guaranteeing the call stack entry is completed.
///
/// Objects of this type are used in JEvent::Get and JEventProcessingArrow::execute
/// (and possibly other places).
class JCallGraphEntryMaker{
public:

    JCallGraphEntryMaker(JCallGraphRecorder &callgraphrecorder, JFactory *factory) : m_call_graph(callgraphrecorder), m_factory(factory){
        m_call_graph.StartFactoryCall(m_factory->GetObjectName(), m_factory->GetTag());
    }

    JCallGraphEntryMaker(JCallGraphRecorder &callgraphrecorder, std::string name) : m_call_graph(callgraphrecorder) {
        // This is used mainly for JEventProcessors
        m_call_graph.StartFactoryCall(name, "");
    }

    ~JCallGraphEntryMaker(){
        m_call_graph.FinishFactoryCall( m_factory ? m_factory->GetDataSource():JCallGraphRecorder::DATA_NOT_AVAILABLE );
    }

protected:
    JCallGraphRecorder &m_call_graph;
    JFactory *m_factory=nullptr;
};


