
add_jana_library(lw_random_hit_source_common
    SOURCES RandomHitSource.cc
    PUBLIC_HEADER RandomHitSource.h
)
target_link_libraries(lw_random_hit_source_common PUBLIC lw_datamodel)

add_jana_plugin(lw_random_hit_source
    SOURCES random_hit_source.cc
)
target_link_libraries(lw_random_hit_source PUBLIC lw_random_hit_source_common)

add_test(NAME examples-lw-00-smoketest COMMAND $<TARGET_FILE:jana> -Pplugins=lw_random_hit_source -Pjana:nevents=10)

set_tests_properties(examples-lw-00-smoketest PROPERTIES
    LABELS "examples"
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)
