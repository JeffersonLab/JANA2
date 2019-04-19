//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#ifndef JANA2_JSUBEVENTARROW_H
#define JANA2_JSUBEVENTARROW_H


#include "JArrow.h"
#include "JEvent.h"

using Event = std::shared_ptr<const JEvent>;


/// Data structure containing all of the metadata needed to merge subevents
/// on a per-event basis without needing a barrier. This is used internally by
/// Queues and Arrows and the user should never need to interact with it directly
struct JSubevent {
    JEvent* parent;
    JObject* subevent_data;
    long subevent_id;
    long subevent_count;
};

/// Abstract class which is meant to extended by the user to contain all
/// subtask-related functions. (Data lives in a JObject instead)
/// Future versions might be templated for two reasons:
///  1. To make the functions non-virtual,
///  2. To replace the generic JObject pointer with something typesafe
/// Future versions could also recycle JObjects by using another Queue.
class JSubeventProcessor {

    virtual void split(JEvent& parent, std::vector<JObject*>& subevents) = 0;

    virtual void process(JObject& subevent) = 0;

    virtual void merge(JEvent& parent, std::vector<JObject*>& subevents) = 0;

};

class JSubeventArrow : public JArrow {
    JSubeventProcessor* _processor;
    Queue<JSubevent>* _inbox;
    Queue<JSubevent>* _outbox;
public:
    JSubeventArrow(std::string name, JSubeventProcessor* processor,
                   Queue<JSubevent>* inbox, Queue<JSubevent>* outbox);
    Status execute() override;
};

class JSplitArrow : public JArrow {
    JSubeventProcessor* _processor;
    Queue<Event>* _inbox;
    Queue<JSubevent>* _outbox;
public:
    JSplitArrow(std::string name, JSubeventProcessor* processor,
                Queue<Event>* inbox, Queue<JSubevent>* outbox);
    Status execute() override;
};

class JMergeArrow : public JArrow {
    JSubeventProcessor* _processor;
    Queue<JSubevent>* _inbox;
    Queue<Event>* _outbox;
public:
    JMergeArrow(std::string name, JSubeventProcessor* processor,
                Queue<JSubevent>* inbox, Queue<Event>* outbox);
    Status execute() override;
};


#endif //JANA2_JSUBEVENTARROW_H


