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

#ifndef JANA2_JEVENTBUILDER_H
#define JANA2_JEVENTBUILDER_H

#include "ZmqMessage.h"
#include "JDataSource.h"

#include <map>
#include <queue>


template <typename S>
struct maybe {
    bool is_none = true;
    S value;
};

using Duration = uint64_t;

template <typename T>
class JEventBuilder {

public:
    JEventBuilder(Duration event_interval, const std::vector<DetectorId>& detectors)
    : m_event_interval(event_interval) {
        for (auto id : detectors) {
            m_inbox.insert({id, {}});
        }
    }

    void push(std::vector<JData<T>>&& messages) {
        for (auto m : messages) {
            auto iter = m_inbox.find(m.detector_id);
            if (iter == m_inbox.end()) {
                throw std::runtime_error("Unexpected detector!");
            }
            else {
                iter.second.push_back(m);
            }
        }
    }

    maybe<Timestamp> find_next_event_start() {
        maybe<Timestamp> result;
        result.is_none = true;
        if (!m_inbox.empty()) {
            result.is_none = false;
            auto iter = m_inbox.begin();
            result.value = iter->second.timestamp;

            while (++iter != m_inbox.end()) {
                if (iter->second.is_none) {
                    result.is_none = true;
                    return result;
                }
                result.value = std::min(result.value, iter->second.value);
            }
        }
        return result;
    }

    bool stage_next_event() {
        if (m_next_event_start.is_none) {
            m_next_event_start = find_next_event_start();
            if (m_next_event_start.is_none) {
                return false;
            }
        }
        Timestamp next_event_finish = m_next_event_start.value + m_event_interval;
        for (auto iter : m_inbox) { // for each detector
            auto & q = iter.second;
            while (q.front().timestamp <= next_event_finish) {
                m_outbox.push_back(std::move(q.front()));
                q.pop_front();
            }
            if (q.empty()) {
                // we've run out of samples, so we can't say whether we have everything or not
                return false;
            }
        }
        return true;
    }


private:
    std::map<DetectorId, std::deque<JData<T>>> m_inbox;
    std::vector<JData<T>> m_outbox;
    maybe<Timestamp> m_next_event_start;
    Duration m_event_interval;




};



#endif //JANA2_JEVENTBUILDER_H
