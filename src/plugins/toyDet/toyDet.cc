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

#include <JANA/Streaming/JEventBuilder.h>
#include <JANA/Streaming/JSessionWindow.h>

#include "JEventProcessor_toyDet.h"
#include "JEventSource_toyDet.h"
#include "JFactoryGenerator_toyDet.h"
#include "JFactory_rawSamples.h"
#include "DummyZmqPublisher.h"
#include "INDRAMessage.h"
#include "../../examples/JExample7/ZmqTransport.h"

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

    auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport("tcp://127.0.0.1:5555"));
    auto window = std::unique_ptr<JSessionWindow<ToyDetMessage>>(new JSessionWindow<ToyDetMessage>(10, {0,1,2}));

    app->Add(new JEventBuilder<ToyDetMessage>(std::move(transport), std::move(window)));

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JFactoryGeneratorT<JFactory_rawSamples>());

    //app->Add(new JEventSourceGeneratorT<JEventSource_toyDet>());
    //app->Add(new JFactoryGenerator_toyDet());

    auto publisher = new std::thread(dummy_publisher_loop);
}
} // "C"
