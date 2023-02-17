
#include <catch.hpp>

#include <datamodel/ExampleClusterCollection.h>
#include <DatamodelGlue.h>  // Hopefully this won't be necessary in the future
#include <JANA/JEvent.h>

namespace podiotests {

TEST_CASE("PodioTestsInsertAndRetrieve") {
    auto* clusters_to_insert = new ExampleClusterCollection;
    clusters_to_insert->push_back({16.0});
    clusters_to_insert->push_back({128.0});

    auto event = std::make_shared<JEvent>();
    event->InsertCollection<ExampleCluster>(clusters_to_insert, "clusters");

    SECTION("Retrieve using JEvent::GetCollection()") {
        auto* collection_retrieved = event->GetCollection<ExampleCluster>("clusters");
        REQUIRE(collection_retrieved->size() == 2);
        REQUIRE((*collection_retrieved)[0].energy() == 16.0);
    }

    SECTION("Retrieve using JEvent::GetCollectionBase()") {
        auto* collection_retrieved_untyped = event->GetCollectionBase("clusters");
        REQUIRE(collection_retrieved_untyped->size() == 2);
        auto* collection_retrieved = dynamic_cast<const ExampleClusterCollection*>(collection_retrieved_untyped);
        REQUIRE(collection_retrieved != nullptr);
        REQUIRE((*collection_retrieved)[0].energy() == 16.0);
    }

    SECTION("Retrieve using JEvent::Get()") {
        std::vector<const ExampleCluster*> clusters_retrieved = event->Get<ExampleCluster>("clusters");
        REQUIRE(clusters_retrieved.size() == 2);
        REQUIRE(clusters_retrieved[0]->energy() == 16.0);
    }

}



} // namespace podiotests
