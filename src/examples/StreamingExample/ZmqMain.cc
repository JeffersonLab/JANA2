
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.



#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/Streaming/JEventBuilder.h>
#include <JANA/Streaming/JStreamingEventSource.h>

#include "ReadoutMessageAuto.h"
#include "ZmqTransport.h"
#include "AHitParser.h"
#include "AHitAnomalyDetector.h"


void dummy_publisher_loop() {

    consume_cpu_ms(3000, 0, false);

    auto transport = ZmqTransport("tcp://127.0.0.1:5555", true);
    transport.initialize();

    for (size_t counter = 1; counter < 11; ++counter) {

        ReadoutMessageAuto message(nullptr);
        message.run_number = 0;
        message.event_number = counter;

        message.payload_size = 4;
        message.payload[0] = randfloat(0,1);
        message.payload[1] = randfloat(-100,100);
        message.payload[2] = randfloat(-100,100);
        message.payload[3] = randfloat(-100,100);

        transport.send(message);
        std::cout << "Send: " << message << "(" << message.get_buffer_capacity() << " bytes)" << std::endl;
        consume_cpu_ms(1000, 0, false);
    }

    // Send end-of-stream message so that JANA knows to shut down
    ReadoutMessageAuto message (nullptr);
    message.set_end_of_stream();
    transport.send(message);
}


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);

    auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport("tcp://127.0.0.1:5555"));
    app->Add(new JStreamingEventSource<ReadoutMessageAuto>(std::move(transport)));

    app->Add(new AHitAnomalyDetector(app, 5000));
    app->Add(new JFactoryGeneratorT<AHitParser>());

    // So we don't have to put this on the cmd line every time
    app->SetParameterValue("jana:legacy_mode", 0);
    app->SetParameterValue("jana:extended_report", 0);

    new std::thread(dummy_publisher_loop);
}
} // "C"


