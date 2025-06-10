

add_jana_plugin(lw_random_hit_source)
target_link_libraries(lw_random_hit_source PUBLIC lw_datamodel)

add_test(NAME examples-lw-00-smoketest COMMAND jana -Pplugins=lw_random_hit_source -Pjana:nevents=10)
set_tests_properties(examples-lw-00-smoketest PROPERTIES LABELS "examples")
