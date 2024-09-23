

#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JComponentManager.h>


// -----------------------------------
// UserDefined
// -----------------------------------
namespace jana::components::jfactorygeneratortests_userdefined {

struct MyData { int x; };

class MyFac : public JFactoryT<MyData> {
    void Init() override {
        GetApplication(); // This will throw if nullptr
    }
    void Process(const std::shared_ptr<const JEvent>&) override {
        std::vector<MyData*> results;
        results.push_back(new MyData {22});
        Set(results);
    }
};

class MyFacGen : public JFactoryGenerator { 

    void GenerateFactories(JFactorySet *factory_set) override {
        factory_set->Add(new MyFac);
    }
};

TEST_CASE("JFactoryGeneratorTests_UserDefined") {
    JApplication app;
    app.Add(new MyFacGen());
    app.Initialize();
    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);

    auto data = event->Get<MyData>();
    REQUIRE(data.at(0)->x == 22);
}
}
