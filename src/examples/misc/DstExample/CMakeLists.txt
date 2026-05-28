

add_jana_plugin(DstExample)

add_test(
    NAME jana-example-dst-tests 
    COMMAND jana -Pplugins=DstExample)
set_tests_properties(jana-example-dst-tests PROPERTIES
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)
