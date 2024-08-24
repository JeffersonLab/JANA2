

// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JANA/Components/JPodioCollection.h"
#include "JANA/Services/JComponentManager.h"
#include "PodioDatamodel/ExampleCluster.h"
#include "catch.hpp"
#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Components/JPodioOutput.h>
#include <PodioDatamodel/ExampleClusterCollection.h>
#include <memory>

namespace jcollection_tests {

struct TestSource : public JEventSource {
    Parameter<std::string> y {this, "y", "asdf", "Does something"};
    TestSource(std::string, JApplication*) {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    static std::string GetDescription() {
        return "Test source";
    }
    void Init() {
        REQUIRE(y() == "asdf");
    }
    Result Emit(JEvent&) {
        return Result::Success;
    }
};

struct TestFactory : public JFactory {
    Parameter<int> x {this, "x", 22, "Does something"};
    jana::components::PodioOutput<ExampleCluster> m_clusters {this, "my_collection"};

    TestFactory() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Init() {
        REQUIRE(x() == 22);
    }
    void Process(const std::shared_ptr<const JEvent>& event) {
        m_clusters()->push_back(MutableExampleCluster(22.2));
        m_clusters()->push_back(MutableExampleCluster(27));
    }
};

struct TestProc : public JEventProcessor {

    PodioInput<ExampleCluster> m_clusters_in {this, InputOptions("my_collection")};

    TestProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Process(const JEvent& event) {
        auto clusters = event.GetCollection<ExampleCluster>("my_collection", true);
        REQUIRE(clusters->size() == 2);

        REQUIRE(m_clusters_in() != nullptr);
        REQUIRE(m_clusters_in()->size() == 2);
        std::cout << "Proc found data: " << m_clusters_in() << std::endl;
    }
};

TEST_CASE("JCollectionEndToEndTest") {
    JApplication app;
    app.Add(new JEventSourceGeneratorT<TestSource>);
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Add(new TestProc);
    app.Add("fake_file.root");
    app.SetParameterValue("jana:nevents", 2);
    app.Run();
}

TEST_CASE("JCollectionTests_GetCollectionBase") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll = event->GetCollectionBase("my_collection");
    REQUIRE(coll != nullptr);
    REQUIRE(coll->size() == 2);
    auto typed_coll = dynamic_cast<const ExampleClusterCollection*>(coll);
    REQUIRE(typed_coll->at(1).energy() == 27);
}

TEST_CASE("JCollectionTests_GetCollection") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll = event->GetCollection<ExampleCluster>("my_collection");
    REQUIRE(coll != nullptr);
    REQUIRE(coll->size() == 2);
    REQUIRE(coll->at(1).energy() == 27);
}

TEST_CASE("JCollectionTests_InsertCollection") {
    JApplication app;
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    ExampleClusterCollection coll_in;
    coll_in.create(15.0);
    coll_in.create(30.0);
    coll_in.create(88.0);
    event->InsertCollection<ExampleCluster>(std::move(coll_in), "sillyclusters");

    auto coll_out = event->GetCollection<ExampleCluster>("sillyclusters");
    REQUIRE(coll_out != nullptr);
    REQUIRE(coll_out->size() == 3);
    REQUIRE(coll_out->at(1).energy() == 30);
}

} // namespace jcollection_tests
