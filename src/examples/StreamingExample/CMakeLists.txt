# JExample 7 plugin

set(StreamingExample_SOURCES SHARED
    ZmqMain.cc
    ZmqTransport.h
    ReadoutMessageAuto.h
    AHit.h
    AHitAnomalyDetector.h
    AHitParser.h
    )

if (USE_ZEROMQ)
    find_package(ZeroMQ REQUIRED)
    add_library(StreamingExample_plugin SHARED ${StreamingExample_SOURCES})
    target_include_directories(StreamingExample_plugin PUBLIC ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(StreamingExample_plugin jana2 ${ZeroMQ_LIBRARIES})
    set_target_properties(StreamingExample_plugin PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS StreamingExample_plugin DESTINATION lib/JANA/plugins)
else()
    message(STATUS "Skipping examples/StreamingExample because USE_ZEROMQ=Off")

endif()



