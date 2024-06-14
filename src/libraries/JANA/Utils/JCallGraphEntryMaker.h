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
        // (This is used mainly for JEventProcessors and called from JEventProcessorArrow::execute )
        m_call_graph.StartFactoryCall(name, "");
    }

    ~JCallGraphEntryMaker(){
        JCallGraphRecorder::JDataSource datasource = JCallGraphRecorder::DATA_NOT_AVAILABLE;
        if (m_factory) {
            auto status = m_factory->GetCreationStatus();
            auto origin = m_factory->GetInsertOrigin();
            if (status == JFactory::CreationStatus::Inserted) {
                if (origin == JCallGraphRecorder::ORIGIN_FROM_SOURCE) {
                    datasource = JCallGraphRecorder::DATA_FROM_SOURCE;
                } 
                else {
                    datasource = JCallGraphRecorder::DATA_FROM_CACHE; 
                    // Really came from factory, but if Inserted, it was a secondary data type.
                }
            }
            else {
                datasource = JCallGraphRecorder::DATA_FROM_FACTORY;
            }
        }
        m_call_graph.FinishFactoryCall(datasource);
    }

protected:
    JCallGraphRecorder &m_call_graph;
    JFactory *m_factory=nullptr;
};


