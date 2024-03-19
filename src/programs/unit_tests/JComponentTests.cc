#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>


namespace jana {
namespace component_unfolder_param_tests {


struct TestUnfolder : public JEventUnfolder {

    Parameter<float> threshold {this, "threshold", 16.0, "The max cutoff threshold [V * A * kg^-1 * m^-2 * sec^-3]"};
    Parameter<int> bucket_count {this, "bucket_count", 22, "The total number of buckets [dimensionless]"};

    TestUnfolder() {
        SetPrefix("my_unfolder");
    }
};


TEST_CASE("JEventUnfolderParametersTests") {
    JApplication app;
    auto* sut = new TestUnfolder;
    app.Add(sut);

    SECTION("JEventUnfolder using default parameters") {
        app.Initialize();
        sut->DoInit();
        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 22);
    }
    SECTION("JEventUnfolder using overridden parameters") {
        app.SetParameterValue("my_unfolder:threshold", 12.0);
        app.Initialize();
        sut->DoInit();
        REQUIRE(sut->threshold() == 12.0);
        REQUIRE(sut->bucket_count() == 22);
    }
}
} // component_unfolder_param_tests





namespace component_processor_param_tests {

struct TestProc : public JEventProcessor {

    Parameter<float> threshold {this, "threshold", 16.0, "The max cutoff threshold [V * A * kg^-1 * m^-2 * sec^-3]"};
    Parameter<int> bucket_count {this, "bucket_count", 22, "The total number of buckets [dimensionless]"};

    TestProc() {
        SetPrefix("my_proc");
    }
};


TEST_CASE("JEventProcessorParametersTests") {
    JApplication app;
    auto* sut = new TestProc;
    app.Add(sut);

    SECTION("JEventProcessor using default parameters") {
        app.Initialize();
        sut->DoInitialize();
        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 22);
    }
    SECTION("JEventProcessor using overridden parameters") {
        app.SetParameterValue("my_proc:threshold", 12.0);
        app.Initialize();
        sut->DoInitialize();
        REQUIRE(sut->threshold() == 12.0);
        REQUIRE(sut->bucket_count() == 22);
    }

}

} // namsepace component_processor_param_tests
} // namespace jana
