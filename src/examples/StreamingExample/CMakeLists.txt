# JExample 7 plugin

set(StreamingExample_SOURCES SHARED
    ZmqMain.cc
    ZmqTransport.h
    ReadoutMessageAuto.h
    AHit.h
    AHitAnomalyDetector.h
    AHitParser.h
    )

find_package(ZeroMQ)
find_package(CZMQ)

if (ZEROMQ_FOUND AND CZMQ_FOUND)

    add_library(StreamingExample_plugin SHARED ${StreamingExample_SOURCES})
    target_include_directories(StreamingExample_plugin PUBLIC ${ZEROMQ_INCLUDE_DIRS} ${CMZQ_INCLUDE_DIRS})
    target_link_libraries(StreamingExample_plugin jana2 ${ZEROMQ_LIBRARIES} ${CZMQ_LIBRARIES})
    set_target_properties(StreamingExample_plugin PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS StreamingExample_plugin DESTINATION plugins)

else()
    message(WARNING "Skipping target `JExample7` due to missing ZeroMQ or CZMQ dependencies")

endif()



