

set (janacontrol_PLUGIN_TESTS_SOURCES
        catch.hpp
        TestsMain.cc
        IntegrationTests.cc
        # Add component tests here
        )

add_executable(janacontrol_plugin_tests ${janacontrol_PLUGIN_TESTS_SOURCES})

#find_package(JANA REQUIRED)
find_package(Threads REQUIRED)


target_include_directories(janacontrol_plugin_tests PUBLIC ../src)
target_include_directories(janacontrol_plugin_tests PUBLIC ${JANA_INCLUDE_DIR})

target_link_libraries(janacontrol_plugin_tests janacontrol)
#target_link_libraries(janacontrol_plugin_tests ${JANA_LIBRARY})
target_link_libraries(janacontrol_plugin_tests jana2)
target_link_libraries(janacontrol_plugin_tests Threads::Threads)

install(TARGETS janacontrol_plugin_tests DESTINATION bin)

