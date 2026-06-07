

add_jana_library(lw_csv_file_writer_common 
    SOURCES CsvWriter.cc
    PUBLIC_HEADER CsvWriter.h
)

target_link_libraries(lw_csv_file_writer_common PUBLIC lw_datamodel)

add_jana_plugin(lw_csv_file_writer
    SOURCES csv_file_writer_plugin.cc
)

target_link_libraries(lw_csv_file_writer PUBLIC lw_csv_file_writer_common)

add_test(NAME examples-lw-05-smoketest COMMAND $<TARGET_FILE:jana> -Pplugins=lw_random_hit_source,lw_csv_file_writer -Pjana:nevents=10)
set_tests_properties(examples-lw-05-smoketest PROPERTIES
    LABELS "examples"
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)


