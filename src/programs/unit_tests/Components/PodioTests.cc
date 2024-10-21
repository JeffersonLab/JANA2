
#include <catch.hpp>

#include <type_traits>

#include <JANA/JEvent.h>
#include <JANA/JMultifactory.h>
#include <JANA/Podio/JFactoryPodioT.h>
#include <JANA/Components/JOmniFactory.h>
#include <JANA/Components/JDataBundle.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include <JANA/Services/JComponentManager.h>

#include <PodioDatamodel/ExampleClusterCollection.h>
#include <PodioDatamodel/ExampleHitCollection.h>
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

template <typename T, typename = void>
struct MyWrapper {
    bool have_podio() {
        return false;
    }
};

template <typename T>
struct MyWrapper<T, std::void_t<typename T::collection_type>> {
    int x = 2;
    bool have_podio() {
        return true;
    }
};

TEST_CASE("SFINAE for JFactoryT || JFactoryPodioT") {

    MyWrapper<int> w;
    REQUIRE(w.have_podio() == false);

    MyWrapper<ExampleCluster> ww;
    REQUIRE(ww.have_podio() == true);

    ww.x = 22;

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
}

TEST_CASE("JFactoryPodioT::Init gets called") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);
    auto fs = new JFactorySet;
    fs->Add(new jana2_tests_podiotests_init::TestFac);
    event->SetFactorySet(fs);
    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool

    auto r = event->GetCollectionBase("clusters");
    REQUIRE(r != nullptr);
    const auto* res = dynamic_cast<const ExampleClusterCollection*>(r);
    REQUIRE(res != nullptr);
    REQUIRE((*res)[0].energy() == 16.0);

    auto fac_untyped = event->GetDataBundle("clusters", false)->GetFactory();
    REQUIRE(fac_untyped != nullptr);
    auto fac = dynamic_cast<jana2_tests_podiotests_init::TestFac*>(fac_untyped);
    REQUIRE(fac != nullptr);
    REQUIRE(fac->init_called == true);
}


namespace multifactory {

struct TestMultiFac : public JMultifactory {
    TestMultiFac() {
        DeclarePodioOutput<ExampleCluster>("sillyclusters");
    }
    bool init_called = false;
    void Init() override {
        init_called = true;
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        ExampleClusterCollection c;
        c.push_back(MutableExampleCluster(16.0 + event->GetEventNumber()));
        SetCollection<ExampleCluster>("sillyclusters", std::move(c));
    }
};

TEST_CASE("PodioTests_JMultifactoryInit") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);
    event->SetEventNumber(0);
    auto fs = new JFactorySet;
    fs->Add(new TestMultiFac);
    event->SetFactorySet(fs);

    // Simulate a trip to the event pool _before_ calling JFactory::Process()
    // This is important because a badly designed JFactory::ClearData() could
    // mangle the Unprocessed status and consequently skip Init().
    event->GetFactorySet()->Release(); // In theory this shouldn't hurt

    auto r = event->GetCollectionBase("sillyclusters");
    REQUIRE(r != nullptr);
    const auto* res = dynamic_cast<const ExampleClusterCollection*>(r);
    REQUIRE(res != nullptr);
    REQUIRE((*res)[0].energy() == 16.0);

    auto multifac = event->GetFactorySet()->GetAllMultifactories().at(0);
    REQUIRE(multifac != nullptr);
    auto multifac_typed = dynamic_cast<TestMultiFac*>(multifac);
    REQUIRE(multifac_typed != nullptr);
    REQUIRE(multifac_typed->init_called == true);
}


TEST_CASE("PodioTests_MultifacMultiple") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);
    event->SetEventNumber(0);
    auto fs = new JFactorySet;
    fs->Add(new TestMultiFac);
    event->SetFactorySet(fs);

    auto r = event->GetCollection<ExampleCluster>("sillyclusters");
    REQUIRE(r->at(0).energy() == 16.0);

    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool
    
    event->SetEventNumber(4);
    r = event->GetCollection<ExampleCluster>("sillyclusters");
    REQUIRE(r->at(0).energy() == 20.0);

}
} // namespace multifactory


