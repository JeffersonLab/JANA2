
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_DETECTORAHITFACTORY_H
#define JANA2_DETECTORAHITFACTORY_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>

#include <chrono>

#include "AHit.h"
#include "ReadoutMessageAuto.h"



class AHitParser : public JFactoryT<AHit> {


public:
    AHitParser() : JFactoryT<AHit>("AHitParser") {
    }

    void Process(const std::shared_ptr<const JEvent>& event) {

        auto readoutMessages = event->Get<ReadoutMessageAuto>();
        for (const auto& readoutMessage : readoutMessages) {
            auto ahit = new AHit();

            ahit->E = reinterpret_cast<const float&>(readoutMessage->payload[0]);
            ahit->x = reinterpret_cast<const float&>(readoutMessage->payload[1]);
            ahit->y = reinterpret_cast<const float&>(readoutMessage->payload[2]);
            ahit->z = reinterpret_cast<const float&>(readoutMessage->payload[3]);
            Insert(ahit);
            std::cout << "Parsing: " << *readoutMessage << std::endl;
        }
    }
};

#endif //JANA2_DETECTORAHITFACTORY_H
