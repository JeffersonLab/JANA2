
add_jana_library(lw_random_hit_source_common
    SOURCES RandomHitSource.cc
    PUBLIC_HEADER RandomHitSource.h
)
target_link_libraries(lw_random_hit_source_common PUBLIC lw_datamodel)

add_jana_plugin(lw_random_hit_source
    SOURCES random_hit_source.cc
)
target_link_libraries(lw_random_hit_source PUBLIC lw_random_hit_source_common)

add_test(NAME examples-lw-00-smoketest COMMAND jana -Pplugins=lw_random_hit_source -Pjana:nevents=10)

set_tests_properties(examples-lw-00-smoketest PROPERTIES LABELS "examples")
