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

#ifndef JANA2_JTESTTRACKER_H
#define JANA2_JTESTTRACKER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/JPerfUtils.h>
#include "JTestDataObjects.h"

class JTestTracker : public JFactoryT<JTestTrackData> {

    size_t m_cputime_ms = 200;
    size_t m_write_bytes = 1000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestTracker() : JFactoryT<JTestTrackData>("JTestTracker") {
        auto params = japp->GetJParameterManager();
        params->GetParameter("jtest:tracker_bytes", m_write_bytes);
        params->GetParameter("jtest:tracker_ms", m_cputime_ms);
        params->GetParameter("jtest:tracker_bytes_spread", m_write_spread);
        params->GetParameter("jtest:tracker_spread", m_cputime_spread);
    };

    void Process(const std::shared_ptr<const JEvent> &aEvent) {

        // Read (large) event data
        auto ed = aEvent->GetSingle<JTestEventData>();
        read_memory(ed->buffer);

        // Do lots of computation
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (small) track data
        auto td = new JTestTrackData;
        write_memory(td->buffer, m_write_bytes, m_write_spread);
        Insert(td);
    }
};

#endif //JANA2_JTESTTRACKER_H
