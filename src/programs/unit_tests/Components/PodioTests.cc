
#include <catch.hpp>

#include <memory>
#include <type_traits>
#include <JANA/JEvent.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JMultifactory.h>
#include <JANA/Components/JOmniFactory.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include <PodioDatamodel/ExampleHitCollection.h>
#include <PodioDatamodel/ExampleClusterCollection.h>
#include <podio/podioVersion.h>

#if podio_VERSION >= PODIO_VERSION(1,2,0)
#include <podio/LinkCollection.h>
#endif

namespace podiotests {


TEST_CASE("PodioTestsInsertAndRetrieve") {
    ExampleClusterCollection clusters_to_insert;
    clusters_to_insert.push_back(MutableExampleCluster(16.0));
    clusters_to_insert.push_back(MutableExampleCluster(128.0));

    auto event = std::make_shared<JEvent>();
    event->InsertCollection<ExampleCluster>(std::move(clusters_to_insert), "clusters");

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

    SECTION("Retrieve directly from podio::Frame") {
        auto frame = event->GetSingle<podio::Frame>();
        auto* collection_retrieved = dynamic_cast<const ExampleClusterCollection*>(frame->get("clusters"));
        REQUIRE(collection_retrieved->size() == 2);
        REQUIRE((*collection_retrieved)[0].energy() == 16.0);
    }

}

template <typename, typename=void>
struct is_podio : std::false_type {};

template <typename T>
struct is_podio<T, std::void_t<typename T::collection_type>> : std::true_type {};

template <typename T>
static constexpr bool is_podio_v = is_podio<T>::value;

struct FakeMultifactory {

    template <typename T, typename std::enable_if_t<is_podio_v<T>>* = nullptr>
    bool DeclareOutput(std::string /*tag*/) {
        return true;
    }

    template <typename T, typename std::enable_if_t<!is_podio_v<T>>* = nullptr>
    bool DeclareOutput(std::string /*tag*/) {
        return false;
    }
};

TEST_CASE("SFINAE for JMultifactory::SetData") {
    FakeMultifactory sut;
    REQUIRE(sut.DeclareOutput<int>("asdf") == false);
    REQUIRE(sut.DeclareOutput<ExampleCluster>("asdf") == true);
}


TEST_CASE("PODIO 'subset' collections handled correctly (not involving factories yet") {
    auto event = std::make_shared<JEvent>();

    auto a1 = MutableExampleCluster(22.2);
    auto a2 = MutableExampleCluster(4.0);
    ExampleClusterCollection clusters_to_insert;
    clusters_to_insert.push_back(a1);
    clusters_to_insert.push_back(a2);
    event->InsertCollection<ExampleCluster>(std::move(clusters_to_insert), "original_clusters");

    auto* retrieved_clusters = event->GetCollection<ExampleCluster>("original_clusters");
    auto b = (*retrieved_clusters)[1];
    REQUIRE(b.energy() == 4.0);

    ExampleClusterCollection subset_clusters;
    subset_clusters.setSubsetCollection(true);
    subset_clusters.push_back(b);
    event->InsertCollection<ExampleCluster>(std::move(subset_clusters), "subset_clusters");


    // Retrieve via event->GetCollection
    const ExampleClusterCollection* retrieved_subset_clusters = event->GetCollection<ExampleCluster>("subset_clusters");
    const ExampleCluster& c = (*retrieved_subset_clusters)[0];
    REQUIRE(c.energy() == 4.0);
    REQUIRE(c.id() == b.id());

}
} // namespace podiotests

namespace jana2_tests_podiotests_init {

struct TestFac : public JFactoryPodioT<ExampleCluster> {
    TestFac() {
        SetTag("clusters");
    }
    bool init_called = false;
    void Init() override {
        init_called = true;
    }
    void Process(const std::shared_ptr<const JEvent>&) override {
        ExampleClusterCollection c;
        c.push_back(MutableExampleCluster(16.0));
        SetCollection(std::move(c));
    }
};

TEST_CASE("JFactoryPodioT::Init gets called") {

    JApplication app;
    app.Add(new JFactoryGeneratorT<jana2_tests_podiotests_init::TestFac>());
    auto event = std::make_shared<JEvent>(&app);
    event->Clear();  // Simulate a trip to the JEventPool

    auto untyped_coll = event->GetCollectionBase("clusters");
    REQUIRE(untyped_coll != nullptr);
    const auto* typed_coll = dynamic_cast<const ExampleClusterCollection*>(untyped_coll);
    REQUIRE(typed_coll != nullptr);
    REQUIRE(typed_coll->size() == 1);
    REQUIRE(typed_coll->at(0).energy() == 16.0);
    auto fac = dynamic_cast<jana2_tests_podiotests_init::TestFac*>(event->GetFactory("ExampleCluster", "clusters"));
    REQUIRE(fac != nullptr);
    REQUIRE(fac->init_called == true);
}
} // namespace jana2_tests_podiotests_init

