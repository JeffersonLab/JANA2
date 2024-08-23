

// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JANA/JEventSource.h"
#include "catch.hpp"
#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JMultifactory.h>
#include <JANA/Services/JComponentManager.h>

namespace omnifactory_tests {

struct TestSource : public JEventSource {
    Parameter<std::string> y {this, "y", "asdf", "Does something"};
    TestSource(std::string res_name, JApplication* app) {
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
    TestFactory() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Init() {
        REQUIRE(x() == 23);
    }
    void Process(const std::shared_ptr<const JEvent>& event) {
    }
};

TEST_CASE("ParametersTest") {
    JApplication app;
    app.Add(new JEventSourceGeneratorT<TestSource>);
    app.Add(new JFactoryGeneratorT<TestFactory>);
    app.Add("fake_file.root");
    app.SetParameterValue("jana:nevents", 2);
    app.SetParameterValue("autoactivate", "");
    app.Run();
}

}
