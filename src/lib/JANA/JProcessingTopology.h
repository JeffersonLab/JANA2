//
// Created by Nathan W Brei on 2019-04-24.
//

#ifndef JANA2_JPROCESSINGTOPOLOGY_H
#define JANA2_JPROCESSINGTOPOLOGY_H


#include "JEventSourceManager.h"

struct JProcessingTopology : public JActivable {

    JEventSourceManager event_source_manager;

    bool all_sources_closed();



};


#endif //JANA2_JPROCESSINGTOPOLOGY_H
