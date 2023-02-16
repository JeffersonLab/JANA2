
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#include <iostream>
#include <podio/CollectionBase.h>
#include <podio/Frame.h>
#include "datamodel/MutableExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include <podio/ROOTFrameWriter.h>
#include <podio/ROOTFrameReader.h>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Podio/JEventProcessorPodio.h>

#include "PodioExampleSource.h"
#include "PodioExampleProcessor.h"
#include "ExampleClusterFactory.h"


void create_hits_file() {

    MutableExampleHit hit1;
    hit1.cellID(22);
    hit1.energy(100.0);
    hit1.x(0);
    hit1.y(0);
    hit1.z(0);

    MutableExampleHit hit2;
    hit2.cellID(49);
    hit2.energy(97.1);
    hit2.x(1);
    hit2.y(2);
    hit2.z(3);

    ExampleHitCollection hits1;
    hits1.push_back(hit1);
    hits1.push_back(hit2);

    podio::Frame event1;
    event1.put(std::move(hits1), "hits");

    podio::ROOTFrameWriter writer("hits.root");
    writer.writeFrame(event1, "events");

    MutableExampleHit hit3;
    hit3.cellID(42);
    hit3.energy(7.6);
    hit3.x(5);
    hit3.y(-5);
    hit3.z(5);

    MutableExampleHit hit4;
    hit4.cellID(618);
    hit4.energy(99.9);
    hit4.x(-3);
    hit4.y(-5);
    hit4.z(1);

    MutableExampleHit hit5;
    hit5.cellID(27);
    hit5.energy(22.22);
    hit5.x(-10);
    hit5.y(10);
    hit5.z(10);

    ExampleHitCollection hits2;
    hits2.push_back(hit3);
    hits2.push_back(hit4);
    hits2.push_back(hit5);

    podio::Frame event2;
    event2.put(std::move(hits2), "hits");

    writer.writeFrame(event2, "events");
    writer.finish();

}

void verify_clusters_file() {
    podio::ROOTFrameReader reader;
    reader.openFile("podio_output.root");
    auto event0 = podio::Frame(reader.readEntry("events", 0));

    std::cout << "Event 0: Expected 2 clusters, got " << event0.get("clusters")->size() << std::endl;

    auto event1 = podio::Frame(reader.readEntry("events", 1));
    std::cout << "Event 1: Expected 3 clusters, got " << event1.get("clusters")->size() << std::endl;
}


int main() {

    create_hits_file();

    JApplication app;
    app.Add(new PodioExampleProcessor);
    app.Add(new JEventProcessorPodio);
    app.Add(new JFactoryGeneratorT<ExampleClusterFactory>());
    app.Add(new PodioExampleSource("hits.root"));
    app.Run();

    verify_clusters_file();

}