#if podio_VERSION >= PODIO_VERSION(1,2,0)

using ClusterClusterLink = podio::LinkCollection<ExampleCluster, ExampleCluster>::value_type;
struct MyOmniFac: jana::components::JOmniFactory<MyOmniFac> {

    PodioInput<ExampleCluster> m_clusters_in {this};
    PodioInput<ClusterClusterLink> m_links_in {this};  // Just to test that the Input machinery is OK
    PodioOutput<ClusterClusterLink> m_links_out {this};

    void Configure() {};

    void ChangeRun(int32_t) {}

    void Execute(int32_t, int32_t) {
        auto link = m_links_out()->create();
        link.setFrom(m_clusters_in()->at(1));
        link.setTo(m_clusters_in()->at(0));
    }
};

TEST_CASE("PodioLink_Test") {
    JApplication app;
    app.Add(new JOmniFactoryGeneratorT<MyOmniFac>("blah", {"clusters", "simple"}, {"complex"}));
    auto event = std::make_shared<JEvent>(&app);
    ExampleClusterCollection clusters;
    auto c1 = clusters.create();
    c1.energy(22);
    auto c2 = clusters.create();
    c2.energy(33);

    event->InsertCollection<ExampleCluster>(std::move(clusters), "clusters");
    event->InsertCollection<ClusterClusterLink>(ClusterClusterLink::collection_type(), "simple");

    auto coll = event->GetCollection<ClusterClusterLink>("complex");
    REQUIRE(coll->size() == 1);
    REQUIRE(coll->at(0).getFrom().energy() == 33);
    REQUIRE(coll->at(0).getTo().energy() == 22);
}
#endif


namespace jana2::tests::podio_cleardata {

struct MyFac: jana::components::JOmniFactory<MyFac> {

    PodioInput<ExampleHit> m_hits_in {this};
    PodioOutput<ExampleCluster> m_clusters_out {this};

    MyFac() { SetTypeName("MyFac");}
    void Configure() {};
    void ChangeRun(int32_t) {}
    void Execute(int32_t, int32_t) {
        auto cluster = m_clusters_out()->create();
        for (auto hit : *m_hits_in()) {
            cluster.addHits(hit);
        }
    }
};

TEST_CASE("PodioClearData_Test") {
    JApplication app;
    app.Add(new JOmniFactoryGeneratorT<MyFac>("blah", {"hits"}, {"clusters"}));
    auto event = std::make_shared<JEvent>(&app);

    SECTION("InsertCollection") {
        ExampleHitCollection hits;
        auto h1 = hits.create();
        h1.cellID(22);

        event->InsertCollection<ExampleHit>(std::move(hits), "hits");
        auto clusters = event->GetCollection<ExampleCluster>("clusters");
        REQUIRE(clusters->size() == 1);
        REQUIRE(clusters->at(0).Hits_size() == 1);
        REQUIRE(clusters->at(0).Hits().at(0).cellID() == 22);

        event->Clear(true);

        ExampleHitCollection hits2;
        auto h2 = hits2.create();
        h2.cellID(3);
        auto h3 = hits2.create();
        h3.cellID(99);
        event->InsertCollection<ExampleHit>(std::move(hits2), "hits");
        auto clusters2 = event->GetCollection<ExampleCluster>("clusters");
        REQUIRE(clusters2->size() == 1);
        REQUIRE(clusters2->at(0).Hits_size() == 2);
        REQUIRE(clusters2->at(0).Hits().at(0).cellID() == 3);
        REQUIRE(clusters2->at(0).Hits().at(1).cellID() == 99);
    }

    SECTION("InsertCollectionAlreadyInFrame") {
        ExampleHitCollection hits;
        auto h1 = hits.create();
        h1.cellID(22);
        podio::Frame* frame1 = new podio::Frame;
        frame1->put(std::move(hits), "hits");
        auto const_hits = frame1->get("hits");
        event->Insert(frame1, "");
        event->InsertCollectionAlreadyInFrame<ExampleHit>(const_hits, "hits");

        auto clusters = event->GetCollection<ExampleCluster>("clusters");

        REQUIRE(clusters->size() == 1);
        REQUIRE(clusters->at(0).Hits_size() == 1);
        REQUIRE(clusters->at(0).Hits().at(0).cellID() == 22);

        event->Clear(true);

        ExampleHitCollection hits2;
        auto h2 = hits2.create();
        h2.cellID(3);
        auto h3 = hits2.create();
        h3.cellID(99);

        podio::Frame* frame2 = new podio::Frame;
        frame2->put(std::move(hits2), "hits");
        auto const_hits2 = frame2->get("hits");
        event->Insert(frame2, "");
        event->InsertCollectionAlreadyInFrame<ExampleHit>(const_hits2, "hits");

        auto clusters2 = event->GetCollection<ExampleCluster>("clusters");

        REQUIRE(clusters2->size() == 1);
        REQUIRE(clusters2->at(0).Hits_size() == 2);
        REQUIRE(clusters2->at(0).Hits().at(0).cellID() == 3);
        REQUIRE(clusters2->at(0).Hits().at(1).cellID() == 99);
    }



} // TEST_CASE
} // namespace jana2::tests::podio_cleardata

