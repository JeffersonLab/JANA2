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

#ifndef JANA2_COMPONENTTESTS_H
#define JANA2_COMPONENTTESTS_H


#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

struct SimpleSource : public JEventSource {

    std::atomic_int open_count {0};
    std::atomic_int event_count {0};

    SimpleSource(std::string source_name, JApplication *japp) : JEventSource(source_name, japp)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const {
        return GetDemangledName<decltype(*this)>();
    }

    void Open() override {
        std::cout << "Calling Open()" << std::endl;
        open_count += 1;
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        std::cout << "Calling GetEvent()" << std::endl;
        if (++event_count == 5) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
    }
};


struct SimpleProcessor : public JEventProcessor {

    std::atomic_int init_count {0};
    std::atomic_int finish_count {0};

    SimpleProcessor(JApplication* app) : JEventProcessor(app) {}

    void Init() override {
        std::cout << "Calling Init()" << std::endl;
        init_count += 1;
    }

    void Process(const std::shared_ptr<const JEvent>& aEvent) override {
        std::cout << "Calling Process()" << std::endl;
    }

    void Finish() override {
        std::cout << "Calling Finish()" << std::endl;
        finish_count += 1;
    }
};


#endif //JANA2_COMPONENTTESTS_H
