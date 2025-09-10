
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <JANA/JApplication.h>
#include "TranslationTable_service.h"

TEST_CASE("TranslationTable_service_BasicTests") {
    JApplication app;

    auto sut = std::make_shared<TranslationTable_service>();
    sut->AddHardcodedRow(22,
                {.crate=1, .slot=1, .channel=1},
                {.detector_id = 7, .cell_id = 9, .indices = {1,2,3}, .x = 0.4, .y = 0.8, .z = 0.0}, {});

    sut->AddHardcodedRow(22,
                {.crate=1, .slot=1, .channel=2},
                {.detector_id = 7, .cell_id = 10, .indices = {1,2,4}, .x = 0.5, .y = 0.8, .z = 0.0}, {});

    sut->AddHardcodedRow(22,
                {.crate=1, .slot=2, .channel=1},
                {.detector_id = 7, .cell_id = 11, .indices = {1,2,5}, .x = 0.6, .y = 0.8, .z = 0.0}, {});

    app.ProvideService<TranslationTable_service>(sut);
    app.Initialize();
    auto tt = app.template GetService<TranslationTable_service>();
    auto lookup_table = tt->GetDAQLookupTable(22);
    const auto& row = lookup_table->at({1, 1, 2});
    REQUIRE(std::get<0>(row).detector_id == 7);
    REQUIRE(std::get<0>(row).cell_id== 10);
    REQUIRE(std::get<0>(row).indices.at(2) == 4);
}

TEST_CASE("RecHit_factory_BasicTests") {
    //REQUIRE(0 == 1);
}
