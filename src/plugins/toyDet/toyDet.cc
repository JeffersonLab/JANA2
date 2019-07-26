//
//    File: toyDet/toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]


#include "../zmq2jana/internals/JEventSource_SingleSample.h"
#include "../zmq2jana/internals/JSampleSource_Zmq.h"

#include "JEventProcessor_toyDet.h"
#include "JEventSource_toyDet.h"
#include "JFactoryGenerator_toyDet.h"
#include "JFactory_rawSamples.h"
#include "DummyZmqPublisher.h"
#include "INDRAMessage.h"

void dummy_publisher_loop() {
    ZmqDummyPublisher pub("run-10-mhz-10-chan-10-ev.dat", "tcp://127.0.0.1:5555", 100, 10, 2);
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Wait for JANA to fire up so we don't lose data
    pub.loop();
}

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->SetParameterValue("nthreads", 1);
    app->SetParameterValue("jana:extended_report", false);

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JEventSourceGeneratorT<JEventSource_SingleSample<ToyDetMessage, JSampleSource_Zmq>>());
    app->Add(new JFactoryGeneratorT<JFactory_rawSamples>());
    app->Add("tcp://127.0.0.1:5555");  // TODO: Fix this later

    //app->Add(new JEventSourceGeneratorT<JEventSource_toyDet>());
    //app->Add(new JFactoryGenerator_toyDet());

    auto publisher = new std::thread(dummy_publisher_loop);
}
} // "C"
