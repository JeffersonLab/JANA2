
#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JService.h>

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

} // namespace jana::jservicetests

