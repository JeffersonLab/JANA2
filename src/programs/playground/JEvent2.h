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

#ifndef JANA2_JEVENT2_H
#define JANA2_JEVENT2_H

#include "JLazyCollection.h"

#include <cstdint>

/// JEvent is extremely simple. All of the interesting parts, the lazy, recursive, heterogeneous map of JObjects,
/// is delegated to the JFactorySet, and JEvent simply becomes a struct of identifiers for bookkeeping that aggregate.
/// This allows us to cleanly reuse Factories and FactorySets for things which aren't necessarily JEvents,
/// e.g. subevents or blocks or streams.

class JEvent : virtual public JLazyCollection<JEvent> {

protected:
    uint64_t m_run_number;
    uint64_t m_event_number;
    uint64_t m_start_timestamp;
    uint64_t m_finish_timestamp;

public:
    JEvent() : JLazyCollection(this) {}

    uint64_t GetRunNumber() { return m_run_number; }
    uint64_t GetEventNumber() { return m_event_number; }
    uint64_t GetStartTimestamp() { return m_start_timestamp; }
    uint64_t GetFinishTimestamp() { return m_finish_timestamp; }

    // Everything else is inherited from JFactorySet
};


/// We don't want the user to be able to change the change the run number, etc, retroactively,
/// but we do want them to be able to set them in the first place. Similarly, we want to allow
/// JEventSources to Insert any JObject they want into the JEvent, but disallow any JFactory from
/// doing the same. We use diamond inheritance in order to enforce these weird permissions.
/// JEventSources are given JEventMutable pointers, but JFactories are only given JEvent pointers.

class JEventMutable : public JEvent, public JLazyCollectionMutable<JEvent> {

public:
    JEventMutable(): JLazyCollection(this), JLazyCollectionMutable(this) {};

    void SetRunNumber(uint64_t run_number) { m_run_number = run_number; }
    void SetEventNumber(uint64_t event_number) { m_event_number = event_number; }
    void SetStartTimestamp(uint64_t start_timestamp) { m_start_timestamp = start_timestamp; }
    void SetFinishTimestamp(uint64_t finish_timestamp) { m_finish_timestamp = finish_timestamp; }
};


// Legacy terminology

using JFactorySet = JLazyCollection<JEvent>;

using JFactory = JAbstractFactory<JEvent>;

template <typename T>
using JFactoryT = JAbstractFactoryT<JEvent, T>;


#endif //JANA2_JEVENT2_H


