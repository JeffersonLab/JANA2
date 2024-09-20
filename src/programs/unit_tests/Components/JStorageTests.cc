

// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JFactory.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JComponentManager.h>
#include <memory>

#include <JANA/Components/JPodioOutput.h>
#include <PodioDatamodel/ExampleClusterCollection.h>

namespace jstorage_tests {

struct TestSource : public JEventSource {
    Parameter<std::string> y {this, "y", "asdf", "Does something"};
    TestSource() {
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
    jana::components::PodioOutput<ExampleCluster> m_clusters {this, "my_collection"};

    TestFactory() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Init() {
    }
    void Process(const std::shared_ptr<const JEvent>&) {
        LOG_WARN(GetLogger()) << "Calling TestFactory::Process" << LOG_END;
        m_clusters()->push_back(MutableExampleCluster(22.2));
        m_clusters()->push_back(MutableExampleCluster(27));
    }
};

struct RegeneratingTestFactory : public JFactory {

    jana::components::PodioOutput<ExampleCluster> m_clusters {this, "my_collection"};

    RegeneratingTestFactory() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        SetFactoryFlag(JFactory_Flags_t::REGENERATE);
    }
    void Init() {
    }
    void Process(const std::shared_ptr<const JEvent>&) {
        LOG_WARN(GetLogger()) << "Calling TestFactory::Process" << LOG_END;
        m_clusters()->push_back(MutableExampleCluster(22.2));
        m_clusters()->push_back(MutableExampleCluster(27));
    }
};

struct TestProc : public JEventProcessor {

    PodioInput<ExampleCluster> m_clusters_in {this, InputOptions{.name="my_collection"}};

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

TEST_CASE("JStorageTests_EventInsertAndRetrieve") {
    JApplication app;
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll_in = std::make_unique<ExampleClusterCollection>();
    coll_in->create().energy(7.6);
    coll_in->create().energy(28);
    coll_in->create().energy(42);

    event->InsertCollection<ExampleCluster>(std::move(*coll_in), "my_collection");

    auto coll_out = event->GetCollectionBase("my_collection");
    REQUIRE(coll_out != nullptr);
    REQUIRE(coll_out->size() == 3);
    auto typed_coll = dynamic_cast<const ExampleClusterCollection*>(coll_out);
    REQUIRE(typed_coll->at(1).energy() == 28);
}

TEST_CASE("JStorageTests_InsertOverridesFactory") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll_in = std::make_unique<ExampleClusterCollection>();
    coll_in->create().energy(7.6);
    coll_in->create().energy(28);
    coll_in->create().energy(42);

    event->InsertCollection<ExampleCluster>(std::move(*coll_in), "my_collection");

    auto coll_out = event->GetCollectionBase("my_collection");
    REQUIRE(coll_out != nullptr);
    REQUIRE(coll_out->size() == 3);
    auto typed_coll = dynamic_cast<const ExampleClusterCollection*>(coll_out);
    REQUIRE(typed_coll->at(1).energy() == 28);

}
TEST_CASE("JStorageTests_RegenerateOverridesInsert") {
    JApplication app;
    app.SetParameterValue("enable_regenerate", true);
    app.Add(new JFactoryGeneratorT<RegeneratingTestFactory>);
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll_in = std::make_unique<ExampleClusterCollection>();
    coll_in->create().energy(7.6);
    coll_in->create().energy(28);
    coll_in->create().energy(42);

    event->InsertCollection<ExampleCluster>(std::move(*coll_in), "my_collection");

    try {
        event->GetCollectionBase("my_collection");
        REQUIRE(1==0);

    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

TEST_CASE("JStorageTests_FactoryProcessAndRetrieveUntyped") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Initialize();

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto coll_out = event->GetCollectionBase("my_collection");
    REQUIRE(coll_out != nullptr);
    REQUIRE(coll_out->size() == 2);
    auto typed_coll = dynamic_cast<const ExampleClusterCollection*>(coll_out);
    REQUIRE(typed_coll->at(1).energy() == 27);
}

TEST_CASE("JStorageTests_FactoryProcessAndRetrieveTyped") {
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

TEST_CASE("JStorageEndToEndTest") {
    JApplication app;
    app.Add(new JEventSourceGeneratorT<TestSource>);
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Add(new TestProc);
    app.Add("fake_file.root");
    app.SetParameterValue("jana:nevents", 2);
    app.Run();
}

} // namespace jcollection_tests
