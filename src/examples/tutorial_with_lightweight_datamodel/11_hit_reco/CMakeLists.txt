
# `liblw_hit_reco_common` is a library that includes our hit reconstruction
# machinery, specifically the TranslationTable service and RecoHit factory.
# The algorithm is simple enough that we keep it in the factory for now. 

add_jana_library(lw_hit_reco_common
    SOURCES
        HitReconstruction_factory.cc
        TranslationTable_service.cc
    PUBLIC_HEADER
        HitReconstruction_factory.h
        TranslationTable_service.h
    TESTS
        HitReconstruction_factory_tests.cc
)

# `add_jana_library` will create a target for us, but we still need to link it against
# its various dependencies by ourselves.

target_link_libraries(lw_hit_reco_common PUBLIC lw_datamodel)

add_jana_plugin(lw_hit_reco SOURCES hit_reco_plugin.cc)
target_link_libraries(lw_hit_reco PUBLIC lw_hit_reco_common)

