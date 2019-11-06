# JExample 7 plugin

find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)

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

