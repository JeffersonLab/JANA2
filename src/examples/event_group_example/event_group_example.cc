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


#include <JANA/JEventProcessor.h>
#include <JANA/JFactoryGenerator.h>


class JEventSource_eventgroups : public JEventSource {
public:
    JEventSource_eventgroups(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
        // Get service from SL
    };

    void GetEvent(std::shared_ptr<JEvent>) override {
        // If event counter = 0, randomly generate some number of events
        // Decrement remaining event counter
        // Close old group
        // Issue a new run number, also a new group
        // Add event to group
        // Optional: Barrier event
    }
};


struct JEventData : public JObject {};


class JFactory_random_delay : public JFactoryT<JEventData> {
public:
    void Process(const std::shared_ptr<const JEvent>& event) override {
        // delay a random amount of time so that runs get jumbled up
    }

};

class JEventProcessor_eventgroups : public JEventProcessor {
public:
    void Process(const std::shared_ptr<const JEvent>& event) override {
        // Perform a computation which induces a random delay
        // cout << evt nr, run nr
        // Mark event as finished
        // If first event in run, do something
        // If last event in run, do something
    }
};


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);
    app->Add(new JEventSource_eventgroups("dummy_source", app));
    app->Add(new JEventProcessor_eventgroups());
    app->Add(new JFactoryGeneratorT<JFactory_random_delay>());

}
} // "C"
