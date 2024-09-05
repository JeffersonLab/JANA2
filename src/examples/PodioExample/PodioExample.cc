
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#include <podio/CollectionBase.h>
#include <podio/Frame.h>

#include "PodioDatamodel/EventInfoCollection.h"
#include "PodioDatamodel/ExampleHitCollection.h"
#include <podio/ROOTFrameWriter.h>
#include <podio/ROOTFrameReader.h>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "PodioExampleProcessor.h"
#include "ExampleClusterFactory.h"
#include "ExampleMultifactory.h"

#include <iostream>
#include <cassert>


void create_hits_file() {

    MutableEventInfo eventinfo1(7, 0, 22);
    EventInfoCollection eventinfos1;
    eventinfos1.push_back(eventinfo1);

    ExampleHitCollection hits1;
    hits1.push_back(MutableExampleHit(22, -1, -1, 0, 100, 0));
    hits1.push_back(MutableExampleHit(49, 1, 1, 0, 15.5, 0));
    hits1.push_back(MutableExampleHit(47, 1, 2, 0, 0.5, 0));
    hits1.push_back(MutableExampleHit(42, 2, 1, 0, 4.0, 0));

    podio::Frame event1;
    event1.put(std::move(hits1), "hits");
    event1.put(std::move(eventinfos1), "eventinfos");

    podio::ROOTFrameWriter writer("hits.root");
    writer.writeFrame(event1, "events");

    MutableEventInfo eventinfo2(8, 0, 22);
    EventInfoCollection eventinfos2;
    eventinfos2.push_back(eventinfo2);

    ExampleHitCollection hits2;
    hits2.push_back(MutableExampleHit(42, 5, -5, 5, 7.6, 0));
    hits2.push_back(MutableExampleHit(618, -3, -5, 1, 99.9, 0));
    hits2.push_back(MutableExampleHit(27, -10, 10, 10, 22.2, 0));
    hits2.push_back(MutableExampleHit(28, -9, 11, 10, 7.8, 0));

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
    assert(event0.get("clusters")->size() == 2);

    auto event1 = podio::Frame(reader.readEntry("events", 1));
    std::cout << "Event 1: Expected 3 clusters, got " << event1.get("clusters")->size() << std::endl;
    assert(event1.get("clusters")->size() == 3);
}


int main() {

    create_hits_file();

    JApplication app;
    app.Add(new PodioExampleProcessor);
    app.Add(new JFactoryGeneratorT<ExampleClusterFactory>());
    app.Add(new JFactoryGeneratorT<ExampleMultifactory>());
    app.Add("hits.root");
    app.AddPlugin("PodioFileReader");
    app.AddPlugin("PodioFileWriter");
    app.SetParameterValue("podio:output_file", "podio_output.root");
    app.SetParameterValue("podio:output_collections", "hits_filtered,clusters_from_hits_filtered,clusters_filtered");
    app.Run();

    verify_clusters_file();

    return app.GetExitCode();
}

