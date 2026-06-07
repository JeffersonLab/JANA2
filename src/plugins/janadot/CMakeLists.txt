
add_jana_plugin(janadot)

find_package(Threads REQUIRED)
target_link_libraries(janadot PUBLIC Threads::Threads)

add_test(NAME jana-plugin-janadot-tests
    COMMAND jana -Pplugins=JTest,janadot -Pjana:nevents=10)
set_tests_properties(jana-plugin-janadot-tests PROPERTIES
    ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
)

# TODO: Test that file is not empty!

