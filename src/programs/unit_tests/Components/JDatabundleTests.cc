
#include "JANA/Components/JDatabundle.h"
#include "JANA/JApplicationFwd.h"
#include <catch.hpp>
#include <JANA/JFactoryT.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEvent.h>

namespace jana::databundletests::jfactoryt {

struct Hit { double E; };

class MyFac : public JFactoryT<Hit> {
    void Process(const std::shared_ptr<const JEvent>&) {
        mData.push_back(new Hit(22.2));
    }
};

TEST_CASE("JDatabundle_JFactoryT") {

    JApplication app;
    app.Add(new JFactoryGeneratorT<MyFac>);
    auto event = std::make_shared<JEvent>(&app);

    SECTION("OldMechanism") {
        auto data = event->Get<Hit>();
        REQUIRE(data.size() == 1);
        REQUIRE(data.at(0)->E == 22.2);
    }
    SECTION("NewMechanism") {

        auto databundle = event->GetFactorySet()->GetDatabundle("jana::databundletests::jfactoryt::Hit");

        REQUIRE(databundle != nullptr); // Databundle should ALWAYS exist whether factory runs or not
        REQUIRE(databundle->GetStatus() == JDatabundle::Status::Empty);
        REQUIRE(databundle->GetFactory() != nullptr);

        databundle->GetFactory()->Create(*event);

        REQUIRE(databundle->GetStatus() == JDatabundle::Status::Created);
        REQUIRE(databundle->GetSize() == 1);

        auto typed_databundle = dynamic_cast<JLightweightDatabundleT<Hit>*>(databundle);

        REQUIRE(typed_databundle != nullptr);
        REQUIRE(typed_databundle->GetData().size() == 1);
        REQUIRE(typed_databundle->GetData().at(0)->E == 22.2);
    }
}

}

