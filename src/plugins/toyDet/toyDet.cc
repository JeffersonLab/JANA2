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

#include <JANA/Streaming/JStreamingEventSource.h>
#include <JANA/JCsvWriter.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "JEventProcessor_toyDet.h"
#include "JFactoryGenerator_toyDet.h"
#include "DASFileSource.h"
#include "FECSampleFactory.h"
#include "INDRAMessage.h"
#include "ZmqTransport.h"

void dummy_publisher_loop() {

    //const char* source_file = "run-5-mhz-80-chan-100-ev.dat";
    std::string dest_socket = "tcp://127.0.0.1:5555";
    size_t channel_count = 10;
    size_t source_count = 2;
    size_t delay_ms = 200;

    std::this_thread::sleep_for(std::chrono::seconds(4));  // Wait for JANA to fire up so we don't lose data
    std::cout << "Starting producer loop" << std::endl;

    size_t event_id = 1;
    uint32_t channel_id = 1;
    uint32_t source_id = 1;

    ZmqTransport transport {dest_socket, true};
    transport.initialize();

    DASEventMessage message;
    FILE* f = fopen("run-5-mhz-80-chan-100-ev.dat", "r");
    if (f == nullptr) {
        std::cout << "Unable to open file, exiting." << std::endl;
        exit(0);
    }
    size_t BYTES_PER_EVENT = 1024*80*5;
    while (fread(message.payload, 1, BYTES_PER_EVENT, f) == BYTES_PER_EVENT) {

        message.record_counter++; // Increment event number
        transport.send(message);
        std::cout << "Send: " << message << " (" << sizeof(DASEventMessage) << " bytes)" << std::endl;
        consume_cpu_ms(delay_ms, 0, false);
    }

    // Send an end-of-stream message
    message = DASEventMessage(0, 0, {});
    transport.send(message);
    std::cout << "Send: end-of-stream" << std::endl;
}

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->SetParameterValue("nthreads", 1);
    app->SetParameterValue("jana:extended_report", false);
    app->SetParameterValue("jana:legacy_mode", 0);

    /// Uncomment these lines in order to read from file directly
    //app->Add("run-5-mhz-80-chan-100-ev.dat");
    //app->Add(new JEventSourceGeneratorT<DASFileSource>());
    //app->Add(new JFactoryGenerator_toyDet());

    /// Uncomment these lines in order to stream file over zmq
    auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport("tcp://127.0.0.1:5555"));
    app->Add(new JStreamingEventSource<DASEventMessage>(std::move(transport)));
    new std::thread(dummy_publisher_loop);

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JCsvWriter<FECSample>());
    app->Add(new JFactoryGeneratorT<FECSampleFactory>());

}
} // "C"


