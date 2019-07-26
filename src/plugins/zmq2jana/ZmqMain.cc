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


#include <JANA/JApplication.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "internals/JEventSource_SingleSample.h"
#include "AHitAnomalyDetector.h"
#include "DummyZmqPublisher.h"
#include "internals/JSampleSource_Zmq.h"
#include "ReadoutMessage.h"
#include "AHitParser.h"

void dummy_publisher_loop() {
	ZmqDummyPublisher pub("tcp://127.0.0.1:5555", "fcal", 3, 2, 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	pub.publish(10);
}

extern "C"{
void InitPlugin(JApplication *app) {

	InitJANAPlugin(app);
	app->Add(new JEventSourceGeneratorT<JEventSource_SingleSample<ReadoutMessage<4>, JSampleSource_Zmq>>(app));
	app->Add(new AHitAnomalyDetector(app, 5000));
	app->Add(new JFactoryGeneratorT<AHitParser>());

	// So we don't have to put this on the cmd line every time
	app->Add("tcp://127.0.0.1:5555");
	app->SetParameterValue("jana:legacy_mode", 0);
	app->SetParameterValue("jana:extended_report", 0);


	auto publisher = new std::thread(dummy_publisher_loop);
}
} // "C"


