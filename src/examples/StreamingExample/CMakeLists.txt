# JExample 7 plugin

set(StreamingExample_SOURCES
    ZmqMain.cc)

set(StreamingExample_HEADERS
    ZmqTransport.h
    ReadoutMessageAuto.h
    AHit.h
    AHitAnomalyDetector.h
    AHitParser.h)

if (USE_ZEROMQ)
    find_package(ZeroMQ REQUIRED)
    add_jana_plugin(StreamingExample 
        SOURCES ${StreamingExample_SOURCES}
        PUBLIC_HEADER ${StreamingExample_HEADERS})

    target_include_directories(StreamingExample PUBLIC ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(StreamingExample PUBLIC ${ZeroMQ_LIBRARIES})

    add_test(NAME jana-example-streaming-tests
        COMMAND jana -Pplugins=StreamingExample)

    set_tests_properties(jana-example-streaming-tests PROPERTIES DISABLED TRUE)

else()

    message(STATUS "Skipping examples/StreamingExample because USE_ZEROMQ=Off")

endif()



