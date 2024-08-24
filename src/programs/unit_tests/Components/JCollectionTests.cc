

// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JANA/Components/JPodioCollection.h"
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
        REQUIRE(x() == 23);
    }
    void Process(const std::shared_ptr<const JEvent>& event) {
        m_clusters()->push_back(MutableExampleCluster(22.2));
        m_clusters()->push_back(MutableExampleCluster(27));
    }
};

struct TestProc : public JEventProcessor {

    PodioInput<ExampleCluster> m_clusters_in {this, InputOptions("my_collection")};

    void Process(const std::shared_ptr<const JEvent>& event) {
        REQUIRE(m_clusters_in() != nullptr);
        REQUIRE(m_clusters_in()->size() == 2);
        std::cout << "Proc found data: " << m_clusters_in() << std::endl;
    }
};

TEST_CASE("ParametersTest") {
    JApplication app;
    app.Add(new JEventSourceGeneratorT<TestSource>);
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Add(new TestProc);
    app.Add("fake_file.root");
    app.SetParameterValue("jana:nevents", 2);
    app.Run();
}

} // namespace jcollection_tests