TEST_CASE("PodioTests_InsertMultiple") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);

    // Insert a cluster

    auto coll1 = ExampleClusterCollection();
    auto cluster1 = coll1.create(22.0);
    auto storage = event->InsertCollection<ExampleCluster>(std::move(coll1), "clusters");

    REQUIRE(storage->GetSize() == 1);
    REQUIRE(storage->GetStatus() == JDataBundle::Status::Inserted);

    // Retrieve and validate cluster

    auto cluster1_retrieved = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(cluster1_retrieved->at(0).energy() == 22.0);

    // Clear event

    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool
    
    // After clearing, the JDataBundle will still exist, but it will be empty
    auto storage2 = event->GetDataBundle("clusters", false);
    REQUIRE(storage2->GetStatus() == JDataBundle::Status::Empty);
    REQUIRE(storage2->GetSize() == 0);

    // Insert a cluster. If event isn't being cleared correctly, this will throw

    auto coll2 = ExampleClusterCollection();
    auto cluster2 = coll2.create(33.0);
    auto storage3 = event->InsertCollection<ExampleCluster>(std::move(coll2), "clusters");
    REQUIRE(storage3->GetStatus() == JDataBundle::Status::Inserted);
    REQUIRE(storage3->GetSize() == 1);

    // Retrieve and validate cluster

    auto cluster2_retrieved = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(cluster2_retrieved->at(0).energy() == 33.0);
}


namespace omnifacmultiple {

struct MyClusterFactory : public JOmniFactory<MyClusterFactory> {

    PodioOutput<ExampleCluster> m_clusters_out{this, "clusters"};

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t evt_nr) {

        auto cs = std::make_unique<ExampleClusterCollection>();
        auto cluster = MutableExampleCluster(101.0 + evt_nr);
        cs->push_back(cluster);
        m_clusters_out() = std::move(cs);
    }
};

TEST_CASE("PodioTests_OmniFacMultiple") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "error");
    app.Add(new JOmniFactoryGeneratorT<MyClusterFactory>("cluster_fac", {}, {"clusters"}));
    app.Initialize();
    auto event = std::make_shared<JEvent>(&app);
    app.GetService<JComponentManager>()->configure_event(*event);
    event->SetEventNumber(22);

    // Check that storage is already present
    auto storage = event->GetDataBundle("clusters", false);
    REQUIRE(storage != nullptr);
    REQUIRE(storage->GetStatus() == JDataBundle::Status::Empty);

    // Retrieve triggers factory
    auto coll = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(coll->size() == 1);
    REQUIRE(coll->at(0).energy() == 123.0);

    // Clear everything
    event->GetFactorySet()->Release();
    event->SetEventNumber(1010);

    // Check that storage has been reset
    storage = event->GetDataBundle("clusters", false);
    REQUIRE(storage != nullptr);
    REQUIRE(storage->GetStatus() == JDataBundle::Status::Empty);
    
    REQUIRE(storage->GetFactory() != nullptr);
    
    // Retrieve triggers factory
    auto coll2 = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(coll2->size() == 1);
    REQUIRE(coll2->at(0).energy() == 1111.0);
}

} // namespace omnifacmultiple


namespace omnifacreadinsert {

struct RWClusterFac : public JOmniFactory<RWClusterFac> {

    PodioInput<ExampleCluster> m_clusters_in{this};
    PodioOutput<ExampleCluster> m_clusters_out{this};

    void Configure() {}
    void ChangeRun(int32_t /*run_nr*/) {}
    void Execute(int32_t /*run_nr*/, uint64_t evt_nr) {

        auto cs = std::make_unique<ExampleClusterCollection>();
        for (const auto& cluster_in : *m_clusters_in()) {
            auto cluster = MutableExampleCluster(1.0 + cluster_in.energy());
            cs->push_back(cluster);
        }
        m_clusters_out() = std::move(cs);
    }
};

TEST_CASE("PodioTests_OmniFacReadInsert") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "error");
    app.Add(new JOmniFactoryGeneratorT<RWClusterFac>("cluster_fac", {"protoclusters"}, {"clusters"}));
    app.Initialize();
    auto event = std::make_shared<JEvent>(&app);
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll1 = ExampleClusterCollection();
    auto cluster1 = coll1.create(22.0);
    auto storage = event->InsertCollection<ExampleCluster>(std::move(coll1), "protoclusters");

    REQUIRE(storage->GetSize() == 1);
    REQUIRE(storage->GetStatus() == JDataBundle::Status::Inserted);

    // Retrieve triggers factory
    auto coll = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(coll->size() == 1);
    REQUIRE(coll->at(0).energy() == 23.0);

}
} // namespace omnifacreadinsert
} // namespace podiotests

