
#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JService.h>
#include <JANA/Components/JOmniFactory.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>

namespace jana::jservicetests {

class DummyService: public JService {
    bool init_started = false;
    bool init_finished = false;
public:
    void Init() override {
        LOG_INFO(GetLogger()) << "Starting DummyService::Init()" << LOG_END;
        init_started = true;
        throw std::runtime_error("Something goes wrong");
        init_finished = true;
        LOG_INFO(GetLogger()) << "Finishing DummyService::Init()" << LOG_END;
    }
};

TEST_CASE("JServiceTests_ExceptionInInit") {
    JApplication app;
    app.ProvideService(std::make_shared<DummyService>());
    try {
        app.Initialize();
        REQUIRE(1 == 0); // Shouldn't be reachable

        auto sut = app.GetService<DummyService>();
        REQUIRE(1 == 0); // Definitely shouldn't be reachable
    }
    catch (JException& e) {
        REQUIRE(e.GetMessage() == "Something goes wrong");
        REQUIRE(e.type_name == "jana::jservicetests::DummyService");
        REQUIRE(e.function_name == "JService::Init");
    }
}

struct DummyData {int x;};

struct DummyOmniFactory: public jana::components::JOmniFactory<DummyOmniFactory> {

    Service<DummyService> m_svc {this};
    Output<DummyData> m_output {this};

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {
        m_output().push_back(new DummyData{22});
    }
};

TEST_CASE("JServiceTests_ExceptionInInit_Issue381") {
    JApplication app;
    app.ProvideService(std::make_shared<DummyService>());

    auto gen = new components::JOmniFactoryGeneratorT<DummyOmniFactory>();
    gen->AddWiring("dummy", {}, {"data"});
    app.Add(gen);

    try {
        app.Initialize();
        REQUIRE(1 == 0); // Shouldn't be reachable
        auto event = std::make_shared<JEvent>(&app);
        auto data = event->Get<DummyData>("data");
        REQUIRE(1 == 0); // Definitely shouldn't be reachable
        REQUIRE(data.at(0)->x == 22);
    }
    catch (JException& e) {
        REQUIRE(e.GetMessage() == "Something goes wrong");
        REQUIRE(e.type_name == "jana::jservicetests::DummyService");
        REQUIRE(e.function_name == "JService::Init");
    }
}

} // namespace jana::jservicetests

