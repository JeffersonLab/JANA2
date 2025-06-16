

add_jana_plugin(lw_csv_file_writer)
target_link_libraries(lw_csv_file_writer PUBLIC lw_datamodel)

add_test(NAME examples-lw-05-smoketest COMMAND jana -Pplugins=lw_random_hit_source,lw_csv_file_writer -Pjana:nevents=10)
set_tests_properties(examples-lw-05-smoketest PROPERTIES LABELS "examples")
