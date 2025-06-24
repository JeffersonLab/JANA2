
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.



#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/Utils/JBenchUtils.h>
#include <JANA/Streaming/JEventBuilder.h>
#include <JANA/Streaming/JStreamingEventSource.h>


#include "ReadoutMessageAuto.h"
#include "ZmqTransport.h"
#include "AHitParser.h"
#include "AHitAnomalyDetector.h"


void dummy_publisher_loop() {

    JBenchUtils bench_utils = JBenchUtils();
    bench_utils.set_seed(6, "ZmqMain.cc:dummy_publisher_loop");
    bench_utils.consume_cpu_ms(3000, 0);

    auto transport = ZmqTransport("tcp://127.0.0.1:5555", true);
    transport.initialize();

    for (size_t counter = 1; counter < 11; ++counter) {

        ReadoutMessageAuto message(nullptr);
        message.run_number = 0;
        message.event_number = counter;

        message.payload_size = 4;
        message.payload[0] = bench_utils.randfloat(0,1);
        message.payload[1] = bench_utils.randfloat(-100,100);
        message.payload[2] = bench_utils.randfloat(-100,100);
        message.payload[3] = bench_utils.randfloat(-100,100);

        transport.send(message);
        std::cout << "Send: " << message << "(" << message.get_buffer_capacity() << " bytes)" << std::endl;
        bench_utils.consume_cpu_ms(1000, 0);
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

    app->Add(new AHitAnomalyDetector(5000));
    app->Add(new JFactoryGeneratorT<AHitParser>());

    new std::thread(dummy_publisher_loop);
}
} // "C"


