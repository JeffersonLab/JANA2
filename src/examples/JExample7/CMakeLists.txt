# JExample 7 plugin

find_package(ZeroMQ)
find_package(CZMQ)

if(ZeroMQ_FOUND AND CZMQ_FOUND)

    add_library(JExample7 SHARED
        ZmqMain.cc
        ZmqTransport.h
        ReadoutMessageAuto.h
        AHit.h
        AHitAnomalyDetector.h
        AHitParser.h
    )

    target_include_directories(JExample7 PUBLIC ${ZEROMQ_INCLUDE_DIRS} ${CMZQ_INCLUDE_DIRS})
    target_link_libraries(JExample7 jana2 ${ZEROMQ_LIBRARIES} ${CZMQ_LIBRARIES})
    set_target_properties(JExample7 PROPERTIES PREFIX "" SUFFIX ".so")

else()

    message(WARNING "Unable to find ZeroMQ/CZMQ! Skipping compilation of JExample7.so")

endif()



