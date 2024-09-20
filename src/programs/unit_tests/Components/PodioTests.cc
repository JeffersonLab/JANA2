
#include <catch.hpp>

#include <type_traits>
#include <PodioDatamodel/ExampleClusterCollection.h>
#include <JANA/JEvent.h>
#include <JANA/JMultifactory.h>
#include <JANA/Podio/JFactoryPodioT.h>

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

    auto fac_untyped = event->GetStorage("clusters", false)->GetFactory();
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
    void Process(const std::shared_ptr<const JEvent>&) override {
        ExampleClusterCollection c;
        c.push_back(MutableExampleCluster(16.0));
        SetCollection<ExampleCluster>("sillyclusters", std::move(c));
    }
};

TEST_CASE("Podio JMultifactory::Init gets called") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);
    auto fs = new JFactorySet;
    fs->Add(new TestMultiFac);
    event->SetFactorySet(fs);
    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool

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
TEST_CASE("PodioTests_InsertMultiple") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);

    // Insert a cluster

    auto coll1 = ExampleClusterCollection();
    auto cluster1 = coll1.create(22.0);
    auto storage = event->InsertCollection<ExampleCluster>(std::move(coll1), "clusters");

    REQUIRE(storage->GetSize() == 1);
    REQUIRE(storage->GetStatus() == JStorage::Status::Inserted);

    // Retrieve and validate cluster

    auto cluster1_retrieved = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(cluster1_retrieved->at(0).energy() == 22.0);

    // Clear event

    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool
    
    // After clearing, the JStorage will still exist, but it will be empty
    auto storage2 = event->GetStorage("clusters", false);
    REQUIRE(storage2->GetStatus() == JStorage::Status::Empty);
    REQUIRE(storage2->GetSize() == 0);

    // Insert a cluster. If event isn't being cleared correctly, this will throw

    auto coll2 = ExampleClusterCollection();
    auto cluster2 = coll2.create(33.0);
    auto storage3 = event->InsertCollection<ExampleCluster>(std::move(coll2), "clusters");
    REQUIRE(storage3->GetStatus() == JStorage::Status::Inserted);
    REQUIRE(storage3->GetSize() == 1);

    // Retrieve and validate cluster

    auto cluster2_retrieved = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(cluster2_retrieved->at(0).energy() == 33.0);
}


} // namespace multifactory
} // namespace podiotests

