
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

    ExampleHitCollection hits1;
    hits1.push_back({22, -1, -1, 0, 100});
    hits1.push_back({49, 1, 1, 0, 15.5});
    hits1.push_back({47, 1, 2, 0, 0.5});
    hits1.push_back({42, 2, 1, 0, 4.0});

    podio::Frame event1;
    event1.put(std::move(hits1), "hits");
    event1.put(std::move(eventinfos1), "eventinfos");

    podio::ROOTFrameWriter writer("hits.root");
    writer.writeFrame(event1, "events");

    EventInfo eventinfo2(8, 22);
    EventInfoCollection eventinfos2;
    eventinfos2.push_back(eventinfo2);

    ExampleHitCollection hits2;
    hits2.push_back({42, 5, -5, 5, 7.6});
    hits2.push_back({618, -3, -5, 1, 99.9});
    hits2.push_back({27, -10, 10, 10, 22.2});
    hits2.push_back({28, -9, 11, 10, 7.8});

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

