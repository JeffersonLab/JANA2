

add_jana_plugin(EventGroupExample)

add_test(NAME jana-example-eventgroup-tests 
    COMMAND $<TARGET_FILE:jana> -Pplugins=EventGroupExample)

set_tests_properties(jana-example-eventgroup-tests PROPERTIES
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)
