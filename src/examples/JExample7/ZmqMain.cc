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
#include <JANA/Streaming/JEventBuilder.h>
#include <JANA/Streaming/JSessionWindow.h>

#include "ReadoutMessageAuto.h"
#include "ZmqTransport.h"
#include "AHitParser.h"
#include "AHitAnomalyDetector.h"


void dummy_publisher_loop() {

    consume_cpu_ms(3000, 0, false);

	auto transport = ZmqTransport("tcp://127.0.0.1:5555", true);
	transport.initialize();

	for (size_t counter = 1; counter < 11; ++counter) {

        ReadoutMessageAuto message(22, counter);
        message.payload_size = 4;
        message.payload[0] = randfloat(0,1);
        message.payload[1] = randfloat(-100,100);
        message.payload[2] = randfloat(-100,100);
        message.payload[3] = randfloat(-100,100);

        transport.send(message);
        std::cout << "Send: " << message << "(" << message.get_buffer_size() << " bytes)" << std::endl;
        consume_cpu_ms(1000, 0, false);
    }

    // Send end-of-stream message so that JANA knows to shut down
	transport.send(ReadoutMessageAuto::end_of_stream());
}


extern "C"{
void InitPlugin(JApplication *app) {

	InitJANAPlugin(app);

    using Msg = ReadoutMessageAuto;

    auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport("tcp://127.0.0.1:5555"));

    auto window = std::unique_ptr<JSessionWindow<Msg>>(new JSessionWindow<Msg>(10, {0,1,2}));

    app->Add(new JEventBuilder<Msg>(std::move(transport), std::move(window)));

	app->Add(new AHitAnomalyDetector(app, 5000));
	app->Add(new JFactoryGeneratorT<AHitParser>());

	// So we don't have to put this on the cmd line every time
	app->SetParameterValue("jana:legacy_mode", 0);
	app->SetParameterValue("jana:extended_report", 0);

	auto publisher = new std::thread(dummy_publisher_loop);
}
} // "C"


