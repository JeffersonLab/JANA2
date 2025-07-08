
#include <catch.hpp>
#include <JANA/Components/JDatabundle.h>
#include <JANA/JApplicationFwd.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEvent.h>

#if JANA2_HAVE_PODIO
#include <PodioDatamodel/ExampleHitCollection.h>
#endif

namespace jana::databundletests::jfactoryt {

struct Hit { double E; };

class MyFacT : public JFactoryT<Hit> {
    void Process(const std::shared_ptr<const JEvent>& event) {
        mData.push_back(new Hit{21.2 + event->GetEventNumber()});
    }
};

TEST_CASE("JDatabundle_JFactoryT") {

    JApplication app;
    app.Add(new JFactoryGeneratorT<MyFacT>);
    auto event = std::make_shared<JEvent>(&app);
    event->SetEventNumber(1);

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

#if JANA2_HAVE_PODIO

class MyFac : public JFactory {

    components::PodioOutput<ExampleHit> m_hits_out {this};

    void Process(const std::shared_ptr<const JEvent>& event) {
        auto hit = MutableExampleHit();
        hit.time(event->GetEventNumber() + 11);
        m_hits_out->push_back(std::move(hit));
    }
};

void CheckPodioCollection(JEvent& event, std::string unique_name, size_t expected_value) {

    // Trigger creation via event->GetCollection
    auto coll = event.GetCollection<ExampleHit>("ExampleHit");
    REQUIRE(coll->size() == 1);
    REQUIRE(coll->at(0).time() == expected_value);

    // Check databundle contents directly as well
    auto databundle = event.GetFactorySet()->GetDatabundle(unique_name);
    REQUIRE(databundle->GetStatus() == JDatabundle::Status::Created);
    REQUIRE(databundle->GetSize() == 1);

    auto typed_databundle = dynamic_cast<JPodioDatabundle*>(databundle);

    REQUIRE(typed_databundle != nullptr);
    REQUIRE(typed_databundle->GetCollection()->size() == 1);
    auto typed_collection = dynamic_cast<const ExampleHitCollection*>(typed_databundle->GetCollection());
    REQUIRE(typed_collection->at(0).time() == expected_value);
}

TEST_CASE("JDatabundle_JFactory") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<MyFac>);
    auto event = std::make_shared<JEvent>(&app);

    auto databundle = event->GetFactorySet()->GetDatabundle("ExampleHit");
    REQUIRE(databundle != nullptr); // Databundle should ALWAYS exist whether factory runs or not
    REQUIRE(databundle->GetStatus() == JDatabundle::Status::Empty);
    REQUIRE(databundle->GetFactory() != nullptr);

    event->SetEventNumber(1);
    CheckPodioCollection(*event, "ExampleHit", 12);

    event->Clear();
    event->SetEventNumber(7);
    CheckPodioCollection(*event, "ExampleHit", 18);

    event->Clear();
    event->SetEventNumber(11);
    CheckPodioCollection(*event, "ExampleHit", 22);
}


#endif

}

