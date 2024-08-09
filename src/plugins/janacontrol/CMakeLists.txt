
if (${USE_ZEROMQ})

    find_package(ZeroMQ REQUIRED)
    find_package(Threads REQUIRED)

    set (janacontrol_PLUGIN_SOURCES
        janacontrol.cc
        JControlZMQ.cc
        JControlEventProcessor.cc)

    set (janacontrol_PLUGIN_HEADERS
        JControlZMQ.h
        JControlEventProcessor.h)

    set (janacontrol_PLUGIN_TESTS
        janacontrol_tests.cc
    )

    add_jana_plugin(janacontrol
        SOURCES ${janacontrol_PLUGIN_SOURCES}
        PUBLIC_HEADER ${janacontrol_PLUGIN_HEADERS}
        TESTS ${janacontrol_PLUGIN_TESTS})

    target_include_directories(janacontrol PUBLIC ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(janacontrol PUBLIC ${ZeroMQ_LIBRARIES})
    target_include_directories(janacontrol_tests PUBLIC ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(janacontrol_tests PUBLIC ${ZeroMQ_LIBRARIES})

else()
    message(STATUS "Skipping plugins/janacontrol because USE_ZEROMQ=Off")

endif()


