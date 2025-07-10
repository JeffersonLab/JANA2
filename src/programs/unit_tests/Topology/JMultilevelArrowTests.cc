
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/Topology/JMultilevelArrow.h>

TEST_CASE("JMultilevelArrow_ConfigurePorts") {

    JMultilevelArrow sut;
    std::vector<JEventLevel> levels {JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::Timeslice};

    SECTION("AllToAll") {
        sut.ConfigurePorts(JMultilevelArrow::Style::AllToAll, levels);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::In) == 0);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::In) == 1);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::In) == 2);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::Out) == 3);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::Out) == 4);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::Out) == 5);
    }

    SECTION("AllToOne") {
        sut.ConfigurePorts(JMultilevelArrow::Style::AllToOne, levels);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::In) == 0);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::In) == 1);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::In) == 2);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::Out) == 3);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::Out) == 3);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::Out) == 3);
    }

    SECTION("OneToAll") {
        sut.ConfigurePorts(JMultilevelArrow::Style::OneToAll, levels);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::In) == 0);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::In) == 0);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::In) == 0);
        REQUIRE(sut.GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::Out) == 1);
        REQUIRE(sut.GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::Out) == 2);
        REQUIRE(sut.GetPortIndex(JEventLevel::Timeslice, JMultilevelArrow::Direction::Out) == 3);

        REQUIRE_THROWS(sut.GetPortIndex(JEventLevel::Subevent, JMultilevelArrow::Direction::Out));
    }
}