namespace jana2::tests::podio_multifactory {

struct MyMultiFac: JMultifactory {


    MyMultiFac() { 
        SetTypeName("MyMultiFac");
        DeclarePodioOutput<ExampleCluster>("clusters");
        DeclarePodioOutput<ExampleCluster>("other_clusters");
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto hits = event->GetCollection<ExampleHit>("hits");
        ExampleClusterCollection clusters;
        auto cluster = clusters->create();
        for (auto hit : *hits) {
            cluster.addHits(hit);
        }
        SetCollection<ExampleCluster>("clusters", std::move(clusters));

        ExampleClusterCollection clusters2;
        auto cluster2 = clusters2->create();
        for (auto hit : *hits) {
            cluster2.addHits(hit);
        }
        SetCollection<ExampleCluster>("other_clusters", std::move(clusters2));
    }
};

TEST_CASE("PodioMultifactoryClearData_Test") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<MyMultiFac>());
    auto event = std::make_shared<JEvent>(&app);

    SECTION("InsertCollection") {
        ExampleHitCollection hits;
        auto h1 = hits.create();
        h1.cellID(22);

        event->InsertCollection<ExampleHit>(std::move(hits), "hits");
        auto clusters = event->GetCollection<ExampleCluster>("clusters");
        REQUIRE(clusters->size() == 1);
        REQUIRE(clusters->at(0).Hits_size() == 1);
        REQUIRE(clusters->at(0).Hits().at(0).cellID() == 22);

        //auto other_clusters = event->GetCollection<ExampleCluster>("other_clusters");
        //REQUIRE(other_clusters->size() == 1);
        //REQUIRE(0 == 1);
        auto frame = event->GetSingle<podio::Frame>();
        const auto& other_clusters = frame->get<ExampleClusterCollection>("other_clusters");
        REQUIRE(other_clusters.size() == 1);


        event->Clear(true);

        ExampleHitCollection hits2;
        auto h2 = hits2.create();
        h2.cellID(3);
        auto h3 = hits2.create();
        h3.cellID(99);
        event->InsertCollection<ExampleHit>(std::move(hits2), "hits");
        auto clusters2 = event->GetCollection<ExampleCluster>("clusters");
        REQUIRE(clusters2->size() == 1);
        REQUIRE(clusters2->at(0).Hits_size() == 2);
        REQUIRE(clusters2->at(0).Hits().at(0).cellID() == 3);
        REQUIRE(clusters2->at(0).Hits().at(1).cellID() == 99);
    }
    
    SECTION("InsertCollectionAlreadyInFrame") {
        ExampleHitCollection hits;
        auto h1 = hits.create();
        h1.cellID(22);
        podio::Frame* frame1 = new podio::Frame;
        frame1->put(std::move(hits), "hits");
        auto const_hits = frame1->get("hits");
        event->InsertCollectionAlreadyInFrame<ExampleHit>(const_hits, "hits");
        event->Insert(frame1, "");

        auto clusters = event->GetCollection<ExampleCluster>("clusters");
        auto clusters3 = event->GetCollection<ExampleCluster>("clusters");
        // Ensure that Process() isn't called twice
        REQUIRE(clusters3->size() == 1);

        REQUIRE(clusters->size() == 1);
        REQUIRE(clusters->at(0).Hits_size() == 1);
        REQUIRE(clusters->at(0).Hits().at(0).cellID() == 22);

        event->Clear(true);

        ExampleHitCollection hits2;
        auto h2 = hits2.create();
        h2.cellID(3);
        auto h3 = hits2.create();
        h3.cellID(99);

        podio::Frame* frame2 = new podio::Frame;
        frame2->put(std::move(hits2), "hits");
        auto const_hits2 = frame2->get("hits");
        event->Insert(frame2, "");
        event->InsertCollectionAlreadyInFrame<ExampleHit>(const_hits2, "hits");

        auto clusters2 = event->GetCollection<ExampleCluster>("clusters");

        REQUIRE(clusters2->size() == 1);
        REQUIRE(clusters2->at(0).Hits_size() == 2);
        REQUIRE(clusters2->at(0).Hits().at(0).cellID() == 3);
        REQUIRE(clusters2->at(0).Hits().at(1).cellID() == 99);
    }

} // TEST_CASE
} // namespace


