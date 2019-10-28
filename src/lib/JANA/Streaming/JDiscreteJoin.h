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

#ifndef JANA2_JDISCRETEEVENTJOIN_H
#define JANA2_JDISCRETEEVENTJOIN_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>

#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Streaming/JTransport.h>
#include <JANA/Streaming/JTrigger.h>

/// JEventBuilder pulls JMessages off of a user-specified JTransport, aggregates them into
/// JEvents using the JWindow of their choice, and decides which to keep via a user-specified
/// JTrigger. The user can choose to merge this Event stream with additional JMessage streams, possibly
/// applying a different trigger at each level. This is useful for level 2/3/n triggers and maybe EPICS data.

template <typename T>
class JDiscreteJoin : public JEventSource {
public:

    JDiscreteJoin(std::unique_ptr<JTransport>&& transport,
                       std::unique_ptr<JTrigger>&& trigger = std::unique_ptr<JTrigger>(new JTrigger()))

            : JEventSource("JEventBuilder")
            , m_transport(std::move(transport))
            , m_trigger(std::move(trigger))
    {
    }

    void Open() override {
        m_transport->initialize();
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto item = new T();  // This is why T requires a zero-arg ctor
        auto result = m_transport->receive(*item);
        switch (result) {
            case JTransport::Result::FINISHED:
                throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JTransport::Result::TRY_AGAIN:
                throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JTransport::Result::FAILURE:
                throw JEventSource::RETURN_STATUS::kERROR;
            default:
                break;
        }
        // At this point, we know that item contains a valid Sample<T>

        event->SetEventNumber(m_next_id);
        m_next_id += 1;
        event->Insert<T>(item);
        std::cout << "Emit: " << *item << std::endl;
    }

    static std::string GetDescription() {
        return "JEventBuilder";
    }


private:

    std::unique_ptr<JTransport> m_transport;
    std::unique_ptr<JTrigger> m_trigger;
    uint64_t m_delay_ms;
    uint64_t m_next_id = 0;
};



#endif //JANA2_JEVENTJOINER_H
