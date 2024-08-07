

file(GLOB JTEST_HEADERS "*.h*")

add_jana_plugin(JTest SOURCES JTestMain.cc PUBLIC_HEADER ${JTEST_HEADERS})

add_test(NAME jana-plugin-jtest-tests
    COMMAND jana -Pplugins=JTest -Pjana:nevents=100)

find_package(Threads REQUIRED)
target_link_libraries(JTest PUBLIC Threads::Threads)

