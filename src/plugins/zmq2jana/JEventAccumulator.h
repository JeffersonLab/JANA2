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

#ifndef JANA2_JEVENTACCUMULATOR_H
#define JANA2_JEVENTACCUMULATOR_H


#include "ZmqMessage.h"
#include <queue>
#include <map>
#include <algorithm>

template <typename T>
struct maybe {
    bool is_none = true;
    T value;
};

using Duration = uint64_t;

class JEventAccumulator {

private:
    std::priority_queue<ZmqMessage> m_inbox;
    std::vector<ZmqMessage> m_outbox;
    std::map<DetectorId, maybe<Timestamp>> m_latest_sample_times;
    maybe<Timestamp> m_latest_complete_time;
    maybe<Timestamp> m_latest_event_start;
    Duration m_event_interval;

public:

    JEventAccumulator(Duration event_interval, const std::vector<DetectorId>& detectors)
    : m_event_interval(event_interval) {
        for (auto id : detectors) {
            m_latest_sample_times.insert({id, maybe<Timestamp>()});
        }
    }

    bool stage_next_event() {
        while (true) {
            if (m_latest_complete_time.is_none) {
                return false;
            }
            if (m_inbox.empty()) {
                return false;
            }
            Timestamp peek_time = m_inbox.top().timestamp;

            if (m_latest_event_start.is_none) {
                m_latest_event_start.is_none = false;
                m_latest_event_start.value = peek_time;
            }

            if (peek_time > m_latest_complete_time.value) {
                return false;
            }
            if (m_latest_event_start.value + m_event_interval < peek_time) {
                return true;
            }
            m_outbox.push_back(m_inbox.top());
            m_inbox.pop();
        }
    }

    void push(std::vector<ZmqMessage>&& messages) {
        for (auto m : messages) {
            auto iter = m_latest_sample_times.find(m.detector_id);
            if (iter == m_latest_sample_times.end()) {
                throw std::runtime_error("Unexpected detector!");
            }
            else {
                maybe<Timestamp> old_timestamp = iter->second;
                if (!old_timestamp.is_none && old_timestamp.value > m.timestamp) {
                    throw std::runtime_error("Partial ordering violation!");
                }
                else {
                    m_inbox.push(m);
                }
            }
        }
        // Find latest complete time
        if (!m_latest_sample_times.empty()) {
            auto iter = m_latest_sample_times.begin();
            m_latest_complete_time = iter->second;

            while (iter != m_latest_sample_times.end()) {
                if (iter->second.is_none) {
                    m_latest_complete_time.is_none = true;
                    break;
                }
                m_latest_complete_time.value = std::min(m_latest_complete_time.value, iter->second.value);
                ++iter;
            }
        }
    }

    bool pop(std::vector<ZmqMessage>& destination) {

    }


};

#endif //JANA2_JEVENTACCUMULATOR_H
