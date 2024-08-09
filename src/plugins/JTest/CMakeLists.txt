

add_jana_plugin(JTest)

add_test(NAME jana-plugin-jtest-tests
    COMMAND jana -Pplugins=JTest -Pjana:nevents=20)

find_package(Threads REQUIRED)
target_link_libraries(JTest PUBLIC Threads::Threads)

