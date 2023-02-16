
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

    EventInfo eventinfo1(7, 22);
    EventInfoCollection eventinfos1;
    eventinfos1.push_back(eventinfo1);

    MutableExampleHit hit1(22, 100, 0, 0, 0);
    MutableExampleHit hit2(49, 97.1, 1, 2, 3);

    ExampleHitCollection hits1;
    hits1.push_back(hit1);
    hits1.push_back(hit2);

    podio::Frame event1;
    event1.put(std::move(hits1), "hits");
    event1.put(std::move(eventinfos1), "eventinfos");

    podio::ROOTFrameWriter writer("hits.root");
    writer.writeFrame(event1, "events");

    EventInfo eventinfo2(8, 22);
    EventInfoCollection eventinfos2;
    eventinfos2.push_back(eventinfo2);

    MutableExampleHit hit3(42, 7.6, 5, -5, 5);
    MutableExampleHit hit4(618, 99.9, -3, -5, 1);
    MutableExampleHit hit5(27, 22.22, -10, 10, 10);

    ExampleHitCollection hits2;
    hits2.push_back(hit3);
    hits2.push_back(hit4);
    hits2.push_back(hit5);

    podio::Frame event2;
    event2.put(std::move(hits2), "hits");
    event2.put(std::move(eventinfos2), "eventinfos");

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

