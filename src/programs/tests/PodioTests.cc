
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

TEST_CASE("PodioTestsShallowCopySemantics") {

    // This one is interesting. The PODIO objects we interact with (e.g. ExampleCluster), are transient lightweight
    // wrappers. What's tricky is maintaining a vector of pointers to these things. We can't simply capture a pointer
    // to the references returned by the collection iterator because those will all self-destruct. What we can do is
    // call the copy constructor. However, we need to make sure that this ctor is doing a shallow copy of the wrapper
    // as opposed to a deep copy of the underlying data.

    SECTION("Shallow copy of object wrapper") {
        auto a = ExampleCluster(22.2);
        auto b = ExampleCluster(a);
        REQUIRE(a.id() == b.id());
        REQUIRE(&(a.energy()) == &(b.energy())); // energy() getter returns a const& into the actual data object
    }

    SECTION("Transience of collections") {
        auto a = MutableExampleCluster(22.2);
        ExampleClusterCollection input_coll;
        input_coll.push_back(a);
        podio::Frame f;
        f.put(std::move(input_coll), "clusters");

        const ExampleClusterCollection* output_coll1 = &f.get<ExampleClusterCollection>("clusters");
        const ExampleClusterCollection* output_coll2 = &f.get<ExampleClusterCollection>("clusters");

        REQUIRE(output_coll1 == output_coll2);
    }

    SECTION("Shallow copy of object upon inserting a collection into a frame") {

        // NOTE: Inserting an object into a collection is NOT necessarily performing a shallow copy.
        //       At any rate, id and ptr are not preserved.
        auto a = MutableExampleCluster(22.2);
        // auto a_ptr = &(a.energy());
        // auto a_id= a.id();

        ExampleClusterCollection input_coll;
        input_coll.push_back(a);
        ExampleCluster b = input_coll[0];
        // auto b_id = b.id();
        auto b_ptr = &(b.energy());

        podio::Frame f;
        f.put(std::move(input_coll), "clusters");
        auto& c = f.get<ExampleClusterCollection>("clusters");
        // auto c_id = c[0].id();
        auto c_ptr = &(c[0].energy());

        REQUIRE(b.energy() == 22.2);
        REQUIRE(c[0].energy() == 22.2);
        REQUIRE(b_ptr == c_ptr);
        // REQUIRE(b_id == c_id); // Inserting a Collection into to a Frame changes the id. Is this a problem?
    }

    SECTION("In practice") {

        auto event = std::make_shared<JEvent>();
        auto a = MutableExampleCluster(22.2);
        // auto a_id = a.id();
        // auto a_ptr = &(a.energy());

        auto* clusters_to_insert = new ExampleClusterCollection;
        clusters_to_insert->push_back(a);
        event->InsertCollection<ExampleCluster>(clusters_to_insert, "clusters");

        // Retrieve via event->GetCollection
        const ExampleClusterCollection* retrieved_via_collection = event->GetCollection<ExampleCluster>("clusters");
        const ExampleCluster& b = (*retrieved_via_collection)[0];
        auto b_id = b.id();
        auto b_ptr = &(b.energy());
        REQUIRE(b.energy() == 22.2);

        // Retrieve via event->Get<T>
        auto retrieved_via_ptr = event->Get<ExampleCluster>("clusters");
        const ExampleCluster* c = retrieved_via_ptr[0];
        auto c_id = c->id();
        auto c_ptr = &(c->energy());

        REQUIRE(c->energy() == 22.2);
        REQUIRE(b_id == c_id);
        REQUIRE(b_ptr == c_ptr);
    }


    SECTION("Make sure we aren't accidentally doing a deep copy when we make an association") {

        podio::Frame frame;
        MutableExampleHit hit1(1, 11,12,13, 7.7);
        ExampleHitCollection hits;
        hits.push_back(hit1);
        frame.put(std::move(hits), "hits");

        MutableExampleCluster cluster1(0.0);
        cluster1.addHits(hit1);
        ExampleClusterCollection clusters;
        clusters.push_back(cluster1);
        frame.put(std::move(clusters), "clusters");

        const auto& hits_out = frame.get<ExampleHitCollection>("hits");
        const auto& clusters_out = frame.get<ExampleClusterCollection>("clusters");

        REQUIRE(&(clusters_out[0].Hits()[0].energy()) == &(hits_out[0].energy()));
        REQUIRE(clusters_out[0].Hits()[0].id() == hits_out[0].id());
    }

    SECTION("Make sure that references established _before_ inserting into frame are preserved") {
        // TODO: This needs multifactories before we can test it end-to-end, but we can certainly test
        //       just the PODIO parts for now

        podio::Frame frame;
        MutableExampleHit hit1(1, 11,12,13, 7.7);
        MutableExampleHit hit2(1, 10,10,10, 9.2);

        // First make the associations
        MutableExampleCluster cluster1(0.0);
        cluster1.addHits(hit1);
        cluster1.addHits(hit2);

        MutableExampleCluster cluster2(9.0);
        cluster2.addHits(hit1);

        // THEN insert into the Collection (This simulates a JANA user calling Set<T>() instead of SetCollection())
        // Note that when inserting into a collection, PODIO rewrites the object ids.
        ExampleHitCollection hits;
        hits.push_back(hit1);
        hits.push_back(hit2);

        ExampleClusterCollection clusters;
        clusters.push_back(cluster1);
        clusters.push_back(cluster2);

        // Insert collections into the frame last. This will rewrite both sets of ids a second time (!), but stress-tests PODIO the most
        frame.put(std::move(hits), "hits");
        frame.put(std::move(clusters), "clusters");

        const auto& hits_out = frame.get<ExampleHitCollection>("hits");
        const auto& clusters_out = frame.get<ExampleClusterCollection>("clusters");

        REQUIRE(&(clusters_out[0].Hits()[0].energy()) == &(hits_out[0].energy()));
        REQUIRE(&(clusters_out[0].Hits()[1].energy()) == &(hits_out[1].energy()));
        REQUIRE(&(clusters_out[1].Hits()[0].energy()) == &(hits_out[0].energy()));

        REQUIRE(clusters_out[0].Hits()[0].id() == hits_out[0].id());
        REQUIRE(clusters_out[0].Hits()[1].id() == hits_out[1].id());
        REQUIRE(clusters_out[1].Hits()[0].id() == hits_out[0].id());
    }
}


} // namespace podiotests
