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

#ifndef JANA2_JZMQSOURCE_H
#define JANA2_JZMQSOURCE_H

#include "zmq.hpp"

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>
#include "JSampleSource.h"

template <typename T, template <typename> class JSampleSourceT>
class JEventSource_SingleSample : public JEventSource {

public:

    JEventSource_SingleSample (std::string name, JApplication* app)
        : JEventSource(name, app)
        , m_sample_source(name, app) {}

    void Open() override {
        m_sample_source.initialize();
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto item = new JSample<T>();  // This is why T requires a zero-arg ctor
        auto result = m_sample_source.pull(*item);
        switch (result) {
            case JSampleSourceStatus::Finished: throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JSampleSourceStatus::TryAgainLater: throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JSampleSourceStatus::Error: throw JEventSource::RETURN_STATUS::kERROR;
            default: break;
        }
        // At this point, we know that item contains a valid Sample<T>

        event->SetEventNumber(m_next_id);
        m_next_id += 1;
        event->Insert<T>(&item->payload);
        std::cout << "Received: " << item->payload << std::endl;
    }

    static std::string GetDescription() {
        return "JEventSource_SingleSample";
    }


private:
    JSampleSourceT<T> m_sample_source;
    uint64_t m_delay_ms;
    uint64_t m_next_id = 0;
};


#endif //JANA2_JZMQSOURCE_H
