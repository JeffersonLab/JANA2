
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#include <iostream>
#include <podio/CollectionBase.h>
#include <podio/Frame.h>
#include "datamodel/MutableExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include <podio/ROOTFrameWriter.h>
#include <podio/ROOTFrameReader.h>
#include "DatamodelGlue.h"

int main() {
    std::cout << "Hello world" << std::endl;
    auto coll = new ExampleHitCollection;
    MutableExampleHit hit;
    hit.cellID(22);
    hit.energy(100.0);
    hit.x(0);
    hit.y(0);
    hit.z(0);
    std::cout << hit << std::endl;
    coll->push_back(hit);
    podio::Frame frame;
    frame.put(std::move(*coll), "SillyCollection");

    podio::ROOTFrameWriter writer("podio_output.root");
    writer.writeFrame(frame, "events");
    writer.finish();

    podio::ROOTFrameReader reader;
    reader.openFile("podio_output.root");
    auto framedata = reader.readEntry("events", 0);
    auto frame2 = podio::Frame(std::move(framedata));

    auto& readcol = frame2.get<ExampleHitCollection>("SillyCollection");
    const auto& readhit = readcol[0];
    std::cout << readhit << std::endl;

}

