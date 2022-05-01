
#include "BlockExampleSource.h"

#include <JANA/JEvent.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("BlockSourceTests") {

auto event = std::make_shared<JEvent>();
auto factories = new JFactorySet;
//factories->Add(new TriggerDecision_Factory);
event->SetFactorySet(factories);
event->SetJApplication(new JApplication);


SECTION("Test a vertical line of hits") {

//    +-+-+-+
//    |x| | |
//    |x| | |
//    |x| | |
//    +-+-+-+

// event->Insert<ProtoECalHit>({
// new ProtoECalHit(0,0,1.0,1),
// new ProtoECalHit(1,0,1.0,1),
// new ProtoECalHit(2,0,1.0,1),
// });
//
// auto tracks = event->Get<ProtoECalTrack>();
//
// REQUIRE(tracks.size() == 1);
// REQUIRE(tracks[0]->matched_pattern == 0);
//
// auto trigger = event->GetSingle<TriggerDecision>();
// REQUIRE(trigger->decision == true);
}

}




