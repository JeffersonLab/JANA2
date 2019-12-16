//
// Created by Nathan Brei on 2019-12-15.
//

#ifndef JANA2_TRIDASEVENT_H
#define JANA2_TRIDASEVENT_H

/// TridasEvent is an adapter between TRIDAS objects and JANA objects. The event source should
/// insert one TridasEvent into each JEvent.
struct TridasEvent : public JObject {

    void * tridas_data;        // TODO: this should be a pointer to an actual TRIDAS object. For now we just pretend.

    int run_number;            // TODO: The event source should extract event & run numbers directly from tridas_object,
    int event_number;          //       so we should remove these once we have an actual tridas_object
    int group_number;          // TODO: Group number should really be encapsulated inside the EventSource instead

    mutable bool should_keep;  // TODO: This needs to be mutable because we will be updating a const JObject
                               //       This won't be a problem with the 'real' TRIDAS, whose should_keep lives behind the
                               //       tridas_data pointer, but we should start thinking about a more elegant way to handle
                               //       the general case.
};


#endif //JANA2_TRIDASEVENT_H
