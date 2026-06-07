

add_jana_plugin(JTest)

add_test(NAME jana-plugin-jtest-tests
    COMMAND jana -Pplugins=JTest -Pjana:nevents=20)
set_tests_properties(jana-plugin-jtest-tests PROPERTIES
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)


