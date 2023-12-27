
#include <catch.hpp>

#include <type_traits>
#include <datamodel/ExampleClusterCollection.h>
#include <DatamodelGlue.h>  // Hopefully this won't be necessary in the future
#include <JANA/JEvent.h>

namespace podiotests {


TEST_CASE("PodioTestsInsertAndRetrieve") {
    ExampleClusterCollection clusters_to_insert;
    clusters_to_insert.push_back(ExampleCluster({16.0}));
    clusters_to_insert.push_back(ExampleCluster({128.0}));

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

    SECTION("Retrieve using JEvent::Get()") {
        std::vector<const ExampleCluster*> clusters_retrieved = event->Get<ExampleCluster>("clusters");
        REQUIRE(clusters_retrieved.size() == 2);
        REQUIRE(clusters_retrieved[0]->energy() == 16.0);
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

#if podio_VERSION < PODIO_VERSION(0, 17, 0)
template <typename T>
struct MyWrapper<T, std::void_t<typename PodioTypeMap<T>::collection_t>> {
    int x = 2;
    bool have_podio() {
        return true;
    }
};
#else
template <typename T>
struct MyWrapper<T, std::void_t<typename T::collection_type>> {
    int x = 2;
    bool have_podio() {
        return true;
    }
};
#endif

TEST_CASE("SFINAE for JFactoryT || JFactoryPodioT") {

    MyWrapper<int> w;
    REQUIRE(w.have_podio() == false);

    MyWrapper<ExampleCluster> ww;
    REQUIRE(ww.have_podio() == true);

    ww.x = 22;

}

template <typename, typename=void>
struct is_podio : std::false_type {};

#if podio_VERSION < PODIO_VERSION(0, 17, 0)
template <typename T>
struct is_podio<T, std::void_t<typename PodioTypeMap<T>::collection_t>> : std::true_type {};
#else
template <typename T>
struct is_podio<T, std::void_t<typename T::collection_type>> : std::true_type {};
#endif

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
    void Process(const std::shared_ptr<const JEvent>& event) override {
        ExampleClusterCollection c;
        c.push_back(ExampleCluster(16.0));
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
    auto fac = dynamic_cast<jana2_tests_podiotests_init::TestFac*>(event->GetFactory<ExampleCluster>("clusters"));
    REQUIRE(fac != nullptr);
    REQUIRE(fac->init_called == true);
}

namespace jana2_tests_podiotests_insert {

struct TestFac : public JFactoryPodioT<ExampleCluster> {
    TestFac() {
        SetTag("clusters");
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        Insert(new ExampleCluster(16.0));
    }
};
}

TEST_CASE("JFactoryPodioT::Insert() and retrieval") {

    JApplication app;
    auto event = std::make_shared<JEvent>(&app);
    auto fs = new JFactorySet;
    fs->Add(new jana2_tests_podiotests_insert::TestFac);
    event->SetFactorySet(fs);
    event->GetFactorySet()->Release();  // Simulate a trip to the JEventPool

    // Retrieve as vector<ExampleCluster*> (Goes through mData)
    auto vcp = event->Get<ExampleCluster>("clusters");
    REQUIRE(vcp.size() == 1);
    REQUIRE(vcp[0]->energy() == 16.0);

    // Retrieve as ExampleClusterCollection (Goes through Frame)
    auto r = event->GetCollection<ExampleCluster>("clusters");
    REQUIRE(r != nullptr);
    REQUIRE(r->size() == 1);
    REQUIRE((*r)[0].energy() == 16.0);
}

} // namespace podiotests
