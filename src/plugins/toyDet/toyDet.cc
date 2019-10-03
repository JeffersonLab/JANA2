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

#include "JEventProcessor_toyDet.h"
#include "JEventSource_toyDet.h"
#include "JFactoryGenerator_toyDet.h"
#include "JFactory_rawSamples.h"

#include "INDRAMessage.h"
#include "ZmqTransport.h"

void dummy_publisher_loop() {

    std::string source_file = "run-10-mhz-10-chan-10-ev.dat";
    std::string dest_socket = "tcp://127.0.0.1:5555";
    size_t channel_count = 10;
    size_t source_count = 2;
    size_t delay_ms = 200;

    std::this_thread::sleep_for(std::chrono::seconds(2));  // Wait for JANA to fire up so we don't lose data
    std::cout << "Starting producer loop" << std::endl;

    size_t event_id = 1;
    uint32_t channel_id = 1;
    uint32_t source_id = 1;

    std::ifstream file_stream(source_file);
    if (!file_stream.is_open()) {
        std::cout << "Unable to open file, exiting." << std::endl;
        exit(0);
    }
    ZmqTransport transport {dest_socket, true};
    transport.initialize();
    std::string line;
    std::vector<double> data;

    while (std::getline(file_stream, line)) {

        // Skip comment lines
        if (line[0] == '#' || line[0] == '@') continue;

        // Parse line as sequence of doubles
        std::stringstream line_stream(line);
        std::string value;
        while (std::getline(line_stream, value, ' ')) {
            data.push_back(std::stod(value));
        }

        // Append all sources to data buffer
        source_id += 1;
        if (source_id <= source_count) continue;

        // Send line as message over ZMQ
        auto message = ToyDetMessage(event_id, channel_id, data);
        transport.send(message);
        std::cout << "Send: " << message << " (" << sizeof(ToyDetMessage) << " bytes)" << std::endl;
        consume_cpu_ms(delay_ms, 0, false);
        data.clear();

        // Update source and channel ids
        source_id = 1;
        channel_id += 1;
        if (channel_id > channel_count) {
            event_id += 1;
            channel_id = 1;
        }
    }

    // Send an end-of-stream message
    auto message = ToyDetMessage(0, 0, data);
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
    //app->Add("run-10-mhz-10-chan-10-ev.dat");
    //app->Add(new JEventSourceGeneratorT<JEventSource_toyDet>());
    //app->Add(new JFactoryGenerator_toyDet());

    /// Uncomment these lines in order to stream file over zmq
    auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport("tcp://127.0.0.1:5555"));
    app->Add(new JStreamingEventSource<ToyDetMessage>(std::move(transport)));
    new std::thread(dummy_publisher_loop);

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JCsvWriter<rawSamples>());
    app->Add(new JFactoryGeneratorT<JFactory_rawSamples>());


}
} // "C"


