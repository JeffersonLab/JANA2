

add_jana_plugin(Tutorial)

add_test(NAME jana-example-tutorial-tests
    COMMAND jana -Pplugins=Tutorial -Pjana:nevents=50)
set_tests_properties(jana-example-tutorial-tests PROPERTIES
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)


